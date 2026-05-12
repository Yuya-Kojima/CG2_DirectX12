#pragma once
#include "Math/Transform.h"
#include <string>

/// <summary>
/// 全てのゲームオブジェクトの親となるベースクラス
/// </summary>
class BaseActor {
public:
    BaseActor() {
        // 初期値の設定
        transform_.scale = {1.0f, 1.0f, 1.0f};
        transform_.rotate = {0.0f, 0.0f, 0.0f};
        transform_.translate = {0.0f, 0.0f, 0.0f};
    }
    virtual ~BaseActor() = default;

    virtual void Initialize() {}
    virtual void Update() {}
    virtual void Draw3D() {} // 3Dモデルなどの描画
    virtual void Draw2D() {} // UIやカーソルなどの2D描画

    // 生存フラグの操作
    void Destroy() { isDead_ = true; }
    bool IsDead() const { return isDead_; }

    // Transform情報へのアクセス
    Transform& GetTransform() { return transform_; }
    const Transform& GetTransform() const { return transform_; }

    // 識別用の名前やタグ
    std::string name_ = "Actor";

protected:
    // 3D空間上の位置・回転・スケール
    Transform transform_;
    
    // 生存フラグ（trueになればManagerが自動で削除する）
    bool isDead_ = false;
};
