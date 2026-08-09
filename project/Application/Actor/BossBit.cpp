#include "Actor/BossBit.h"
#include "Actor/Boss.h"
#include "Collision/SphereCollider.h"

BossBit::BossBit() {}
BossBit::~BossBit() {}

void BossBit::Initialize() {
    Enemy::Initialize();
    
    // 装甲なので硬めに設定。
    hp_ = 30;
    
    // プレイヤーがロックオンできるようにタグを設定
    SetTag(ActorTag::LockOnTarget);
    
    if (collider_) {
        collider_->SetRadius(1.2f); // 装甲板として少し大きめの当たり判定
    }
}

void BossBit::SetOffset(const Vector3& offset) {
    baseOffset_ = offset;
    offset_ = offset;
}

void BossBit::Update() {
    // 親が死んでいるか存在しなければ自壊
    if (!boss_ || boss_->GetHP() <= 0) {
        hp_ = 0;
        isDead_ = true;
    }

    if (!isDead_ && boss_) {
        // 親の現在座標とスケールを取得
        Vector3 bossPos = boss_->GetTransform().translate;
        Vector3 bossScale = boss_->GetTransform().scale;
        Vector3 bossRot = boss_->GetTransform().rotate;
        
        offset_ = baseOffset_; // 今は固定

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
        
        // 自身のスケールもボスの大きさに合わせる
        transform_.scale.x = bossScale.x * 0.6f;
        transform_.scale.y = bossScale.y * 0.6f;
        transform_.scale.z = bossScale.z * 0.6f;
    }

    Enemy::Update();
}
