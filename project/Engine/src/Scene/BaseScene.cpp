#include "Scene/BaseScene.h"
#include "Core/EngineBase.h"
#include "Renderer/PostProcess.h"

void BaseScene::Initialize(EngineBase* engine) {
	postProcess_ = std::make_unique<PostProcess>();
	postProcess_->Initialize(engine->GetDx12Core());
}

BaseScene::BaseScene() = default;
BaseScene::~BaseScene() = default;

void BaseScene::SetActiveCamera(GameCamera* camera) {
	activeCamera_ = camera;
}

GameCamera* BaseScene::GetActiveCamera() const {
	return activeCamera_;
}