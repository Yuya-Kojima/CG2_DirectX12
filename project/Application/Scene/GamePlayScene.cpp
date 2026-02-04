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

    // 棒(Stick)生成（フォールバック） - single default stick becomes one element in sticks_
    if (sticks_.empty() && !stageHasStick) {
        Vector3 sp = { 0.0f, 0.1f, 2.0f };
        if (player_) {
            const Vector3& pp = player_->GetPosition();
            sp = { pp.x + 0.6f, pp.y + 0.1f, pp.z + 0.6f };
        }
        Stick* s = new Stick();
        s->Initialize(engine_->GetObject3dRenderer(), sp);
        s->SetLevel(level_);
        s->SetLayer(CollisionMask::Item);
        s->SetId(nextId++);
        sticks_.push_back(s);
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
        if (!sticks_.empty()) {
            // use first stick as reference for fallback spawn
            Stick* ref = sticks_[0];
            if (ref) {
                Vector3 sp = ref->GetPosition();
                // 日本語: スティックの少し左（負X方向）にスポーンさせて、左から当てる挙動を見やすくする
                float leftOffset = npcSpawnLeftOffset_;
                npPos.x = sp.x - leftOffset;
                npPos.z = sp.z;
                float groundY = 0.0f;
                if (level_ && level_->RaycastDown({ npPos.x, sp.y + 2.0f, npPos.z }, 4.0f, groundY)) {
                    npPos.y = groundY + 0.5f;
                } else {
                    npPos.y = sp.y + 0.5f;
                }
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
                    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
                    if (!sticks_.empty() && sticks_[0]) { auto p = sticks_[0]->GetPosition(); sx = p.x; sy = p.y; sz = p.z; }
                    Logger::Log(std::format("[Scene] Fallback: spawned Npc pos=({:.2f},{:.2f},{:.2f}) straightDir=({:.2f},{:.2f},{:.2f}) stick=({:.2f},{:.2f},{:.2f})\n",
                        npPos.x, npPos.y, npPos.z, desiredDir.x, desiredDir.y, desiredDir.z, sx, sy, sz));
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
                    // allow multiple sticks: always create and append
                    Stick* s = new Stick();
                    s->Initialize(engine_->GetObject3dRenderer(), o.position);
                    s->SetLevel(level_);
                    s->SetLayer(o.layer);
                    uint32_t id = allocateId(o.id);
                    s->SetId(id);
                    sticks_.push_back(s);
                    Logger::Log("Stage: spawned Stick from JSON");
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
                        // record spawn point from JSON
                        playerSpawn_ = o.position;
                        havePlayerSpawn_ = true;
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
        // Note: fallback hazard spawning removed. Hazards should be provided by stage JSON.
    }

    // --- 天球モデルの用意 ---
    ModelManager::GetInstance()->LoadModel("SkyDome.obj");
    skyObject3d_ = std::make_unique<Object3d>();
    skyObject3d_->Initialize(engine_->GetObject3dRenderer());
    skyObject3d_->SetModel("SkyDome.obj");
    skyObject3d_->SetTranslation({ 0.0f, 0.0f, 0.0f });
    skyObject3d_->SetScale({ skyScale_, skyScale_, skyScale_ });
    skyObject3d_->SetRotation({ 0.0f, 0.0f, 0.0f });
    skyObject3d_->SetEnableLighting(false);
    skyObject3d_->SetColor(Vector4{ 3.0f,3.0f,3.0f,1.0f });

    auto* sm = SoundManager::GetInstance();
    // キー名は "title_bgm" / "title_se"
    // resources配下のファイルを指定
    sm->Load("title_bgm", "resources/sounds/BGM/GameBGM.mp3");
    sm->Load("select_se", "resources/sounds/SE/select.mp3");
    sm->Load("push_se", "resources/sounds/SE/push.mp3");
    // タイトル開始時にBGMをループ再生
    sm->PlayBGM("title_bgm");
}

// ---------------------------------------------------------
// 終了処理
// ---------------------------------------------------------
void GamePlayScene::Finalize()
{
    auto* sm = SoundManager::GetInstance();
    sm->StopBGM();
    sm->StopAllSE();
    // 登録したキーをアンロード
    sm->Unload("title_bgm");
    sm->Unload("push_se");
    sm->Unload("select_se");

    // オブジェクト破棄前に、レベルに登録されたID参照をクリア
    if (level_) {
        if (player_)
            level_->ClearOwnerId(player_->GetId());
        if (npc_)
            level_->ClearOwnerId(npc_->GetId());
        for (auto s : sticks_) {
            if (s) level_->ClearOwnerId(s->GetId());
        }
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
    for (auto s : sticks_) delete s;
    sticks_.clear();
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
    auto* input = engine_->GetInputManager();
    assert(input);

    // オーディオ更新
    SoundManager::GetInstance()->Update();

    // デバッグ用：Enterでシーン遷移
    if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
        SceneManager::GetInstance()->ChangeScene("STAGESELECT");
    }

    // デバッグ用：Rでステージリセット（再読み込み）
    if (engine_->GetInputManager()->IsTriggerKey(DIK_R) || input->IsPadTrigger(PadButton::Start)) {
        try { Logger::Log("Debug: R pressed - resetting stage\n"); } catch(...) {}
        SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
        SoundManager::GetInstance()->PlaySE("push_se");
        return;
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
            // gotoGoal behavior: do NOT automatically steer the NPC toward the goal.
            // The player should influence NPC movement (via stick/interaction) to guide it.
            // Leave NPC straightDir_ unchanged here; keep a safety fallback target if needed.
            if (player_)
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

    // If NPC has returned to spawn after reaching goal, end the level
    if (npc_ && npc_->IsReturning() && npc_->HasReturnedToSpawn() && !goalReached_) {
        goalReached_ = true;
        Logger::Log("Goal reached by NPC (returned to spawn)!");
        // ここで選択中ステージをクリア済みにマーク
        StageSelection::SetCleared(StageSelection::GetSelected(), true);
        SceneManager::GetInstance()->ChangeScene("CLEAR");
    }

    // --- 3. 木の棒(Stick)の拾う・置く処理 ---
    if (input && !sticks_.empty()) {
        if (input->IsTriggerKey(DIK_SPACE) || input->IsPadTrigger(PadButton::A)) {
            // Determine if player currently holds a stick
            Stick* heldStick = nullptr;
            for (auto s : sticks_) { if (s && s->IsHeld()) { heldStick = s; break; } }

            if (!heldStick) {
                // Try to pick nearest eligible stick
                if (player_) {
                    if (player_->IsHandHeld()) {
                        Logger::Log("Cannot pick up: player is already holding an item.");
                    } else {
                        const Vector3& pp = player_->GetPosition();
                        Vector3 playerHalfExt = player_->GetHalfExtents();
                        Stick* best = nullptr; float bestDist2 = FLT_MAX;
                        for (auto s : sticks_) {
                            if (!s || s->IsHeld()) continue;
                            // skip if NPC mounted
                            if (npc_ && npc_->IsMounted() && npc_->GetMountedOwnerId() == s->GetId()) continue;
                            Vector3 sc, sh; s->GetConservativeAABB(sc, sh);
                            const float pickupExpand = 0.5f; sh.x += pickupExpand; sh.z += pickupExpand;
                            float aMinX = pp.x - playerHalfExt.x; float aMaxX = pp.x + playerHalfExt.x;
                            float aMinZ = pp.z - playerHalfExt.z; float aMaxZ = pp.z + playerHalfExt.z;
                            float bMinX = sc.x - sh.x; float bMaxX = sc.x + sh.x;
                            float bMinZ = sc.z - sh.z; float bMaxZ = sc.z + sh.z;
                            if (!(aMaxX < bMinX || aMinX > bMaxX || aMaxZ < bMinZ || aMinZ > bMaxZ)) {
                                float dx = sc.x - pp.x; float dz = sc.z - pp.z; float d2 = dx*dx + dz*dz;
                                if (d2 < bestDist2) { bestDist2 = d2; best = s; }
                            }
                        }
                        if (best) {
                            best->SetHoldSideFromPlayerPos(pp);
                            best->PickUpExternal();
                            best->SetHeld(true);
                            Vector3 hoff = best->GetHoldOffset();
                            player_->SetHandPoseHeld(hoff.x >= 0.0f);
                            Logger::Log("Stick picked up.");
                        }
                    }
                }
            } else {
                // Drop held stick
                Stick* s = heldStick;
                Vector3 dropPos;
                if (player_) {
                    Vector3 p = player_->GetPosition();
                    Vector3 off = s->GetHoldOffset();
                    float pyaw = player_->GetYaw();
                    float cy = std::cos(pyaw); float sy = std::sin(pyaw);
                    Vector3 worldOff; worldOff.x = off.x * cy + off.z * sy; worldOff.z = -off.x * sy + off.z * cy; worldOff.y = off.y;
                    dropPos = { p.x + worldOff.x, p.y + worldOff.y - 0.4f, p.z + worldOff.z };
                } else {
                    dropPos = s->GetPosition();
                }

                float hitY;
                if (level_ && level_->RaycastDown({ dropPos.x, dropPos.y + 1.0f, dropPos.z }, 2.0f, hitY)) dropPos.y = hitY; else dropPos.y = 0.0f;
                if (level_) level_->ResolveCollision(dropPos, 0.3f, true);

                s->SetRotation(s->GetHeldRotation());

                // nudge away from player if overlapping
                if (player_) {
                    const Vector3& pp = player_->GetPosition();
                    const float minDropDist = 0.9f;
                    Vector3 off = s->GetHoldOffset();
                    float pyaw = player_->GetYaw(); float cy = std::cos(pyaw); float sy = std::sin(pyaw);
                    Vector3 dir = { off.x * cy + off.z * sy, 0.0f, -off.x * sy + off.z * cy };
                    float dlen = std::sqrt(dir.x*dir.x + dir.z*dir.z); if (dlen < 1e-6f) { dir = {1,0,0}; dlen = 1.0f; }
                    dir.x /= dlen; dir.z /= dlen;
                    bool placed = false; const int kMaxNudge = 8;
                    for (int i=0;i<kMaxNudge;++i) {
                        float step = 0.25f * (i+1);
                        Vector3 trial = { pp.x + dir.x * step, dropPos.y, pp.z + dir.z * step };
                        float hitY2; if (level_ && level_->RaycastDown({ trial.x, trial.y + 1.0f, trial.z }, 2.0f, hitY2)) trial.y = hitY2;
                        if (level_) level_->ResolveCollision(trial, 0.3f, true);
                        float dxp = trial.x - pp.x; float dzp = trial.z - pp.z; if (dxp*dxp + dzp*dzp >= minDropDist*minDropDist) { dropPos = trial; placed = true; break; }
                    }
                    if (!placed) { dropPos.x = pp.x + dir.x * 1.25f; dropPos.z = pp.z + dir.z * 1.25f; }
                }

                // check placement overlaps
                bool canPlace = true;
                // compute conservative XZ extents for the stick using its OBB half extents and yaw
                Vector3 center; Vector3 half; float yaw=0.0f; s->GetOBB(center, half, yaw);
                float cy = std::cos(yaw); float sy = std::sin(yaw); float absCy = std::fabs(cy); float absSy = std::fabs(sy);
                float extX = absCy * half.x + absSy * half.z; float extZ = absSy * half.x + absCy * half.z;
                if (level_) {
                    Vector3 qmin { dropPos.x - extX - 0.2f, 0.0f, dropPos.z - extZ - 0.2f };
                    Vector3 qmax { dropPos.x + extX + 0.2f, 2.0f, dropPos.z + extZ + 0.2f };
                    std::vector<const Level::OBB*> nearbyObbs; level_->QueryOBBs(qmin, qmax, nearbyObbs);
                    for (const Level::OBB* o : nearbyObbs) {
                        if (o->ownerId == s->GetId()) continue;
                        float oy = o->yaw; float ocy = std::cos(oy); float osy = std::sin(oy);
                        float oabsCy = std::fabs(ocy); float oabsSy = std::fabs(osy);
                        float oExtX = oabsCy * o->halfExtents.x + oabsSy * o->halfExtents.z;
                        float oExtZ = oabsSy * o->halfExtents.x + oabsCy * o->halfExtents.z;
                        float minAx = dropPos.x - extX - 0.05f; float maxAx = dropPos.x + extX + 0.05f;
                        float minAz = dropPos.z - extZ - 0.05f; float maxAz = dropPos.z + extZ + 0.05f;
                        float minBx = o->center.x - oExtX - 0.05f; float maxBx = o->center.x + oExtX + 0.05f;
                        float minBz = o->center.z - oExtZ - 0.05f; float maxBz = o->center.z + oExtZ + 0.05f;
                        if (!(maxAx < minBx || minAx > maxBx || maxAz < minBz || minAz > maxBz)) { canPlace = false; break; }
                    }
                    if (canPlace) {
                        std::vector<const Level::AABB*> nearbyAABBs; level_->QueryWalls(qmin, qmax, nearbyAABBs);
                        for (const Level::AABB* a : nearbyAABBs) {
                            float minAx = dropPos.x - extX - 0.05f; float maxAx = dropPos.x + extX + 0.05f;
                            float minAz = dropPos.z - extZ - 0.05f; float maxAz = dropPos.z + extZ + 0.05f;
                            float minBx = a->min.x - 0.05f; float maxBx = a->max.x + 0.05f;
                            float minBz = a->min.z - 0.05f; float maxBz = a->max.z + 0.05f;
                            if (!(maxAx < minBx || minAx > maxBx || maxAz < minBz || minAz > maxBz)) { canPlace = false; break; }
                        }
                    }
                }
                if (npc_) {
                    const Vector3& np = npc_->GetPosition();
                    float minX = dropPos.x - extX - 0.1f; float maxX = dropPos.x + extX + 0.1f;
                    float minZ = dropPos.z - extZ - 0.1f; float maxZ = dropPos.z + extZ + 0.1f;
                    if (np.x >= minX && np.x <= maxX && np.z >= minZ && np.z <= maxZ) canPlace = false;
                }

                if (!canPlace) {
                    Logger::Log("Cannot drop: placement blocked (NPC/OBJ overlap or wall). ");
                } else {
                    s->SetPosition(dropPos);
                    s->DropExternal();
                    s->SetHeld(false);
                    if (player_) player_->SetHandPoseDropped();
                    Logger::Log("Stick dropped.");
                }
            }
        }
    }

    // --- 4. オブジェクトの状態更新 ---
    // update sticks
    for (auto s : sticks_) {
        if (!s) continue;
        bool prevCollision = s->GetCollisionEnabled();
        s->Update(1.0f / 60.0f);
        s->UpdateCollisionTimer(1.0f / 60.0f);
        if (!prevCollision && s->GetCollisionEnabled() && level_ && player_) {
            Vector3 ppos = player_->GetPosition();
            const float playerRadius = 0.5f;
            level_->ResolveCollision(ppos, playerRadius, true);
            player_->SetPosition(ppos);
        }
    }
    if (goal_)
        goal_->Update(1.0f / 60.0f);
    
    // --- Hazard / Out-of-bounds handling ---
    if (level_) {
        // update last safe player position: record when player is inside XZ bounds and not on hazard
        if (player_) {
            const Vector3& pp = player_->GetPosition();
            bool insideXZ = (pp.x >= level_->GetMinX() && pp.x <= level_->GetMaxX() && pp.z >= level_->GetMinZ() && pp.z <= level_->GetMaxZ());
            if (insideXZ && !level_->IsHazardHit(pp, player_->GetHalfExtents().x)) {
                lastSafePlayerPos_ = pp;
                haveLastSafePlayerPos_ = true;
            }
        }
        // helper: find nearest walkable tile center and its ground Y
        auto findSafeRespawn = [&](const Vector3& ref) -> Vector3 {
            Vector3 fallback = ref;
            const NavigationGrid& nav = level_->GetNavGrid();
            int nx = nav.GetNX();
            int nz = nav.GetNZ();
            float bestDist2 = FLT_MAX;
            Vector3 bestCenter = fallback;
            for (int z = 0; z < nz; ++z) {
                for (int x = 0; x < nx; ++x) {
                    if (!nav.IsWalkable(x, z)) continue;
                    Vector3 c = nav.TileCenter(x, z);
                    float dx = c.x - ref.x;
                    float dz = c.z - ref.z;
                    float d2 = dx*dx + dz*dz;
                    if (d2 < bestDist2) {
                        bestDist2 = d2;
                        bestCenter = c;
                    }
                }
            }
            float hitY = 0.0f;
            if (level_->RaycastDown({ bestCenter.x, ref.y + 2.0f, bestCenter.z }, 4.0f, hitY)) {
                bestCenter.y = hitY;
            } else {
                bestCenter.y = 0.0f;
            }
            return bestCenter;
        };

        // NPC: hazard or XZ out-of-bounds -> reset stage
        if (npc_) {
            const float npcCheckRadius = 0.1f;
            if (level_->IsHazardHit(npc_->GetPosition(), npcCheckRadius)) {
                Logger::Log("NPC hit hazard: resetting stage\n");
                SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
                return;
            }
            const Vector3& np = npc_->GetPosition();
            if (np.x < level_->GetMinX() || np.x > level_->GetMaxX() || np.z < level_->GetMinZ() || np.z > level_->GetMaxZ()) {
                Logger::Log("NPC out of XZ bounds: resetting stage\n");
                SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
                return;
            }
        }

        // Player: hazard -> respawn near safe tile; XZ out-of-bounds -> respawn
        if (player_) {
            if (level_->IsHazardHit(player_->GetPosition(), player_->GetHalfExtents().x)) {
                Logger::Log("Player hit hazard: respawning near safe tile\n");
                Vector3 respawn = player_->GetPosition();
                if (havePlayerSpawn_) respawn = findSafeRespawn(playerSpawn_);
                else respawn = findSafeRespawn(player_->GetPosition());
                player_->SetPosition(respawn);
            }
            const Vector3& pp = player_->GetPosition();
            if (pp.x < level_->GetMinX() || pp.x > level_->GetMaxX() || pp.z < level_->GetMinZ() || pp.z > level_->GetMaxZ()) {
                Logger::Log("Player out of XZ bounds: respawning near safe tile\n");
                Vector3 respawn = player_->GetPosition();
                if (havePlayerSpawn_) respawn = findSafeRespawn(playerSpawn_);
                else respawn = findSafeRespawn(player_->GetPosition());
                player_->SetPosition(respawn);
            }
        }
    }
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

    // --- 6. 掴んでいる棒をプレイヤーに追従させる (複数対応) ---
    if (!sticks_.empty() && player_) {
        for (auto s : sticks_) {
            if (!s) continue;
            if (!s->IsHeld()) continue;
            const Vector3& pp = player_->GetPosition();
            Vector3 offset = s->GetHoldOffset();
            float pyaw = player_->GetYaw();
            float cy = std::cos(pyaw);
            float sy = std::sin(pyaw);
            Vector3 worldOff;
            worldOff.x = offset.x * cy + offset.z * sy;
            worldOff.z = -offset.x * sy + offset.z * cy;
            worldOff.y = offset.y;
            Vector3 holdPos = { pp.x + worldOff.x, pp.y + worldOff.y, pp.z + worldOff.z };
            float playerYaw = player_->GetYaw();
            float sideSign = (offset.x >= 0.0f) ? 1.0f : -1.0f;
            float desiredYaw = playerYaw + sideSign * 3.14159265f * 0.5f;
            s->SetHeldRotation({ 0.0f, desiredYaw, 0.0f });
            s->SetPosition(holdPos);
            if (player_) player_->SetHandPoseHeld(offset.x >= 0.0f);
            constexpr float kRotateSpeed = 1.5f;
            if (engine_->GetInputManager()->IsPressKey(DIK_Q)) s->RotateHeldYaw(-kRotateSpeed * (1.0f / 60.0f));
            if (engine_->GetInputManager()->IsPressKey(DIK_E)) s->RotateHeldYaw(kRotateSpeed * (1.0f / 60.0f));
            if (engine_->GetInputManager()->Pad(0).IsConnected()) {
                float rx = engine_->GetInputManager()->Pad(0).GetRightX();
                const float kDead = 0.15f;
                if (std::fabs(rx) > kDead) s->RotateHeldYaw(rx * kRotateSpeed * (1.0f / 60.0f));
            }
            // only support a single held stick visually for player's hand pose; multiple held sticks are rare
        }
    }


    // 天球の更新
    if (skyObject3d_) {
        const ICamera* activeCam = engine_->GetObject3dRenderer()->GetDefaultCamera();
        if (activeCam) {
            Vector3 camPos = activeCam->GetTranslate();
            skyObject3d_->SetTranslation({ camPos.x, camPos.y, camPos.z });
        }
        skyRotate_ += 0.0005f;
        if (skyRotate_ > 3.14159265f * 2.0f) skyRotate_ -= 3.14159265f * 2.0f;
        skyObject3d_->SetRotation({ 0.0f, skyRotate_, 0.0f });
        skyObject3d_->Update();
    }

#ifdef USE_IMGUI
    // ImGui: stick debug window (scene-local toggle)
    if (showImGui_) {
        ImGui::Begin("Stick Debug");
        // show info for the first stick instance if any
        if (!sticks_.empty() && sticks_[0]) {
            Stick* stick = sticks_[0];
            const Vector3& sp = stick->GetPosition();
            const Vector3& sr = stick->GetRotation();
            ImGui::Text("Stick id: %u", stick->GetId());
            ImGui::Text("Layer: %u", stick->GetLayer());
            ImGui::Text("Held: %s", stick->IsHeld() ? "true" : "false");
            ImGui::Text("Collision enabled: %s", stick->GetCollisionEnabled() ? "true" : "false");
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
                // ここで選択中ステージをクリア済みにマーク
                StageSelection::SetCleared(StageSelection::GetSelected(), true);
                SceneManager::GetInstance()->ChangeScene("CLEAR");
            }
        }
        if (!goalReached_ && npc_) {
            if (checkHit(npc_->GetPosition())) {
                // When NPC reaches the goal, begin its return to spawn instead of
                // immediately ending the level. The scene will transition when
                // npc_->HasReturnedToSpawn() becomes true (checked earlier).
                if (!npc_->IsReturning()) {
                    npc_->BeginReturnToSpawn();
                    try { Logger::Log("NPC reached goal: begin return to spawn\n"); } catch(...) {}
                }
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

    if (skyObject3d_) {
        skyObject3d_->Draw();
    }

    // 描画順：レベル環境 -> 各種アクター
    if (player_)
        player_->Draw();
    if (level_)
        level_->Draw();
    if (npc_)
        npc_->Draw();
    for (auto s : sticks_) if (s) s->Draw();
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