#pragma once
#include <string>
#include <unordered_set>

class StageSelection {
public:
	// 選択中のステージファイル名（例: "level01.json"）
	static const std::string& GetSelected() { return selected_; }
	static void SetSelected(const std::string& name) { selected_ = name; }

	// クリア情報操作
	static bool IsCleared(const std::string& stageFile);
	static void SetCleared(const std::string& stageFile, bool cleared);

private:
	// デフォルトは level01.json
	inline static std::string selected_ = "level01.json";

	// クリア済みステージ集合（内部でファイルからロード／保存する）
	inline static std::unordered_set<std::string> cleared_; // 定義は cpp 側で初期化処理を行う

	// 追加: StageSelection.cpp 内の EnsureLoaded, SaveToFile を friend にする
	friend void EnsureLoaded();
	friend void SaveToFile();
};