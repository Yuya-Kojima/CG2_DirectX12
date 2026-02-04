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
#include "Core/WindowSystem.h"
#include <memory>
#include <cstring>
#include <string>
#include <chrono>
#include <algorithm> // clamp
#include <cmath>     // sin

void TitleScene::Initialize(EngineBase* engine)
{
	// 参照をコピー
	engine_ = engine;

	//===========================
	// 初期選択・クレジットフラグ初期化
	//===========================
	selectedButtonIndex_ = 0; // Start を初期選択
	creditsActive_ = false;

	// 初回時刻を設定
	lastUpdateTime_ = std::chrono::steady_clock::now();
	pinYaw_ = 0.0f;
	pinElapsed_ = 0.0f;
	// 必要ならここでピンの速度や振幅を調整する
	pinSpinSpeed_ = 1.5f;
	pinBobAmplitude_ = 0.05f;
	pinBobFrequency_ = 2.0f;

	// イージング用初期化（デフォルトでピンは Start の位置）
	pinStartTranslation_ = Vector3{ -1.7f, -1.3f, 0.0f };
	pinTargetTranslation_ = pinStartTranslation_;
	pinMoveTimer_ = 0.0f;
	pinIsMoving_ = false;

	// 揺れ用初期化
	pinSwayAmplitude_ = 0.10f;
	pinSwayFrequency_ = 0.8f;
	pinSwayPhase_ = 0.0f;

	// タイトルのボビング初期値
	titleBobAmplitude_ = 0.05f;
	titleBobFrequency_ = 0.6f;
	titleBobPhase_ = 0.0f;

	//===========================
	// 3Dオブジェクト関係の初期化
	//===========================

	// カメラの生成と初期化
	camera_ = new GameCamera();
	camera_->SetRotate({ 0.0f, 0.0f, 0.0f });
	camera_->SetTranslate({ 0.0f, 0.0f, -15.0f });

	// デバッグカメラ
	debugCamera_ = new DebugCamera();
	debugCamera_->Initialize({ 0.0f, 4.0f, -10.0f });

	// デフォルトカメラのセット
	engine_->GetObject3dRenderer()->SetDefaultCamera(camera_);

	// ===== ライティング設定を追加 =====
	auto* renderer = engine_->GetObject3dRenderer();

	// Directional Light（主光源）
	if (auto* dl = renderer->GetDirectionalLightData()) {
		dl->color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f }; // やや暖色寄り
		dl->direction = Normalize(Vector3{ -0.3f, -1.0f, -0.5f });
		dl->intensity = 10.0f;
	}

	//===========================
	// タイトル3Dモデル初期化
	//===========================

	// モデルをロードしておく
	ModelManager::GetInstance()->LoadModel("Title.obj");

	// オブジェクトを生成・初期化してモデルを割り当てる
	titleObject3d_ = std::make_unique<Object3d>();
	titleObject3d_->Initialize(engine_->GetObject3dRenderer());
	titleObject3d_->SetModel("Title.obj");

	// 初期Transform（必要なら調整）
	titleObject3d_->SetScale({ 0.5f, 0.5f, 0.5f });
	titleObject3d_->SetRotation({ 0.0f, 0.0f, 0.0f });
	titleObject3d_->SetTranslation({ 0.0f, 1.6f, 0.0f });

	// 初期位置を基準として保存（ボビングの基準）
	titleStartTranslation_ = titleObject3d_->GetTranslation();

	// 初期色（マテリアル）
	titleObject3d_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });

	// ボタン用3Dオブジェクトの初期化
	// モデルをロードしておく
	ModelManager::GetInstance()->LoadModel("button1.obj");

	// オブジェクトを生成・初期化してモデルを割り当てる
	button1Object3d_ = std::make_unique<Object3d>();
	button1Object3d_->Initialize(engine_->GetObject3dRenderer());
	button1Object3d_->SetModel("button1.obj");

	// 初期Transform
	button1Object3d_->SetScale({ 1.0f, 1.0f, 1.0f });
	button1Object3d_->SetRotation({ 0.0f, 0.0f, 0.0f });
	button1Object3d_->SetTranslation({ 0.0f, -1.8f, 0.0f });

	// モデルをロードしておく
	ModelManager::GetInstance()->LoadModel("button2.obj");

	// オブジェクトを生成・初期化してモデルを割り当てる
	button2Object3d_ = std::make_unique<Object3d>();
	button2Object3d_->Initialize(engine_->GetObject3dRenderer());
	button2Object3d_->SetModel("button2.obj");

	// 初期Transform
	button2Object3d_->SetScale({ 1.0f, 1.0f, 1.0f });
	button2Object3d_->SetRotation({ 0.0f, 0.0f, 0.0f });
	button2Object3d_->SetTranslation({ 0.0f, -1.8f, 0.0f });

	ModelManager::GetInstance()->LoadModel("pin.obj");

	// オブジェクトを生成・初期化してモデルを割り当てる
	pinObject3d_ = std::make_unique<Object3d>();
	pinObject3d_->Initialize(engine_->GetObject3dRenderer());
	pinObject3d_->SetModel("pin.obj");

	// 初期Transform
	pinObject3d_->SetScale({ 1.0f, 1.0f, 1.0f });
	pinObject3d_->SetRotation({ 0.0f, 0.0f, 0.0f });
	pinObject3d_->SetTranslation(pinStartTranslation_);

	// モデルをロードしておく
	ModelManager::GetInstance()->LoadModel("Title1.obj");

	// オブジェクトを生成・初期化してモデルを割り当てる
	titleFloorObject3d_ = std::make_unique<Object3d>();
	titleFloorObject3d_->Initialize(engine_->GetObject3dRenderer());
	titleFloorObject3d_->SetModel("Title1.obj");

	// 初期Transform（必要なら調整）
	titleFloorObject3d_->SetScale({ 0.5f, 0.5f, 0.5f });
	titleFloorObject3d_->SetRotation({ 0.0f, 0.0f, 0.0f });
	titleFloorObject3d_->SetTranslation({ 0.0f, 1.0f, 0.0f });

	// モデルをロードしておく
	ModelManager::GetInstance()->LoadModel("Title2.obj");

	// オブジェクトを生成・初期化してモデルを割り当てる
	titleBotObject3d_ = std::make_unique<Object3d>();
	titleBotObject3d_->Initialize(engine_->GetObject3dRenderer());
	titleBotObject3d_->SetModel("Title2.obj");

	// 初期Transform（必要なら調整） — 初期 X を 0.0f にしておく
	titleBotObject3d_->SetScale({ 0.5f, 0.5f, 0.5f });
	titleBotObject3d_->SetRotation({ 0.0f, 0.0f, 0.0f });
	titleBotObject3d_->SetTranslation({ 0.0f, 1.0f, 0.0f });

	// モデルをロードしておく（Title3 は反転時に使用）
	ModelManager::GetInstance()->LoadModel("Title3.obj");

	// ----- TitleBot 等速直線運動初期化 -----
	if (titleBotObject3d_) {
		// 等速で左へ移動開始（X: 0.0 -> -7.1）
		titleBotSpeed_ = 2.0f; // world units / sec（調整可能）
		titleBotDirection_ = -1; // 左
		titleBotIsMoving_ = true;
		titleBotPhase_ = TitleBotPhase::FirstMove;
	}

	//-- クレジット用3Dオブジェクトの初期化 ---
	ModelManager::GetInstance()->LoadModel("Credits.obj");
	Credits3d_ = std::make_unique<Object3d>();
	Credits3d_->Initialize(engine_->GetObject3dRenderer());
	Credits3d_->SetModel("Credits.obj");
	Credits3d_->SetScale({ 1.0f, 1.0f, 1.0f });
	Credits3d_->SetRotation({ 0.0f, 0.0f, 0.0f });
	Credits3d_->SetTranslation({ 0.0f, 2.1f, 0.0f });

	// --- 天球モデルの用意 ---
	ModelManager::GetInstance()->LoadModel("SkyDome.obj");
	skyObject3d_ = std::make_unique<Object3d>();
	skyObject3d_->Initialize(engine_->GetObject3dRenderer());
	skyObject3d_->SetModel("SkyDome.obj");
	skyObject3d_->SetTranslation({ 0.0f, 0.0f, 0.0f });
	skyObject3d_->SetScale({ skyScale_, skyScale_, skyScale_ });
	skyObject3d_->SetRotation({ 0.0f, 0.0f, 0.0f });
	skyObject3d_->SetEnableLighting(false);
	skyObject3d_->SetColor(Vector4{ 3.0f,3.0f,3.0f,1.0f });

	//===========================
	// パーティクル関係の初期化
	//===========================
}

void TitleScene::Finalize()
{
	delete debugCamera_;
	debugCamera_ = nullptr;

	delete camera_;
	camera_ = nullptr;
}

void TitleScene::Update()
{

	// Sound更新
	SoundManager::GetInstance()->Update();

	// 時間差分計算（dt: 秒）
	auto now = std::chrono::steady_clock::now();
	float dt = std::chrono::duration<float>(now - lastUpdateTime_).count();
	// clamp 不要に大きくならないようにフォールバック
	if (dt <= 0.0f || dt > 0.5f) {
		dt = 1.0f / 60.0f;
	}
	lastUpdateTime_ = now;

	// ===========================
	// 入力：選択（W / S）および決定（SPACE）
	// ===========================
	// W または S を押すと選択をトグル（0 <-> 1）
	if (engine_->GetInputManager()->IsTriggerKey(DIK_W) ||
		engine_->GetInputManager()->IsTriggerKey(DIK_S)) {
		selectedButtonIndex_ = 1 - selectedButtonIndex_;

		// イージング移動開始
		if (pinObject3d_) {
			pinStartTranslation_ = pinObject3d_->GetTranslation();
			pinTargetTranslation_ = (selectedButtonIndex_ == 0)
				? Vector3{ -1.7f, -1.3f, 0.0f }
			: Vector3{ -1.7f, -2.4f, 0.0f };
			pinMoveTimer_ = 0.0f;
			pinIsMoving_ = true;
		}
	}

	// 決定
	if (engine_->GetInputManager()->IsTriggerKey(DIK_SPACE)) {
		if (creditsActive_) {
			// クレジット表示中はスペースで戻る
			creditsActive_ = false;
		} else {
			if (selectedButtonIndex_ == 0) {
				// Start
				SceneManager::GetInstance()->ChangeScene("STAGESELECT");
			} else {
				// Credits をタイトル内で表示
				creditsActive_ = true;
			}
		}
	}

#ifdef USE_IMGUI
	//デバッグカメラ切替
	if (engine_->GetInputManager()->IsTriggerKey(DIK_P)) {
		if (useDebugCamera_) {
			useDebugCamera_ = false;
		} else {
			useDebugCamera_ = true;
		}
	}
#endif

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

	//=======================
	// 3Dオブジェクトの更新
	//=======================
	if (titleObject3d_) {
		// タイトルの上下揺れ（サイン波）を適用
		titleBobPhase_ += dt * titleBobFrequency_ * 2.0f * 3.14159265f;
		float bob = std::sin(titleBobPhase_) * titleBobAmplitude_;

		Vector3 tpos = titleStartTranslation_;
		tpos.y += bob;
		titleObject3d_->SetTranslation(tpos);

		titleObject3d_->Update();
	}

	if (titleFloorObject3d_) {
		titleFloorObject3d_->Update();
	}

	// --- titleBot の等速直線運動更新（ループ動作） ---
	if (titleBotObject3d_ && titleBotIsMoving_) {
		// 現在位置取得
		Vector3 cur = titleBotObject3d_->GetTranslation();

		// 移動量 = direction * speed * dt
		float dx = static_cast<float>(titleBotDirection_) * titleBotSpeed_ * dt;
		cur.x += dx;

		// 左へ移動中に -7.1 を越えたら到達処理（到達時にモデルを Title3 に変更）
		if (titleBotDirection_ == -1 && cur.x <= -7.1f) {
			// 到達点にスナップ
			cur.x = -7.1f;
			titleBotObject3d_->SetTranslation(cur);

			// モデルを差し替え（反転時に Title3.obj に変更）
			titleBotObject3d_->SetModel("Title3.obj");

			// モデル変更時の要望に合わせ、X を 0.0f にリセットして右方向へ等速移動を開始
			cur.x = 0.0f;
			titleBotObject3d_->SetTranslation(cur);
			titleBotDirection_ = +1;
			titleBotPhase_ = TitleBotPhase::SecondMove;
		}
		// 右へ移動中に +7.1 を越えたら到達処理（到達時にモデルを Title2 に戻し、再び左へ）
		else if (titleBotDirection_ == +1 && cur.x >= 7.1f) {
			// 到達点にスナップ
			cur.x = 7.1f;
			titleBotObject3d_->SetTranslation(cur);

			// モデルを差し替え（反転時に元の Title2.obj に戻す）
			titleBotObject3d_->SetModel("Title2.obj");

			// モデル変更時に X を 0.0f にリセットして左方向へ等速移動を開始（ループ）
			cur.x = 0.0f;
			titleBotObject3d_->SetTranslation(cur);
			titleBotDirection_ = -1;
			titleBotPhase_ = TitleBotPhase::FirstMove;
		} else {
			// 通常更新
			titleBotObject3d_->SetTranslation(cur);
		}

		titleBotObject3d_->Update();
	} else if (titleBotObject3d_) {
		// 移動していない場合でも Update は呼ぶ
		titleBotObject3d_->Update();
	}

	// titleBot1Object3d_ は描画しない運用（生成は残すが Update は不要）

	if (button1Object3d_) {
		button1Object3d_->Update();
	}

	if (button2Object3d_) {
		button2Object3d_->Update();
	}

	if (Credits3d_) {
		Credits3d_->Update();
	}

	if (pinObject3d_) {

		// サイン波位相更新（左右揺れ）
		pinSwayPhase_ += dt * pinSwayFrequency_ * 2.0f * 3.14159265f;
		// オフセット X
		float sway = std::sin(pinSwayPhase_) * pinSwayAmplitude_;

		// ピン移動の更新（イージング）
		Vector3 pos;
		if (pinIsMoving_) {
			pinMoveTimer_ += dt;
			float t = pinMoveTimer_ / pinMoveDuration_;
			if (t >= 1.0f) {
				t = 1.0f;
				pinIsMoving_ = false;
			}
			float e = EaseOutCubic(t);
			pos = Lerp(pinStartTranslation_, pinTargetTranslation_, e);
		} else {
			// 移動していなければ選択に合わせて確実にロックしておく
			pos = (selectedButtonIndex_ == 0)
				? Vector3{ -1.7f, -1.3f, 0.0f }
			: Vector3{ -1.7f, -2.4f, 0.0f };
		}

		// 揺れを X に合成
		pos.x += sway;
		pinObject3d_->SetTranslation(pos);

		// 少し傾けて見た目を強調（Z軸回転）
		pinObject3d_->SetRotation({ 0.0f, 0.0f, 0.0f });

		pinObject3d_->Update();
	}

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

	// ===== ライティング UI（既存） =====
	{
		auto* renderer = engine_->GetObject3dRenderer();

		static bool showDirectionalLight = true;
		static bool showPointLight = false;
		static bool showSpotLight = false;
		static float directionalIntensityBackup = 1.0f;
		static float pointIntensityBackup = 1.0f;
		static float spotIntensityBackup = 4.0f;

		ImGui::Begin("Lighting");

		// DirectionalLight
		bool changedDirectional =
			ImGui::Checkbox("Enable DirectionalLight", &showDirectionalLight);
		if (changedDirectional) {
			if (auto* dl = renderer->GetDirectionalLightData()) {
				if (!showDirectionalLight) {
					directionalIntensityBackup = dl->intensity;
					dl->intensity = 0.0f;
				} else {
					dl->intensity = (directionalIntensityBackup > 0.0f)
						? directionalIntensityBackup
						: 1.0f;
				}
			}
		}

		// PointLight
		bool changedPoint = ImGui::Checkbox("Enable PointLight", &showPointLight);
		if (changedPoint) {
			if (auto* pl = renderer->GetPointLightData()) {
				if (!showPointLight) {
					pointIntensityBackup = pl->intensity;
					pl->intensity = 0.0f;
				} else {
					pl->intensity =
						(pointIntensityBackup > 0.0f) ? pointIntensityBackup : 1.0f;
				}
			}
		}

		// SpotLight
		bool changedSpot = ImGui::Checkbox("Enable SpotLight", &showSpotLight);
		if (changedSpot) {
			if (auto* sl = renderer->GetSpotLightData()) {
				if (!showSpotLight) {
					spotIntensityBackup = sl->intensity;
					sl->intensity = 0.0f;
				} else {
					sl->intensity =
						(spotIntensityBackup > 0.0f) ? spotIntensityBackup : 1.0f;
				}
			}
		}

		ImGui::End();

		// Directional
		if (auto* dl = renderer->GetDirectionalLightData()) {
			if (showDirectionalLight) {
				ImGui::Begin("DirectionalLight");
				ImGui::ColorEdit3("Color", &dl->color.x);
				ImGui::DragFloat("Intensity", &dl->intensity, 0.01f, 0.0f, 10.0f);
				ImGui::End();
			}
		}

		// Point
		if (auto* pl = renderer->GetPointLightData()) {
			if (showPointLight) {
				ImGui::Begin("PointLight");
				ImGui::ColorEdit3("Color", &pl->color.x);
				ImGui::DragFloat3("Position", &pl->position.x, 0.05f, -20.0f, 20.0f);
				ImGui::DragFloat("Intensity", &pl->intensity, 0.05f, 0.0f, 10.0f);
				ImGui::DragFloat("Radius", &pl->radius, 0.1f, 0.01f, 100.0f);
				ImGui::DragFloat("Decay", &pl->decay, 0.05f, 0.01f, 8.0f);
				ImGui::End();
			}
		}

		// Spot
		if (auto* sl = renderer->GetSpotLightData()) {
			if (showSpotLight) {
				ImGui::Begin("SpotLight");
				ImGui::ColorEdit3("Color", &sl->color.x);
				ImGui::DragFloat3("Position", &sl->position.x, 0.05f, -20.0f, 20.0f);
				ImGui::DragFloat("Intensity", &sl->intensity, 0.05f, 0.0f, 10.0f);

				static float yawDeg = 0.0f;
				static float pitchDeg = -20.0f;
				ImGui::SliderFloat("Yaw(deg)", &yawDeg, -180.0f, 180.0f);
				ImGui::SliderFloat("Pitch(deg)", &pitchDeg, -89.0f, 89.0f);

				float yaw = DegToRad(yawDeg);
				float pitch = DegToRad(pitchDeg);
				sl->direction = {
					std::cos(pitch) * std::sin(yaw),
					std::sin(pitch),
					std::cos(pitch) * std::cos(yaw),
				};

				ImGui::DragFloat("Distance", &sl->distance, 0.1f, 0.01f, 100.0f);
				ImGui::DragFloat("Decay", &sl->decay, 0.05f, 0.01f, 8.0f);

				static float spotAngleDeg = 30.0f;
				ImGui::DragFloat("Angle(deg)", &spotAngleDeg, 0.1f, 1.0f, 89.0f);
				sl->cosAngle = std::cos(DegToRad(spotAngleDeg));
				ImGui::End();
			}
		}
	}

	// ===== タイトル3Dモデル編集 UI =====
	if (titleObject3d_) {
		ImGui::Begin("Title 3D");

		// Translation
		Vector3 trans = Credits3d_->GetTranslation();
		float transf[3] = { trans.x, trans.y, trans.z };
		if (ImGui::DragFloat3("Translation", transf, 0.01f, -100.0f, 100.0f)) {
			Credits3d_->SetTranslation({ transf[0], transf[1], transf[2] });
			// 更新した基準位置も同期しておく
			titleStartTranslation_ = Credits3d_->GetTranslation();
		}

		// Rotation
		Vector3 rot = titleObject3d_->GetRotation();
		float rotDeg[3] = {
			static_cast<float>(rot.x * 180.0f / 3.14159265f),
			static_cast<float>(rot.y * 180.0f / 3.14159265f),
			static_cast<float>(rot.z * 180.0f / 3.14159265f)
		};
		if (ImGui::DragFloat3("Rotation(deg)", rotDeg, 0.25f, -360.0f, 360.0f)) {
			titleObject3d_->SetRotation({
				DegToRad(rotDeg[0]),
				DegToRad(rotDeg[1]),
				DegToRad(rotDeg[2])
				});
		}

		// Scale
		Vector3 scale = titleObject3d_->GetScale();
		float scalef[3] = { scale.x, scale.y, scale.z };
		if (ImGui::DragFloat3("Scale", scalef, 0.01f, 0.001f, 100.0f)) {
			titleObject3d_->SetScale({ scalef[0], scalef[1], scalef[2] });
		}
		ImGui::End();
	}

	if (titleBotObject3d_) {
		ImGui::Begin("titleBotObject3d_");

		// Translation
		Vector3 trans = titleBotObject3d_->GetTranslation();
		float transf[3] = { trans.x, trans.y, trans.z };
		if (ImGui::DragFloat3("Translation", transf, 0.01f, -100.0f, 100.0f)) {
			titleBotObject3d_->SetTranslation({ transf[0], transf[1], transf[2] });
		}

		// Rotation
		Vector3 rot = titleBotObject3d_->GetRotation();
		float rotDeg[3] = {
			static_cast<float>(rot.x * 180.0f / 3.14159265f),
			static_cast<float>(rot.y * 180.0f / 3.14159265f),
			static_cast<float>(rot.z * 180.0f / 3.14159265f)
		};
		if (ImGui::DragFloat3("Rotation(deg)", rotDeg, 0.25f, -360.0f, 360.0f)) {
			titleBotObject3d_->SetRotation({
				DegToRad(rotDeg[0]),
				DegToRad(rotDeg[1]),
				DegToRad(rotDeg[2])
				});
		}

		// Scale
		Vector3 scale = titleBotObject3d_->GetScale();
		float scalef[3] = { scale.x, scale.y, scale.z };
		if (ImGui::DragFloat3("Scale", scalef, 0.01f, 0.001f, 100.0f)) {
			titleBotObject3d_->SetScale({ scalef[0], scalef[1], scalef[2] });
		}
		ImGui::End();
	}

	// ===== カメラ編集 UI =====
	if (camera_) {
		ImGui::Begin("Camera");

		// 翻訳（ワールド座標）
		Vector3 camTrans = camera_->GetTranslate();
		float camTransF[3] = { camTrans.x, camTrans.y, camTrans.z };
		if (ImGui::DragFloat3("Position", camTransF, 0.01f, -1000.0f, 1000.0f)) {
			camera_->SetTranslate({ camTransF[0], camTransF[1], camTransF[2] });
		}

		// 回転（内部はラジアンだが UI は度で扱う）
		const Vector3 camRot = camera_->GetRotate();
		float camRotDeg[3] = {
			static_cast<float>(camRot.x * 180.0f / 3.14159265f),
			static_cast<float>(camRot.y * 180.0f / 3.14159265f),
			static_cast<float>(camRot.z * 180.0f / 3.14159265f)
		};
		if (ImGui::DragFloat3("Rotation (deg)", camRotDeg, 0.1f, -360.0f, 360.0f)) {
			camera_->SetRotate({
				DegToRad(camRotDeg[0]),
				DegToRad(camRotDeg[1]),
				DegToRad(camRotDeg[2])
				});
		}

		// FOV（UIは度、内部はラジアン）
		static float fovDeg = 0.0f;
		static bool fovInitialized = false;
		if (!fovInitialized) {
			// Default approximate: 0.45 rad -> ~25.8 deg
			fovDeg = 0.45f * 180.0f / 3.14159265f;
			fovInitialized = true;
		}
		if (ImGui::DragFloat("FOV (deg)", &fovDeg, 0.1f, 1.0f, 179.0f)) {
			camera_->SetFovY(DegToRad(fovDeg));
		}

		// near / far
		// No getters provided; keep local caches.
		static float nearClip = 0.1f;
		static float farClip = 100.0f;
		if (ImGui::DragFloat("Near Clip", &nearClip, 0.001f, 0.001f, 100.0f)) {
			camera_->SetNearClip(nearClip);
		}
		if (ImGui::DragFloat("Far Clip", &farClip, 0.1f, 0.1f, 10000.0f)) {
			camera_->SetFarClip(farClip);
		}

		ImGui::End();
	}

#endif // USE_IMGUI

}

void TitleScene::Draw()
{
	Draw3D();
	Draw2D();
}

void TitleScene::Draw3D()
{
	engine_->Begin3D();

	if (skyObject3d_) {
		skyObject3d_->Draw();
	}

	// クレジット表示中はタイトルとピンを非表示にする
	if (!creditsActive_) {
		// タイトル3Dモデルを描画

		if (titleFloorObject3d_) {
			titleFloorObject3d_->Draw();
		}

		if (titleBotObject3d_) {
			titleBotObject3d_->Draw();
		}

		if (titleObject3d_) {
			titleObject3d_->Draw();
		}
		
		// 選択中のボタンのみ描画する
		if (selectedButtonIndex_ == 0) {
			if (button1Object3d_) {
				button1Object3d_->Draw();
			}
		} else {
			if (button2Object3d_) {
				button2Object3d_->Draw();
			}
		}

		// ピンを描画
		if (pinObject3d_) {
			pinObject3d_->Draw();
		}
	} else {
		// クレジット表示中：ボタン等を描画しない（必要ならここにクレジット専用描画を追加）
		if (Credits3d_) {
			Credits3d_->Draw();
		}
	}

	//ここから下で3DオブジェクトのDrawを呼ぶ
}

void TitleScene::Draw2D()
{
	engine_->Begin2D();

	//ここから下で2DオブジェクトのDrawを呼ぶ
}