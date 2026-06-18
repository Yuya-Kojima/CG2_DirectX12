#pragma once
#include "Audio/SoundManager.h"
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include <vector>

class Sprite;
class Object3d;
class SpriteRenderer;
class Object3dRenderer;
class DebugCamera;
class InputKeyState;
class ParticleEmitter;
class PostProcess;

#include "Render/SkyBox/SkyBox.h"
#include "Camera/RailCamera.h"
#include "Actor/Player.h"
#include "Actor/Enemy.h"

struct SpawnEvent {
  float spawnTime = 0.0f;
  std::string prefabName = "ZakoEnemy";
  Vector3 spawnOffset = {0.0f, 0.0f, 50.0f}; // カメラからの相対位置（奥に50）
  int moveType = 0; // 0:Straight, 1:Parallel, 2:SineWave
  bool hasSpawned = false; // 実行時の管理用フラグ
};

enum class GameState {
  Play,
  Clear,
  GameOver
};

class GamePlayScene : public BaseScene {

private: // メンバ変数(ゲーム用)
  // カメラ
  std::unique_ptr<GameCamera> camera_ = nullptr;
  Transform cameraTransform_{};

  // レールカメラ
  std::unique_ptr<RailCamera> railCamera_ = nullptr;
  std::vector<Vector3> waypoints_;

  // スカイボックス
  std::unique_ptr<Skybox> skybox_ = nullptr;

  // プレイヤー
  std::unique_ptr<Player> player_ = nullptr;

  // デバッグカメラ
  std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

  // デバッグカメラスイッチ
  bool useDebugCamera_ = false;

  // プレイ/ストップモードフラグ
  bool previousGlobalPlayMode_ = false;
  bool isPaused_ = false;
  bool doStep_ = false;
  float playStartT_ = 0.0f; // Play開始時のtの値を保存しておく変数

  // ゲーム状態
  GameState gameState_ = GameState::Play;

  // ダミー敵管理
  bool hasSpawnedDummy_ = false;
  std::vector<std::unique_ptr<Enemy>> runtimeEnemies_;
  std::vector<Enemy*> enemyPtrs_;

  // スポーンイベント
  std::vector<SpawnEvent> spawnEvents_;

  // 動的配置オブジェクト (ドラッグ＆ドロップで追加された背景・モデル等)
  std::vector<std::unique_ptr<Object3d>> sceneObjects_;

  // 環境マッピング確認用オブジェクト
  std::unique_ptr<Object3d> metallicObject_ = nullptr;

  // エディタ用：選択中のオブジェクトタイプ
  enum class EditorSelectType {
    None,
    RailCamera,
    Environment,
    SceneObject,
    SpawnEvent
  };
  EditorSelectType currentSelectType_ = EditorSelectType::None;
  int selectedWaypointIndex_ = -1;
  int selectedSceneObjectIndex_ = -1;
  int selectedSpawnEventIndex_ = -1;

public: // Undo/Redo用アクセッサ
  std::vector<SpawnEvent>& GetSpawnEvents() { return spawnEvents_; }
  void SelectSpawnEvent(int index) { 
    selectedSpawnEventIndex_ = index; 
    currentSelectType_ = (index >= 0) ? EditorSelectType::SpawnEvent : EditorSelectType::None; 
  }

public: // メンバ関数
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(EngineBase *engine) override;

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize() override;

  /// <summary>
  /// 更新
  /// </summary>
  void Update() override;

  /// <summary>
  /// 描画
  /// </summary>
  void Draw() override;

  /// <summary>
  /// 2Dオブジェクト描画
  /// </summary>
  void Draw2D() override;

  /// <summary>
  /// 3Dオブジェクト描画
  /// </summary>
  void Draw3D() override;

  /// <summary>
  /// エディタ用UIとGizmoの描画
  /// </summary>
  void DrawEditorUI() override;
  void OnFileDropped(const std::string& filePath) override;

private:
  void SpawnSceneObject(const std::string& modelPath, const Vector3& position);

  /// <summary>
  /// レベルデータの読み込み
  /// </summary>
  void LoadLevel(const std::string& filename = "level_editor.json");

  /// <summary>
  /// レベルデータの保存
  /// </summary>
  void SaveLevel(const std::string& filename = "level_editor.json");

private: // メンバ変数(システム用)
private:
  /*ポインタ参照
  ------------------*/
  // エンジン
  EngineBase *engine_ = nullptr;
};
