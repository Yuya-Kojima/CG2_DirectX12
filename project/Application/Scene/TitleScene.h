#pragma once
#include "Audio/SoundManager.h"
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Math/Transform.h"
#include "Scene/BaseScene.h"
#include <vector>

class Sprite;
class Object3d;
class SpriteRenderer;
class Object3dRenderer;
class DebugCamera;
class InputKeyState;
class ParticleEmitter;
class Skybox;

class TitleScene : public BaseScene {

private: // メンバ変数(ゲーム用)
  // スカイボックス
  std::unique_ptr<Skybox> skybox_ = nullptr;

  // 3Dオブジェクト
  std::unique_ptr<Object3d> dragonObject_ = nullptr;
  std::unique_ptr<Object3d> cloudsObject_ = nullptr;
  std::unique_ptr<Object3d> cloudsObjectFar_ = nullptr;

  // ドラゴン配置・モーション用パラメータ
  Transform dragonTransform_{};
  Vector3 baseDragonPos_ = {0.0f, 0.0f, 0.0f};
  Vector3 baseDragonRot_ = {0.0f, 0.0f, 0.0f};
  float motionTimer_ = 0.0f;

  // 雲海スクロール
  float cloudsScrollZ_ = 0.0f;

  // シネマティックカメラ管理
public:
  enum class TitleCameraCut {
    RearWide,      // カット1: 後方ワイド追従
    FrontTracking, // カット2: 斜め前方並走
    OverTheWing,   // カット3: 翼越し（コックピット視点）
  };

private:
  TitleCameraCut currentCut_ = TitleCameraCut::RearWide;
  float cutTimer_ = 0.0f;
  const float kCutDuration_ = 7.0f; // 各カットの持続秒数
  bool isManualCut_ = false;        // デバッグ手動固定フラグ

  // ゲームスタート発進演出
  bool isStarting_ = false;
  float startTimer_ = 0.0f;
  const float kStartDuration_ = 1.3f;
  TitleCameraCut startCut_ = TitleCameraCut::RearWide;
  Vector3 startCamPos_{};
  Vector3 startCamRot_{};
  float startCamFov_ = 0.70f;
  Vector3 startDragonPos_{};
  Vector3 startDragonRot_{};
  bool hasTransitioned_ = false;

public:  // メンバ関数
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
  /// エディタUI描画
  /// </summary>
  void DrawEditorUI() override;

private: // メンバ変数(システム用)
         // カメラ
  std::unique_ptr<GameCamera> camera_ = nullptr;

  // デバッグカメラ
  std::unique_ptr<DebugCamera> debugCamera_ = nullptr;

  // デバッグカメラ使用
  bool useDebugCamera_ = false;

private:
  /*ポインタ参照
  ------------------*/
  // エンジン
  EngineBase *engine_ = nullptr;
};
