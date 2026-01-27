#pragma once
#include "Math/Vector3.h"
#include <map>
#include <string>
#include <vector>

namespace Stage {

// ステージ上に配置される個別のオブジェクト（アクターやプロップ）のデータ構造

struct StageObject {
    std::string type; // オブジェクトのタイプ ("actor", "prop" など)
    std::string className; // アクタークラス名 (オプション)
    std::string model; // 使用する3Dモデルのファイルパス
    Vector3 position { 0, 0, 0 }; // 配置座標
    Vector3 rotation { 0, 0, 0 }; // 回転角
    Vector3 scale { 1, 1, 1 }; // スケール（大きさ）
    bool collision = false; // 衝突判定の有無
    uint32_t layer = 0; // 衝突レイヤー設定
    uint32_t id = 0; // オブジェクト固有のID
    std::map<std::string, std::string> properties; // カスタムプロパティ（AI行動設定など）
};

// ステージ全体の構成データ

struct StageData {
    std::string name; // ステージ名
    int width = 0; // タイル数（横）
    int height = 0; // タイル数（奥）
    float tileSize = 1.0f; // 1タイルのワールドサイズ
    std::vector<int> tiles; // 地形タイルのID配列（width * height）
    std::vector<int> collision; // 衝突用タイルデータ（オプション）
    std::vector<StageObject> objects; // 配置されているオブジェクトのリスト
};

// JSON形式のステージファイルを読み込むローダークラス

class StageLoader {
public:
    /**
     * @brief JSONファイルを解析してStageData構造体に格納します
     * @param path ファイルパス
     * @param out 格納先の参照
     * @return 読み込み成功ならtrue
     */
    static bool LoadStage(const std::string& path, StageData& out);
};

} // namespace Stage