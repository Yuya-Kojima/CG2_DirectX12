#include "Stage/StageLoader.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace {

/**
 * @brief 文字列の両端から空白文字を取り除く補助関数
 */
static void Trim(std::string& s)
{
    while (!s.empty() && std::isspace((unsigned char)s.back()))
        s.pop_back();
    while (!s.empty() && std::isspace((unsigned char)s.front()))
        s.erase(s.begin());
}

/**
 * @brief オブジェクト内の "properties": { ... } セクションを抽出してマップに格納
 */
static bool ExtractPropertiesFromObject(const std::string& obj, std::map<std::string, std::string>& out)
{
    // "properties" キーを探す
    size_t pos = obj.find("\"properties\"");
    if (pos == std::string::npos)
        pos = obj.find("properties");
    if (pos == std::string::npos)
        return false;

    // コロンと開始ブレース '{' を探す
    size_t colon = obj.find(':', pos);
    if (colon == std::string::npos)
        return false;
    size_t start = obj.find('{', colon);
    if (start == std::string::npos)
        return false;

    // 閉じブレース '}' を探す
    size_t end = obj.find('}', start);
    if (end == std::string::npos)
        return false;

    std::string body = obj.substr(start + 1, end - start - 1);
    size_t cur = 0;
    while (cur < body.size()) {
        // キー（文字列）を抽出
        size_t kstart = body.find('"', cur);
        if (kstart == std::string::npos)
            break;
        size_t kend = body.find('"', kstart + 1);
        if (kend == std::string::npos)
            break;
        std::string key = body.substr(kstart + 1, kend - kstart - 1);

        // 値の抽出
        size_t colon2 = body.find(':', kend);
        if (colon2 == std::string::npos)
            break;
        size_t vstart = colon2 + 1;
        while (vstart < body.size() && std::isspace((unsigned char)body[vstart]))
            ++vstart;
        if (vstart >= body.size())
            break;

        std::string value;
        if (body[vstart] == '"') {
            // 値が文字列の場合
            size_t vend = body.find('"', vstart + 1);
            if (vend == std::string::npos)
                break;
            value = body.substr(vstart + 1, vend - vstart - 1);
            cur = vend + 1;
        } else {
            // 値が数値や真偽値などの場合
            size_t vend = vstart;
            while (vend < body.size() && body[vend] != ',')
                ++vend;
            value = body.substr(vstart, vend - vstart);
            Trim(value);
            cur = vend + 1;
        }
        out[key] = value;
    }
    return !out.empty();
}

/**
 * @brief 特定のキーに対応する文字列値を抽出
 */
static bool ExtractStringValue(const std::string& json, const std::string& key, std::string& out)
{
    size_t pos = json.find(std::string("\"") + key + "\"");
    if (pos == std::string::npos)
        return false;
    size_t colon = json.find(':', pos);
    if (colon == std::string::npos)
        return false;
    size_t start = json.find('"', colon);
    if (start == std::string::npos)
        return false;
    start++;
    size_t end = json.find('"', start);
    if (end == std::string::npos)
        return false;
    out = json.substr(start, end - start);
    return true;
}

/**
 * @brief 特定のキーに対応する数値（float/int）を抽出
 */
static bool ExtractNumberValue(const std::string& json, const std::string& key, float& out)
{
    size_t pos = json.find(std::string("\"") + key + "\"");
    if (pos == std::string::npos)
        pos = json.find(key);
    if (pos == std::string::npos)
        return false;
    size_t colon = json.find(':', pos);
    if (colon == std::string::npos)
        return false;
    size_t start = colon + 1;
    while (start < json.size() && std::isspace((unsigned char)json[start]))
        ++start;
    size_t end = start;
    // 数値として有効な文字が続く限り読み進める
    while (end < json.size() && (std::isdigit((unsigned char)json[end]) || json[end] == '.' || json[end] == '-'))
        ++end;
    if (end == start)
        return false;
    try {
        out = std::stof(json.substr(start, end - start));
    } catch (...) {
        return false;
    }
    return true;
}

/**
 * @brief 数値配列 [1, 2, 3] を std::vector<int> に抽出
 */
static bool ExtractIntArray(const std::string& json, const std::string& key, std::vector<int>& out)
{
    size_t pos = json.find(std::string("\"") + key + "\"");
    if (pos == std::string::npos)
        pos = json.find(key);
    if (pos == std::string::npos)
        return false;
    size_t colon = json.find(':', pos);
    if (colon == std::string::npos)
        return false;
    size_t start = json.find('[', colon);
    if (start == std::string::npos)
        return false;
    size_t end = json.find(']', start);
    if (end == std::string::npos)
        return false;

    std::string body = json.substr(start + 1, end - start - 1);
    std::stringstream ss(body);
    while (!ss.eof()) {
        std::string token;
        if (!std::getline(ss, token, ','))
            break;
        Trim(token);
        if (token.empty())
            continue;
        try {
            out.push_back(std::stoi(token));
        } catch (...) { /* 不正なトークンはスキップ */
        }
    }
    return true;
}

/**
 * @brief オブジェクトの配列 "objects": [ { ... }, { ... } ] を分割して抽出
 */
static bool ExtractObjectsArray(const std::string& json, std::vector<std::string>& outObjects)
{
    size_t pos = json.find("\"objects\"");
    if (pos == std::string::npos)
        pos = json.find("objects");
    if (pos == std::string::npos)
        return false;
    size_t colon = json.find(':', pos);
    if (colon == std::string::npos)
        return false;
    size_t start = json.find('[', colon);
    if (start == std::string::npos)
        return false;
    size_t end = json.find(']', start);
    if (end == std::string::npos)
        return false;

    std::string body = json.substr(start + 1, end - start - 1);
    size_t cur = 0;
    while (cur < body.size()) {
        size_t s = body.find('{', cur);
        if (s == std::string::npos)
            break;
        int depth = 0; // ブレースのネスト深さをカウント（入れ子構造に対応）
        size_t i = s;
        for (; i < body.size(); ++i) {
            if (body[i] == '{')
                depth++;
            else if (body[i] == '}') {
                depth--;
                if (depth == 0) {
                    i++;
                    break;
                }
            }
        }
        if (i > s) {
            outObjects.push_back(body.substr(s, i - s));
            cur = i;
        } else
            break;
    }
    return !outObjects.empty();
}

/**
 * @brief オブジェクト文字列から文字列属性を取得するラッパー
 */
static bool ExtractStringFromObject(const std::string& obj, const std::string& key, std::string& out)
{
    return ExtractStringValue(obj, key, out);
}

/**
 * @brief floatの配列 [x, y, z] を Vector3 に抽出
 */
static bool ExtractFloatArrayFromObject(const std::string& obj, const std::string& key, Vector3& outV)
{
    size_t pos = obj.find(std::string("\"") + key + "\"");
    if (pos == std::string::npos)
        pos = obj.find(key);
    if (pos == std::string::npos)
        return false;
    size_t colon = obj.find(':', pos);
    if (colon == std::string::npos)
        return false;
    size_t start = obj.find('[', colon);
    if (start == std::string::npos)
        return false;
    size_t end = obj.find(']', start);
    if (end == std::string::npos)
        return false;

    std::string body = obj.substr(start + 1, end - start - 1);
    std::stringstream ss(body);
    std::string token;
    int idx = 0;
    while (std::getline(ss, token, ',')) {
        Trim(token);
        if (token.empty())
            continue;
        try {
            float v = std::stof(token);
            if (idx == 0)
                outV.x = v;
            else if (idx == 1)
                outV.y = v;
            else if (idx == 2)
                outV.z = v;
        } catch (...) {
        }
        idx++;
        if (idx >= 3)
            break;
    }
    return true;
}

/**
 * @brief 真偽値（true/false）を抽出
 */
static bool ExtractBoolFromObject(const std::string& obj, const std::string& key, bool& out)
{
    size_t pos = obj.find(std::string("\"") + key + "\"");
    if (pos == std::string::npos)
        pos = obj.find(key);
    if (pos == std::string::npos)
        return false;
    size_t colon = obj.find(':', pos);
    if (colon == std::string::npos)
        return false;
    size_t start = colon + 1;
    while (start < obj.size() && std::isspace((unsigned char)obj[start]))
        ++start;
    // 文字列の比較により判定
    if (obj.compare(start, 4, "true") == 0) {
        out = true;
        return true;
    }
    if (obj.compare(start, 5, "false") == 0) {
        out = false;
        return true;
    }
    return false;
}

} // 匿名名前空間終了

namespace Stage {

bool StageLoader::LoadStage(const std::string& path, StageData& out)
{
    out = StageData(); // データをリセット
    std::ifstream ifs(path);
    if (!ifs.is_open())
        return false; // ファイルが開けない

    // ファイル内容をすべて文字列ストリームへ
    std::stringstream buf;
    buf << ifs.rdbuf();
    std::string s = buf.str();

    // 1. 基本パラメータの抽出
    ExtractStringValue(s, "name", out.name);
    float ftmp = 0.0f;
    if (ExtractNumberValue(s, "tileSize", ftmp))
        out.tileSize = ftmp;
    if (ExtractNumberValue(s, "width", ftmp))
        out.width = int(ftmp);
    if (ExtractNumberValue(s, "height", ftmp))
        out.height = int(ftmp);

    // 2. タイル配列と衝突情報の抽出
    ExtractIntArray(s, "tiles", out.tiles);
    ExtractIntArray(s, "collision", out.collision);

    // 3. 配置オブジェクトの抽出
    std::vector<std::string> objBodies;
    if (ExtractObjectsArray(s, objBodies)) {
        for (const auto& b : objBodies) {
            StageObject o;
            // 個別のオブジェクト属性をパース
            ExtractStringFromObject(b, "type", o.type);
            ExtractStringFromObject(b, "class", o.className);
            ExtractStringFromObject(b, "model", o.model);
            ExtractFloatArrayFromObject(b, "position", o.position);
            ExtractFloatArrayFromObject(b, "rotation", o.rotation);
            ExtractFloatArrayFromObject(b, "scale", o.scale);

            bool btmp;
            if (ExtractBoolFromObject(b, "collision", btmp))
                o.collision = btmp;

            float ltmp;
            if (ExtractNumberValue(b, "layer", ltmp))
                o.layer = uint32_t(ltmp);
            if (ExtractNumberValue(b, "id", ltmp))
                o.id = uint32_t(ltmp);

            // ユーザー定義の追加プロパティ（AIターゲット指定など）
            std::map<std::string, std::string> props;
            if (ExtractPropertiesFromObject(b, props))
                o.properties = props;

            out.objects.push_back(o);
            
            // try to parse optional OBB data if present in object (keys: obb_center, obb_halfExtents, obb_yaw)
            // Note: ExtractFloatArrayFromObject/ExtractNumberValue operate on the object string `b`.
            // This allows JSON like: "obb_center": [x,y,z], "obb_halfExtents": [x,y,z], "obb_yaw": 0.785
            {
                Vector3 tmpv;
                if (ExtractFloatArrayFromObject(b, "obb_center", tmpv)) {
                    out.objects.back().obbCenter = tmpv;
                }
                if (ExtractFloatArrayFromObject(b, "obb_halfExtents", tmpv)) {
                    out.objects.back().obbHalfExtents = tmpv;
                }
                float yawv;
                if (ExtractNumberValue(b, "obb_yaw", yawv)) {
                    out.objects.back().obbYaw = yawv;
                }
            }
        }
    }

    // 4. バリデーション（整合性チェック）
    if (out.width <= 0 || out.height <= 0)
        return false;

    // タイルデータ数が面積と合わない場合の処理
    if ((int)out.tiles.size() != out.width * out.height) {
        if (out.tiles.empty()) {
            // tilesが省略されている場合は0（デフォルト）で埋める
            out.tiles.assign(out.width * out.height, 0);
        } else {
            return false; // データ数が不整合ならエラー
        }
    }

    return true;
}

} // namespace Stage