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

  // デバッグカメラ使用
  bool useDebugCamera_ = false;

  // ダミー敵管理
  bool hasSpawnedDummy_ = false;
  std::vector<std::unique_ptr<Enemy>> enemies_;
  std::vector<Enemy*> enemyPtrs_;

  // エディタ用：選択中のオブジェクトタイプ
  enum class EditorSelectType {
    None,
    Enemy,
    RailCamera,
    Environment
  };
  EditorSelectType currentSelectType_ = EditorSelectType::None;
  int selectedEnemyIndex_ = -1;
  int selectedWaypointIndex_ = -1;

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

  /// <summary>
  /// レベルデータの読み込み
  /// </summary>
  void LoadLevel();

  /// <summary>
  /// レベルデータの保存
  /// </summary>
  void SaveLevel();

  /// <summary>
  /// 敵を1体追加する
  /// </summary>
  void AddEnemy(const Vector3& position);

private: // メンバ変数(システム用)
private:
  /*ポインタ参照
  ------------------*/
  // エンジン
  EngineBase *engine_ = nullptr;
};
