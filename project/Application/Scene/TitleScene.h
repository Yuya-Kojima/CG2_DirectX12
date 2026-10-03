#pragma once
#include "Audio/SoundManager.h"
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Math/Transform.h"
#include "Scene/BaseScene.h"
#include <deque>
#include <vector>

class Object3d;
class DebugCamera;
class GameCamera;
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
  Vector3 currentLightDir_{0.55f, -0.45f, 0.65f}; // カット別ライト向きの現在値

  // 翼端トレイル履歴
  std::deque<Vector3> rightTrailHistory_;
  std::deque<Vector3> leftTrailHistory_;
  const size_t kMaxTrailHistory_ = 60;
  float trailWidth_ = 0.06f;
  float trailAlpha_ = 0.75f;
  float trailWindShift_ = 0.25f;
  bool enableTrail_ = true;

  // 気流・風ストリークパーティクル
  struct WindStreak {
    Vector3 pos;
    float baseLength;
    float speed;
    float alpha;
    float width;
  };
  std::vector<WindStreak> windStreaks_;
  bool enableWind_ = true;
  float windSpeedMult_ = 1.0f;
  float windAlpha_ = 0.70f;
  float windLengthMult_ = 1.0f;
  const size_t kMaxWindStreaks_ = 36;

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

private: // 更新サブ処理（パイプライン用プライベート関数）
  /// <summary>
  /// 注視回転角（ピッチ・ヨー）の計算ヘルパー
  /// </summary>
  static Vector3 CalcLookAtRot(const Vector3 &eye, const Vector3 &target);

  /// <summary>
  /// 入力・UIの更新（呼吸アニメーション・フェード）
  /// </summary>
  void UpdateUI();

  /// <summary>
  /// 発進演出の更新（カメラ・ドラゴンの加速、ポストプロセス連動）
  /// </summary>
  void UpdateLaunchSequence(Vector3 &outTargetCamPos, Vector3 &outTargetCamRot, float &outLaunchAccelCurve);

  /// <summary>
  /// 通常鑑賞モードの更新（待機飛行モーション、雲海スクロール、カット巡回）
  /// </summary>
  void UpdateIdleMotion(Vector3 &outTargetCamPos, Vector3 &outTargetCamRot);

  /// <summary>
  /// カメラカット連動ライティングの更新
  /// </summary>
  void UpdateLighting();

  /// <summary>
  /// カメラの更新とアクティブカメラの確定
  /// </summary>
  const ICamera *UpdateActiveCamera(const Vector3 &targetCamPos, const Vector3 &targetCamRot);

  /// <summary>
  /// 3Dオブジェクト（ドラゴン・雲海）の更新とDoF調整
  /// </summary>
  void Update3DObjects(const ICamera *activeCamera);

  /// <summary>
  /// 翼端トレイルの更新と描画登録
  /// </summary>
  void UpdateWingTrails(const ICamera *activeCamera, float launchAccelCurve);

  /// <summary>
  /// 気流・風ストリークの更新と描画登録
  /// </summary>
  void UpdateWindStreaks(const ICamera *activeCamera);
};
