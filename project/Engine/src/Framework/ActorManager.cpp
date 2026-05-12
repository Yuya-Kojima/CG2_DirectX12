#include "Framework/ActorManager.h"

ActorManager* ActorManager::GetInstance() {
    static ActorManager instance;
    return &instance;
}

void ActorManager::Initialize() {
    actors_.clear();
}

void ActorManager::Update() {
    // 1. リスト内のすべてのActorを更新
    for (auto& actor : actors_) {
        actor->Update();
    }

    // 2. 死亡フラグが立っているActorをリストから削除（メモリ解放）
    actors_.remove_if([](const std::unique_ptr<BaseActor>& actor) {
        return actor->IsDead();
    });
}

void ActorManager::Draw3D() {
    // リスト内のすべてのActorの3D部分を描画
    for (auto& actor : actors_) {
        actor->Draw3D();
    }
}

void ActorManager::Draw2D() {
    // リスト内のすべてのActorの2D部分を描画
    for (auto& actor : actors_) {
        actor->Draw2D();
    }
}

void ActorManager::AddActor(std::unique_ptr<BaseActor> actor) {
    // 追加時に初期化を呼んでおく
    actor->Initialize(); 
    // リストに登録
    actors_.push_back(std::move(actor));
}

void ActorManager::Clear() {
    actors_.clear();
}

void ActorManager::Finalize() {
    Clear();
}
