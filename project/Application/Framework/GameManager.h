#pragma once
#include <string>

/// <summary>
/// ゲーム全体を通して保持すべきデータを管理するシングルトン
/// </summary>
class GameManager {
private:
    GameManager() = default;
    ~GameManager() = default;
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // 現在プレイ中のレベル（ステージ）のファイル名
    std::string currentLevelFileName_ = "level_editor.json"; // デフォルト

public:
    static GameManager* GetInstance();

    // 現在のレベルファイル名を設定・取得
    void SetCurrentLevel(const std::string& fileName) { currentLevelFileName_ = fileName; }
    const std::string& GetCurrentLevel() const { return currentLevelFileName_; }
};
