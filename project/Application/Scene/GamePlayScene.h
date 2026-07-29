#pragma once
#include "Audio/SoundManager.h"
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include <vector>
#include <unordered_map>
#include <string>

class Sprite;
class Object3d;
class SpriteRenderer;
class Object3dRenderer;
class DebugCamera;
class InputKeyState;
class ParticleEmitter;
class PostProcess;
class BillboardParticleEmitter;
class MeshParticleEmitter;

#include "Render/SkyBox/SkyBox.h"
#include "Camera/RailCamera.h"
#include "Actor/Player.h"
#include "Actor/Enemy.h"
#include "Actor/Boss.h"

struct SpawnEvent {
  float spawnTime = 0.0f;
  std::string prefabName = "ZakoEnemy";
  Vector3 spawnOffset = {0.0f, 0.0f, 50.0f}; // カメラからの相対位置（奥50）
  std::string splineName = "";               // 使用するレール名（空なら直線移動）
  float splineDuration = 5.0f;               // レールを走り切る秒数
  bool isWorldSpaceSpline = false;           // ワールド空間か、カメラローカル空間か
  bool hasSpawned = false; // 実行管理用フラグ
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

  // ヒットストップタイマー
  int hitStopTimer_ = 0;

  // ダミー敵管理
  std::vector<std::unique_ptr<Enemy>> runtimeEnemies_;
  std::vector<Enemy*> enemyPtrs_;

  // ロード済みスプラインデータ (Blender JSON等から)
  std::unordered_map<std::string, std::vector<Vector3>> loadedSplines_;
  void LoadSplines(); // splines.jsonを読み込んでloadedSplines_に格納

  // スポーンイベント
  std::vector<SpawnEvent> spawnEvents_;

  // 動的配置オブジェクト (ドラッグ＆ドロップで追加された背景・モデル等)
  std::vector<std::unique_ptr<Object3d>> sceneObjects_;

  // ドラッグ中のプレビュー用オブジェクト
  std::unique_ptr<Object3d> previewObject_ = nullptr;
  std::string previewModelPath_ = "";
  bool isPreviewHovering_ = false;

  // 環境マッピング確認用オブジェクト
  std::unique_ptr<Object3d> metallicObject_ = nullptr;


  // ボス専用エミッター
  std::unique_ptr<BillboardParticleEmitter> bossExplosionParticleGroup_;
  std::unique_ptr<BillboardParticleEmitter> bossDustParticleGroup_;
  std::unique_ptr<ParticleEmitter> bossExplosionEmitter_;
  std::unique_ptr<ParticleEmitter> bossDustEmitter_;

  bool hasBossStartedDying_ = false;

  // ポストエフェクト
  bool isGrayscale_ = false;
  float dissolveTimer_ = 0.0f;
  float effectTime_ = 0.0f;
  float damageEffectTimer_ = 0.0f;
  bool testDamageEffect_ = false;

  // エディタ用：選択中のオブジェクトタイプ
  enum class EditorSelectType {
    None,
    RailCamera,
    Environment,
    SceneObject,
    SpawnEvent,
    Player,
    Effect,
    Prefab
  };
  EditorSelectType currentSelectType_ = EditorSelectType::None;
  int selectedWaypointIndex_ = -1;
  int selectedSceneObjectIndex_ = -1;
  int selectedSpawnEventIndex_ = -1;
  
  // プレハブ編集用
  std::string selectedPrefabName_ = "";
  std::unique_ptr<Enemy> tempPrefabEditEnemy_ = nullptr;

public: // Undo/Redo用アクセッサ
  std::vector<SpawnEvent>& GetSpawnEvents() { return spawnEvents_; }
  void SelectSpawnEvent(int index) { 
    selectedSpawnEventIndex_ = index; 
    currentSelectType_ = (index >= 0) ? EditorSelectType::SpawnEvent : EditorSelectType::None; 
  }

public: // メンバ関数
  /// <summary>
  /// ヒットストップの要求（フレーム数指定、複数回呼ばれた場合は長い方を優先）
  /// </summary>
  void RequestHitStop(int frames);

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
  void OnFileDropped(const std::string &filePath, const Vector2& ndcPos) override;
  void OnDragHovering(const std::string &filePath, const Vector2& ndcPos) override;
  void OnDragHoverEnd() override;

  // Waypoint
  void AppendWaypoint(const Vector3 &pos);

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
