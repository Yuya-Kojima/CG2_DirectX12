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
#include <algorithm>

namespace {
	// easing: easeOutBack
	static float EaseOutBack(float t)
	{
		const float c1 = 1.70158f;
		const float c3 = c1 + 1.0f;
		float p = t - 1.0f;
		return 1.0f + c3 * p * p * p + c1 * p * p;
	}
}

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
	// 選択状態の復元（位置計算の前に行う）
	// -------------------------
	const std::string& sel = StageSelection::GetSelected();
	auto it = std::find(options_.begin(), options_.end(), sel);
	if (it != options_.end()) {
		currentIndex_ = static_cast<int>(std::distance(options_.begin(), it));
	} else {
		currentIndex_ = 0;
	}

	// -------------------------
	// プレイヤー表示位置を自動配置（選択中を中央に固定）
	// ただし index4<->5 の間だけギャップを specialGap にする
	// -------------------------
	stagePositions_.clear();
	// ステージ間の通常間隔
	const float spacing = 15.0f;
	// ステージ5->6 の間だけこのギャップ
	const float specialGap = 40.0f;
	const float baseY = 3.3f;
	const float baseZ = 0.0f;
	const int n = static_cast<int>(options_.size());

	if (n > 0) {
		stagePositions_.resize(n);
		// currentIndex_ を中心 (x = 0) に置く
		stagePositions_[currentIndex_] = { 0.0f, baseY, baseZ };

		// 右側を順に配置（currentIndex_ + 1 .. n-1）
		for (int i = currentIndex_ + 1; i < n; ++i) {
			// i-1 と i の間が 4->5（0-based）なら specialGap を使う
			float gap = ((i - 1) == 4) ? specialGap : spacing;
			float prevX = stagePositions_[i - 1].x;
			stagePositions_[i] = { prevX + gap, baseY, baseZ };
		}

		// 左側を順に配置（currentIndex_ - 1 .. 0）
		for (int i = currentIndex_ - 1; i >= 0; --i) {
			// i と i+1 の間が 4->5（つまり i==4）なら specialGap
			float gap = (i == 4) ? specialGap : spacing;
			float nextX = stagePositions_[i + 1].x;
			stagePositions_[i] = { nextX - gap, baseY, baseZ };
		}
	}

	// カメラ生成
	camera_ = std::make_unique<GameCamera>();
	camera_->SetRotate({ 0.3f, 0.0f, 0.0f });
	camera_->SetTranslate({ cameraTranslate_[0], cameraTranslate_[1], cameraTranslate_[2] });
	camera_->SetFarClip(1000.0f);

	cameraTargetTranslate_[0] = cameraTranslate_[0];
	cameraTargetTranslate_[1] = cameraTranslate_[1];
	cameraTargetTranslate_[2] = cameraTranslate_[2];
	cameraMoving_ = false;

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

	// --- ステージモデルの用意 ---
	ModelManager::GetInstance()->LoadModel("stageSelect.obj");
	stage1Object3d_ = std::make_unique<Object3d>();
	stage1Object3d_->Initialize(engine_->GetObject3dRenderer());
	stage1Object3d_->SetModel("stageSelect.obj");

	ModelManager::GetInstance()->LoadModel("stageSelect2.obj");
	stage2Object3d_ = std::make_unique<Object3d>();
	stage2Object3d_->Initialize(engine_->GetObject3dRenderer());
	stage2Object3d_->SetModel("stageSelect2.obj");
	stage2Object3d_->SetTranslation({ stage2Translate_[0], stage2Translate_[1], stage2Translate_[2] });
	stage2Object3d_->SetScale({ stage2Scale_[0], stage2Scale_[1], stage2Scale_[2] });

	// --- 天球モデルの用意 ---
	ModelManager::GetInstance()->LoadModel("SkyDome.obj");
	skyObject3d_ = std::make_unique<Object3d>();
	skyObject3d_->Initialize(engine_->GetObject3dRenderer());
	skyObject3d_->SetModel("SkyDome.obj");
	skyObject3d_->SetTranslation({ 0.0f, 0.0f, 0.0f });
	skyObject3d_->SetScale({ skyScale_, skyScale_, skyScale_ });
	skyObject3d_->SetRotation({ 0.0f, 0.0f, 0.0f });

	// --- プレイヤーモデルの用意 ---
	ModelManager::GetInstance()->LoadModel("player_body.obj");
	playerObject3d_ = std::make_unique<Object3d>();
	playerObject3d_->Initialize(engine_->GetObject3dRenderer());
	playerObject3d_->SetModel("player_body.obj");
	playerObject3d_->SetScale({ playerScale_[0], playerScale_[1], playerScale_[2] });

	// transformをstagePositions_に合わせて設定
	const Vector3& sp = stagePositions_[static_cast<size_t>(currentIndex_)];
	playerTranslate_[0] = -30.0f;
	playerTranslate_[1] = sp.y; playerTranslate_[2] = sp.z;

	// オフセットを保存（以降の移動でも同じオフセットを使う）
	playerXOffset_ = playerTranslate_[0] - sp.x;

	// --- プレイヤーのオフセットを使って各ステージUIを生成 ---
	stageUIObjects_.clear();
	ModelManager::GetInstance()->LoadModel("stageUI.obj");
	stageUIObjects_.reserve(options_.size());
	stageUIVisible_.clear();
	stageUIAnimating_.clear();
	stageUIAnimTimer_.clear();
	stageUIBaseScale_.clear();
	for (size_t i = 0; i < options_.size(); ++i) {
		auto uiObj = std::make_unique<Object3d>();
		uiObj->Initialize(engine_->GetObject3dRenderer());
		uiObj->SetModel("stageUI.obj");
		float x = stagePositions_[i].x + playerXOffset_;
		float y = 10.0f;
		float z = stagePositions_[i].z;
		uiObj->SetTranslation({ x, y, z });
		uiObj->SetScale({ stageUIScale_[0], stageUIScale_[1], stageUIScale_[2] });
		stageUIBaseScale_.push_back(Vector3(stageUIScale_[0], stageUIScale_[1], stageUIScale_[2]));

		stageUIObjects_.push_back(std::move(uiObj));
		stageUIVisible_.push_back(false);
		stageUIAnimating_.push_back(false);
		stageUIAnimTimer_.push_back(0.0f);
	}

	// ターゲットを初期位置に設定
	playerTargetTranslate_ = { playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] };
	playerMoving_ = false;
	prevPlayerMoving_ = false;

	playerRotateDeg_[0] = 0.0f; playerRotateDeg_[1] = 180.0f; playerRotateDeg_[2] = 0.0f;
	// 初期の正面角を保存
	playerInitialFrontDeg_ = playerRotateDeg_[1];

	playerObject3d_->SetTranslation({ playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] });
	const float degToRad = 3.14159265358979323846f / 180.0f;
	playerObject3d_->SetRotation({ playerRotateDeg_[0] * degToRad, playerRotateDeg_[1] * degToRad, playerRotateDeg_[2] * degToRad });
	playerObject3d_->SetScale({ playerScale_[0], playerScale_[1], playerScale_[2] });
}

void StageSelectScene::Finalize()
{
	// オブジェクトを解放
	stage1Object3d_.reset();
	stage2Object3d_.reset();
	stageUIObject3d_.reset();
	stageUIObjects_.clear();
	stageUIVisible_.clear();
	stageUIAnimating_.clear();
	stageUIAnimTimer_.clear();
	stageUIBaseScale_.clear();
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

	// カメラへ反映
	if (useDebugCamera_) {
		if (debugCamera_) {
			debugCamera_->Update(*input);
			engine_->GetObject3dRenderer()->SetDefaultCamera(debugCamera_->GetCamera());
		}

	} else {
		if (camera_) {
			// --- カメラを目標に向かって滑らかに補間 ---
			{
				const float dtLocal = dt;
				float alpha = std::min(1.0f, cameraMoveSpeed_ * dtLocal);

				if (cameraMoving_) {
					for (int i = 0; i < 3; ++i) {
						float diff = cameraTargetTranslate_[i] - cameraTranslate_[i];
						cameraTranslate_[i] += diff * alpha;
					}
					float dx = cameraTargetTranslate_[0] - cameraTranslate_[0];
					float dy = cameraTargetTranslate_[1] - cameraTranslate_[1];
					float dz = cameraTargetTranslate_[2] - cameraTranslate_[2];
					float distSq = dx * dx + dy * dy + dz * dz;
					if (distSq <= cameraSnapThreshold_ * cameraSnapThreshold_) {
						cameraTranslate_[0] = cameraTargetTranslate_[0];
						cameraTranslate_[1] = cameraTargetTranslate_[1];
						cameraTranslate_[2] = cameraTargetTranslate_[2];
						cameraMoving_ = false;
					}
				}
			}

			camera_->SetTranslate({ cameraTranslate_[0], cameraTranslate_[1], cameraTranslate_[2] });
			camera_->SetRotate({ cameraRotateDeg_[0] * degToRad, cameraRotateDeg_[1] * degToRad, cameraRotateDeg_[2] * degToRad });
			camera_->Update();
			engine_->GetObject3dRenderer()->SetDefaultCamera(camera_.get());
		}
	}

	// 各オブジェクトの更新
	if (stage1Object3d_) {
		stage1Object3d_->Update();
	}

	if (stage2Object3d_) {
		stage2Object3d_->Update();
	}

	for (auto& uiObj : stageUIObjects_) {
		if (uiObj) {
			uiObj->Update();
		}
	}

	if (playerObject3d_) {
		playerObject3d_->Update();
	}

	// --- 遷移アニメ優先処理 ---
	if (transitionActive_) {
		// 進行
		transitionTimer_ += dt;
		float t = std::min(1.0f, transitionTimer_ / transitionDuration_);

		const float delayBeforeScale = 2.00f;

		float scaleInput = 0.0f;

		if (t <= delayBeforeScale) {
			scaleInput = 0.0f;
		} else {

			scaleInput = (t - delayBeforeScale) / (1.0f - delayBeforeScale);

			if (scaleInput < 0.0f) {
				scaleInput = 0.0f;
			}

			if (scaleInput > 1.0f) {
				scaleInput = 1.0f;
			}
		}

		// スケールは EaseOutBack、Y の落下はイーズイン(quad)で表現
		float scaleE = EaseOutBack(t);
		float fallE = t * t;

		// 位置：X/Zは固定、Yは下へ加速で落ちる
		Vector3 pos;
		pos.x = transitionStartPos_.x;
		pos.z = transitionStartPos_.z;
		pos.y = transitionStartPos_.y + (transitionTargetPos_.y - transitionStartPos_.y) * fallE;

		// スケール：滑らかに縮小
		Vector3 scale = Lerp(transitionStartScale_, transitionTargetScale_, scaleE);

		playerTranslate_[0] = pos.x;
		playerTranslate_[1] = pos.y;
		playerTranslate_[2] = pos.z;

		playerScale_[0] = scale.x;
		playerScale_[1] = scale.y;
		playerScale_[2] = scale.z;

		// 回転：Y軸スピン ＋ X軸で少し前傾して吸い込まれ感を強調
		playerRotateDeg_[1] = transitionStartRotateYDeg_ + transitionSpinDeg_ * t;


		if (playerObject3d_) {
			playerObject3d_->SetTranslation({ playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] });
			playerObject3d_->SetScale({ playerScale_[0], playerScale_[1], playerScale_[2] });
			playerObject3d_->SetRotation({ playerRotateDeg_[0] * degToRad, playerRotateDeg_[1] * degToRad, playerRotateDeg_[2] * degToRad });
		}

		// アニメ完了でシーン切替（フェードを使う場合はここで ChangeScene を遅延させてください）
		if (t >= 1.0f) {
			transitionActive_ = false;

			playerTranslate_[0] = transitionTargetPos_.x;
			playerTranslate_[1] = transitionTargetPos_.y;
			playerTranslate_[2] = transitionTargetPos_.z;
			playerScale_[0] = transitionTargetScale_.x;
			playerScale_[1] = transitionTargetScale_.y;
			playerScale_[2] = transitionTargetScale_.z;
			playerRotateDeg_[1] = transitionStartRotateYDeg_ + transitionSpinDeg_;

			StageSelection::SetSelected(options_[currentIndex_]);
			SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
		}

		// 遷移中は他の入力処理を無視してreturn（天球やImGuiは動かしておきたいならここを調整）
		prevPlayerMoving_ = playerMoving_;
		return;
	}

	// --- 通常入力（ステージ切替等） ---
	if (playerObject3d_) {
		// 左右キーでのステージ切替（共通処理）
		if (input->IsTriggerKey(DIK_A) || input->IsTriggerKey(DIK_LEFT) ||
			input->IsTriggerKey(DIK_D) || input->IsTriggerKey(DIK_RIGHT)) {

			int prevIndex = currentIndex_;
			int newIndex = prevIndex;

			if (input->IsTriggerKey(DIK_A) || input->IsTriggerKey(DIK_LEFT)) {
				newIndex = (prevIndex > 0) ? prevIndex - 1 : static_cast<int>(options_.size()) - 1;
			}
			if (input->IsTriggerKey(DIK_D) || input->IsTriggerKey(DIK_RIGHT)) {
				newIndex = (prevIndex + 1) % static_cast<int>(options_.size());
			}

			currentIndex_ = newIndex;
			const Vector3& sp = stagePositions_[static_cast<size_t>(currentIndex_)];
			const int n = static_cast<int>(options_.size());

			if ((prevIndex == 4 && newIndex == 5) || (prevIndex == 0 && newIndex == n - 1)) {
				cameraTargetTranslate_[0] = 100.0f;
				cameraMoving_ = true;
			} else if ((prevIndex == 5 && newIndex == 4) || (prevIndex == n - 1 && newIndex == 0)) {
				cameraTargetTranslate_[0] = 0.0f;
				cameraMoving_ = true;
			}

			playerTargetTranslate_ = { sp.x + playerXOffset_, sp.y, sp.z };
			playerMoving_ = true;
		}

		// --- プレイヤー滑らか移動 ---
		if (playerMoving_) {
			Vector3 curr{ playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] };
			float alpha = std::min(1.0f, playerMoveSpeed_ * dt);
			Vector3 next = Lerp(curr, playerTargetTranslate_, alpha);

			Vector3 diff = { next.x - playerTargetTranslate_.x, next.y - playerTargetTranslate_.y, next.z - playerTargetTranslate_.z };
			float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
			if (distSq <= playerSnapThreshold_ * playerSnapThreshold_) {
				next = playerTargetTranslate_;
				playerMoving_ = false;
			}

			// 回転（移動方向に合わせる）
			Vector3 moveVec = { playerTargetTranslate_.x - curr.x, 0.0f, playerTargetTranslate_.z - curr.z };
			const float eps = 1e-6f;
			if (std::abs(moveVec.x) > eps || std::abs(moveVec.z) > eps) {
				int sideSign = 1;
				if (std::abs(moveVec.x) > eps) sideSign = (moveVec.x > 0.0f) ? 1 : -1;
				else sideSign = (moveVec.z >= 0.0f) ? 1 : -1;

				float sideDeg = playerInitialFrontDeg_ + (sideSign > 0 ? -90.0f : 90.0f);
				float remainX = playerTargetTranslate_.x - curr.x;
				float remainZ = playerTargetTranslate_.z - curr.z;
				float remainDist = std::sqrt(remainX * remainX + remainZ * remainZ);

				float desiredDeg = sideDeg;
				if (remainDist < playerReturnStartDistance_) {
					float t = 1.0f - (remainDist / std::max(1e-6f, playerReturnStartDistance_));
					if (t < 0.0f) t = 0.0f;
					if (t > 1.0f) t = 1.0f;
					desiredDeg = sideDeg + (playerInitialFrontDeg_ - sideDeg) * t;
				}

				float currentDeg = playerRotateDeg_[1];
				float delta = desiredDeg - currentDeg;
				while (delta > 180.0f) delta -= 360.0f;
				while (delta < -180.0f) delta += 360.0f;

				float maxDelta = playerRotateSpeedDeg_ * dt;
				if (remainDist < playerReturnStartDistance_ * 0.5f) {
					float extra = playerReturnRotateSpeedDeg_ * dt * 0.5f;
					maxDelta = std::max(maxDelta, extra);
				}

				if (std::abs(delta) <= maxDelta) currentDeg = desiredDeg;
				else currentDeg += (delta > 0.0f ? maxDelta : -maxDelta);

				playerRotateDeg_[1] = currentDeg;
				playerObject3d_->SetRotation({ playerRotateDeg_[0] * degToRad, playerRotateDeg_[1] * degToRad, playerRotateDeg_[2] * degToRad });
			}

			playerTranslate_[0] = next.x;
			playerTranslate_[1] = next.y;
			playerTranslate_[2] = next.z;
		} else {
			// 移動完了後に正面へ戻す
			const float targetFrontDeg = playerInitialFrontDeg_;
			float currentDeg = playerRotateDeg_[1];
			float delta = targetFrontDeg - currentDeg;
			while (delta > 180.0f) delta -= 360.0f;
			while (delta < -180.0f) delta += 360.0f;
			float maxDelta = playerReturnRotateSpeedDeg_ * dt;

			if (std::abs(delta) <= maxDelta) currentDeg = targetFrontDeg;
			else currentDeg += (delta > 0.0f ? maxDelta : -maxDelta);

			playerRotateDeg_[1] = currentDeg;
			playerObject3d_->SetRotation({ playerRotateDeg_[0] * degToRad, playerRotateDeg_[1] * degToRad, playerRotateDeg_[2] * degToRad });
		}

		playerObject3d_->SetTranslation({ playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] });

		// 移動の停止/開始で Stage UI を表示/非表示
		bool justStopped = (!playerMoving_) && prevPlayerMoving_;
		bool justStarted = playerMoving_ && !prevPlayerMoving_;

		if (justStopped) {
			if (!stageUIObjects_.empty()) {
				size_t idx = static_cast<size_t>(std::max(0, std::min(currentIndex_, static_cast<int>(stageUIObjects_.size() - 1))));
				if (idx < stageUIObjects_.size() && stageUIObjects_[idx]) {
					stageUIVisible_[idx] = true;
					stageUIAnimating_[idx] = true;
					stageUIAnimTimer_[idx] = 0.0f;
					stageUIObjects_[idx]->SetScale({ 0.0f, 0.0f, 0.0f });
					stageUIObjects_[idx]->SetTranslation({ playerTranslate_[0], 10.0f, playerTranslate_[2] });
				}
			}
		} else if (justStarted) {
			if (!stageUIObjects_.empty()) {
				size_t idx = static_cast<size_t>(std::max(0, std::min(currentIndex_, static_cast<int>(stageUIObjects_.size() - 1))));
				if (idx < stageUIObjects_.size() && stageUIObjects_[idx]) {
					stageUIVisible_[idx] = false;
					stageUIAnimating_[idx] = false;
					stageUIAnimTimer_[idx] = 0.0f;
					stageUIObjects_[idx]->SetScale({ 0.0f, 0.0f, 0.0f });
				}
			}
		}

		if (!stageUIObjects_.empty()) {
			size_t idx = static_cast<size_t>(std::max(0, std::min(currentIndex_, static_cast<int>(stageUIObjects_.size() - 1))));
			if (idx < stageUIObjects_.size() && stageUIObjects_[idx]) {
				stageUIObjects_[idx]->SetTranslation({ playerTranslate_[0], 10.0f, playerTranslate_[2] });
			}
		}
	}

	// アニメーション更新（選択中以外は非表示）
	for (size_t i = 0; i < stageUIObjects_.size(); ++i) {
		if (!stageUIObjects_[i]) continue;

		if (stageUIAnimating_[i]) {
			stageUIAnimTimer_[i] += dt;
			float t = stageUIAnimTimer_[i] / stageUIAnimDuration_;
			if (t >= 1.0f) {
				t = 1.0f;
				stageUIAnimating_[i] = false;
			}
			float e = EaseOutBack(t);
			Vector3 base = stageUIBaseScale_[i];
			stageUIObjects_[i]->SetScale({ base.x * e, base.y * e, base.z * e });
		} else {
			if (!stageUIVisible_[i]) {
				stageUIObjects_[i]->SetScale({ 0.0f, 0.0f, 0.0f });
			} else {
				Vector3 base = stageUIBaseScale_[i];
				stageUIObjects_[i]->SetScale({ base.x, base.y, base.z });
			}
		}
	}

	// SPACEで決定 -> 下に吸い込まれる遷移アニメ開始
	if (!playerMoving_ && input->IsTriggerKey(DIK_SPACE) && !transitionActive_) {
		transitionActive_ = true;
		transitionTimer_ = 0.0f;
		transitionDuration_ = 0.9f; // 調整可

		transitionStartPos_ = { playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] };
		transitionStartScale_ = { playerScale_[0], playerScale_[1], playerScale_[2] };
		transitionStartRotateYDeg_ = playerRotateDeg_[1];

		const float fallDepth = 8.0f; // 下にどれだけ吸い込むか
		transitionTargetPos_ = { transitionStartPos_.x, transitionStartPos_.y - fallDepth, transitionStartPos_.z };

		transitionTargetScale_ = { 0.05f, 0.05f, 0.05f };
		transitionSpinDeg_ = 1080.0f; // スピン量
	}

	// Escでタイトルへ戻る
	if (input->IsTriggerKey(DIK_ESCAPE)) {
		SceneManager::GetInstance()->ChangeScene("TITLE");
	}

	// Update prev flag
	prevPlayerMoving_ = playerMoving_;

	// 天球の更新
	if (skyObject3d_) {
		const ICamera* activeCam = engine_->GetObject3dRenderer()->GetDefaultCamera();
		if (activeCam) {
			Vector3 camPos = activeCam->GetTranslate();
			skyObject3d_->SetTranslation({ camPos.x, camPos.y, camPos.z });
		}
		skyRotate_ += 0.0005f;
		if (skyRotate_ > 3.14159265f * 2.0f) skyRotate_ -= 3.14159265f * 2.0f;
		skyObject3d_->SetRotation({ 0.0f, skyRotate_, 0.0f });
		skyObject3d_->Update();
	}

#ifdef USE_IMGUI
	auto* renderer = engine_->GetObject3dRenderer();

	static bool showDirectionalLight = true;
	static bool showPointLight = false;
	static bool showSpotLight = false;
	static float directionalIntensityBackup = 1.0f;
	static float pointIntensityBackup = 1.0f;
	static float spotIntensityBackup = 4.0f;

	ImGui::Begin("Camera Controls");
	ImGui::DragFloat3("Translate", cameraTranslate_, 0.1f);
	ImGui::DragFloat3("Rotate (deg)", cameraRotateDeg_, 0.5f, -360.0f, 360.0f);
	if (ImGui::Button("Reset")) {
		cameraTranslate_[0] = 0.0f; cameraTranslate_[1] = 4.0f; cameraTranslate_[2] = -10.0f;
		cameraRotateDeg_[0] = 17.1887f; cameraRotateDeg_[1] = 0.0f; cameraRotateDeg_[2] = 0.0f;
	}
	ImGui::End();

	if (playerObject3d_) {
		ImGui::Begin("Player (Preview)");
		ImGui::DragFloat3("Translate", playerTranslate_, 0.05f);
		ImGui::DragFloat3("Rotate (deg)", playerRotateDeg_, 1.0f, -360.0f, 360.0f);
		ImGui::DragFloat3("Scale", playerScale_, 0.05f, 0.01f, 20.0f);
		playerObject3d_->SetTranslation({ playerTranslate_[0], playerTranslate_[1], playerTranslate_[2] });
		playerObject3d_->SetRotation({ playerRotateDeg_[0] * degToRad, playerRotateDeg_[1] * degToRad, playerRotateDeg_[2] * degToRad });
		playerObject3d_->SetScale({ playerScale_[0], playerScale_[1], playerScale_[2] });
		ImGui::End();
	}

	if (stage2Object3d_) {
		ImGui::Begin("Stage2 (Preview)");
		ImGui::DragFloat3("Translate##Stage2", stage2Translate_, 0.05f);
		ImGui::DragFloat3("Rotate (deg)##Stage2", stage2RotateDeg_, 1.0f, -360.0f, 360.0f);
		ImGui::DragFloat3("Scale##Stage2", stage2Scale_, 0.05f, 0.01f, 20.0f);
		stage2Object3d_->SetTranslation({ stage2Translate_[0], stage2Translate_[1], stage2Translate_[2] });
		stage2Object3d_->SetRotation({ stage2RotateDeg_[0] * degToRad, stage2RotateDeg_[1] * degToRad, stage2RotateDeg_[2] * degToRad });
		stage2Object3d_->SetScale({ stage2Scale_[0], stage2Scale_[1], stage2Scale_[2] });
		ImGui::End();
	}

	ImGui::Begin("StageSelect");
	ImGui::Text("Selected: %s", options_[currentIndex_].c_str());
	ImGui::Text("UseDebugCamera: %s", useDebugCamera_ ? "ON" : "OFF");
	ImGui::End();

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
			sl->direction = Normalize(sl->direction);
		}
		static float skyBrightness = 0.0f;
		if (ImGui::SliderFloat("Sky Brightness (point)", &skyBrightness, 0.0f, 5.0f)) {
			if (pl) {
				const ICamera* cam = renderer->GetDefaultCamera();
				if (cam) pl->position = cam->GetTranslate();
				pl->intensity = skyBrightness;
				pl->radius = 30.0f * (0.5f + skyBrightness);
			}
			if (dl) dl->intensity = std::max(dl->intensity, 0.15f);
		}
		ImGui::End();
	}
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
	if (skyObject3d_) {
		skyObject3d_->Draw();
	}

	if (playerObject3d_) {
		playerObject3d_->Draw();
	}

	if (stage1Object3d_) {
		stage1Object3d_->Draw();
	}

	if (stage2Object3d_) {
		stage2Object3d_->Draw();
	}

	// 現状プレイヤーがいるステージのUIのみ描画
	if (!stageUIObjects_.empty()) {
		size_t idx = static_cast<size_t>(std::max(0, std::min(currentIndex_, static_cast<int>(stageUIObjects_.size() - 1))));
		if (idx < stageUIObjects_.size() && stageUIObjects_[idx]) {
			if (stageUIVisible_[idx] || stageUIAnimating_[idx]) {
				stageUIObjects_[idx]->Draw();
			}
		}
	}
}

void StageSelectScene::Draw2D()
{
	engine_->Begin2D();
	// シンプル実装：現状は描画なし。
	// 選択UI（Spriteやフォント）をここに追加
}