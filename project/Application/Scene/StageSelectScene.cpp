#include "StageSelectScene.h"
#include "Scene/SceneManager.h"
#include "Scene/StageSelection.h"
#include "Input/InputKeyState.h"
#include "Renderer/Object3dRenderer.h"
#include "Camera/GameCamera.h"
#include "Debug/DebugCamera.h"
#include "Model/ModelManager.h"
#include "Object3d/Object3d.h"
#include "Debug/ImGuiManager.h"
#include <cassert>

void StageSelectScene::Initialize(EngineBase* engine)
{
	engine_ = engine;

	// ステージ候補（ファイル名のみ）
	options_.push_back("level01.json");
	options_.push_back("level02.json");
	options_.push_back("level03.json");

	// 初期インデックスは以前の選択を反映
	const std::string& sel = StageSelection::GetSelected();
	auto it = std::find(options_.begin(), options_.end(), sel);
	if (it != options_.end())
	{
		currentIndex_ = static_cast<int>(std::distance(options_.begin(), it));
	}
	else
	{
		currentIndex_ = 0;
	}

	// カメラ生成
	camera_ = std::make_unique<GameCamera>();
	camera_->SetRotate({ 0.3f, 0.0f, 0.0f });
	camera_->SetTranslate({ cameraTranslate_[0], cameraTranslate_[1], cameraTranslate_[2] });

	// デバッグカメラ（任意）
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize({ 0.0f, 4.0f, -10.0f });

	// 表示するモデルを事前ロード（SetModel 前に LoadModel 必須）
	// 必要に応じて別モデル名に変更
	ModelManager::GetInstance()->LoadModel("stageSelect.obj");

	// Object3d を生成してモデルを割り当て
	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(engine_->GetObject3dRenderer());
	object3d_->SetModel("stageSelect.obj");
}

void StageSelectScene::Finalize()
{
	object3d_.reset();
	debugCamera_.reset();
	camera_.reset();

	engine_ = nullptr;
}

void StageSelectScene::Update()
{
	auto* input = engine_->GetInputManager();
	assert(input);

	// 左右で選択
	if (input->IsTriggerKey(DIK_LEFT))
	{
		if (currentIndex_ > 0) currentIndex_--;
		else currentIndex_ = static_cast<int>(options_.size()) - 1;
	}

	if (input->IsTriggerKey(DIK_RIGHT))
	{
		currentIndex_ = (currentIndex_ + 1) % static_cast<int>(options_.size());
	}

	// Enterで決定 -> GamePlay シーンへ（StageSelection にセット）
	if (input->IsTriggerKey(DIK_RETURN))
	{
		StageSelection::SetSelected(options_[currentIndex_]);
		SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
	}

	// Escでタイトルへ戻る
	if (input->IsTriggerKey(DIK_ESCAPE))
	{
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}

	// Pキーでデバッグカメラ切替
	if (input->IsTriggerKey(DIK_P))
	{
		useDebugCamera_ = !useDebugCamera_;
	}

	// ImGui でカメラを操作（UIは度）
#ifdef USE_IMGUI
	ImGui::Begin("Camera Controls");

	// Translate
	ImGui::DragFloat3("Translate", cameraTranslate_, 0.1f);

	// Rotate (deg)
	ImGui::DragFloat3("Rotate (deg)", cameraRotateDeg_, 0.5f, -360.0f, 360.0f);

	// オプション：リセットボタン
	if (ImGui::Button("Reset"))
	{
		cameraTranslate_[0] = 0.0f; cameraTranslate_[1] = 4.0f; cameraTranslate_[2] = -10.0f;
		cameraRotateDeg_[0] = 17.1887f; cameraRotateDeg_[1] = 0.0f; cameraRotateDeg_[2] = 0.0f;
	}
	ImGui::End();
#endif // USE_IMGUI

	// カメラへ反映（ImGui または通常更新）
	const float degToRad = 3.14159265358979323846f / 180.0f;
	if (useDebugCamera_)
	{
		if (debugCamera_)
		{
			debugCamera_->Update(*input);
			engine_->GetObject3dRenderer()->SetDefaultCamera(debugCamera_->GetCamera());
		}
	}
	else
	{
		if (camera_)
		{
			// ImGui がある場合は UI の値を優先して反映
			camera_->SetTranslate({ cameraTranslate_[0], cameraTranslate_[1], cameraTranslate_[2] });
			camera_->SetRotate({ cameraRotateDeg_[0] * degToRad, cameraRotateDeg_[1] * degToRad, cameraRotateDeg_[2] * degToRad });
			camera_->Update();
			engine_->GetObject3dRenderer()->SetDefaultCamera(camera_.get());
		}
	}

	if (object3d_)
	{
		object3d_->Update();
	}

#ifdef USE_IMGUI
	ImGui::Begin("StageSelect");
	ImGui::Text("Selected: %s", options_[currentIndex_].c_str());
	ImGui::Text("UseDebugCamera: %s", useDebugCamera_ ? "ON" : "OFF");
	ImGui::End();
#endif // USE_IMGUI
}

void StageSelectScene::Draw()
{
	Draw3D();
	Draw2D();
}

void StageSelectScene::Draw3D()
{
	engine_->Begin3D();
	// 3D の表示が必要ならここに記述
	if (object3d_)
	{
		object3d_->Draw();
	}
}

void StageSelectScene::Draw2D()
{
	engine_->Begin2D();
	// シンプル実装：現状は描画なし。
	// 将来、選択UI（Spriteやフォント）をここに追加してください。
}