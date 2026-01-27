#include "GamePlayScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Input/InputKeyState.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"
#include "Debug/Logger.h"
#include <format>
#include "Actor/Player.h"
#include "Actor/Level.h"
#include "Actor/Hazard.h"
#include "Actor/Npc.h"
#include "Actor/Stick.h"
#include "Actor/Goal.h"
#include "Physics/CollisionSystem.h"
#include "Stage/StageLoader.h"

void GamePlayScene::Initialize(EngineBase *engine) {

  // 参照をコピー
  engine_ = engine;

  //===========================
  // テクスチャファイルの読み込み
  //===========================

  //===========================
  // オーディオファイルの読み込み
  //===========================

  //===========================
  // スプライト関係の初期化
  //===========================

  //===========================
  // 3Dオブジェクト関係の初期化
  //===========================

  // カメラの生成と初期化
  camera_ = new GameCamera();
  camera_->SetRotate({0.3f, 0.0f, 0.0f});
  camera_->SetTranslate({0.0f, 4.0f, -10.0f});

  // デバッグカメラ
  debugCamera_ = new DebugCamera();
  debugCamera_->Initialize({0.0f, 4.0f, -10.0f});

  // デフォルトカメラのセット
  engine_->GetObject3dRenderer()->SetDefaultCamera(camera_);

  // create player
  player_ = new Player();
  player_->Initialize(engine_->GetInputManager(), engine_->GetObject3dRenderer());
  player_->SetLayer(CollisionMask::Player);
  // Attach the scene camera to the player with smoothing
  player_->AttachCameraImmediate(camera_);
  // position player to match DebugScene visible object (monsterBall) so it is in view
  player_->SetPosition({1.0f, 1.0f, 0.0f});

  // create level - attempt to load from stage JSON
  level_ = new Level();
  Stage::StageData sd;
  // prefer stage file located inside Application/resources if present
  bool loaded = false;
  if (!loaded) loaded = Stage::StageLoader::LoadStage("Application/resources/stages/level01.json", sd);
  if (!loaded) loaded = Stage::StageLoader::LoadStage("resources/stages/level01.json", sd);
  if (loaded) {
    level_->Initialize(engine_->GetObject3dRenderer(), sd.width, sd.height, sd.tileSize);
  } else {
    level_->Initialize(engine_->GetObject3dRenderer(), 16, 16, 1.0f);
  }
  player_->AttachLevel(level_);

  // preload models referenced by stage objects
  if (loaded) {
    for (const auto &o : sd.objects) {
      if (!o.model.empty()) {
        std::string m = ModelManager::ResolveModelPath(o.model);
        ModelManager::GetInstance()->LoadModel(m);
      }
    }
  }

  // spawn objects from stage (simple mapping)
  static uint32_t nextId = 1;
  player_->SetLayer(CollisionMask::Player);
  player_->SetId(nextId++);
  if (loaded) {
    for (const auto &o : sd.objects) {
      std::string cls = o.className;
      if (cls == "Hazard") {
        float radius = (o.scale.x > 0.0f) ? (o.scale.x * 0.5f) : 0.5f;
        level_->AddHazard(o.position, radius);
      } else if (cls == "Goal") {
        if (!goal_) {
          goal_ = new Goal();
          goal_->Initialize(engine_->GetObject3dRenderer(), o.position);
        }
      } else if (cls == "Stick") {
        if (!stick_) {
          stick_ = new Stick();
          stick_->Initialize(engine_->GetObject3dRenderer(), o.position);
          stick_->SetLevel(level_);
          stick_->SetLayer(CollisionMask::Item);
          stick_->SetId(nextId++);
        }
      } else if (cls == "Npc") {
        if (!npc_) {
          npc_ = new Npc();
          npc_->Initialize(engine_->GetObject3dRenderer(), o.position);
          npc_->SetNavGrid(&level_->GetNavGrid());
          npc_->SetLayer(CollisionMask::NPC);
          npc_->SetId(nextId++);
          // apply behavior property if provided in stage
          auto it = o.properties.find("behavior");
          if (it != o.properties.end()) {
            npc_->SetBehavior(it->second);
            try { Logger::Log(std::format("Npc spawn behavior='{}'\n", it->second)); } catch(...) {}
          }
        }
      }
    }

    // tile collision -> create wall AABBs
    if (!sd.collision.empty()) {
      for (int z = 0; z < sd.height; ++z) {
        for (int x = 0; x < sd.width; ++x) {
          int idx = z * sd.width + x;
          if (idx >= 0 && idx < (int)sd.collision.size() && sd.collision[idx] != 0) {
            const auto &nav = level_->GetNavGrid();
            Vector3 center = nav.TileCenter(x, z);
            float hs = sd.tileSize * 0.5f;
            Vector3 min = { center.x - hs, 0.0f, center.z - hs };
            Vector3 max = { center.x + hs, 2.0f, center.z + hs };
            level_->AddWallAABB(min, max, true, 0, 0);
          }
        }
      }
      level_->RebuildWallGrid();
    }
  } else {
    // fallback: create default NPC / Stick / Goal as before
    npc_ = new Npc();
    npc_->Initialize(engine_->GetObject3dRenderer(), { -2.0f, 0.0f, 2.0f });
    npc_->SetNavGrid(&level_->GetNavGrid());
    npc_->SetLayer(CollisionMask::NPC);
    npc_->SetId(nextId++);
    stick_ = new Stick();
    stick_->Initialize(engine_->GetObject3dRenderer(), { 0.0f, 0.0f, 2.0f });
    stick_->SetLevel(level_);
    stick_->SetLayer(CollisionMask::Item);
    stick_->SetId(nextId++);
    goal_ = new Goal();
    goal_->Initialize(engine_->GetObject3dRenderer(), { 8.0f, 0.0f, 0.0f });
  }

  // モデルの読み込み

  // オブジェクトの生成と初期化

  //===========================
  // パーティクル関係の初期化
  //===========================
}

void GamePlayScene::Finalize() {
  // clear owner ids before deleting actors to avoid stale references in level
  if (level_) {
    if (player_) level_->ClearOwnerId(player_->GetId());
    if (npc_) level_->ClearOwnerId(npc_->GetId());
    if (stick_) level_->ClearOwnerId(stick_->GetId());
  }

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

void GamePlayScene::Update() {

  // Sound更新
  SoundManager::GetInstance()->Update();

  // タイトルシーンへ移行
  if (engine_->GetInputManager()->IsTriggerKey(DIK_RETURN)) {
    SceneManager::GetInstance()->ChangeScene("DEBUG");
  }

  // デバッグカメラ切り替え
  if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
    if (useDebugCamera_) {
      useDebugCamera_ = false;
    } else {
      useDebugCamera_ = true;
    }
  }

  //=======================
  // スプライトの更新
  //=======================

  //=======================
  // 3Dオブジェクトの更新
  //=======================

  // カメラ更新はプレイヤーの Update() 後に行う（プレイヤーがカメラを制御するため）

  // update player
  if (player_) player_->Update(1.0f / 60.0f);
  if (level_) level_->Update(1.0f / 60.0f);
  // NPC target selection: behavior from stage properties can override default
  if (npc_) {
    Vector3 targetPos = {0.0f, 0.0f, 0.0f};
    const std::string &beh = npc_->GetBehavior();
    if (beh == "followPlayer") {
      if (player_) targetPos = player_->GetPosition();
    } else if (beh == "gotoGoal") {
      if (goal_) targetPos = goal_->GetPosition();
      else if (player_) targetPos = player_->GetPosition();
    } else if (beh == "idle") {
      targetPos = npc_->GetPosition();
    } else {
      // default: prefer goal position if present, otherwise follow player
      if (player_) targetPos = player_->GetPosition();
      if (goal_) targetPos = goal_->GetPosition();
    }
    npc_->Update(1.0f / 60.0f, targetPos, *level_);
  }
  // handle pickup / drop input (E)
  auto input = engine_->GetInputManager();
  if (input && stick_) {
    // trigger key pressed this frame
    if (input->IsTriggerKey(DIK_E)) {
      // if not held, try pickup when close enough
      if (!stick_->IsHeld()) {
        const Vector3 &pp = player_->GetPosition();
        const Vector3 &sp = stick_->GetPosition();
        float dx = pp.x - sp.x;
        float dy = pp.y - sp.y;
        float dz = pp.z - sp.z;
        float dist2 = dx*dx + dy*dy + dz*dz;
        const float pickupRadius = 1.5f;
        if (dist2 <= pickupRadius*pickupRadius) {
          stick_->PickUpExternal();
          stick_->SetHeld(true);
          Logger::Log("Stick picked up.");
        }
        } else {
        // drop in front of player
        stick_->DropExternal();
        stick_->SetHeld(false);
        Vector3 p = player_->GetPosition();
        // simple forward offset
        Vector3 dropPos = { p.x + 0.6f, p.y + 0.3f, p.z + 0.6f };
        // snap to ground using level raycast
        float hitY;
        if (level_ && level_->RaycastDown({ dropPos.x, dropPos.y + 1.0f, dropPos.z }, 2.0f, hitY)) {
          dropPos.y = hitY;
        } else {
          dropPos.y = 0.0f;
        }
        stick_->SetPosition(dropPos);
        // after placing ensure no penetration with walls
        if (level_) level_->ResolveCollision(dropPos, 0.3f, true);
        stick_->SetPosition(dropPos);
        Logger::Log("Stick dropped.");
      }
    }
  }
  if (stick_) stick_->Update(1.0f / 60.0f);
  // update stick collision timer to re-enable collision after drop
  if (stick_) stick_->UpdateCollisionTimer(1.0f / 60.0f);
  // update stick collision timer to re-enable collision after drop
  if (stick_) stick_->UpdateCollisionTimer(1.0f / 60.0f);
  if (goal_) goal_->Update(1.0f / 60.0f);

  //=======================
  // カメラの更新（プレイヤー更新の後に行う）
  //=======================
  GameCamera *activeCamera = nullptr;

  if (useDebugCamera_) {
    debugCamera_->Update(*engine_->GetInputManager());
    activeCamera = debugCamera_->GetCamera();
  } else {
    // update scene camera state from stored values (player may have attached/modified it)
    if (camera_) camera_->Update();
    activeCamera = camera_;
  }

  // debug: log camera/player positions to help diagnose camera jumps
  if (activeCamera) {
    try {
      const Vector3 &cpos = activeCamera->GetTranslate();
      const Vector3 &crot = activeCamera->GetRotate();
      Vector3 ppos = {0.0f, 0.0f, 0.0f};
      if (player_) ppos = player_->GetPosition();
      Logger::Log(std::format("[CameraDebug] cam=({:.3f},{:.3f},{:.3f}) rot=({:.3f},{:.3f},{:.3f}) player=({:.3f},{:.3f},{:.3f})\n",
                              cpos.x, cpos.y, cpos.z,
                              crot.x, crot.y, crot.z,
                              ppos.x, ppos.y, ppos.z));
    } catch(...) {}
  }

  // アクティブカメラを描画で使用する
  if (forceFixedCamera_) {
    // Use the scene camera if available (persistent), otherwise use a static
    // fallback camera. Previously we used a local automatic GameCamera which
    // produced a dangling pointer when Draw() ran — avoid that.
    if (camera_) {
      camera_->SetRotate({0.3f, 0.0f, 0.0f});
      camera_->SetTranslate({0.0f, 4.0f, -10.0f});
      camera_->Update();
      engine_->GetObject3dRenderer()->SetDefaultCamera(camera_);
    } else {
      static GameCamera fixed;
      fixed.SetRotate({0.3f, 0.0f, 0.0f});
      fixed.SetTranslate({0.0f, 4.0f, -10.0f});
      fixed.Update();
      engine_->GetObject3dRenderer()->SetDefaultCamera(&fixed);
    }
  } else {
    engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);
  }

  // if stick is held, follow the player (hold offset)
  if (stick_ && stick_->IsHeld() && player_) {
    const Vector3 &pp = player_->GetPosition();
    Vector3 holdPos = { pp.x + 0.5f, pp.y + 1.0f, pp.z }; // right/above player
    stick_->SetPosition(holdPos);
    stick_->SetRotation({ -0.6f, 0.0f, 0.5f });
  }

  // goal check
  if (goal_ && player_) {
    const Vector3 &gp = goal_->GetPosition();
    const Vector3 &pp = player_->GetPosition();
    float dx = gp.x - pp.x; float dy = gp.y - pp.y; float dz = gp.z - pp.z;
    float d2 = dx*dx + dy*dy + dz*dz;
    const float goalRadius = 1.2f;
    if (d2 <= goalRadius*goalRadius && !goalReached_) {
      goalReached_ = true;
      Logger::Log("Goal reached!");
      // small delay could be added; for now change scene once
      SceneManager::GetInstance()->ChangeScene("DEBUG");
    }
  }

  // NPC reaching goal should also satisfy goal check
  if (goal_ && npc_) {
    const Vector3 &gp = goal_->GetPosition();
    const Vector3 &np = npc_->GetPosition();
    float dx = gp.x - np.x; float dy = gp.y - np.y; float dz = gp.z - np.z;
    float d2 = dx*dx + dy*dy + dz*dz;
    const float goalRadius = 1.2f;
    if (d2 <= goalRadius*goalRadius && !goalReached_) {
      goalReached_ = true;
      Logger::Log("Goal reached by NPC!");
      SceneManager::GetInstance()->ChangeScene("DEBUG");
    }
  }
}

void GamePlayScene::Draw() {
  Draw3D();
  Draw2D();
}

void GamePlayScene::Draw3D() {
  engine_->Begin3D();

  // ここから下で3DオブジェクトのDrawを呼ぶ
  if (player_) player_->Draw();
  if (level_) level_->Draw();
  if (npc_) npc_->Draw();
  if (stick_) stick_->Draw();
  if (goal_) goal_->Draw();
}

void GamePlayScene::Draw2D() {
  engine_->Begin2D();

  // ここから下で2DオブジェクトのDrawを呼ぶ
}