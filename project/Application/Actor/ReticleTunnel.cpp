#include "ReticleTunnel.h"
#include "Camera/ICamera.h"
#include "Input/Input.h"
#include "Math/MathUtil.h"
#include "Render/Renderer/SpriteRenderer.h"
#include "Debug/Logger.h"
#include <string>
#include <algorithm>
#include <fstream>

void ReticleTunnel::Initialize(SpriteRenderer* spriteRenderer) {
  spriteRenderer_ = spriteRenderer;

  // 直線表現用のスプライトを生成（白テクスチャを使用）
  for (size_t i = 0; i < kMaxLineSprites; ++i) {
    lineSprites_[i] = std::make_unique<Sprite>();
    // 白色の基本スプライトとして初期化
    lineSprites_[i]->Initialize(spriteRenderer_, "white.png");
    // 直線の中央を中心に回転・スケーリングするためにアンカーポイントを (0.5, 0.5) に設定
    lineSprites_[i]->SetAnchorPoint({ 0.5f, 0.5f });
    lineSprites_[i]->SetColor({ 0.0f, 0.9f, 1.0f, 0.85f }); // シアンカラー（高輝度）
  }

  ResetPosition();
}

void ReticleTunnel::ResetPosition() {
  position_ = { 640.0f, 360.0f };
  nearRot_ = 0.0f;
  midRot_ = 0.0f;
  farRot_ = 0.0f;
}

void ReticleTunnel::Update(Input* input, const ICamera* camera, const Vector3& playerPos, bool isLockOn) {
  if (!camera) return;

  // ロックオン時のLerpアニメーション更新 (数フレームで滑らかに開閉)
  if (isLockOn) {
    lockOnLerp_ = (std::min)(lockOnLerp_ + 0.15f, 1.0f);
  } else {
    lockOnLerp_ = (std::max)(lockOnLerp_ - 0.20f, 0.0f);
  }

  // -------------------------------------------------------------
  // 1. 入力によるダイレクトな 2D 照準移動
  // -------------------------------------------------------------
  Vector2 moveInput = { 0.0f, 0.0f };
  if (input) {
    // スティックおよび方向キー入力の取得
    moveInput.x = input->Pad().GetLeftX();
    moveInput.y = input->Pad().GetLeftY();
    if (moveInput.x == 0.0f && moveInput.y == 0.0f) {
      if (input->IsPressKey(DIK_W) || input->IsPressKey(DIK_UP)) moveInput.y += 1.0f;
      if (input->IsPressKey(DIK_S) || input->IsPressKey(DIK_DOWN)) moveInput.y -= 1.0f;
      if (input->IsPressKey(DIK_A) || input->IsPressKey(DIK_LEFT)) moveInput.x -= 1.0f;
      if (input->IsPressKey(DIK_D) || input->IsPressKey(DIK_RIGHT)) moveInput.x += 1.0f;
    }
  }

  // スティック入力からダイレクトに座標を動かす（慣性なし）
  position_.x += moveInput.x * config_.reticleMaxSpeed;
  position_.y -= moveInput.y * config_.reticleMaxSpeed; // 画面Y軸反転を考慮

  // 画面可動域内に制限（1280x720 想定）
  position_.x = std::clamp(position_.x, 60.0f, 1220.0f);
  position_.y = std::clamp(position_.y, 40.0f, 680.0f);

  // -------------------------------------------------------------
  // 2. 3D 視線レイ（Ray）および 3D 目標地点の計算
  // -------------------------------------------------------------
  float ndcX = (position_.x / 1280.0f) * 2.0f - 1.0f;
  float ndcY = 1.0f - (position_.y / 720.0f) * 2.0f;

  Matrix4x4 viewMatrix = camera->GetViewMatrix();
  Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();
  Matrix4x4 viewProjMatrix = Multiply(viewMatrix, projectionMatrix);

  Matrix4x4 cameraWorld = Inverse(viewMatrix);
  Vector3 cameraPos = { cameraWorld.m[3][0], cameraWorld.m[3][1], cameraWorld.m[3][2] };
  Vector3 cameraRight = { cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2] };
  Vector3 cameraUp = { cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2] };
  Vector3 cameraForward = { cameraWorld.m[2][0], cameraWorld.m[2][1], cameraWorld.m[2][2] };

  float fovY = DegToRad(45.0f);
  float maxMoveY = config_.targetDistance * std::tan(fovY * 0.5f);
  float maxMoveX = maxMoveY * (1280.0f / 720.0f);

  target3DPos_ = cameraPos +
    cameraForward * config_.targetDistance +
    cameraRight * (ndcX * maxMoveX) +
    cameraUp * (ndcY * maxMoveY);

  Vector3 toTarget = target3DPos_ - playerPos;
  rayDirection_ = SafeNormalize(toTarget);

  // -------------------------------------------------------------
  // 3. 手前・中・奥の 3D 矩形頂点計算
  // -------------------------------------------------------------
  // ロックオン中は回転速度を最大5倍に加速して「チャージ感」を出す
  float rotScale = 1.0f + (lockOnLerp_ * 4.0f);
  nearRot_ += (config_.nearRotSpeed * rotScale) * (1.0f / 60.0f);
  midRot_ += (config_.midRotSpeed * rotScale) * (1.0f / 60.0f);
  farRot_ += (config_.farRotSpeed * rotScale) * (1.0f / 60.0f);

  auto CalculateSquareVertices = [&](float distance, float size, float rotation, Vector2 outScreen[4]) {
    Vector3 center = playerPos + rayDirection_ * distance;

    // 視線レイ(rayDirection_)に直交するローカルの右・上軸を外積で計算
    // cameraUpとrayDirectionの外積で「新しい右軸」を作成
    Vector3 rayRight = SafeNormalize(Cross(cameraUp, rayDirection_));
    // rayDirectionとrayRightの外積で「新しい上軸」を作成
    Vector3 rayUp = Cross(rayDirection_, rayRight);

    // 新しいローカル軸を元に回転を適用
    float cosR = std::cos(rotation);
    float sinR = std::sin(rotation);
    Vector3 rightAxis = rayRight * cosR + rayUp * sinR;
    Vector3 upAxis = rayRight * (-sinR) + rayUp * cosR;

    float halfS = size * 0.5f;
    Vector3 v3D[4] = {
      center - rightAxis * halfS + upAxis * halfS, // Top-Left
      center + rightAxis * halfS + upAxis * halfS, // Top-Right
      center + rightAxis * halfS - upAxis * halfS, // Bottom-Right
      center - rightAxis * halfS - upAxis * halfS  // Bottom-Left
    };

    for (int k = 0; k < 4; ++k) {
      outScreen[k] = WorldToScreen(v3D[k], viewProjMatrix, 1280.0f, 720.0f);
    }
  };

  Vector2 nearScreen[4];
  Vector2 midScreen[4];
  Vector2 farScreen[4];

  // ロックオン中は枠のサイズを広げる（最大1.3倍）
  float sizeScale = 1.0f + (lockOnLerp_ * 0.3f);

  CalculateSquareVertices(config_.nearDistance, config_.nearSize * sizeScale, nearRot_, nearScreen);
  CalculateSquareVertices(config_.midDistance, config_.midSize * sizeScale, midRot_, midScreen);
  CalculateSquareVertices(config_.farDistance, config_.farSize * sizeScale, farRot_, farScreen);

  // -------------------------------------------------------------
  // 4. 2D 透視投影座標を直線スプライトへ適用
  // -------------------------------------------------------------
  size_t spriteIdx = 0;

  // A. 各正方形の枠線
  auto ApplySquareLines = [&](const Vector2 screen[4], float thickness) {
    // ロックオン時のメイン色計算 (通常は水色、ロックオンで赤色へLerp)
    Vector4 mainColor = {
      0.0f + lockOnLerp_ * 1.0f, // R: 0.0 -> 1.0
      1.0f - lockOnLerp_ * 0.8f, // G: 1.0 -> 0.2
      1.0f - lockOnLerp_ * 0.8f, // B: 1.0 -> 0.2
      1.0f
    };

    for (int i = 0; i < 4; ++i) {
      if (spriteIdx + 1 < kMaxLineSprites) {
        // 1. 黒い縁取り（アウトライン）少し太く、半透明の黒
        UpdateLineSprite(spriteIdx++, screen[i], screen[(i + 1) % 4], thickness + 2.0f, {0.0f, 0.0f, 0.0f, 0.7f});
        // 2. メインの線
        UpdateLineSprite(spriteIdx++, screen[i], screen[(i + 1) % 4], thickness, mainColor);
      }
    }
  };

  // 空気遠近法：奥に行くほど線を細くして立体感を強調
  ApplySquareLines(nearScreen, config_.lineThickness);
  ApplySquareLines(midScreen, config_.lineThickness * 0.6f);
  ApplySquareLines(farScreen, config_.lineThickness * 0.3f);

  static bool s_logged = false;
  if (!s_logged) {
    s_logged = true;
    std::string msg = "=== ReticleTunnel Debug ===\n";
    msg += "cameraRight: " + std::to_string(cameraRight.x) + ", " + std::to_string(cameraRight.y) + ", " + std::to_string(cameraRight.z) + "\n";
    msg += "cameraUp: " + std::to_string(cameraUp.x) + ", " + std::to_string(cameraUp.y) + ", " + std::to_string(cameraUp.z) + "\n";
    msg += "nearScreen[0]: " + std::to_string(nearScreen[0].x) + ", " + std::to_string(nearScreen[0].y) + "\n";
    msg += "nearScreen[1]: " + std::to_string(nearScreen[1].x) + ", " + std::to_string(nearScreen[1].y) + "\n";
    msg += "nearScreen[2]: " + std::to_string(nearScreen[2].x) + ", " + std::to_string(nearScreen[2].y) + "\n";
    msg += "===========================\n";
    
    std::ofstream ofs("reticle_debug.txt");
    ofs << msg;
  }
}

void ReticleTunnel::UpdateLineSprite(size_t index, const Vector2& p1, const Vector2& p2, float thickness, const Vector4& color) {
  if (index >= kMaxLineSprites || !lineSprites_[index]) return;

  Line2DTransform transform = CalculateLine2DTransform(p1, p2, thickness);
  lineSprites_[index]->SetPosition(transform.position);
  lineSprites_[index]->SetSize(transform.size);
  lineSprites_[index]->SetRotation(transform.rotation);
  lineSprites_[index]->SetColor(color);

  Transform uvTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
  lineSprites_[index]->Update(uvTransform);
}

void ReticleTunnel::Draw() {
  for (auto& sprite : lineSprites_) {
    if (sprite) {
      sprite->Draw();
    }
  }
}
