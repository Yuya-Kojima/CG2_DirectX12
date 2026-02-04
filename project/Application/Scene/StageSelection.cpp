#include "StageSelection.h"
#include <fstream>
#include <algorithm>
#include <cctype>
#include <mutex>
#include <unordered_set>

static std::mutex g_stageSelectionMutex;
static bool g_loaded = false;
static const char* kSaveFile = "stages_cleared.txt";

namespace {
	inline std::string Trim(const std::string& s)
	{
		size_t a = 0;
		while (a < s.size() && std::isspace((unsigned char)s[a])) ++a;
		size_t b = s.size();
		while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
		return s.substr(a, b - a);
	}
} // namespace

// ヘッダの friend 宣言と一致するグローバル関数として定義する
void EnsureLoaded()
{
	std::lock_guard<std::mutex> lk(g_stageSelectionMutex);
	if (g_loaded) return;
	std::ifstream ifs(kSaveFile);
	if (!ifs) {
		g_loaded = true;
		return;
	}
	std::string line;
	while (std::getline(ifs, line)) {
		line = Trim(line);
		if (line.empty()) continue;
		StageSelection::cleared_.insert(line); // friend 関係によりアクセス可能
	}
	g_loaded = true;
}

void SaveToFile()
{
	std::lock_guard<std::mutex> lk(g_stageSelectionMutex);
	std::ofstream ofs(kSaveFile, std::ios::trunc);
	if (!ofs) return;
	for (const auto& name : StageSelection::cleared_) {
		ofs << name << "\n";
	}
}

bool StageSelection::IsCleared(const std::string& stageFile)
{
	EnsureLoaded();
	std::lock_guard<std::mutex> lk(g_stageSelectionMutex);
	return cleared_.find(stageFile) != cleared_.end();
}

void StageSelection::SetCleared(const std::string& stageFile, bool cleared)
{
	EnsureLoaded();
	{
		std::lock_guard<std::mutex> lk(g_stageSelectionMutex);
		if (cleared) {
			cleared_.insert(stageFile);
		} else {
			cleared_.erase(stageFile);
		}
	}
	SaveToFile();
}