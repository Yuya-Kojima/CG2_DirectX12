#include "Scene/BaseScene.h"

void BaseScene::SetActiveCamera(GameCamera* camera) {
	activeCamera_ = camera;
}

GameCamera* BaseScene::GetActiveCamera() const {
	return activeCamera_;
}