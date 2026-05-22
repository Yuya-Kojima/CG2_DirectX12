#include "Debug/DebugCamera.h"

void DebugCamera::Initialize(const Vector3& translate) {
	matRot_ = MakeIdentity4x4();
	translation_ = translate;
	viewMatrix_ = MakeIdentity4x4();
}

void DebugCamera::Update(const Input& input) {

	// 右クリック中のみカメラ操作を有効にする (AAAエンジンの標準操作)
	if (input.IsMousePress(1)) {
		// マウスによる視点移動
		const float mouseSpeed = 0.005f;
		rotate_.y += input.GetMouseDeltaX() * mouseSpeed;
		rotate_.x += input.GetMouseDeltaY() * mouseSpeed;

		// 矢印キーでの視点移動も一応残しておく
		const float rotateSpeed = 0.05f;
		if (input.IsPressKey(DIK_UP)) { rotate_.x -= rotateSpeed; }
		if (input.IsPressKey(DIK_DOWN)) { rotate_.x += rotateSpeed; }
		if (input.IsPressKey(DIK_RIGHT)) { rotate_.y += rotateSpeed; }
		if (input.IsPressKey(DIK_LEFT)) { rotate_.y -= rotateSpeed; }

		// キーボードによる移動 (WASD + EQ)
		float speed = 0.3f;
		
		// Shiftキーを押している間は高速移動（AAAエンジンの標準機能）
		if (input.IsPressKey(DIK_LSHIFT) || input.IsPressKey(DIK_RSHIFT)) {
			speed *= 3.0f;
		}

		Vector3 moveLocal = { 0, 0, 0 };

		if (input.IsPressKey(DIK_W)) { moveLocal.z += speed; } // 前
		if (input.IsPressKey(DIK_S)) { moveLocal.z -= speed; } // 後ろ
		if (input.IsPressKey(DIK_D)) { moveLocal.x += speed; } // 右
		if (input.IsPressKey(DIK_A)) { moveLocal.x -= speed; } // 左
		if (input.IsPressKey(DIK_E)) { moveLocal.y += speed; } // 上
		if (input.IsPressKey(DIK_Q)) { moveLocal.y -= speed; } // 下

		if (moveLocal.x != 0 || moveLocal.y != 0 || moveLocal.z != 0) {
			Vector3 moveWorld = TransformNormal(moveLocal, matRot_);
			translation_ = Add(translation_, moveWorld);
		}
	}



	//Vector3 cameraPos = TransformNormal(translation_, matRot_);

	// 行列の更新
	//Matrix4x4 translateMatrix = MakeTranslateMatrix(cameraPos);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translation_);

	Matrix4x4 worldMatrix = Multiply(matRot_, translateMatrix);

	viewMatrix_ = Inverse(worldMatrix);

	matRot_ =
		MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate_, { 0.0f, 0.0f, 0.0f });

	camera_.SetTranslate(translation_);
	camera_.SetRotate(rotate_);
	camera_.Update();
}
