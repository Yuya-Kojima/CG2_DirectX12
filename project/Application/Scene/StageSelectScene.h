#pragma once
#include "Core/EngineBase.h"
#include "Scene/BaseScene.h"
#include <vector>
#include <string>

#include <memory>

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

	float cameraTranslate_[3] = { 0.0f, 46.5f, -81.4f };
	float cameraRotateDeg_[3] = { 26.0f, 0.0f, 0.0f };
};