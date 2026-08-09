#include "Actor/BossCore.h"
#include "Actor/Boss.h"
#include "Actor/BossBit.h"

void BossCore::Initialize() {
  Enemy::Initialize();
  
  // コア自身は死なない（ボス本体が死ねば消滅する）
  hp_ = 99999; 
  
  // 初期状態では装甲に守られているためロックオン不可にする
  SetTag(ActorTag::Untagged);
}

void BossCore::Update() {
  if (boss_) {
    const auto& bossPos = boss_->GetTransform().translate;
    const auto& bossScale = boss_->GetTransform().scale;
    const auto& bossRot = boss_->GetTransform().rotate;
    
    // Y軸回転を適用（ボスがプレイヤーの方を向く回転に追従する）
    float cosY = std::cos(bossRot.y);
    float sinY = std::sin(bossRot.y);
    
    Vector3 scaledOffset;
    scaledOffset.x = offset_.x * bossScale.x;
    scaledOffset.y = offset_.y * bossScale.y;
    scaledOffset.z = offset_.z * bossScale.z;
    
    Vector3 rotatedOffset;
    rotatedOffset.x = scaledOffset.x * cosY + scaledOffset.z * sinY;
    rotatedOffset.y = scaledOffset.y;
    rotatedOffset.z = -scaledOffset.x * sinY + scaledOffset.z * cosY;

    transform_.translate.x = bossPos.x + rotatedOffset.x;
    transform_.translate.y = bossPos.y + rotatedOffset.y;
    transform_.translate.z = bossPos.z + rotatedOffset.z;
    
    // 向きもボスに合わせる
    transform_.rotate = bossRot;
    
    // スケールはボスの0.3倍程度
    transform_.scale.x = bossScale.x * 0.3f;
    transform_.scale.y = bossScale.y * 0.3f;
    transform_.scale.z = bossScale.z * 0.3f;
    
    // 装甲（シールド）が破壊されたらロックオン可能にする
    if (shield_ && shield_->IsDead()) {
      if (GetTag() != ActorTag::LockOnTarget) {
        SetTag(ActorTag::LockOnTarget);
      }
    }
  }
  
  Enemy::Update();
}

void BossCore::TakeDamage(int damage, bool isSelfDestruct) {
  // 自分が受けたダメージを親（ボス）に全額転送する
  if (boss_) {
    boss_->TakeDamage(damage, isSelfDestruct);
  }
  
  // 攻撃が当たったフィードバックとして白く点滅させる（フレーム数で指定）
  hitFlashTimer_ = 10;
}
