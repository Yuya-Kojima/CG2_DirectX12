#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cassert>
// パスは externals への相対パス
#include "../../externals/nlohmann/json.hpp"
#include "Math/Vector3.h"
#include "Math/MathUtil.h"

// "レベルデータ" というデータの塊なので個別のオブジェクトとは扱いが違う
struct LevelData {

    struct ColliderData {
        std::string type; // "BOX", "SPHERE" など
        Vector3 center;
        Vector3 size;     // BOXの場合
        float radius;     // SPHEREの場合
    };

    struct ObjectData {
        // オブジェクト名
        std::string name;
        // オブジェクト種類 (例: "MESH")
        std::string type;
        // ファイル名 (例: "suzanne.obj")
        std::string fileName;

        // トランスフォーム
        Vector3 translation;
        Vector3 rotation;
        Vector3 scaling;

        // コライダー情報 (オプション)
        bool hasCollider = false;
        ColliderData collider;

        // 子オブジェクトのツリー構造
        std::vector<ObjectData> children;
    };

    // シーン直下のオブジェクトの配列（ツリーのルート要素群）
    std::vector<ObjectData> objects;

    // 再帰的にオブジェクトをパースする関数
    void ParseObjectRecursive(nlohmann::json& jsonObject, ObjectData& objectData) {
        
        // オブジェクト名
        if (jsonObject.contains("name")) {
            objectData.name = jsonObject["name"].get<std::string>();
        }

        // オブジェクト種類
        if (jsonObject.contains("type")) {
            objectData.type = jsonObject["type"].get<std::string>();
        }

        // ファイル名
        if (jsonObject.contains("file_name")) {
            objectData.fileName = jsonObject["file_name"].get<std::string>();
        }

        // トランスフォームのパラメータ読み込み
        if (jsonObject.contains("transform")) {
            nlohmann::json& transform = jsonObject["transform"];
            // 平行移動 (BlenderのYとZを入れ替え)
            objectData.translation.x = (float)transform["translation"][0];
            objectData.translation.y = (float)transform["translation"][2];
            objectData.translation.z = (float)transform["translation"][1];
            // 回転角 (符号を反転、YとZを入れ替え、度数法からラジアンに変換)
            objectData.rotation.x = DegToRad(-(float)transform["rotation"][0]);
            objectData.rotation.y = DegToRad(-(float)transform["rotation"][2]);
            objectData.rotation.z = DegToRad(-(float)transform["rotation"][1]);
            // スケーリング (YとZを入れ替え)
            objectData.scaling.x = (float)transform["scaling"][0];
            objectData.scaling.y = (float)transform["scaling"][2];
            objectData.scaling.z = (float)transform["scaling"][1];
        }

        // コライダーのパラメータ読み込み
        if (jsonObject.contains("collider")) {
            objectData.hasCollider = true;
            nlohmann::json& colliderJson = jsonObject["collider"];
            objectData.collider.type = colliderJson["type"].get<std::string>();
            
            if (colliderJson.contains("center")) {
                objectData.collider.center.x = (float)colliderJson["center"][0];
                objectData.collider.center.y = (float)colliderJson["center"][2];
                objectData.collider.center.z = (float)colliderJson["center"][1];
            }
            if (objectData.collider.type == "BOX" && colliderJson.contains("size")) {
                objectData.collider.size.x = (float)colliderJson["size"][0];
                objectData.collider.size.y = (float)colliderJson["size"][2];
                objectData.collider.size.z = (float)colliderJson["size"][1];
            }
            if (objectData.collider.type == "SPHERE" && colliderJson.contains("radius")) {
                objectData.collider.radius = (float)colliderJson["radius"];
            }
        }

        // 子オブジェクトの再帰パース
        if (jsonObject.contains("children")) {
            nlohmann::json& jsonChildren = jsonObject["children"];
            assert(jsonChildren.is_array());

            for (nlohmann::json& childJson : jsonChildren) {
                // 子用のObjectDataを作成してパース
                ObjectData childData;
                ParseObjectRecursive(childJson, childData);
                // パース済みのデータをリストに追加
                objectData.children.push_back(childData);
            }
        }
    }

    // パース関数
    void LoadFile(const std::string& fullpath) {
        std::ifstream file(fullpath);
        if (file.fail()) {
            assert(0);
        }

        nlohmann::json deserialized;
        file >> deserialized;

        assert(deserialized.is_object());
        assert(deserialized.contains("name"));
        assert(deserialized["name"].is_string());

        std::string name = deserialized["name"].get<std::string>();
        assert(name.compare("scene") == 0);

        nlohmann::json& jsonObjects = deserialized["objects"];
        assert(jsonObjects.is_array());

        // シーン直下のオブジェクトから再帰パース開始
        for (nlohmann::json& objectJson : jsonObjects) {
            ObjectData objectData;
            ParseObjectRecursive(objectJson, objectData);
            objects.push_back(objectData);
        }
    }
};
