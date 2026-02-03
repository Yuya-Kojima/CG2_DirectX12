#pragma once
#include "Audio/SoundManager.h"
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include <vector>
#include <memory>
#include <chrono>

class Sprite;
class Object3d;
class SpriteRenderer;
class Object3dRenderer;
class DebugCamera;
class InputKeyState;
class ParticleEmitter;

class TitleScene : public BaseScene {

private: // メンバ変数(ゲーム用)
public:  // メンバ関数
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(EngineBase* engine) override;

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

private: // メンバ変数(システム用)
	// カメラ
	GameCamera* camera_ = nullptr;

	// デバッグカメラ
	DebugCamera* debugCamera_ = nullptr;

	//デバッグカメラ使用
	bool useDebugCamera_ = false;

private:
	/*ポインタ参照
	------------------*/
	// エンジン
	EngineBase* engine_ = nullptr;

	// タイトル用3Dオブジェクト
	std::unique_ptr<Object3d> titleObject3d_;

	// ボタン用3Dオブジェクト
	std::unique_ptr<Object3d> button1Object3d_;
	std::unique_ptr<Object3d> button2Object3d_;

	// ピン用3Dオブジェクト
	std::unique_ptr<Object3d> pinObject3d_;

	// 0 = Start, 1 = Credits
	int selectedButtonIndex_ = 0;

	// クレジット表示中フラグ（TitleScene 内で表示）
	bool creditsActive_ = false;

	// ----- pin animation -----
	// 回転角（ラジアン） -- 置換: 常時回転から揺れに変更
	float pinYaw_ = 0.0f;
	// 回転速度（rad/s）
	float pinSpinSpeed_ = 1.5f;
	// ボビング（上下揺れ）振幅（ワールド単位）
	float pinBobAmplitude_ = 0.05f;
	// ボビング周波数（Hz）
	float pinBobFrequency_ = 2.0f;
	// 経過時間の積算（秒） -- 古い用途向けに残す
	float pinElapsed_ = 0.0f;
	// 最終更新時刻（フレーム間の dt を計算）
	std::chrono::steady_clock::time_point lastUpdateTime_{};

	// イージング移動用：開始位置、目標位置、移動時間、タイマー、フラグ
	Vector3 pinStartTranslation_{};
	Vector3 pinTargetTranslation_{};
	float pinMoveDuration_ = 0.15f; // 秒（移動にかける時間）
	float pinMoveTimer_ = 0.0f;
	bool pinIsMoving_ = false;

	// 左右揺れ（サイン波）用パラメータ
	float pinSwayAmplitude_ = 0.10f; // X方向振幅（ワールド単位）
	float pinSwayFrequency_ = 0.8f;  // 周波数（Hz）
	float pinSwayPhase_ = 0.0f;      // 位相（内部）
};