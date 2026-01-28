#include "GamePlayScene.h"
#include "Actor/Goal.h"
#include "Actor/Hazard.h"
#include "Actor/Level.h"
#include "Actor/Npc.h"
#include "Actor/Player.h"
#include "Actor/Stick.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Debug/Logger.h"
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
#include <algorithm>
#include <filesystem>
#include <format>

// ---------------------------------------------------------
// 初期化
// ---------------------------------------------------------
void GamePlayScene::Initialize(EngineBase* engine)
{
    // エンジン本体への参照を保持
    engine_ = engine;

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
    if (!loaded)
        loaded = Stage::StageLoader::LoadStage("Application/resources/stages/level01.json", sd);
    if (!loaded)
        loaded = Stage::StageLoader::LoadStage("resources/stages/level01.json", sd);

    if (loaded) {
        // ロード成功時はJSON内のグリッド設定で初期化
        level_->Initialize(engine_->GetObject3dRenderer(), sd.width, sd.height, sd.tileSize);
    } else {
        // ロード失敗時はデフォルト設定で初期化
        level_->Initialize(engine_->GetObject3dRenderer(), 16, 16, 1.0f);
    }

    // --- フォールバック生成 (シーン側で個別初期化を行う) ---
    static uint32_t nextId = 1;

    // プレイヤー生成（フォールバック）
    if (!player_) {
        player_ = new Player();
        player_->Initialize(engine_->GetInputManager(), engine_->GetObject3dRenderer());
        player_->SetLayer(CollisionMask::Player);
        player_->AttachCameraImmediate(camera_);
        player_->SetId(nextId++);
        player_->AttachLevel(level_);
        Logger::Log("[Scene] Fallback: spawned Player\n");
    }

    // 棒(Stick)生成（フォールバック）
    if (!stick_) {
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
    if (!goal_) {
        goal_ = new Goal();
        goal_->Initialize(engine_->GetObject3dRenderer(), { 8.0f, 0.0f, 0.0f });
        Logger::Log("[Scene] Fallback: spawned Goal\n");
    }

    // --- ステージ情報のログ出力 ---
    if (loaded) {
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

    // デバッグ用：Enterでタイトル(DEBUGシーン)へ
    if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
        SceneManager::GetInstance()->ChangeScene("DEBUG");
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
        if (input->IsTriggerKey(DIK_SPACE)) {
            if (!stick_->IsHeld()) {
                // 未保持：プレイヤーが存在する場合のみ距離をチェックして拾う
                if (player_) {
                    const Vector3& pp = player_->GetPosition();
                    const Vector3& sp = stick_->GetPosition();
                    float dx = pp.x - sp.x;
                    float dy = pp.y - sp.y;
                    float dz = pp.z - sp.z;
                    float dist2 = dx * dx + dy * dy + dz * dz;
                    const float pickupRadius = 1.5f;

                    if (dist2 <= pickupRadius * pickupRadius) {
                        // decide which side to hold based on current relative position
                        stick_->SetHoldSideFromPlayerPos(pp);
                        stick_->PickUpExternal();
                        stick_->SetHeld(true);
                        Logger::Log("Stick picked up.");
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

                // Ensure the stick rotation uses the held rotation when dropping
                stick_->SetRotation(stick_->GetHeldRotation());

                // If drop position overlaps the player, try nudging along the hold direction
                // until we find a placeable position (or give up after a few attempts).
                if (player_) {
                    const Vector3& pp = player_->GetPosition();
                    const float minDropDist = 0.9f; // minimum allowed distance to player

                    // Direction to push in XZ plane: prefer the hold offset direction
                    Vector3 off = stick_->GetHoldOffset();
                    Vector3 dir = { off.x, 0.0f, off.z };
                    float dlen = std::sqrt(dir.x*dir.x + dir.z*dir.z);
                    if (dlen < 1e-6f) {
                        dir = { 1.0f, 0.0f, 0.0f };
                        dlen = 1.0f;
                    }
                    dir.x /= dlen; dir.z /= dlen;

                    bool placed = false;
                    const int kMaxNudge = 8;
                    for (int i = 0; i < kMaxNudge; ++i) {
                        float step = 0.25f * (i + 1); // increase step each attempt
                        Vector3 trial = { pp.x + dir.x * step, dropPos.y, pp.z + dir.z * step };

                        // snap to ground
                        float hitY2;
                        if (level_ && level_->RaycastDown({ trial.x, trial.y + 1.0f, trial.z }, 2.0f, hitY2)) {
                            trial.y = hitY2;
                        }

                        // make sure not colliding with walls/obbs
                        if (level_)
                            level_->ResolveCollision(trial, 0.3f, true);

                        float dxp = trial.x - pp.x;
                        float dzp = trial.z - pp.z;
                        if (dxp*dxp + dzp*dzp >= minDropDist * minDropDist) {
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

                // 位置を反映してから DropExternal を呼ぶ（OBB 登録が最新位置で行われるように）
                stick_->SetPosition(dropPos);
                stick_->DropExternal();
                stick_->SetHeld(false);

                Logger::Log("Stick dropped.");
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
  const ICamera *activeCamera = nullptr;

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
        // Keep the stick's rotation as the heldRotation (which was set on pickup to preserve lay angle)
        stick_->SetPosition(holdPos);
        stick_->SetRotation(stick_->GetRotation());

        // Rotate held stick in discrete 15-degree steps on Q/E (triggered once per press)
        constexpr float k15deg = 3.14159265f * (15.0f / 180.0f); // radians
        if (engine_->GetInputManager()->IsTriggerKey(DIK_Q)) {
            stick_->AdjustHeldYaw(-k15deg);
        }
        if (engine_->GetInputManager()->IsTriggerKey(DIK_E)) {
            stick_->AdjustHeldYaw(k15deg);
        }
    }

#ifdef USE_IMGUI
    // ImGui: stick debug window
    if (true) {
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
        const float goalRadius = 1.2f;
        const Vector3& gp = goal_->GetPosition();

        // プレイヤーがゴールに接触
        if (player_) {
            const Vector3& pp = player_->GetPosition();
            float dx = gp.x - pp.x;
            float dy = gp.y - pp.y;
            float dz = gp.z - pp.z;
            if ((dx * dx + dy * dy + dz * dz) <= goalRadius * goalRadius) {
                goalReached_ = true;
                Logger::Log("Goal reached by Player!");
                SceneManager::GetInstance()->ChangeScene("DEBUG");
            }
        }
        // NPCがゴールに接触 (NPC救出目標などの想定)
        if (!goalReached_ && npc_) {
            const Vector3& np = npc_->GetPosition();
            float dx = gp.x - np.x;
            float dy = gp.y - np.y;
            float dz = gp.z - np.z;
            if ((dx * dx + dy * dy + dz * dz) <= goalRadius * goalRadius) {
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