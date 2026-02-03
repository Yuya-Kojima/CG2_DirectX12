#include "GamePlayScene.h"
#include "Actor/Goal.h"
#include "Actor/Hazard.h"
#include "Actor/Level.h"
#include "Actor/Npc.h"
#include "Actor/Physics.h"
#include "Actor/Player.h"
#include "Actor/Stick.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Debug/Logger.h"
#include "Input/Input.h"
#include "Input/InputKeyState.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Physics/CollisionSystem.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Stage/StageLoader.h"
#include "Texture/TextureManager.h"
#ifdef USE_IMGUI
#include "Debug/ImGuiManager.h"
#include "imgui.h"
#endif
#include "Scene/StageSelection.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>

// ---------------------------------------------------------
// 初期化
// ---------------------------------------------------------
void GamePlayScene::Initialize(EngineBase* engine)
{
    // エンジン本体への参照を保持
    engine_ = engine;
    // this scene should not show ImGui windows by default
    showImGui_ = false;

    //===========================
    // テクスチャ・オーディオ・スプライトの初期化
    //===========================
    // (必要に応じてリソースロード処理を追加)

    //===========================
    // 3Dオブジェクト関係の初期化
    //===========================

    // メインカメラの生成と初期設定 (俯瞰視点)
    camera_ = new GameCamera();
    camera_->SetRotate({ 1.0f, 0.0f, 0.0f });
    camera_->SetTranslate({ 0.0f, 50.0f, -30.0f });

    // デバッグ用カメラの生成
    debugCamera_ = new DebugCamera();
    debugCamera_->Initialize({ 0.0f, 4.0f, -10.0f });

    // レンダラに使用するカメラを登録
    engine_->GetObject3dRenderer()->SetDefaultCamera(camera_);

    // --- ステージ読み込み ---
    level_ = new Level();
    Stage::StageData sd;
    bool loaded = false;

    // JSONファイルからステージ構成をロード (優先パス順)
    const std::string selected = StageSelection::GetSelected();
    if (!loaded)
        loaded = Stage::StageLoader::LoadStage("Application/resources/stages/" + selected, sd);
    if (!loaded)
        loaded = Stage::StageLoader::LoadStage("resources/stages/" + selected, sd);

    if (loaded) {
        // ロード成功時はJSON内のグリッド設定で初期化
        level_->Initialize(engine_->GetObject3dRenderer(), sd.width, sd.height, sd.tileSize);
        // Create floor tiles based on map data: draw only where map value == 0
        level_->CreateFloorTiles(sd.tiles);
    } else {
        // ロード失敗時はデフォルト設定で初期化
        level_->Initialize(engine_->GetObject3dRenderer(), 16, 16, 1.0f);
        // create full floor when no stage loaded
        level_->CreateFloorTiles();
    }

    // Debug: report how many objects were parsed from stage and list classes/ids
    if (loaded) {
        try {
            Logger::Log(std::format("[StageDebug] loaded objects count={}\n", (int)sd.objects.size()));
        } catch (...) {
            Logger::Log(std::string("[StageDebug] loaded objects count=") + std::to_string((int)sd.objects.size()) + "\n");
        }
        for (const auto& o : sd.objects) {
            try {
                Logger::Log(std::format("[StageDebug] object class='{}' id={} pos=({:.2f},{:.2f},{:.2f}) model='{}'\n",
                    o.className, o.id, o.position.x, o.position.y, o.position.z, o.model));
            } catch (...) {
                Logger::Log(std::string("[StageDebug] object class='") + o.className + "' id=" + std::to_string(o.id) + "\n");
            }
        }
    }

    // --- フォールバック生成 (シーン側で個別初期化を行う) ---
    static uint32_t nextId = 1;

    // If stage loaded, detect whether stage defines core actors so we can avoid
    // spawning fallbacks unnecessarily. This prevents fallback instances from
    // blocking stage-provided actors (fallback ran before we processed objects).
    bool stageHasPlayer = false;
    bool stageHasStick = false;
    bool stageHasNpc = false;
    bool stageHasGoal = false;
    if (loaded) {
        for (const auto& o : sd.objects) {
            std::string cls = o.className;
            for (auto& c : cls)
                c = (char)tolower(c);
            if (cls == "player")
                stageHasPlayer = true;
            if (cls == "stick")
                stageHasStick = true;
            if (cls == "npc")
                stageHasNpc = true;
            if (cls == "goal")
                stageHasGoal = true;
        }
    }

    // プレイヤー生成（フォールバック）
    if (!player_ && !stageHasPlayer) {
        player_ = new Player();
        player_->Initialize(engine_->GetInputManager(), engine_->GetObject3dRenderer());
        player_->SetLayer(CollisionMask::Player);
        player_->AttachCameraImmediate(camera_);
        player_->SetId(nextId++);
        player_->AttachLevel(level_);
        Logger::Log("[Scene] Fallback: spawned Player\n");
    }

    // 棒(Stick)生成（フォールバック）
    if (!stick_ && !stageHasStick) {
        Vector3 sp = { 0.0f, 0.1f, 2.0f };
        if (player_) {
            const Vector3& pp = player_->GetPosition();
            sp = { pp.x + 0.6f, pp.y + 0.1f, pp.z + 0.6f };
        }
        stick_ = new Stick();
        stick_->Initialize(engine_->GetObject3dRenderer(), sp);
        stick_->SetLevel(level_);
        stick_->SetLayer(CollisionMask::Item);
        stick_->SetId(nextId++);
        Logger::Log("[Scene] Fallback: spawned Stick\n");
    }

    // ゴール生成（フォールバック）
    if (!goal_ && !stageHasGoal) {
        goal_ = new Goal();
        goal_->Initialize(engine_->GetObject3dRenderer(), { 8.0f, 0.0f, 0.0f });
        Logger::Log("[Scene] Fallback: spawned Goal\n");
    }

    // NPC生成（フォールバック） - 同様の形式で作成
    if (!npc_ && !stageHasNpc) {
        // place NPC so that it will run into the stick after starting to move
        Vector3 npPos = { 2.0f, 0.0f, 2.0f };
        if (player_) {
            const Vector3& pp = player_->GetPosition();
            // プレイヤーの左側（負X方向）に一旦配置
            npPos = { pp.x - 1.5f, pp.y, pp.z };
        }

        // If a stick exists, compute a spawn position a few meters away so NPC
        // will move toward the stick and collide with it. Set straight direction
        // toward the stick.
        Vector3 desiredDir = { 1.0f, 0.0f, 0.0f };
        if (stick_) {
            Vector3 sp = stick_->GetPosition();
            // 日本語: スティックの少し左（負X方向）にスポーンさせて、左から当てる挙動を見やすくする
            // シーンのメンバ変数 npcSpawnLeftOffset_ を使って調整可能にする
            float leftOffset = npcSpawnLeftOffset_;
            npPos.x = sp.x - leftOffset;
            npPos.z = sp.z;
            // 地面にスナップしたうえで少し上に出しておく(見やすさのため)
            float groundY = 0.0f;
            if (level_ && level_->RaycastDown({ npPos.x, sp.y + 2.0f, npPos.z }, 4.0f, groundY)) {
                // 少し上げて重なりを避ける
                npPos.y = groundY + 0.5f;
            } else {
                npPos.y = sp.y + 0.5f;
            }
            // 進行方向はスティックの中心へ向かうように設定（左から当てる挙動）
            float dx = sp.x - npPos.x;
            float dz = sp.z - npPos.z;
            float dlen = std::sqrt(dx * dx + dz * dz);
            if (dlen > 1e-6f) {
                desiredDir.x = dx / dlen;
                desiredDir.z = dz / dlen;
                desiredDir.y = 0.0f;
            } else {
                desiredDir = { 1.0f, 0.0f, 0.0f };
            }
        }

        npc_ = new Npc();
        npc_->Initialize(engine_->GetObject3dRenderer(), npPos);
        npc_->AttachLevel(level_);
        npc_->SetNavGrid(&level_->GetNavGrid());
        npc_->SetLayer(CollisionMask::NPC);
        npc_->SetId(nextId++);
        // デフォルトは単純直進させる（経路探索不要）
        npc_->SetStraightDirection(desiredDir);
        npc_->SetState(Npc::State::Straight);
        try {
            Logger::Log(std::format("[Scene] Fallback: spawned Npc pos=({:.2f},{:.2f},{:.2f}) straightDir=({:.2f},{:.2f},{:.2f}) stick=({:.2f},{:.2f},{:.2f})\n",
                npPos.x, npPos.y, npPos.z, desiredDir.x, desiredDir.y, desiredDir.z,
                stick_ ? stick_->GetPosition().x : 0.0f, stick_ ? stick_->GetPosition().y : 0.0f, stick_ ? stick_->GetPosition().z : 0.0f));
        } catch (...) {
            Logger::Log(std::string("[Scene] Fallback: spawned Npc (format error)\n"));
        }
        Logger::Log("[Scene] Fallback: spawned Npc\n");
    }

    // --- ステージ情報のログ出力 ---
    // Decide whether the stage contains hazards coming from the tilemap.
    // We intentionally treat hazards defined in the tiles array as the
    // authoritative source. Stage object entries named "Hazard" are
    // ignored because hazards/floor should be generated only from the
    // tilemap.
    bool hadStageHazard = false;
    if (loaded) {
        hadStageHazard = std::any_of(sd.tiles.begin(), sd.tiles.end(), [](int v) { return v == 1; });

        for (const auto& o : sd.objects) {
            try {
                Logger::Log(std::format("[StageObjects] type='{}' class='{}' model='{}' id={}\n", o.type, o.className, o.model, o.id));
            } catch (...) {
                Logger::Log("[StageObjects] (format error)\n");
            }
            try {
                Logger::Log(std::format("[Scene] object class='{}' pos=({:.2f},{:.2f},{:.2f}) id={}\n", o.className, o.position.x, o.position.y, o.position.z, o.id));
            } catch (...) {
                Logger::Log(std::string("[Scene] object class='") + o.className + "'\n");
            }
            // Note: hazards defined as objects are intentionally ignored.
            // NPC spawning from stage object list is disabled here.
            // NPC instances are created as fallbacks below if none exist.
        }

        // --- Stage-defined objects: create actors for known classes (Stick, Npc, Player, Goal) ---
        if (loaded && level_) {
            auto parseVec3 = [](const std::string& s) -> Vector3 {
                Vector3 v { 0.0f, 0.0f, 0.0f };
                size_t p1 = s.find(',');
                if (p1 == std::string::npos)
                    return v;
                size_t p2 = s.find(',', p1 + 1);
                try {
                    v.x = std::stof(s.substr(0, p1));
                    v.y = std::stof(s.substr(p1 + 1, p2 - (p1 + 1)));
                    v.z = std::stof(s.substr(p2 + 1));
                } catch (...) {
                }
                return v;
            };

            auto allocateId = [&](uint32_t requested) -> uint32_t {
                if (requested != 0) {
                    if (requested >= nextId)
                        nextId = requested + 1;
                    return requested;
                }
                return nextId++;
            };

            for (const auto& o : sd.objects) {
                // normalize class name
                std::string cls = o.className;
                for (auto& c : cls)
                    c = (char)tolower(c);

                if (cls == "stick") {
                    if (!stick_) {
                        stick_ = new Stick();
                        stick_->Initialize(engine_->GetObject3dRenderer(), o.position);
                        stick_->SetLevel(level_);
                        stick_->SetLayer(o.layer);
                        uint32_t id = allocateId(o.id);
                        stick_->SetId(id);
                        Logger::Log("Stage: spawned Stick from JSON");
                    }
                    // OBB registration for the stick is handled inside Stick class
                    // (Stick::DropExternal / SetId). Do not register here to avoid
                    // duplicate / out-of-sync colliders that can cause invisible
                    // hitboxes. The Stick instance will register/update its OBB
                    // with the Level when appropriate.
                } else if (cls == "npc") {
                    if (!npc_) {
                        npc_ = new Npc();
                        npc_->Initialize(engine_->GetObject3dRenderer(), o.position);
                        npc_->AttachLevel(level_);
                        npc_->SetLayer(o.layer);
                        uint32_t id = allocateId(o.id);
                        npc_->SetId(id);
                        // behavior property (store but do not force state)
                        auto it = o.properties.find("behavior");
                        if (it != o.properties.end()) {
                            npc_->SetBehavior(it->second);
                        }
                        // optional straightDir property as "x,y,z"
                        auto it2 = o.properties.find("straightDir");
                        if (it2 != o.properties.end()) {
                            Vector3 sdv = parseVec3(it2->second);
                            npc_->SetStraightDirection(sdv);
                            npc_->SetState(Npc::State::Straight);
                        } else {
                            // default: keep a consistent forward direction (positive X) so
                            // stage NPCs always move in the expected straight direction
                            npc_->SetStraightDirection({ 1.0f, 0.0f, 0.0f });
                            npc_->SetState(Npc::State::Straight);
                        }
                        Logger::Log("Stage: spawned Npc from JSON");
                    }
                } else if (cls == "player") {
                    if (!player_) {
                        player_ = new Player();
                        player_->Initialize(engine_->GetInputManager(), engine_->GetObject3dRenderer());
                        player_->AttachLevel(level_);
                        player_->SetLayer(o.layer);
                        uint32_t id = allocateId(o.id);
                        player_->SetId(id);
                        player_->SetPosition(o.position);
                        Logger::Log("Stage: spawned Player from JSON");
                    }
                } else if (cls == "goal") {
                    if (!goal_) {
                        goal_ = new Goal();
                        goal_->Initialize(engine_->GetObject3dRenderer(), o.position);
                        // assign id if provided (Goal doesn't store id but keep for level owner mapping)
                        uint32_t gid = allocateId(o.id);
                        (void)gid;
                        Logger::Log("Stage: spawned Goal from JSON");
                    }
                    if (o.obbHalfExtents.x != 0.0f || o.obbHalfExtents.z != 0.0f) {
                        goalHalf_.x = o.obbHalfExtents.x;
                        goalHalf_.z = o.obbHalfExtents.z;
                        goalHalf_.y = o.obbHalfExtents.y > 0.0f ? o.obbHalfExtents.y : goalHalf_.y;
                        goalHasAABB_ = true;
                    }
                }
            }
        }
    }

    // If the stage defined an object with class "Goal" and provided obb info,
    // use that to drive AABB-based goal checks instead of sphere radius.
    if (loaded) {
        for (const auto& o : sd.objects) {
            if (o.className == "Goal" || o.type == "goal" || o.className == "goal") {
                // place goal actor at provided position (if not already)
                if (!goal_) {
                    goal_ = new Goal();
                    goal_->Initialize(engine_->GetObject3dRenderer(), o.position);
                }
                // If OBB or half extents provided, set conservative AABB
                if (o.obbHalfExtents.x != 0.0f || o.obbHalfExtents.z != 0.0f) {
                    // use XZ half extents; default Y to reasonable value
                    goalHalf_.x = o.obbHalfExtents.x;
                    goalHalf_.z = o.obbHalfExtents.z;
                    goalHalf_.y = (o.obbHalfExtents.y > 0.0f) ? o.obbHalfExtents.y : 1.0f;
                    goalHasAABB_ = true;
                    Logger::Log("Stage-defined Goal AABB loaded.");
                }
                break;
            }
        }
    }

    // --- オブジェクト・アクターの生成 ---
    // JSON はマップ（タイル／コリジョン）のみを供給し、個々のオブジェクト生成は
    // 各クラス／初期化処理側で行うように変更しました。
    if (loaded) {
        // タイルマップに基づく壁コリジョンの生成
        if (!sd.collision.empty()) {
            for (int z = 0; z < sd.height; ++z) {
                for (int x = 0; x < sd.width; ++x) {
                    int idx = z * sd.width + x;
                    if (idx >= 0 && idx < (int)sd.collision.size() && sd.collision[idx] != 0) {
                        const auto& nav = level_->GetNavGrid();
                        Vector3 center = nav.TileCenter(x, z);
                        float hs = sd.tileSize * 0.5f;
                        Vector3 min = { center.x - hs, 0.0f, center.z - hs };
                        Vector3 max = { center.x + hs, 2.0f, center.z + hs };
                        level_->AddWallAABB(min, max, true, 0, 0);
                    }
                }
            }
            // 壁データをグリッドに反映
            level_->RebuildWallGrid();
        }
        // Hazards: create multiple fallback hazards only if stage didn't define any
        if (!hadStageHazard && level_) {
            const float defaultRadius = 0.5f;
            const float spacing = 2.0f;
            Vector3 base = { 4.0f, 0.0f, 4.0f };
            if (player_) {
                const Vector3& pp = player_->GetPosition();
                base = { pp.x + spacing, pp.y, pp.z + spacing };
            }

            // arrange hazards in a small cross pattern around base
            const Vector3 offs[] = {
                { 0.0f, 0.0f, 0.0f },
                { spacing, 0.0f, 0.0f },
                { -spacing, 0.0f, 0.0f },
                { 0.0f, 0.0f, spacing },
                { 0.0f, 0.0f, -spacing }
            };

            int created = 0;
            for (const auto& off : offs) {
                Vector3 p = { base.x + off.x, base.y + off.y, base.z + off.z };
                level_->AddHazard(p, defaultRadius);
                ++created;
            }
            Logger::Log(std::string("[Scene] Fallback: spawned ") + std::to_string(created) + " Hazards\n");
        }
    }
}

// ---------------------------------------------------------
// 終了処理
// ---------------------------------------------------------
void GamePlayScene::Finalize()
{
    // オブジェクト破棄前に、レベルに登録されたID参照をクリア
    if (level_) {
        if (player_)
            level_->ClearOwnerId(player_->GetId());
        if (npc_)
            level_->ClearOwnerId(npc_->GetId());
        if (stick_)
            level_->ClearOwnerId(stick_->GetId());
    }

    // メモリの解放
    delete debugCamera_;
    debugCamera_ = nullptr;

    delete camera_;
    camera_ = nullptr;

    delete player_;
    player_ = nullptr;
    delete npc_;
    npc_ = nullptr;
    delete stick_;
    stick_ = nullptr;
    delete goal_;
    goal_ = nullptr;
    delete level_;
    level_ = nullptr;
}

// ---------------------------------------------------------
// フレーム更新
// ---------------------------------------------------------
void GamePlayScene::Update()
{
    // オーディオ更新
    SoundManager::GetInstance()->Update();

    // デバッグ用：Enterでシーン遷移
    if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
        SceneManager::GetInstance()->ChangeScene("STAGESELECT");
    }

    // Pキーでデバッグカメラをトグル
    if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
        useDebugCamera_ = !useDebugCamera_;
    }

    // --- 1. アクターと環境の更新 ---
    if (player_)
        player_->Update(1.0f / 60.0f);
    if (level_)
        level_->Update(1.0f / 60.0f);

    // --- 2. NPCのAI制御 ---
    if (npc_) {
        Vector3 targetPos = { 0.0f, 0.0f, 0.0f };
        const std::string& beh = npc_->GetBehavior();

        // ビヘイビアに応じた目標地点の設定
        if (beh == "followPlayer") {
            if (player_)
                targetPos = player_->GetPosition();
        } else if (beh == "gotoGoal") {
            if (goal_)
                targetPos = goal_->GetPosition();
            else if (player_)
                targetPos = player_->GetPosition();
        } else if (beh == "idle") {
            targetPos = npc_->GetPosition();
        } else {
            // デフォルト：プレイヤーかゴールの位置を目指す
            if (player_)
                targetPos = player_->GetPosition();
            if (goal_)
                targetPos = goal_->GetPosition();
        }
        npc_->Update(1.0f / 60.0f, targetPos, *level_);
    }

    // --- 3. 木の棒(Stick)の拾う・置く処理 ---
    auto input = engine_->GetInputManager();
    if (input && stick_) {
        // 日本語: キーボードのスペースキーまたはゲームパッドのAボタンで拾う/置くを行う
        if (input->IsTriggerKey(DIK_SPACE) || input->IsPadTrigger(PadButton::A)) {
            if (!stick_->IsHeld()) {
                // 未保持：プレイヤーが存在する場合のみ距離をチェックして拾う
                if (player_) {
                    // NPCがスティックに乗っている場合は拾えないようにする
                    if (npc_ && npc_->IsMounted() && npc_->GetMountedOwnerId() == stick_->GetId()) {
                        Logger::Log("Cannot pick up: NPC is on the stick.");
                        // do nothing
                    } else {
                        const Vector3& pp = player_->GetPosition();

                        // NPCが乗っていない場合は拾える
                        Vector3 playerHalfExt = player_->GetHalfExtents();

                        // プレイヤーのAABB半分の大きさ（XZのみ）
                        Vector3 stickCenter, stickHalf;
                        stick_->GetConservativeAABB(stickCenter, stickHalf);

                        // StickのAABB半分の大きさ（XZのみ）
                        const float pickupExpand = 0.5f; // adjust this value to tune pickup ease
                        stickHalf.x += pickupExpand;
                        stickHalf.z += pickupExpand;

                        bool picked = false;
                        // AABB vs AABB quick test in XZ
                        float aMinX = pp.x - playerHalfExt.x;
                        float aMaxX = pp.x + playerHalfExt.x;
                        float aMinZ = pp.z - playerHalfExt.z;
                        float aMaxZ = pp.z + playerHalfExt.z;

                        float bMinX = stickCenter.x - stickHalf.x;
                        float bMaxX = stickCenter.x + stickHalf.x;
                        float bMinZ = stickCenter.z - stickHalf.z;
                        float bMaxZ = stickCenter.z + stickHalf.z;

                        if (!(aMaxX < bMinX || aMinX > bMaxX || aMaxZ < bMinZ || aMinZ > bMaxZ)) {
                            picked = true;
                        } else {
                            //// fallback distance check
                            // const float pickupRadius = 1.5f;
                            // float dx = pp.x - stickCenter.x;
                            // float dy = pp.y - stickCenter.y;
                            // float dz = pp.z - stickCenter.z;
                            // float dist2 = dx*dx + dy*dy + dz*dz;
                            // if (dist2 <= pickupRadius * pickupRadius) picked = true;
                        }

                        if (picked) {
                            // 拾う処理
                            stick_->SetHoldSideFromPlayerPos(pp);
                            stick_->PickUpExternal();
                            stick_->SetHeld(true);
                            // 手のポーズを持っている状態に切り替え
                            if (player_) {
                                Vector3 hoff = stick_->GetHoldOffset();
                                bool sideRight = (hoff.x >= 0.0f);
                                player_->SetHandPoseHeld(sideRight);
                            }
                            Logger::Log("Stick picked up.");
                        }
                    }
                }
            } else {
                // 保持中：プレイヤーがいる場合は目の前に置く。いない場合は現位置に置く。
                Vector3 dropPos;
                if (player_) {
                    Vector3 p = player_->GetPosition();
                    Vector3 off = stick_->GetHoldOffset();
                    dropPos = { p.x + off.x, p.y + off.y - 0.4f, p.z + off.z };
                } else {
                    dropPos = stick_->GetPosition();
                }

                // 地面に接地させ、壁への埋まりを解消
                float hitY;
                if (level_ && level_->RaycastDown({ dropPos.x, dropPos.y + 1.0f, dropPos.z }, 2.0f, hitY)) {
                    dropPos.y = hitY;
                } else {
                    dropPos.y = 0.0f;
                }
                if (level_)
                    level_->ResolveCollision(dropPos, 0.3f, true);

                // ResolveCollision で壁への埋まりを解消
                stick_->SetRotation(stick_->GetHeldRotation());

                // If drop position overlaps the player, try nudging along the hold direction
                // until we find a placeable position (or give up after a few attempts).
                if (player_) {
                    const Vector3& pp = player_->GetPosition();
                    const float minDropDist = 0.9f;

                    Vector3 off = stick_->GetHoldOffset();
                    Vector3 dir = { off.x, 0.0f, off.z };
                    float dlen = std::sqrt(dir.x * dir.x + dir.z * dir.z);
                    if (dlen < 1e-6f) {
                        dir = { 1.0f, 0.0f, 0.0f };
                        dlen = 1.0f;
                    }
                    dir.x /= dlen;
                    dir.z /= dlen;

                    bool placed = false;
                    const int kMaxNudge = 8;
                    for (int i = 0; i < kMaxNudge; ++i) {
                        float step = 0.25f * (i + 1); // increase step each attempt
                        Vector3 trial = { pp.x + dir.x * step, dropPos.y, pp.z + dir.z * step };

                        float hitY2;
                        if (level_ && level_->RaycastDown({ trial.x, trial.y + 1.0f, trial.z }, 2.0f, hitY2)) {
                            trial.y = hitY2;
                        }

                        // make sure not colliding with walls/obbs
                        if (level_)
                            level_->ResolveCollision(trial, 0.3f, true);

                        float dxp = trial.x - pp.x;
                        float dzp = trial.z - pp.z;
                        if (dxp * dxp + dzp * dzp >= minDropDist * minDropDist) {
                            dropPos = trial;
                            placed = true;
                            break;
                        }
                    }

                    // fallback: small offset if nothing found
                    if (!placed) {
                        dropPos.x = pp.x + dir.x * 1.25f;
                        dropPos.z = pp.z + dir.z * 1.25f;
                    }
                }

                
                bool canPlace = true;
                if (npc_) {
                    const Vector3& np = npc_->GetPosition();
                    // compute conservative XZ extents for the stick using its OBB half extents and yaw
                    Vector3 obbCenter, obbHalf;
                    float obbYaw = 0.0f;
                    stick_->GetOBB(obbCenter, obbHalf, obbYaw);
                    float cy = std::cos(obbYaw);
                    float sy = std::sin(obbYaw);
                    float absCy = std::fabs(cy);
                    float absSy = std::fabs(sy);
                    float extX = absCy * obbHalf.x + absSy * obbHalf.z;
                    float extZ = absSy * obbHalf.x + absCy * obbHalf.z;

                    // use dropPos as center for placement test
                    float minX = dropPos.x - extX - 0.1f; // small margin
                    float maxX = dropPos.x + extX + 0.1f;
                    float minZ = dropPos.z - extZ - 0.1f;
                    float maxZ = dropPos.z + extZ + 0.1f;

                    if (np.x >= minX && np.x <= maxX && np.z >= minZ && np.z <= maxZ) {
                        canPlace = false;
                    }
                }

                if (!canPlace) {
                    Logger::Log("Cannot drop: NPC is underneath the intended placement.");
                    // Don't drop; keep held state
                } else {
                    // 位置を反映してから DropExternal を呼ぶ（OBB 登録が最新位置で行われるように）
                    stick_->SetPosition(dropPos);
                    stick_->DropExternal();
                    stick_->SetHeld(false);

                    // 手のポーズを下ろした状態へ
                    if (player_) {
                        player_->SetHandPoseDropped();
                    }

                    Logger::Log("Stick dropped.");
                }
            }
        }
    }

    // --- 4. オブジェクトの状態更新 ---
    if (stick_) {
        // remember previous collision state to detect when collision becomes enabled
        bool prevCollision = stick_->GetCollisionEnabled();
        stick_->Update(1.0f / 60.0f);
        stick_->UpdateCollisionTimer(1.0f / 60.0f);

        // If collision was just enabled (OBB likely registered), ensure player is not
        // overlapping the newly placed stick by resolving player position against level.
        if (!prevCollision && stick_->GetCollisionEnabled() && level_ && player_) {
            Vector3 ppos = player_->GetPosition();
            // use player's approximate radius (matches Player::halfSize_ default)
            const float playerRadius = 0.5f;
            level_->ResolveCollision(ppos, playerRadius, true);
            player_->SetPosition(ppos);
        }
    }
    if (goal_)
        goal_->Update(1.0f / 60.0f);
    //=======================
    // カメラの更新
    //=======================
    const ICamera* activeCamera = nullptr;

    // --- 5. カメラの状態確定 ---
    if (useDebugCamera_) {
        debugCamera_->Update(*engine_->GetInputManager());
        activeCamera = debugCamera_->GetCamera();
    } else {
        if (camera_)
            camera_->Update();
        activeCamera = camera_;
    }

    // レンダラに現在有効なカメラを渡す
    if (forceFixedCamera_) {
        // 固定カメラモードの場合
        if (camera_) {
            camera_->SetRotate({ 0.3f, 0.0f, 0.0f });
            camera_->SetTranslate({ 0.0f, 4.0f, -10.0f });
            camera_->Update();
            engine_->GetObject3dRenderer()->SetDefaultCamera(camera_);
        } else {
            static GameCamera fixed;
            fixed.SetRotate({ 0.3f, 0.0f, 0.0f });
            fixed.SetTranslate({ 0.0f, 4.0f, -10.0f });
            fixed.Update();
            engine_->GetObject3dRenderer()->SetDefaultCamera(&fixed);
        }
    } else {
        engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);
    }

    // --- 6. 掴んでいる棒をプレイヤーに追従させる ---
    if (stick_ && stick_->IsHeld() && player_) {
        const Vector3& pp = player_->GetPosition();
        // decide hold offset based on which side the stick was relative to player
        stick_->SetHoldSideFromPlayerPos(pp);
        Vector3 offset = stick_->GetHoldOffset();
        Vector3 holdPos = { pp.x + offset.x, pp.y + offset.y, pp.z + offset.z };
        // Ensure the stick rotation is the held rotation and that the stick lies horizontally on the held side
        float yaw = (offset.x >= 0.0f) ? 3.14159265f * 0.5f : -3.14159265f * 0.5f;
        // only update position here; heldRotation is set on pickup and adjusted via input (Q/E)
        stick_->SetPosition(holdPos);

        // update player's hand pose every frame to follow held offset
        if (player_) {
            bool sideRight = (offset.x >= 0.0f);
            player_->SetHandPoseHeld(sideRight);
        }

        // Continuous small increments while held (キーボードQ/Eで回転)
        constexpr float kRotateSpeed = 1.5f; // radians per second
        if (engine_->GetInputManager()->IsPressKey(DIK_Q)) {
            stick_->RotateHeldYaw(-kRotateSpeed * (1.0f / 60.0f));
        }
        if (engine_->GetInputManager()->IsPressKey(DIK_E)) {
            stick_->RotateHeldYaw(kRotateSpeed * (1.0f / 60.0f));
        }
        // 日本語: ゲームパッドの右スティックX軸で回転制御（接続されている場合）
        if (engine_->GetInputManager()->Pad(0).IsConnected()) {
            float rx = engine_->GetInputManager()->Pad(0).GetRightX();
            const float kDead = 0.15f; // デッドゾーン
            if (std::fabs(rx) > kDead) {
                // スティックの傾きに比例して回転。速度調整のためにkRotateSpeedを乗算
                stick_->RotateHeldYaw(rx * kRotateSpeed * (1.0f / 60.0f));
            }
        }
    }

#ifdef USE_IMGUI
    // ImGui: stick debug window (scene-local toggle)
    if (showImGui_) {
        ImGui::Begin("Stick Debug");
        if (stick_) {
            const Vector3& sp = stick_->GetPosition();
            const Vector3& sr = stick_->GetRotation();
            ImGui::Text("Stick id: %u", stick_->GetId());
            ImGui::Text("Layer: %u", stick_->GetLayer());
            ImGui::Text("Held: %s", stick_->IsHeld() ? "true" : "false");
            ImGui::Text("Collision enabled: %s", stick_->GetCollisionEnabled() ? "true" : "false");
            ImGui::Separator();
            ImGui::Text("Position");
            ImGui::Text("  x: %.3f", sp.x);
            ImGui::Text("  y: %.3f", sp.y);
            ImGui::Text("  z: %.3f", sp.z);
            ImGui::Separator();
            ImGui::Text("Rotation");
            ImGui::Text("  x: %.3f", sr.x);
            ImGui::Text("  y: %.3f", sr.y);
            ImGui::Text("  z: %.3f", sr.z);
            // Show OBBs
            ImGui::Separator();
            ImGui::Text("Level OBBs:");
            if (level_) {
                const auto& obbs = level_->GetOBBs();
                for (size_t i = 0; i < obbs.size(); ++i) {
                    const auto& o = obbs[i];
                    ImGui::Text("OBB[%zu]: center=(%.3f,%.3f,%.3f) half=(%.3f,%.3f,%.3f) yaw=%.3f ownerId=%u", i, o.center.x, o.center.y, o.center.z, o.halfExtents.x, o.halfExtents.y, o.halfExtents.z, o.yaw, o.ownerId);

                    // Conservative AABB that contains the rotated OBB (useful for broad-phase debug)
                    float cy = std::cos(o.yaw);
                    float sy = std::sin(o.yaw);
                    float absCy = std::fabs(cy);
                    float absSy = std::fabs(sy);

                    // XZ extents after rotation: |R| * halfExtents (only XZ plane rotation)
                    float extX = absCy * o.halfExtents.x + absSy * o.halfExtents.z;
                    float extZ = absSy * o.halfExtents.x + absCy * o.halfExtents.z;

                    float minX = o.center.x - extX;
                    float maxX = o.center.x + extX;
                    float minZ = o.center.z - extZ;
                    float maxZ = o.center.z + extZ;

                    ImGui::Text("  conservativeAABB X=[%.3f, %.3f] Z=[%.3f, %.3f]", minX, maxX, minZ, maxZ);
                }
            }
        } else {
            ImGui::Text("No stick instance");
        }
        if (player_) {
            const Vector3& pp = player_->GetPosition();
            ImGui::Separator();
            ImGui::Text("Player Position: x=%.3f y=%.3f z=%.3f", pp.x, pp.y, pp.z);
        }
        ImGui::End();
    }
#endif

    // --- 7. ゴール(勝利)判定 ---
    if (goal_ && !goalReached_) {
        const Vector3& gp = goal_->GetPosition();

        // Use conservative AABB/XZ check when available to avoid relying on pure distance
        auto checkHit = [&](const Vector3& p) -> bool {
            // If scene-level goal AABB is provided, use XZ overlap + Y tolerance
            if (goalHasAABB_) {
                float minX = gp.x - goalHalf_.x;
                float maxX = gp.x + goalHalf_.x;
                float minZ = gp.z - goalHalf_.z;
                float maxZ = gp.z + goalHalf_.z;
                if (p.x < minX || p.x > maxX)
                    return false;
                if (p.z < minZ || p.z > maxZ)
                    return false;
                // allow some vertical tolerance
                if (std::abs(p.y - gp.y) > goalHalf_.y + 0.5f)
                    return false;
                return true;
            } else {
                // fallback: spherical check on XZ plane (ignore Y)
                const float r = 1.2f;
                float dx = gp.x - p.x;
                float dz = gp.z - p.z;
                return (dx * dx + dz * dz) <= (r * r);
            }
        };

        if (player_) {
            if (checkHit(player_->GetPosition())) {
                goalReached_ = true;
                Logger::Log("Goal reached by Player!");
                SceneManager::GetInstance()->ChangeScene("DEBUG");
            }
        }
        if (!goalReached_ && npc_) {
            if (checkHit(npc_->GetPosition())) {
                goalReached_ = true;
                Logger::Log("Goal reached by NPC!");
                SceneManager::GetInstance()->ChangeScene("DEBUG");
            }
        }
    }
}

// ---------------------------------------------------------
// 描画処理
// ---------------------------------------------------------
void GamePlayScene::Draw()
{
    Draw3D();
    Draw2D();
}

// ---------------------------------------------------------
// 3D描画
// ---------------------------------------------------------
void GamePlayScene::Draw3D()
{
    engine_->Begin3D();

    // 描画順：レベル環境 -> 各種アクター
    if (player_)
        player_->Draw();
    if (level_)
        level_->Draw();
    if (npc_)
        npc_->Draw();
    if (stick_)
        stick_->Draw();
    if (goal_)
        goal_->Draw();
}

// ---------------------------------------------------------
// 2D描画
// ---------------------------------------------------------
void GamePlayScene::Draw2D()
{
    engine_->Begin2D();
    // UIやスコア表示などをここに記述
}