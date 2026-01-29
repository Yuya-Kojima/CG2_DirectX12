#pragma once
#include <string>

class StageSelection
{
public:
	// 選択中のステージファイル名（例: "level01.json"）
	static const std::string& GetSelected() { return selected_; }
	static void SetSelected(const std::string& name) { selected_ = name; }

private:
	// デフォルトは level01.json
	inline static std::string selected_ = "level01.json";
};