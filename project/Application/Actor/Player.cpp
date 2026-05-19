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
#include <Windows.h>
#include <algorithm>

Player::Player() = default;
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
    reticleSprite_ = std::make_unique<Sprite>();
    reticleSprite_->Initialize(spriteRenderer_, "resources/white1x1.png");
    reticleSprite_->SetSize({30.0f, 30.0f});
    reticleSprite_->SetColor({0.0f, 1.0f, 0.0f, 0.5f}); // 半透明の緑
  }

  // コライダーの初期化
  collider_ = std::make_unique<SphereCollider>(this);
  collider_->SetRadius(1.0f); // プレイヤーの当たり判定の大きさ
  collider_->SetAttribute(kCollisionAttributePlayer); // 自分は「自機」
  collider_->SetMask(kCollisionAttributeEnemy |
                     kCollisionAttributeEnemyBullet); // 敵や敵の弾と当たる
  CollisionManager::GetInstance()->Register(collider_.get());
}

void Player::Update() {
  // プレイヤー自身の更新処理

  // 照準の移動操作
  if (input_) {
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

  // 照準スプライトの更新
  if (reticleSprite_) {
    reticleSprite_->SetPosition({reticlePosition_.x - 15.0f, reticlePosition_.y - 15.0f});
    Transform defaultUV;
    defaultUV.scale = {1.0f, 1.0f, 1.0f};
    defaultUV.rotate = {0.0f, 0.0f, 0.0f};
    defaultUV.translate = {0.0f, 0.0f, 0.0f};
    reticleSprite_->Update(defaultUV);
  }

  // 自機の追従と姿勢制御 (パターンAの実装)
  if (camera_) {
    // 1. レティクルのNDC（正規化デバイス座標）を計算 (-1.0 ~ 1.0)
    float ndcX = (reticlePosition_.x / 1280.0f) * 2.0f - 1.0f;
    float ndcY = 1.0f - (reticlePosition_.y / 720.0f) * 2.0f;

    // 2. カメラ空間のベクトルを取得
    Matrix4x4 viewMatrix = camera_->GetViewMatrix();
    Matrix4x4 cameraWorld = Inverse(viewMatrix);
    Vector3 cameraPos = {cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2]};
    Vector3 cameraRight = {cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2]};
    Vector3 cameraUp = {cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2]};
    Vector3 cameraForward = {cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2]};

    // 3. 自機の目標座標を計算 (カメラから一定距離前方に配置)
    float distance = 10.0f; // カメラからの距離
    float maxMoveX = 8.0f; // 左右の最大移動幅
    float maxMoveY = 4.0f; // 上下の最大移動幅
    
    Vector3 targetPos = cameraPos 
                      + Vector3{cameraForward.x * distance, cameraForward.y * distance, cameraForward.z * distance}
                      + Vector3{cameraRight.x * (ndcX * maxMoveX), cameraRight.y * (ndcX * maxMoveX), cameraRight.z * (ndcX * maxMoveX)}
                      + Vector3{cameraUp.x * (ndcY * maxMoveY), cameraUp.y * (ndcY * maxMoveY), cameraUp.z * (ndcY * maxMoveY)};

    // 4. 自機を目標座標へ遅延追従 (Lerp)
    Vector3 prevPos = transform_.translate;
    transform_.translate = Lerp(transform_.translate, targetPos, 0.15f);

    // 5. 姿勢制御 (バンク角・ピッチ角)
    Vector3 velocity = {transform_.translate.x - prevPos.x, 
                        transform_.translate.y - prevPos.y, 
                        transform_.translate.z - prevPos.z};
    
    // 速度をカメラローカルに変換してバンク・ピッチを計算
    float localVelX = Dot(velocity, cameraRight);
    float localVelY = Dot(velocity, cameraUp);

    float camRotY = std::atan2(cameraForward.x, cameraForward.z);
    float xzLen = std::sqrt(cameraForward.x * cameraForward.x + cameraForward.z * cameraForward.z);
    float camRotX = std::atan2(-cameraForward.y, xzLen);

    // 速度に応じて機体を傾ける
    float targetRoll = -localVelX * 1.5f; // 右移動で右下がり(負のZ回転)
    float targetPitch = camRotX + localVelY * 1.5f;
    
    transform_.rotate.z = Lerp(transform_.rotate.z, targetRoll, 0.1f);
    transform_.rotate.x = Lerp(transform_.rotate.x, targetPitch, 0.1f);
    transform_.rotate.y = Lerp(transform_.rotate.y, camRotY, 0.1f); // ヨーも滑らかにカメラに追従
  }

  // 3Dモデルの更新
  if (object3d_) {
    object3d_->SetTranslation(transform_.translate);
    object3d_->SetRotation(transform_.rotate);
    object3d_->SetScale(transform_.scale);
    object3d_->Update();
  }

  // 射撃入力とステート管理（1ボタン方式）
  if (input_) {
    // スペースキーまたはパッドのRB(右バンパー)を射撃/ロックオンボタンとする
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
  if (lockOn_ && camera_) {
    bool isLockOnMode = (attackState_ == AttackState::LockOn);
    lockOn_->Update(enemies_, camera_->GetViewProjectionMatrix(), reticlePosition_, isLockOnMode);
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

    // ActorManagerに弾を登録して、自動でUpdate・Drawされるようにする
    ActorManager::GetInstance()->AddActor(std::move(bullet));
  }
}

void Player::FireNormalShot() {
  if (!object3dRenderer_ || !object3d_ || !camera_) return;

  // プレイヤーの現在位置を弾の始点とする
  Vector3 startPos = object3d_->GetTranslation();

  // レティクルのNDC座標を計算 (-1.0 ~ 1.0)
  float ndcX = (reticlePosition_.x / 1280.0f) * 2.0f - 1.0f;
  float ndcY = 1.0f - (reticlePosition_.y / 720.0f) * 2.0f;

  // カメラ空間のベクトルを取得
  Matrix4x4 viewMatrix = camera_->GetViewMatrix();
  Matrix4x4 cameraWorld = Inverse(viewMatrix);
  Vector3 cameraRight = {cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2]};
  Vector3 cameraUp = {cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2]};
  Vector3 cameraForward = {cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2]};

  // 画面の少し奥（距離50.0f）のレティクル位置を目標座標とする
  float distance = 50.0f; 
  float maxMoveX = 40.0f; // 画面端の場合のX移動幅
  float maxMoveY = 22.5f; // 画面端の場合のY移動幅
  
  Vector3 targetPos = startPos 
                    + Vector3{cameraForward.x * distance, cameraForward.y * distance, cameraForward.z * distance}
                    + Vector3{cameraRight.x * (ndcX * maxMoveX), cameraRight.y * (ndcX * maxMoveX), cameraRight.z * (ndcX * maxMoveX)}
                    + Vector3{cameraUp.x * (ndcY * maxMoveY), cameraUp.y * (ndcY * maxMoveY), cameraUp.z * (ndcY * maxMoveY)};

  // 始点から目標座標への方向ベクトルを弾の速度ベクトルにする
  Vector3 toTarget = {
      targetPos.x - startPos.x,
      targetPos.y - startPos.y,
      targetPos.z - startPos.z
  };
  Vector3 velocity = Normalize(toTarget);
  
  float speed = 2.0f; // 通常弾は速い
  velocity.x *= speed;
  velocity.y *= speed;
  velocity.z *= speed;

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
  // メインカーソルの描画
  if (reticleSprite_) {
    reticleSprite_->Draw();
  }

  // ロックオンカーソルの描画
  if (lockOn_) {
    lockOn_->Draw();
  }
}

void Player::OnCollision(Collider *other) {
  // ぶつかった時にデバッグ出力
  OutputDebugStringA("========================\n");
  OutputDebugStringA("Player hit by something!\n");
  OutputDebugStringA("========================\n");
}
