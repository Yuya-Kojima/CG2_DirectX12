#include "TitleScene.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Input/InputKeyState.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Particle/Particle.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Scene/SceneManager.h"
#include "Sprite/Sprite.h"
#include "Texture/TextureManager.h"
#include "Debug/ImGuiManager.h"

void TitleScene::Initialize(EngineBase *engine) {

	// 参照をコピー
	engine_ = engine;

	//===========================
	// テクスチャファイルの読み込み
	//===========================

	//===========================
	// オーディオファイルの読み込み
	//===========================

	//===========================
	// スプライト関係の初期化
	//===========================

	//===========================
	// 3Dオブジェクト関係の初期化
	//===========================

	// カメラの生成と初期化
	camera_ = new GameCamera();
	camera_->SetRotate({ 0.3f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 4.0f, -10.0f });

	// デバッグカメラ
	debugCamera_ = new DebugCamera();
	debugCamera_->Initialize({ 0.0f, 4.0f, -10.0f });

	// デフォルトカメラのセット
	engine_->GetObject3dRenderer()->SetDefaultCamera(camera_);

	// モデルの読み込み

	// オブジェクトの生成と初期化

	//===========================
	// パーティクル関係の初期化
	//===========================
}

void TitleScene::Finalize() {

	delete debugCamera_;
	debugCamera_ = nullptr;

	delete camera_;
	camera_ = nullptr;
}

void TitleScene::Update() {

	// Sound更新
	SoundManager::GetInstance()->Update();

	// タイトルシーンへ移行
	if (engine_->GetInputManager()->IsTriggerKey(DIK_SPACE))
	{
		SceneManager::GetInstance()->ChangeScene("STAGESELECT");
	}

	//デバッグカメラ切り替え
	if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
		if (useDebugCamera_) {
			useDebugCamera_ = false;
		} else {
			useDebugCamera_ = true;
		}
	}

	//=======================
	// スプライトの更新
	//=======================

	//=======================
	// 3Dオブジェクトの更新
	//=======================


	//=======================
	// カメラの更新
	//=======================
	const ICamera* activeCamera = nullptr;

    if (useDebugCamera_) {
        debugCamera_->Update(*engine_->GetInputManager());
        activeCamera = debugCamera_->GetCamera();
    } else {
        camera_->Update();
        activeCamera = camera_;
    }

	//アクティブカメラを描画で使用する
	engine_->GetObject3dRenderer()->SetDefaultCamera(activeCamera);

#ifdef USE_IMGUI

	// デモウィンドウ表示ON
	ImGui::ShowDemoWindow();

	// 現在アクティブなシーン名を表示
	ImGui::Begin("Scene Info");
	ImGui::Text("TitleScene");
	ImGui::End();
#endif // USE_IMGUI

}

void TitleScene::Draw() {
	Draw3D();
	Draw2D();
}

void TitleScene::Draw3D() {
	engine_->Begin3D();

	//ここから下で3DオブジェクトのDrawを呼ぶ
}

void TitleScene::Draw2D() {
	engine_->Begin2D();

	//ここから下で2DオブジェクトのDrawを呼ぶ

}