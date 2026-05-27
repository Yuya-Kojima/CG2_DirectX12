#include "PrefabManager.h"
#include "Actor/Enemy.h"
#include "Render/Object3d/Object3d.h"
#include <fstream>
#include <filesystem>
#include "../../externals/nlohmann/json.hpp"

PrefabManager* PrefabManager::GetInstance() {
    static PrefabManager instance;
    return &instance;
}

void PrefabManager::Initialize(Object3dRenderer* renderer) {
    object3dRenderer_ = renderer;
    // prefabs フォルダがなければ作成
    if (!std::filesystem::exists("resources/prefabs")) {
        std::filesystem::create_directories("resources/prefabs");
    }
}

bool PrefabManager::SavePrefab(const std::string& prefabName, Enemy* enemy) {
    if (!enemy) return false;

    nlohmann::json root;
    
    // 基本パラメータ
    root["tag"] = enemy->GetTag();
    if (enemy->GetModel()) {
        root["modelPath"] = enemy->GetModel()->GetModelPath();
    }
    
    // Transform情報 (スケールのみ保存。位置と回転は出現時に決まることが多いため)
    const Transform& t = enemy->GetTransform();
    root["scale"] = {t.scale.x, t.scale.y, t.scale.z};
    
    // 今後 HP や攻撃力 などのパラメータが増えたらここに追記する
    // root["hp"] = enemy->GetHP();

    std::string filepath = "resources/prefabs/" + prefabName + ".prefab";
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << std::setw(4) << root << std::endl;
        return true;
    }
    return false;
}

std::unique_ptr<Enemy> PrefabManager::InstantiateEnemy(const std::string& prefabName, const Transform& transform) {
    std::string filepath = "resources/prefabs/" + prefabName + ".prefab";
    std::ifstream file(filepath);
    
    // 生成して初期化
    auto newEnemy = std::make_unique<Enemy>();
    newEnemy->Initialize();

    std::string modelPath = "resources/suzanne.obj"; // デフォルト

    if (file.is_open()) {
        nlohmann::json root;
        file >> root;
        
        // タグの復元
        if (root.contains("tag")) {
            newEnemy->SetTag(root["tag"]);
        }
        
        // モデルパスの復元
        if (root.contains("modelPath")) {
            modelPath = root["modelPath"];
        }
        
        // スケールの復元
        if (root.contains("scale")) {
            Vector3 s = {root["scale"][0], root["scale"][1], root["scale"][2]};
            newEnemy->GetTransform().scale = s;
        }
        
        // 位置と回転は引数で渡されたものを適用
        newEnemy->GetTransform().translate = transform.translate;
        newEnemy->GetTransform().rotate = transform.rotate;
    } else {
        // プレハブが見つからない場合はデフォルトパラメータを使用
        newEnemy->GetTransform() = transform;
    }

    // モデルを持たせる
    auto dummyModel = std::make_unique<Object3d>();
    dummyModel->Initialize(object3dRenderer_);
    dummyModel->SetModel(modelPath);
    newEnemy->SetModel(std::move(dummyModel));

    return newEnemy;
}
