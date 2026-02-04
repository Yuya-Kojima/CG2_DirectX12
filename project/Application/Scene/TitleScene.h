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
	std::unique_ptr<Object3d> titleFloorObject3d_;
	std::unique_ptr<Object3d> titleBotObject3d_;
	std::unique_ptr<Object3d> titleBot1Object3d_;

	// ボタン用3Dオブジェクト
	std::unique_ptr<Object3d> button1Object3d_;
	std::unique_ptr<Object3d> button2Object3d_;

	// ピン用3Dオブジェクト
	std::unique_ptr<Object3d> pinObject3d_;

	// クレジット用3Dオブジェクト
	std::unique_ptr<Object3d> Credits3d_;
	std::unique_ptr<Object3d> CreditsSounds3d_;

	// 操作方法用3Dオブジェクト
	std::unique_ptr<Object3d> controls1Object3d_;
	std::unique_ptr<Object3d> controls2Object3d_;

	// 天球（スカイドーム）
	std::unique_ptr<Object3d> skyObject3d_;
	float skyScale_ = 1.0f;
	float skyRotate_ = 0.0f;

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

	// ----- title bobbing -----
	// タイトルモデルの上下揺れ用パラメータ
	float titleBobAmplitude_ = 0.05f; // Y方向振幅（ワールド単位）
	float titleBobFrequency_ = 0.6f;  // 周波数（Hz）
	float titleBobPhase_ = 0.0f;      // 位相（内部）
	Vector3 titleStartTranslation_{};  // 基準位置（初期位置）

	// ----- title bottom sequence -----
	// titleBot のシーケンシャルな移動とモデル差し替えを行うための状態
	enum class TitleBotPhase {
		FirstMove,   // 最初に左へ移動（X: 0 -> -7.1）
		SecondMove,  // モデル差替え後 0.0 から右へ移動（X: 0 -> +7.1）
		Done
	};
	TitleBotPhase titleBotPhase_ = TitleBotPhase::FirstMove;

	// 等速直線運動用パラメータ（titleBot）
	float titleBotSpeed_ = 2.0f; // world units / sec（等速）
	int titleBotDirection_ = -1; // -1 = 左へ, +1 = 右へ
	bool titleBotIsMoving_ = false;

	// --- pad previous axis values (for trigger detection on axes) ---
	// 左スティックの前フレーム Y（タイトル：上下入力判定用）
	float prevPadLeftY_ = 0.0f;
};