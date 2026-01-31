#define NOMINMAX
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
#include <cmath>
#include <filesystem>

void StageSelectScene::Initialize(EngineBase* engine)
{
	engine_ = engine;

	// -------------------------
	// ステージ一覧を自動検出（優先パス順）
	// -------------------------
	options_.clear();
	const std::vector<std::string> searchPaths = {
		"Application/resources/stages",
		"resources/stages"
	};

	for (const auto& base : searchPaths) {
		if (!std::filesystem::exists(base)) continue;

		std::vector<std::string> found;
		for (const auto& entry : std::filesystem::directory_iterator(base)) {
			if (!entry.is_regular_file()) continue;
			auto p = entry.path();
			if (p.has_extension() && p.extension() == ".json") {
				found.push_back(p.filename().generic_string());
			}
		}
		if (!found.empty()) {
			// 名前順でソートして安定化
			std::sort(found.begin(), found.end());
			options_ = std::move(found);
			// 優先パスで見つかったらそれを使う
			break;
		}
	}

	// 見つからなければデフォルトを用意
	if (options_.empty()) {
		options_.push_back("level01.json");
		options_.push_back("level02.json");
		options_.push_back("level03.json");
	}

	// -------------------------
	// プレイヤー表示位置を自動配置（横並び・中央揃え）
	// -------------------------
	stagePositions_.clear();
	// ステージ間の間隔
	const float spacing = 15.0f;
	const float baseY = 3.3f;
	const float baseZ = 0.0f;
	const int n = static_cast<int>(options_.size());
	for (int i = 0; i < n; ++i) {
		// 中央揃え
		float x = (i - (n - 1) * 0.5f) * spacing;
		stagePositions_.push_back({ x, baseY, baseZ });
	}

	// -------------------------
	// 選択状態の復元
	// -------------------------
	const std::string& sel = StageSelection::GetSelected();
	auto it = std::find(options_.begin(), options_.end(), sel);
	if (it != options_.end()) {
		currentIndex_ = static_cast<int>(std::distance(options_.begin(), it));
	} else {
		currentIndex_ = 0;
	}


	// カメラ生成
	camera_ = std::make_unique<GameCamera>();
	camera_->SetRotate({ 0.3f, 0.0f, 0.0f });
	camera_->SetTranslate({ cameraTranslate_[0], cameraTranslate_[1], cameraTranslate_[2] });

	// デバッグカメラ
	debugCamera_ = std::make_unique<DebugCamera>();
	debugCamera_->Initialize({ 0.0f, 4.0f, -10.0f });

	if (auto* renderer = engine_->GetObject3dRenderer()) {
		// 平行光を少し斜め前方から当てる
		if (auto* dl = renderer->GetDirectionalLightData()) {
			dl->color = Vector4(1.0f, 0.98f, 0.92f, 1.0f);
			dl->direction = Normalize(Vector3(-0.3f, -1.0f, -0.2f));
			dl->intensity = 1.0f;
		}
	}

	// 表示するモデルを事前ロード
	ModelManager::GetInstance()->LoadModel("stageSelect.obj");

	// Object3d を生成してモデルを割り当て
	object3d_ = std::make_unique<Object3d>();
	object3d_->Initialize(engine_->GetObject3dRenderer());
	object3d_->SetModel("stageSelect.obj");

	// --- 天球モデルの用意 ---
	ModelManager::GetInstance()->LoadModel("SkyDome.obj");
	skyObject3d_ = std::make_unique<Object3d>();
	skyObject3d_->Initialize(engine_->GetObject3dRenderer());
	skyObject3d_->SetModel("SkyDome.obj");
	skyObject3d_->SetTranslation({ 0.0f, 0.0f, 0.0f });
	skyObject3d_->SetScale({ skyScale_, skyScale_, skyScale_ });
	skyObject3d_->SetRotation({ 0.0f, 0.0f, 0.0f });

	// --- プレイヤーモデルの用意 ---
	ModelManager::GetInstance()->LoadModel("player.obj");
	playerObject3d_ = std::make_unique<Object3d>();
	playerObject3d_->Initialize(engine_->GetObject3dRenderer());
	playerObject3d_->SetModel("player.obj");

	// transformをstagePositions_に合わせて設定
	const Vector3& sp = stagePositions_[static_cast<size_t>(currentIndex_)];
	playerTranslate_[0] = sp.x; playerTranslate_[1] = sp.y; playerTranslate_[2] = sp.z;

	// ターゲットを初期位置に設定（瞬間移動を防ぐ）
	playerTargetTranslate_ = sp;
	playerMoving_ = false;

	playerRotateDeg_[0] = 0.0f; playerRotateDeg_[1] = 180.0f; playerRotateDeg_[2] = 0.0f;
	// 初期の正面角を保存
	playerInitialFrontDeg_ = playerRotateDeg_[1];

	playerScale_[0] = 2.5f; playerScale_[1] = 2.5f; playerScale_[2] = 2.5f;

	playerObject3d_->SetTranslation({ playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] });
	const float degToRad = 3.14159265358979323846f / 180.0f;
	playerObject3d_->SetRotation({ playerRotateDeg_[0] * degToRad, playerRotateDeg_[1] * degToRad, playerRotateDeg_[2] * degToRad });
	playerObject3d_->SetScale({ playerScale_[0], playerScale_[1], playerScale_[2] });
}

void StageSelectScene::Finalize()
{
	// オブジェクトを解放
	object3d_.reset();
	playerObject3d_.reset();
	skyObject3d_.reset();
	debugCamera_.reset();
	camera_.reset();

	engine_ = nullptr;
}

void StageSelectScene::Update()
{
	auto* input = engine_->GetInputManager();
	assert(input);

	const float dt = 1.0f / 60.0f;
	const float degToRad = 3.14159265358979323846f / 180.0f;



	if (object3d_) {
		object3d_->Update();
	}
	
	// プレイヤーのObject3dの更新	
	if (playerObject3d_) {
		playerObject3d_->Update();
	}

	// --- A/Dキーでステージ切替(プレイヤー表示位置を次/前のステージ位置へ)---

	if (playerObject3d_) {

		if (input->IsTriggerKey(DIK_A)) {
			// 左へ：前のステージ
			if (currentIndex_ > 0) currentIndex_--;
			else currentIndex_ = static_cast<int>(options_.size()) - 1;
			const Vector3& sp = stagePositions_[static_cast<size_t>(currentIndex_)];
			playerTargetTranslate_ = sp;
			playerMoving_ = true;
		}
		if (input->IsTriggerKey(DIK_D)) {
			// 右へ：次のステージ
			currentIndex_ = (currentIndex_ + 1) % static_cast<int>(options_.size());
			const Vector3& sp = stagePositions_[static_cast<size_t>(currentIndex_)];
			playerTargetTranslate_ = sp;
			playerMoving_ = true;
		}

		// --- 滑らか移動処理 ---
		if (playerMoving_) {
			Vector3 curr{ playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] };
			// 補間率
			float alpha = std::min(1.0f, playerMoveSpeed_ * dt);
			Vector3 next = Lerp(curr, playerTargetTranslate_, alpha);

			// スナップ判定
			Vector3 diff = { next.x - playerTargetTranslate_.x, next.y - playerTargetTranslate_.y, next.z - playerTargetTranslate_.z };
			float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
			if (distSq <= playerSnapThreshold_ * playerSnapThreshold_) {
				// 到達
				next = playerTargetTranslate_;
				playerMoving_ = false;
			}

			// --- プレイヤーの回転 ---
			Vector3 moveVec = { playerTargetTranslate_.x - curr.x, 0.0f, playerTargetTranslate_.z - curr.z };
			const float eps = 1e-6f;
			if (std::abs(moveVec.x) > eps || std::abs(moveVec.z) > eps) {
				int sideSign = 1;
				if (std::abs(moveVec.x) > eps) {
					sideSign = (moveVec.x > 0.0f) ? 1 : -1;
				} else {
					// X がほぼ 0 の場合は Z の方向で決める
					sideSign = (moveVec.z >= 0.0f) ? 1 : -1;
				}

				// 初期正面を基準に ±90deg の相対角を作る
				float sideDeg = playerInitialFrontDeg_ + (sideSign > 0 ? -90.0f : 90.0f);

				// 残り距離を計算
				float remainX = playerTargetTranslate_.x - curr.x;
				float remainZ = playerTargetTranslate_.z - curr.z;
				float remainDist = std::sqrt(remainX * remainX + remainZ * remainZ);

				// 停止直前から正面へ補間を開始する
				float desiredDeg = sideDeg;
				if (remainDist < playerReturnStartDistance_) {
					// 0 -> startDistance の範囲で t=0..1
					float t = 1.0f - (remainDist / std::max(1e-6f, playerReturnStartDistance_));
					if (t < 0.0f) t = 0.0f;
					if (t > 1.0f) t = 1.0f;
					// 線形補間
					desiredDeg = sideDeg + (playerInitialFrontDeg_ - sideDeg) * t;
				}

				// 現在角度（Y軸、度）
				float currentDeg = playerRotateDeg_[1];

				// 差を [-180,180] に丸める
				float delta = desiredDeg - currentDeg;
				while (delta > 180.0f) delta -= 360.0f;
				while (delta < -180.0f) delta += 360.0f;

				// 回転速度は移動中と停止時で分けて扱う
				float maxDelta = playerRotateSpeedDeg_ * dt;

				if (remainDist < playerReturnStartDistance_ * 0.5f) {
					// 近づいたらもう少し速く回す
					float extra = playerReturnRotateSpeedDeg_ * dt * 0.5f;
					maxDelta = std::max(maxDelta, extra);
				}

				if (std::abs(delta) <= maxDelta) {
					currentDeg = desiredDeg;
				} else {
					currentDeg += (delta > 0.0f ? maxDelta : -maxDelta);
				}
				playerRotateDeg_[1] = currentDeg;

				// 角度をラジアンにして Object3d に反映
				playerObject3d_->SetRotation({ playerRotateDeg_[0] * degToRad, playerRotateDeg_[1] * degToRad, playerRotateDeg_[2] * degToRad });
			}

			playerTranslate_[0] = next.x;
			playerTranslate_[1] = next.y;
			playerTranslate_[2] = next.z;

		} else {
			// --- 移動完了後：初期正面に戻る処理（戻る速度を速く） ---
			const float targetFrontDeg = playerInitialFrontDeg_;
			float currentDeg = playerRotateDeg_[1];
			float delta = targetFrontDeg - currentDeg;
			while (delta > 180.0f) delta -= 360.0f;
			while (delta < -180.0f) delta += 360.0f;
			// ここで停止時専用の速さを使用する
			float maxDelta = playerReturnRotateSpeedDeg_ * dt;

			if (std::abs(delta) <= maxDelta) {
				currentDeg = targetFrontDeg;
			} else {
				currentDeg += (delta > 0.0f ? maxDelta : -maxDelta);
			}

			playerRotateDeg_[1] = currentDeg;
			playerObject3d_->SetRotation({ playerRotateDeg_[0] * degToRad, playerRotateDeg_[1] * degToRad, playerRotateDeg_[2] * degToRad });
		}

		// 変更を反映（ImGui と競合しないよう即時反映）
		playerObject3d_->SetTranslation({ playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] });
	}

	// Enterで決定 -> GamePlay シーンへ
	if (!playerMoving_ && input->IsTriggerKey(DIK_RETURN)) {
		StageSelection::SetSelected(options_[currentIndex_]);
		SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
	}

	// Escでタイトルへ戻る
	if (input->IsTriggerKey(DIK_ESCAPE)) {
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}



	// 天球の更新
	if (skyObject3d_) {
		const ICamera* activeCam = engine_->GetObject3dRenderer()->GetDefaultCamera();
		if (activeCam) {
			playerObject3d_ ? void(0) : void(0);
			// camera の位置を取得して天球の中心に設定
			Vector3 camPos = activeCam->GetTranslate();
			skyObject3d_->SetTranslation({ camPos.x, camPos.y, camPos.z });
		}
		// ゆっくり自転
		skyRotate_ += 0.0005f;
		if (skyRotate_ > 3.14159265f * 2.0f) skyRotate_ -= 3.14159265f * 2.0f;
		skyObject3d_->SetRotation({ 0.0f, skyRotate_, 0.0f });
		skyObject3d_->Update();
	}



	//=================================================
	// デバック表示
	//=================================================

#ifdef USE_IMGUI

	// Pキーでデバッグカメラ切替
	if (input->IsTriggerKey(DIK_P)) {
		useDebugCamera_ = !useDebugCamera_;
	}

	// カメラへ反映（ImGui または通常更新）
	if (useDebugCamera_) {
		if (debugCamera_) {
			debugCamera_->Update(*input);
			engine_->GetObject3dRenderer()->SetDefaultCamera(debugCamera_->GetCamera());
		}
	} else {

		if (camera_) {
			// ImGui がある場合は UI の値を優先して反映
			camera_->SetTranslate({ cameraTranslate_[0], cameraTranslate_[1], cameraTranslate_[2] });
			camera_->SetRotate({ cameraRotateDeg_[0] * degToRad, cameraRotateDeg_[1] * degToRad, cameraRotateDeg_[2] * degToRad });
			camera_->Update();
			engine_->GetObject3dRenderer()->SetDefaultCamera(camera_.get());
		}
	}

#endif // USE_IMGUI

	// ImGui でカメラを操作（UIは度）
#ifdef USE_IMGUI
	ImGui::Begin("Camera Controls");

	// Translate
	ImGui::DragFloat3("Translate", cameraTranslate_, 0.1f);

	// Rotate (deg)
	ImGui::DragFloat3("Rotate (deg)", cameraRotateDeg_, 0.5f, -360.0f, 360.0f);

	// オプション：リセットボタン
	if (ImGui::Button("Reset")) {
		cameraTranslate_[0] = 0.0f; cameraTranslate_[1] = 4.0f; cameraTranslate_[2] = -10.0f;
		cameraRotateDeg_[0] = 17.1887f; cameraRotateDeg_[1] = 0.0f; cameraRotateDeg_[2] = 0.0f;
	}
	ImGui::End();
#endif // USE_IMGUI

	// プレイヤーの ImGui 操作パネル（位置・回転・スケール）
#ifdef USE_IMGUI
	if (playerObject3d_) {
		ImGui::Begin("Player (Preview)");

		ImGui::DragFloat3("Translate", playerTranslate_, 0.05f);
		ImGui::DragFloat3("Rotate (deg)", playerRotateDeg_, 1.0f, -360.0f, 360.0f);
		ImGui::DragFloat3("Scale", playerScale_, 0.05f, 0.01f, 20.0f);

		// 設定を即時反映
		playerObject3d_->SetTranslation({ playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] });
		playerObject3d_->SetRotation({ playerRotateDeg_[0] * degToRad, playerRotateDeg_[1] * degToRad, playerRotateDeg_[2] * degToRad });
		playerObject3d_->SetScale({ playerScale_[0], playerScale_[1], playerScale_[2] });

		ImGui::End();
	}
#endif // USE_IMGUI

#ifdef USE_IMGUI
	ImGui::Begin("StageSelect");
	ImGui::Text("Selected: %s", options_[currentIndex_].c_str());
	ImGui::Text("UseDebugCamera: %s", useDebugCamera_ ? "ON" : "OFF");
	ImGui::End();
#endif // USE_IMGUI

#ifdef USE_IMGUI
	// ライティング調整パネル（StageSelect 用）
	{
		auto* renderer = engine_->GetObject3dRenderer();
		if (renderer) {
			auto* dl = renderer->GetDirectionalLightData();
			auto* pl = renderer->GetPointLightData();
			auto* sl = renderer->GetSpotLightData();

			ImGui::Begin("Lighting (StageSelect)");

			if (dl) {
				ImGui::Text("Directional Light");
				ImGui::ColorEdit3("Dir Color", &dl->color.x);
				ImGui::DragFloat3("Dir Direction", &dl->direction.x, 0.01f, -1.0f, 1.0f);
				ImGui::DragFloat("Dir Intensity", &dl->intensity, 0.01f, 0.0f, 10.0f);
				// 正規化して向きを保つ
				dl->direction = Normalize(dl->direction);
			}

			if (pl) {
				ImGui::Separator();
				ImGui::Text("Point Light");
				ImGui::ColorEdit3("Point Color", &pl->color.x);
				ImGui::DragFloat3("Point Position", &pl->position.x, 0.05f, -100.0f, 100.0f);
				ImGui::DragFloat("Point Intensity", &pl->intensity, 0.01f, 0.0f, 10.0f);
				ImGui::DragFloat("Point Radius", &pl->radius, 0.1f, 0.1f, 200.0f);
				ImGui::DragFloat("Point Decay", &pl->decay, 0.01f, 0.01f, 8.0f);
			}

			if (sl) {
				ImGui::Separator();
				ImGui::Text("Spot Light");
				ImGui::ColorEdit3("Spot Color", &sl->color.x);
				ImGui::DragFloat3("Spot Position", &sl->position.x, 0.05f, -100.0f, 100.0f);
				ImGui::DragFloat3("Spot Direction", &sl->direction.x, 0.01f, -1.0f, 1.0f);
				ImGui::DragFloat("Spot Intensity", &sl->intensity, 0.01f, 0.0f, 10.0f);
				ImGui::DragFloat("Spot Distance", &sl->distance, 0.1f, 0.01f, 200.0f);
				ImGui::DragFloat("Spot Decay", &sl->decay, 0.01f, 0.01f, 8.0f);
				// 余弦は UI 上では角度で操作するのが親切だがここは直接編集しない
				sl->direction = Normalize(sl->direction);
			}

			// 天球を明るくするための簡易コントロール:
			// カメラ位置に点光を置いて天球だけを明るく見せる用途に使えます。
			static float skyBrightness = 0.0f;
			if (ImGui::SliderFloat("Sky Brightness (point)", &skyBrightness, 0.0f, 5.0f)) {
				if (pl) {
					const ICamera* cam = renderer->GetDefaultCamera();
					if (cam) {
						// 点光をカメラ位置へ移動して天球を局所的に明るくする
						pl->position = cam->GetTranslate();
					}
					pl->intensity = skyBrightness;
					pl->radius = 30.0f * (0.5f + skyBrightness); // 明るさに応じて届く範囲を拡大
				}
				// ついでに平行光の最低照度を上げることで天球が暗くなり過ぎる問題を緩和
				if (dl) {
					dl->intensity = std::max(dl->intensity, 0.15f);
				}
			}

			ImGui::End();
		}
	}
#endif
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
	// 天球は最初に描画して常に背景に表示
	if (skyObject3d_) {
		skyObject3d_->Draw();
	}

	// プレイヤーモデルを先に描画してから背景などを描く
	if (playerObject3d_) {
		playerObject3d_->Draw();
	}
	
	if (object3d_) {
		object3d_->Draw();
	}
}

void StageSelectScene::Draw2D()
{
	engine_->Begin2D();
	// シンプル実装：現状は描画なし。
	// 選択UI（Spriteやフォント）をここに追加
}