#pragma once
#include <list>

class Collider;

class CollisionManager {
public:
    static CollisionManager* GetInstance();

    void Initialize();
    void Update(); // 全コライダーのUpdateを呼んだ後、総当り判定を行う
    void DrawDebug(); // 全コライダーのデバッグ描画を行う
    void Clear();  // リストをクリアする

    // コライダーの登録と解除
    void Register(Collider* collider);
    void Remove(Collider* collider);

    // レイキャスト（視線などの線分との衝突判定）
    // mask: 対象とする属性のビットマスク。一致するものだけを判定対象とする
    bool Raycast(const struct Ray& ray, uint32_t mask, Collider** outCollider, float* outDistance = nullptr);

private:
    CollisionManager() = default;
    ~CollisionManager() = default;
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;

    void CheckAllCollisions();

private:
    std::list<Collider*> colliders_;
};
