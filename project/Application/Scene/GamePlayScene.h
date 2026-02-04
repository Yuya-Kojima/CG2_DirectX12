#pragma once
#include "Audio/SoundManager.h"
#include "Core/EngineBase.h"
#include "Math/MathUtil.h"
#include "Scene/BaseScene.h"
#include <vector>

class Sprite;
class Object3d;
class SpriteRenderer;
class Object3dRenderer;
class DebugCamera;
class InputKeyState;
class ParticleEmitter;
class Player;
class Level;
class Stick;
class Npc;
class Goal;

class GamePlayScene : public BaseScene {

private: // メンバ変数(ゲーム用)
	// カメラ
	GameCamera* camera_ = nullptr;

	// デバッグカメラ
	DebugCamera* debugCamera_ = nullptr;

	// デバッグカメラ使用
	bool useDebugCamera_ = false;
	// ImGui表示フラグ
	bool showImGui_ = false;
	// 固定カメラ強制フラグ
	bool forceFixedCamera_ = false;

public: // メンバ関数
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
private:
	/*ポインタ参照
	------------------*/
	// エンジン
	EngineBase* engine_ = nullptr;
	// player
	Player* player_ = nullptr;
	// actors
	std::vector<Stick*> sticks_;
	Npc* npc_ = nullptr;
	Goal* goal_ = nullptr;
	bool goalReached_ = false;
	Vector3 goalHalf_ = { 1.2f, 1.2f, 1.2f }; // goal AABB half extents
	bool goalHasAABB_ = false;
	// level
	Level* level_ = nullptr;
	float npcSpawnLeftOffset_ = 1.0f;
	// player spawn point saved from stage (used for respawn)
	Vector3 playerSpawn_ = { 0.0f, 0.0f, 0.0f };
	bool havePlayerSpawn_ = false;
	// last safe player position (inside stage and not on hazard)
	Vector3 lastSafePlayerPos_ = { 0.0f, 0.0f, 0.0f };
	bool haveLastSafePlayerPos_ = false;
	// (probe removed) 

	 // 天球（スカイドーム）
	std::unique_ptr<Object3d> skyObject3d_;
	float skyScale_ = 1.0f;
	float skyRotate_ = 0.0f;

	std::unique_ptr<Sprite> missonSprite_ = nullptr;
	Transform uvMissonfontSpriteTransform_{};

	std::unique_ptr<Sprite> misson1Sprite_ = nullptr;
	Transform uvMisson1SpriteTransform_{};

	std::unique_ptr<Sprite> misson2Sprite_ = nullptr;
	Transform uvMisson2SpriteTransform_{};

	std::unique_ptr<Sprite> operationSprite_ = nullptr;
	Transform uvOperationTransform_{};
};
