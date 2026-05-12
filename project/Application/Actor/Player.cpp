#include "Player.h"
#include "LockOn.h"
#include "Camera/ICamera.h"

Player::Player() = default;
Player::~Player() = default;

void Player::Initialize() {
    // プレイヤー自身の初期化処理
    
    // ロックオン機能の生成と初期化
    if (spriteRenderer_) {
        lockOn_ = std::make_unique<LockOn>();
        lockOn_->Initialize(spriteRenderer_);
    }
}

void Player::Update() {
    // プレイヤー自身の更新処理
    
    // ロックオンの更新処理
    if (lockOn_ && camera_ && target_) {
        std::vector<Object3d*> enemies = { target_ };
        lockOn_->Update(enemies, camera_->GetViewProjectionMatrix());
    }
}

void Player::Draw() {
    // プレイヤー自身の描画処理（将来的に3Dモデルを描画する）
    
    // カーソルの描画
    if (lockOn_) {
        lockOn_->Draw();
    }
}
