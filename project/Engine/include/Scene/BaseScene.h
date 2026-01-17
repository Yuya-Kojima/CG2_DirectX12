#pragma once

class EngineBase;
class SceneManager;
class GameCamera;

class BaseScene {

public:
	virtual ~BaseScene() = default;

	virtual void Initialize(EngineBase* engine) = 0;
	virtual void Finalize() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void Draw2D() = 0;
	virtual void Draw3D() = 0;

private:
	SceneManager* sceneManager_ = nullptr;

	GameCamera* activeCamera_ = nullptr;

public:
	virtual void SetSceneManger(SceneManager* sceneManager) {
		sceneManager_ = sceneManager;
	}

	void SetActiveCamera(GameCamera* camera);

	GameCamera* GetActiveCamera() const;
};
