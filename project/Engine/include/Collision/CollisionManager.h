#pragma once
#include <list>

class Collider;

class CollisionManager {
public:
    static CollisionManager* GetInstance();

    void Initialize();
    void Update(); // 全コライダーのUpdateを呼んだ後、総当り判定を行う
    void Clear();  // リストをクリアする

    // コライダーの登録と解除
    void Register(Collider* collider);
    void Remove(Collider* collider);

private:
    CollisionManager() = default;
    ~CollisionManager() = default;
    CollisionManager(const CollisionManager&) = delete;
    CollisionManager& operator=(const CollisionManager&) = delete;

    void CheckAllCollisions();

private:
    std::list<Collider*> colliders_;
};
