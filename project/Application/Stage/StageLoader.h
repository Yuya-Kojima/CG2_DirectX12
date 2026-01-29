#pragma once
#include "Math/Vector3.h"
#include <map>
#include <string>
#include <vector>

namespace Stage {

// ============================================================
// ステージ上に配置される個別オブジェクトの定義
// アクター・装飾物・ギミックなど、マップに置かれる実体データ
// ============================================================
struct StageObject {
    std::string type; // オブジェクト種別 
    std::string className; // 生成するアクターのクラス名
    std::string model; // 使用する3Dモデルのファイルパス
    Vector3 position { 0, 0, 0 }; // ワールド座標上の配置位置
    Vector3 rotation { 0, 0, 0 }; // 回転角
    Vector3 scale { 1, 1, 1 }; // 拡大率
    bool collision = false; // このオブジェクトが衝突判定を持つかどうか

    // 以下はオプションのOBB情報
    Vector3 obbCenter { 0, 0, 0 }; // OBBの中心オフセット
    Vector3 obbHalfExtents { 0, 0, 0 }; // OBBの半サイズ
    float obbYaw = 0.0f; // OBBのY軸回転角

    uint32_t layer = 0; // 衝突レイヤー番号
    uint32_t id = 0; // オブジェクト固有ID

    // カスタムプロパティ
    std::map<std::string, std::string> properties;
};

// ------------------------------------------------------------
// 注意:
// model / rotation / scale / obb系 / layer / id / properties などは
// JSONから読み込まれるが、現状の GamePlayScene 側では一部しか使用していない。
// 将来拡張のために保持している未使用パラメータも含まれる。
// ------------------------------------------------------------

// ============================================================
// ステージ全体データ構造
// ============================================================
struct StageData {
    std::string name; // ステージ名
    int width = 0; // 横方向のタイル数
    int height = 0; // 奥方向のタイル数
    float tileSize = 1.0f; // 1タイルのワールドサイズ
    std::vector<int> tiles; // 地形タイルID配列
    std::vector<int> collision; // 衝突専用タイル情報
    std::vector<StageObject> objects; // 配置オブジェクト一覧
};

// ============================================================
// JSONステージファイル読み込みクラス
// ============================================================
class StageLoader {
public:
    /**
     * JSONファイルを解析して StageData に格納する
     * @param path JSONファイルパス
     * @param out  出力先 StageData
     * @return 読み込み成功なら true
     */
    static bool LoadStage(const std::string& path, StageData& out);
};

} // namespace Stage