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
#include <algorithm>
#include <filesystem>
#include <format>
#include "Scene/StageSelection.h"

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
    const std::string selected = StageSelection::GetSelected();
    if (!loaded)
        loaded = Stage::StageLoader::LoadStage("Application/resources/stages/" + selected, sd);
    if (!loaded)
        loaded = Stage::StageLoader::LoadStage("resources/stages/" + selected, sd);

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
        if (input->IsTriggerKey(DIK_E)) {
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
                        stick_->PickUpExternal();
                        stick_->SetHeld(true);
                        Logger::Log("Stick picked up.");
                    }
                }
            } else {
                // 保持中：プレイヤーがいる場合は目の前に置く。いない場合は現位置に置く。
                stick_->DropExternal();
                stick_->SetHeld(false);
                Vector3 dropPos;
                if (player_) {
                    Vector3 p = player_->GetPosition();
                    dropPos = { p.x + 0.6f, p.y + 0.3f, p.z + 0.6f };
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
                stick_->SetPosition(dropPos);
                Logger::Log("Stick dropped.");
            }
        }
    }

    // --- 4. オブジェクトの状態更新 ---
    if (stick_) {
        stick_->Update(1.0f / 60.0f);
        stick_->UpdateCollisionTimer(1.0f / 60.0f);
    }
    if (goal_)
        goal_->Update(1.0f / 60.0f);

    // --- 5. カメラの状態確定 ---
    GameCamera* activeCamera = nullptr;
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
        Vector3 holdPos = { pp.x + 0.5f, pp.y + 1.0f, pp.z };
        stick_->SetPosition(holdPos);
        stick_->SetRotation({ -0.6f, 0.0f, 0.5f });
    }

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