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

class StageSelectScene : public BaseScene
{
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
	std::unique_ptr<Object3d> object3d_;
	bool useDebugCamera_ = false;

	// プレイヤーモデル用Object3d
	std::unique_ptr<Object3d> playerObject3d_;
	// ImGuiで編集するためのワーク変数（回転は度でUIを扱う）
	float playerTranslate_[3] = { 0.0f, 0.0f, 0.0f };
	float playerRotateDeg_[3] = { 0.0f, 0.0f, 0.0f };
	float playerScale_[3] = { 1.0f, 1.0f, 1.0f };

	// 各ステージに対応するプレイヤー表示位置（X,Y,Z）
	std::vector<Vector3> stagePositions_;

	// 天球（スカイドーム）
	std::unique_ptr<Object3d> skyObject3d_;
	float skyScale_ = 1.0f;
	float skyRotate_ = 0.0f;

	float cameraTranslate_[3] = { 0.0f, 44.2f, -86.5f };
	float cameraRotateDeg_[3] = { 20.0f, 0.0f, 0.0f };
};