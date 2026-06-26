#include "Actor/Player.h"
#include "Actor/LockOn.h"
#include "Actor/HomingBullet.h"
#include "Actor/NormalBullet.h"
#include "Framework/ActorManager.h"
#include "Camera/ICamera.h"
#include "Collision/CollisionManager.h"
#include "Collision/SphereCollider.h"
#include "Input/Input.h"
#include "Sprite/Sprite.h"
#include "Render/Object3d/Object3d.h"
#include "Math/MathUtil.h"
#include "Debug/Logger.h"
#include <Windows.h>
#include <algorithm>
#include "Actor/Enemy.h"
#include "Collision/CollisionConfig.h"
#include "Math/Geometry.h"
#include <fstream>
#include <filesystem>
#include "../../externals/nlohmann/json.hpp"

Player::Player(const ICamera* camera) : camera_(camera) {
  assert(camera_);
}
Player::~Player() {
  if (collider_) {
    CollisionManager::GetInstance()->Remove(collider_.get());
  }
}

void Player::Initialize() {
  // プレイヤー自身の初期化処理
  reticlePosition_ = {1280.0f / 2.0f, 720.0f / 2.0f};

  // ロックオン機能の生成と初期化
  if (spriteRenderer_) {
    lockOn_ = std::make_unique<LockOn>();
    lockOn_->Initialize(spriteRenderer_);

    // メイン照準カーソル
    // 外枠 
    for (int i = 0; i < 4; ++i) {
      auto line = std::make_unique<Sprite>();
      line->Initialize(spriteRenderer_, "resources/white1x1.png");
      line->SetSize({40.0f, 2.0f}); // 長さ40、太さ2
      line->SetColor({1.0f, 0.5f, 0.0f, 0.8f}); // オレンジ
      line->SetAnchorPoint({0.5f, 0.5f}); // 中心をアンカーに
      reticleOuterSprites_.push_back(std::move(line));
    }
    // 内枠
    for (int i = 0; i < 4; ++i) {
      auto line = std::make_unique<Sprite>();
      line->Initialize(spriteRenderer_, "resources/white1x1.png");
      line->SetSize({25.0f, 2.0f}); // 長さ25、太さ2
      line->SetColor({1.0f, 1.0f, 0.0f, 0.9f}); // 黄色
      line->SetAnchorPoint({0.5f, 0.5f}); // 中心をアンカーに
      reticleInnerSprites_.push_back(std::move(line));
    }
  }

  // コライダーの初期化
  collider_ = std::make_unique<SphereCollider>(this);
  collider_->SetRadius(1.0f); // プレイヤーの当たり判定の大きさ
  collider_->SetAttribute(kCollisionAttributePlayer); // 自分は「自機」
  collider_->SetMask(kCollisionAttributeEnemy |
                     kCollisionAttributeEnemyBullet); // 敵や敵の弾と当たる
  CollisionManager::GetInstance()->Register(collider_.get());

  // アクション設定の読み込み
  LoadActionConfig();
}

void Player::Update() {
  if (!object3d_) return;

  // プレイヤー自身の更新処理

  // 照準の移動操作
  if (input_) {
#ifdef USE_IMGUI
    // デバッグキー：Kキーでダメージを受ける
    if (input_->IsTriggerKey(DIK_K)) {
      TakeDamage(1);
    }
#endif

    float moveSpeed = 10.0f;
    
    // キーボードと十字キー入力
    if (input_->IsPressKey(DIK_W) || input_->IsPadPress(PadButton::DPadUp)) {
      reticlePosition_.y -= moveSpeed;
    }
    if (input_->IsPressKey(DIK_S) || input_->IsPadPress(PadButton::DPadDown)) {
      reticlePosition_.y += moveSpeed;
    }
    if (input_->IsPressKey(DIK_A) || input_->IsPadPress(PadButton::DPadLeft)) {
      reticlePosition_.x -= moveSpeed;
    }
    if (input_->IsPressKey(DIK_D) || input_->IsPadPress(PadButton::DPadRight)) {
      reticlePosition_.x += moveSpeed;
    }

    // 左スティック入力の加算 (上が正、画面座標Yは下が正なのでマイナス)
    reticlePosition_.x += input_->Pad().GetLeftX() * moveSpeed;
    reticlePosition_.y -= input_->Pad().GetLeftY() * moveSpeed;

    // 画面外に出ないようにクランプ
    reticlePosition_.x = std::clamp(reticlePosition_.x, 0.0f, 1280.0f);
    reticlePosition_.y = std::clamp(reticlePosition_.y, 0.0f, 720.0f);
  }

  // 照準スプライト（多重レティクル）の更新
  reticleOuterRot_ += 0.03f; // 外枠はゆっくり右回転

  Transform defaultUV;
  defaultUV.scale = {1.0f, 1.0f, 1.0f};
  defaultUV.rotate = {0.0f, 0.0f, 0.0f};
  defaultUV.translate = {0.0f, 0.0f, 0.0f};

  // 視界行列と投影行列の取得
  Matrix4x4 viewProj = Multiply(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
  
  // 自機のワールド座標を取得してスクリーン座標に変換
  Matrix4x4 playerWorld = object3d_->GetWorldMatrix();
  Vector3 playerPos = {playerWorld.m[3][0], playerWorld.m[3][1], playerWorld.m[3][2]};
  Vector2 playerScreenPos = WorldToScreen(playerPos, viewProj, 1280.0f, 720.0f);

  float ix = reticlePosition_.x;
  float iy = reticlePosition_.y;

  float t = 0.85f; 
  float ox = Lerp(playerScreenPos.x, ix, t);
  float oy = Lerp(playerScreenPos.y, iy, t);
  
  // 外枠のサイズを大幅に大きくし、分離しても重なるようにする
  float outerSize = 50.0f;
  
  for (int i = 0; i < 4; ++i) {
    float angle = reticleOuterRot_ + (i * 3.14159265f / 2.0f);
    
    float posX = ox + std::sin(angle) * outerSize;
    float posY = oy - std::cos(angle) * outerSize;
    
    reticleOuterSprites_[i]->SetPosition({posX, posY});
    reticleOuterSprites_[i]->SetRotation(angle);
    reticleOuterSprites_[i]->Update(defaultUV);
  }

  // 内枠の更新（十字キー型）
  float innerSize = 15.0f;
  
  reticleInnerSprites_[0]->SetPosition({ix, iy - innerSize}); // 上
  reticleInnerSprites_[0]->SetRotation(3.141592f / 2.0f);
  reticleInnerSprites_[1]->SetPosition({ix, iy + innerSize}); // 下
  reticleInnerSprites_[1]->SetRotation(3.141592f / 2.0f);
  reticleInnerSprites_[2]->SetPosition({ix - innerSize, iy}); // 左
  reticleInnerSprites_[2]->SetRotation(0.0f);
  reticleInnerSprites_[3]->SetPosition({ix + innerSize, iy}); // 右
  reticleInnerSprites_[3]->SetRotation(0.0f);

  for (int i = 0; i < 4; ++i) {
    reticleInnerSprites_[i]->Update(defaultUV);
  }

  // 自機の追従と姿勢制御 
  // レティクルのNDCを計算 
  float ndcX = (reticlePosition_.x / 1280.0f) * 2.0f - 1.0f;
  float ndcY = 1.0f - (reticlePosition_.y / 720.0f) * 2.0f;

  // カメラ空間のベクトルを取得
  Matrix4x4 viewMatrix = camera_->GetViewMatrix();
  Matrix4x4 cameraWorld = Inverse(viewMatrix);
  Vector3 cameraPos = {cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2]};
  Vector3 cameraRight = {cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2]};
  Vector3 cameraUp = {cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2]};
  Vector3 cameraForward = {cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2]};

  //  自機の目標座標を計算 (カメラから一定距離前方に配置)
  // カメラからの距離
  float distance = 10.0f; 
  
  //自機の移動範囲をレティクルより狭くする（画面内に収める）
  float playerMaxMoveX = 3.0f; // 
  float playerMaxMoveY = 1.5f; // 
  
  //自機の基本位置をレティクル中心より下に設定する
  float playerBaseOffsetY = -2.0f;
  
  Vector3 targetPos = cameraPos 
                    + Vector3{cameraForward.x * distance, cameraForward.y * distance, cameraForward.z * distance}
                    + Vector3{cameraRight.x * (ndcX * playerMaxMoveX), cameraRight.y * (ndcX * playerMaxMoveX), cameraRight.z * (ndcX * playerMaxMoveX)}
                    + Vector3{cameraUp.x * (ndcY * playerMaxMoveY + playerBaseOffsetY), cameraUp.y * (ndcY * playerMaxMoveY + playerBaseOffsetY), cameraUp.z * (ndcY * playerMaxMoveY + playerBaseOffsetY)};

  //  弾の実際の目標地点を計算し、自機をそちらに向かせる
  float bulletTargetDist = 1000.0f;
  float bulletMaxMoveY = bulletTargetDist * std::tan(45.0f * 3.14159265f / 180.0f / 2.0f);
  float bulletMaxMoveX = bulletMaxMoveY * (1280.0f / 720.0f);
  Vector3 bulletTargetPos = cameraPos
    + Vector3{cameraForward.x * bulletTargetDist, cameraForward.y * bulletTargetDist, cameraForward.z * bulletTargetDist}
    + Vector3{cameraRight.x * (ndcX * bulletMaxMoveX), cameraRight.y * (ndcX * bulletMaxMoveX), cameraRight.z * (ndcX * bulletMaxMoveX)}
    + Vector3{cameraUp.x * (ndcY * bulletMaxMoveY), cameraUp.y * (ndcY * bulletMaxMoveY), cameraUp.z * (ndcY * bulletMaxMoveY)};

  // 自機を目標座標へ遅延追従 
  Vector3 prevPos = transform_.translate;
  
  // カメラのワープ時はLerpせずに瞬時にスナップする
  Vector3 diff = {targetPos.x - transform_.translate.x, targetPos.y - transform_.translate.y, targetPos.z - transform_.translate.z};
  float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
  if (distSq > 100.0f) { // 距離が10以上の場合はワープと判定
    transform_.translate = targetPos;
  } else {
    transform_.translate = Lerp(transform_.translate, targetPos, 0.15f);
  }

  // 姿勢制御 
  Vector3 velocity = {transform_.translate.x - prevPos.x, 
                      transform_.translate.y - prevPos.y, 
                      transform_.translate.z - prevPos.z};
  
  // 速度をカメラローカルに変換してバンク・ピッチを計算
  float localVelX = Dot(velocity, cameraRight);
  float localVelY = Dot(velocity, cameraUp);

  float camRotY = std::atan2(cameraForward.x, cameraForward.z);
  float xzLen = std::sqrt(cameraForward.x * cameraForward.x + cameraForward.z * cameraForward.z);
  float camRotX = std::atan2(-cameraForward.y, xzLen);

  //  自機から弾の目標地点へのベクトル
  Vector3 toTarget = {bulletTargetPos.x - transform_.translate.x,
                      bulletTargetPos.y - transform_.translate.y,
                      bulletTargetPos.z - transform_.translate.z};
  float aimYaw = std::atan2(toTarget.x, toTarget.z);
  float xzLenToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.z * toTarget.z);
  float aimPitch = std::atan2(-toTarget.y, xzLenToTarget);

  // 速度に応じて機体を傾ける
  // ピッチとヨーの基本姿勢を、カメラ正面ではなく照準の方向にする
  float targetRoll = -localVelX * 2.0f; 
  float targetPitch = aimPitch + localVelY * 1.5f; 
  float targetYaw = aimYaw + localVelX * 0.8f; 

  // Lerpで回転を滑らかに
  transform_.rotate.z = Lerp(transform_.rotate.z, targetRoll, 0.1f);
  transform_.rotate.x = Lerp(transform_.rotate.x, targetPitch, 0.1f);
  transform_.rotate.y = Lerp(transform_.rotate.y, targetYaw, 0.1f);

  UpdateTransform();

  // 射撃入力とステート管理
  if (input_) {
    // スペースキーまたはパッドのRBを射撃/ロックオンボタンとする
    bool isTrigger = input_->IsTriggerKey(DIK_SPACE) || input_->IsPadTrigger(PadButton::RB);
    bool isPress = input_->IsPressKey(DIK_SPACE) || input_->IsPadPress(PadButton::RB);
    bool isRelease = input_->IsReleaseKey(DIK_SPACE) || input_->IsPadRelease(PadButton::RB);

    float deltaTime = 1.0f / 60.0f; // 固定フレームレート前提

    if (isTrigger) {
      attackState_ = AttackState::Pressing;
      pressTimer_ = 0.0f;
    }

    if (attackState_ == AttackState::Pressing) {
      if (isPress) {
        pressTimer_ += deltaTime;
        if (pressTimer_ >= 0.15f) { // 0.15秒以上押したらロックオンモードへ
          attackState_ = AttackState::LockOn;
        }
      }
    }

    if (isRelease) {
      if (attackState_ == AttackState::Pressing) {
        // 短押し：通常ショット発射！
        FireNormalShot();
      } else if (attackState_ == AttackState::LockOn) {
        // ロックオンモード終了：ホーミング弾を一斉発射！
        FireHomingShot();
      }
      attackState_ = AttackState::Idle;
      if (lockOn_) {
        lockOn_->Clear(); // ロックオン状態を解除
      }
    }
  }

  // ロックオンの更新処理
  if (lockOn_) {
    bool isLockOnMode = (attackState_ == AttackState::LockOn);
    lockOn_->Update(enemies_, camera_->GetViewProjectionMatrix(), reticlePosition_, 
                    isLockOnMode, actionConfig_.lockOnRadius);
  }
}

bool Player::IsLockOnMode() const {
  return attackState_ == AttackState::LockOn;
}

void Player::FireHomingShot() {
  if (!lockOn_ || !object3dRenderer_ || !object3d_) return;

  const auto& targets = lockOn_->GetTargets();
  if (targets.empty()) return;

  // プレイヤーの現在位置を弾の始点とする
  Vector3 startPos = object3d_->GetTranslation();

  // ロックオンしている敵すべてに対して弾を発射
  for (size_t i = 0; i < targets.size(); ++i) {
    auto bullet = std::make_unique<HomingBullet>();
    
    // 発射時のばらつき（初速ベクトル）を作る
    // 程よい山なりにするための初速
    float spreadX = ((float)i - (float)targets.size() / 2.0f) * 0.3f;
    Vector3 initialVelocity = {spreadX, 0.6f, 0.8f}; // 前方に強めに、上には少しだけ打ち上げる

    bullet->Initialize(object3dRenderer_, startPos, targets[i], initialVelocity);
    
    // アクションエディタのパラメータを適用
    bullet->SetHomingParams(
      actionConfig_.homingSpeed, 
      actionConfig_.homingFallTime, 
      actionConfig_.homingStrengthIncrease, 
      actionConfig_.homingStrengthMax
    );

    // ActorManagerに弾を登録して、自動でUpdate・Drawされるようにする
    ActorManager::GetInstance()->AddActor(std::move(bullet));
  }
}

void Player::FireNormalShot() {
  if (!object3dRenderer_ || !object3d_) return;

  // プレイヤーのワールド座標を弾の始点とする
  Matrix4x4 playerWorld = object3d_->GetWorldMatrix();
  Vector3 startPos = {playerWorld.m[3][0], playerWorld.m[3][1], playerWorld.m[3][2]};

  // レティクルのNDC座標を計算 (-1.0 ~ 1.0)
  float ndcX = (reticlePosition_.x / 1280.0f) * 2.0f - 1.0f;
  float ndcY = 1.0f - (reticlePosition_.y / 720.0f) * 2.0f;

  // カメラ空間のベクトルを取得
  Matrix4x4 viewMatrix = camera_->GetViewMatrix();
  Matrix4x4 cameraWorld = Inverse(viewMatrix);
  Vector3 cameraPos = {cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2]};
  Vector3 cameraRight = {cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2]};
  Vector3 cameraUp = {cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2]};
  Vector3 cameraForward = {cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2]};

  // 照準の奥深くを目標座標とすることで、交差して弾が外れるのを防ぐ
  float distance = 1000.0f; 
  float maxMoveY = distance * std::tan(45.0f * 3.14159265f / 180.0f / 2.0f);
  float maxMoveX = maxMoveY * (1280.0f / 720.0f);
  
  // 弾の目標地点は「自機から」ではなく「カメラから」見たレティクルの延長線上で計算する！
  Vector3 targetPos = cameraPos 
                    + Vector3{cameraForward.x * distance, cameraForward.y * distance, cameraForward.z * distance}
                    + Vector3{cameraRight.x * (ndcX * maxMoveX), cameraRight.y * (ndcX * maxMoveX), cameraRight.z * (ndcX * maxMoveX)}
                    + Vector3{cameraUp.x * (ndcY * maxMoveY), cameraUp.y * (ndcY * maxMoveY), cameraUp.z * (ndcY * maxMoveY)};

  // 弾(startPos)から最終目標地点へのベクトルを弾の初速度ベクトルとする
  Vector3 toTarget = {targetPos.x - startPos.x, targetPos.y - startPos.y, targetPos.z - startPos.z};
  Vector3 velocity = Normalize(toTarget);
  
  float speed = 10.0f; // 少しスピードを落として弾道を見やすくする
  velocity.x *= speed; velocity.y *= speed; velocity.z *= speed;

  auto bullet = std::make_unique<NormalBullet>();
  bullet->Initialize(object3dRenderer_, startPos, velocity, enemies_);
  ActorManager::GetInstance()->AddActor(std::move(bullet));
}

void Player::Draw3D() {
  // プレイヤー自身の描画処理
  if (object3d_) {
    object3d_->Draw();
  }
}

void Player::Draw2D() {
  // メイン照準の描画
  for (auto& sprite : reticleOuterSprites_) {
    sprite->Draw();
  }
  for (auto& sprite : reticleInnerSprites_) {
    sprite->Draw();
  }

  // ロックオン照準の描画
  if (lockOn_) {
    lockOn_->Draw();
  }
}

void Player::OnCollision(Collider *other) {
  // ぶつかった時にデバッグ出力
  OutputDebugStringA("========================\n");
  OutputDebugStringA("Player hit by something!\n");
  OutputDebugStringA("========================\n");

  TakeDamage(1);
}

void Player::TakeDamage(int damage) {
  if (hp_ > 0) {
    hp_ -= damage;
    if (hp_ < 0) {
      hp_ = 0;
    }
    
    if (hp_ == 0) {
      OutputDebugStringA("Player is DEAD!\n");
    } else {
      OutputDebugStringA("Player took damage! Current HP: ");
      OutputDebugStringA(std::to_string(hp_).c_str());
      OutputDebugStringA("\n");
    }
  }
}

void Player::UpdateTransform() {
  if (object3d_) {
    object3d_->SetTranslation(transform_.translate);
    object3d_->SetRotation(transform_.rotate);
    object3d_->SetScale(transform_.scale);
    object3d_->Update();
  }
}

void Player::ForceSnapToCamera() {

  float ndcX = (reticlePosition_.x / 1280.0f) * 2.0f - 1.0f;
  float ndcY = 1.0f - (reticlePosition_.y / 720.0f) * 2.0f;

  Matrix4x4 viewMatrix = camera_->GetViewMatrix();
  Matrix4x4 cameraWorld = Inverse(viewMatrix);
  Vector3 cameraPos = {cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2]};
  Vector3 cameraRight = {cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2]};
  Vector3 cameraUp = {cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2]};
  Vector3 cameraForward = {cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2]};

  float distance = 10.0f;
  float playerMaxMoveX = 5.0f;
  float playerMaxMoveY = 2.5f;
  float playerBaseOffsetY = -2.0f;
  
  Vector3 targetPos = cameraPos 
                    + Vector3{cameraForward.x * distance, cameraForward.y * distance, cameraForward.z * distance}
                    + Vector3{cameraRight.x * (ndcX * playerMaxMoveX), cameraRight.y * (ndcX * playerMaxMoveX), cameraRight.z * (ndcX * playerMaxMoveX)}
                    + Vector3{cameraUp.x * (ndcY * playerMaxMoveY + playerBaseOffsetY), cameraUp.y * (ndcY * playerMaxMoveY + playerBaseOffsetY), cameraUp.z * (ndcY * playerMaxMoveY + playerBaseOffsetY)};

  transform_.translate = targetPos;
  UpdateTransform();
}

void Player::SaveActionConfig() {
  nlohmann::json root;
  root["lockOnRadius"] = actionConfig_.lockOnRadius;
  root["homingSpeed"] = actionConfig_.homingSpeed;
  root["homingFallTime"] = actionConfig_.homingFallTime;
  root["homingStrengthIncrease"] = actionConfig_.homingStrengthIncrease;
  root["homingStrengthMax"] = actionConfig_.homingStrengthMax;

  if (!std::filesystem::exists("resources/config")) {
    std::filesystem::create_directories("resources/config");
  }

  std::ofstream file("resources/config/PlayerActionConfig.json");
  if (file.is_open()) {
    file << std::setw(4) << root << std::endl;
    isActionConfigDirty_ = false;
  }
}

void Player::LoadActionConfig() {
  std::ifstream file("resources/config/PlayerActionConfig.json");
  if (file.is_open()) {
    nlohmann::json root;
    try {
      file >> root;
      if (root.contains("lockOnRadius")) actionConfig_.lockOnRadius = root["lockOnRadius"];
      if (root.contains("homingSpeed")) actionConfig_.homingSpeed = root["homingSpeed"];
      if (root.contains("homingFallTime")) actionConfig_.homingFallTime = root["homingFallTime"];
      if (root.contains("homingStrengthIncrease")) actionConfig_.homingStrengthIncrease = root["homingStrengthIncrease"];
      if (root.contains("homingStrengthMax")) actionConfig_.homingStrengthMax = root["homingStrengthMax"];
    } catch (...) {
      // Parse error, keep defaults
    }
  }
  isActionConfigDirty_ = false;
}

