#pragma once
#include "Core/EngineBase.h"
#include "Scene/BaseScene.h"
#include <vector>
#include <string>
#include <memory>
#include "Math/MathUtil.h"

class GameCamera;
class DebugCamera;
class Object3d;

class StageSelectScene : public BaseScene {
public:

	void Initialize(EngineBase* engine) override;

	void Finalize() override;

	void Update() override;

	void Draw() override;

	void Draw2D() override;

	void Draw3D() override;

private:

	EngineBase* engine_ = nullptr;
	std::vector<std::string> options_;
	int currentIndex_ = 0;

	// カメラは必要なら後から拡張
	std::unique_ptr<GameCamera> camera_;
	std::unique_ptr<DebugCamera> debugCamera_;
	bool useDebugCamera_ = false;

	// ステージ選択画面用Object3d
	std::unique_ptr<Object3d> stage1Object3d_;
	std::unique_ptr<Object3d> stage2Object3d_;
	std::unique_ptr<Object3d> stage3Object3d_;
	float stage2Translate_[3] = { 100.0f, 0.0f, 0.0f };
	float stage2RotateDeg_[3] = { 0.0f, 0.0f, 0.0f };
	float stage2Scale_[3] = { 1.0f, 1.0f, 1.0f };

	// ステージUI用Object3d (既存の単体保持は残す)
	std::unique_ptr<Object3d> stageUIObject3d_;
	// 単体設定用
	float stageUITranslate_[3] = { 0.0f, 10.0f, 10.0f };
	float stageUIRotateDeg_[3] = { 0.0f, 0.0f, 0.0f };
	float stageUIScale_[3] = { 3.5f, 3.5f, 3.5f };

	// 追加: ステージ数分の Stage UI を保持する配列
	std::vector<std::unique_ptr<Object3d>> stageUIObjects_;

	// アニメーション制御（各 UI ごと）
	std::vector<bool> stageUIVisible_;
	std::vector<bool> stageUIAnimating_;
	std::vector<float> stageUIAnimTimer_; 
	std::vector<Vector3> stageUIBaseScale_;

	// プレイヤーモデル用Object3d
	std::unique_ptr<Object3d> playerObject3d_;
	float playerTranslate_[3] = { 0.0f, 0.0f, 0.0f };
	float playerRotateDeg_[3] = { 0.0f, 0.0f, 0.0f };
	float playerScale_[3] = { 2.5f, 2.5f, 2.5f };

	// 滑らか移動用ターゲットとフラグ
	Vector3 playerTargetTranslate_{ 0.0f, 0.0f, 0.0f };
	
	// プレイヤーが移動中かどうか
	bool playerMoving_ = false;
	
	// 前フレームの移動フラグ（停止検出用）
	bool prevPlayerMoving_ = false;

	// 補間スピード
	float playerMoveSpeed_ = 6.0f;
	
	// この距離未満でスナップ
	float playerSnapThreshold_ = 0.01f;

	// プレイヤー回転の補間速度（度／秒）
	float playerRotateSpeedDeg_ = 720.0f;

	// 停止時に正面へ戻るときの速さ（度／秒）
	float playerReturnRotateSpeedDeg_ = 1440.0f;

	// 停止直前に回転を開始する距離
	float playerReturnStartDistance_ = 0.5f;

	// 初期の角度を保持用
	float playerInitialFrontDeg_ = 180.0f;

	// 各ステージに対応するプレイヤー表示位置（X,Y,Z）
	std::vector<Vector3> stagePositions_;

	// プレイヤーをステージ位置に対してオフセットして表示したい場合のxのオフセット
	float playerXOffset_ = 0.0f;

	// 天球（スカイドーム）
	std::unique_ptr<Object3d> skyObject3d_;
	float skyScale_ = 1.0f;
	float skyRotate_ = 0.0f;

	// カメラ現在値
	float cameraTranslate_[3] = { 0.0f, 44.2f, -86.5f };
	float cameraRotateDeg_[3] = { 20.0f, 0.0f, 0.0f };

	// カメラの目標位置。遷移時はこの値をセットして補間する
	float cameraTargetTranslate_[3] = { 0.0f, 44.2f, -86.5f };
	// カメラが目標へ向かって移動中か
	bool cameraMoving_ = false;
	// カメラ移動速度
	float cameraMoveSpeed_ = 5.0f;
	// カメラのスナップ閾値
	float cameraSnapThreshold_ = 0.01f;

	// 表示アニメーション長さ（秒）
	float stageUIAnimDuration_ = 0.5f;

	// プレイヤー遷移(シーン移行演出)用
	bool transitionActive_ = false;
	float transitionTimer_ = 0.0f;
	float transitionDuration_ = 0.9f; 
	Vector3 transitionStartPos_;
	Vector3 transitionTargetPos_;
	Vector3 transitionStartScale_;
	Vector3 transitionTargetScale_;
	float transitionStartRotateYDeg_ = 0.0f;
	float transitionSpinDeg_ = 720.0f; 
};