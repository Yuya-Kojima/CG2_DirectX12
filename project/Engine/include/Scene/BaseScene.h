#pragma once
#include <memory>

class EngineBase;
class SceneManager;
class GameCamera;
class PostProcess;

class BaseScene {

public:
	BaseScene();
	virtual ~BaseScene();

	virtual void Initialize(EngineBase* engine);
	virtual void Finalize() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void Draw2D() = 0;
	virtual void Draw3D() = 0;

private:
	SceneManager* sceneManager_ = nullptr;

	GameCamera* activeCamera_ = nullptr;

protected:
	std::unique_ptr<PostProcess> postProcess_ = nullptr;

public:
	virtual void SetSceneManger(SceneManager* sceneManager) {
		sceneManager_ = sceneManager;
	}

	void SetActiveCamera(GameCamera* camera);

	GameCamera* GetActiveCamera() const;

	PostProcess* GetPostProcess() const { return postProcess_.get(); }
};
