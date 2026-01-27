#pragma once
#include "Math/Vector3.h"
#include "Object3d/Object3d.h"
#include "Renderer/Object3dRenderer.h"
#include <memory>

class Level; // Levelクラスの前方宣言

/**
 * @brief スティック（棒）オブジェクトのクラス
 * 拾う/落とす動作、状態に応じた回転制御、および衝突判定の動的登録を管理します。
 */
class Stick {
public:
    // --- 公開メンバ関数 ---

    // 初期化: 描画用レンダラと初期座標を設定し、モデルをロードします
    void Initialize(Object3dRenderer* renderer, const Vector3& pos);

    // 更新: 状態（持ち/置き）に応じた回転補間や行列更新を行います
    void Update(float dt);

    // 描画: 3Dモデルを表示します
    void Draw();

    // 拾う動作（外部用APIのラッパー）
    void PickUp();

    // 落とす動作（外部用APIのラッパー）
    void Drop();

    // 外部（プレイヤー等のコントローラ）から呼び出されるピックアップ処理
    void PickUpExternal();

    // 外部から呼び出されるドロップ処理
    void DropExternal();

    // 現在手に持たれているかどうかを取得
    bool IsHeld() const { return held_; }
    void SetHeld(bool v) { held_ = v; }

    // 座標の取得と設定（設定時は即座に描画オブジェクトにも反映）
    const Vector3& GetPosition() const { return pos_; }
    void SetPosition(const Vector3& p)
    {
        pos_ = p;
        if (obj_)
            obj_->SetTranslation(pos_);
    }

    // 回転角の設定と取得
    void SetRotation(const Vector3& r)
    {
        rotation_ = r;
        if (obj_)
            obj_->SetRotation(rotation_);
    }
    const Vector3& GetRotation() const { return rotation_; }

    // 衝突判定無効化タイマーの更新（落とした直後の誤判定防止用）
    void UpdateCollisionTimer(float dt);

    // 衝突レイヤー（ビットマスク）の設定・取得
    uint32_t GetLayer() const { return layer_; }
    void SetLayer(uint32_t v) { layer_ = v; }

    // インスタンスIDの設定・取得
    void SetId(uint32_t id) { id_ = id; }
    uint32_t GetId() const { return id_; }

    // 所属するレベル（ステージ）へのポインタを設定・取得
    void SetLevel(class Level* lvl) { level_ = lvl; }
    Level* GetLevel() const { return level_; }

    // 公開変数（便宜上publicに配置されている管理用データ）
    uint32_t layer_ = 0; // 衝突レイヤー
    uint32_t id_ = 0; // オブジェクト固有ID

private:
    // --- 内部メンバ変数 ---

    Object3dRenderer* renderer_ = nullptr; // 描画エンジンの参照
    std::unique_ptr<Object3d> obj_; // 3Dモデルの実体

    Vector3 pos_ { 0, 0, 0 }; // ワールド座標
    bool held_ = false; // 保持フラグ（trueなら誰かが持っている）
    bool collisionEnabled_ = true; // 衝突判定が有効かどうか

    // 設置時にレベルに登録した衝突用AABBのオーナーID（削除時に使用）
    uint32_t registeredOwnerId_ = 0;

    // 現在衝突判定が有効かどうかの内部チェック
    bool IsCollisionEnabled() const;

    // ドロップ直後の衝突無効化用タイマー（秒）
    float collisionDisableTimer_ = 0.0f;
    float collisionDisableDuration_ = 0.5f;

    // 回転に関するパラメータ
    Vector3 rotation_ { 0, 0, 0 }; // 現在の回転角
    Vector3 heldRotation_ { -0.6f, 0.0f, 0.5f }; // 手に持っている時の角度
    Vector3 dropRotation_ { 1.57f, 0.0f, 0.0f }; // 地面に置いた時の角度（横倒しなど）
    float rotationLerpSpeed_ = 10.0f; // 角度変化の補間速度

    Level* level_ = nullptr; // 所属ステージへの参照
};