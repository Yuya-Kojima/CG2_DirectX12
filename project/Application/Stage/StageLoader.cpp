#include "Stage/StageLoader.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace {

// ============================================================
// 文字列の前後にある空白を除去する関数
// ============================================================
static void Trim(std::string& s)
{
    while (!s.empty() && std::isspace((unsigned char)s.back()))
        s.pop_back();
    while (!s.empty() && std::isspace((unsigned char)s.front()))
        s.erase(s.begin());
}

// ============================================================
// JSONオブジェクト内の "properties" セクションを抽出
// ============================================================
static bool ExtractPropertiesFromObject(const std::string& obj, std::map<std::string, std::string>& out)
{
    size_t pos = obj.find("\"properties\"");
    if (pos == std::string::npos)
        pos = obj.find("properties");
    if (pos == std::string::npos)
        return false;

    size_t colon = obj.find(':', pos);
    if (colon == std::string::npos)
        return false;

    size_t start = obj.find('{', colon);
    if (start == std::string::npos)
        return false;

    size_t end = obj.find('}', start);
    if (end == std::string::npos)
        return false;

    std::string body = obj.substr(start + 1, end - start - 1);
    size_t cur = 0;

    while (cur < body.size()) {
        size_t kstart = body.find('"', cur);
        if (kstart == std::string::npos)
            break;
        size_t kend = body.find('"', kstart + 1);
        if (kend == std::string::npos)
            break;

        std::string key = body.substr(kstart + 1, kend - kstart - 1);

        size_t colon2 = body.find(':', kend);
        if (colon2 == std::string::npos)
            break;

        size_t vstart = colon2 + 1;
        while (vstart < body.size() && std::isspace((unsigned char)body[vstart]))
            ++vstart;

        std::string value;
        if (body[vstart] == '"') {
            size_t vend = body.find('"', vstart + 1);
            if (vend == std::string::npos)
                break;
            value = body.substr(vstart + 1, vend - vstart - 1);
            cur = vend + 1;
        } else {
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

// ============================================================
// 文字列値抽出
// ============================================================
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

// ============================================================
// 数値抽出（float / int 共通処理）
// ============================================================
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

// ============================================================
// 整数配列抽出
// ============================================================
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
        } catch (...) {
            // 不正値は無視
        }
    }

    return true;
}

// ============================================================
// objects配列抽出
// ============================================================
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

        int depth = 0;
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

// ============================================================
// Vector3抽出
// ============================================================
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

// ============================================================
// bool値抽出
// ============================================================
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

// ============================================================
// ステージ読み込み本体処理
// ============================================================
bool StageLoader::LoadStage(const std::string& path, StageData& out)
{
    out = StageData(); // 既存データを初期化

    std::ifstream ifs(path);
    if (!ifs.is_open())
        return false;

    std::stringstream buf;
    buf << ifs.rdbuf();
    std::string s = buf.str();

    // 基本情報
    ExtractStringValue(s, "name", out.name);

    float ftmp = 0.0f;
    if (ExtractNumberValue(s, "tileSize", ftmp))
        out.tileSize = ftmp;
    if (ExtractNumberValue(s, "width", ftmp))
        out.width = int(ftmp);
    if (ExtractNumberValue(s, "height", ftmp))
        out.height = int(ftmp);

    // タイル情報
    ExtractIntArray(s, "tiles", out.tiles);
    ExtractIntArray(s, "collision", out.collision);

    // オブジェクト情報
    std::vector<std::string> objBodies;
    if (ExtractObjectsArray(s, objBodies)) {
        for (const auto& b : objBodies) {
            StageObject o;

            ExtractStringValue(b, "type", o.type);
            ExtractStringValue(b, "class", o.className);
            ExtractStringValue(b, "model", o.model);
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

            std::map<std::string, std::string> props;
            if (ExtractPropertiesFromObject(b, props))
                o.properties = props;

            out.objects.push_back(o);

            // --- OBB追加情報読み取り ---
            Vector3 tmpv;
            if (ExtractFloatArrayFromObject(b, "obb_center", tmpv))
                out.objects.back().obbCenter = tmpv;
            if (ExtractFloatArrayFromObject(b, "obb_halfExtents", tmpv))
                out.objects.back().obbHalfExtents = tmpv;

            float yawv;
            if (ExtractNumberValue(b, "obb_yaw", yawv))
                out.objects.back().obbYaw = yawv;
        }
    }

    // データ整合性チェック
    if (out.width <= 0 || out.height <= 0)
        return false;

    if ((int)out.tiles.size() != out.width * out.height) {
        if (out.tiles.empty()) {
            out.tiles.assign(out.width * out.height, 0);
        } else {
            return false;
        }
    }

    return true;
}

} // namespace Stage