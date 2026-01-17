#include "Debug/DebugCamera.h"

void DebugCamera::Initialize(const Vector3& translate) {
	matRot_ = MakeIdentity4x4();
	translation_ = translate;
	viewMatrix_ = MakeIdentity4x4();
}

void DebugCamera::Update(const InputKeyState& input) {

	// 追加回転分の回転行列を生成
	const float rotateSpeed = 0.05f;

	//if (input.IsPressKey(DIK_C)) {
	//	matRotDelta = Multiply(MakeRotateYMatrix(rotateSpeed), matRotDelta);
	//}

	//if (input.IsPressKey(DIK_Z)) {
	//	matRotDelta = Multiply(MakeRotateYMatrix(-rotateSpeed), matRotDelta);
	//}

	//if (input.IsPressKey(DIK_UP)) {
	//	matRotDelta = Multiply(MakeRotateXMatrix(-rotateSpeed), matRotDelta);
	//}

	//if (input.IsPressKey(DIK_DOWN)) {
	//	matRotDelta = Multiply(MakeRotateXMatrix(rotateSpeed), matRotDelta);
	//}

	//if (input.IsPressKey(DIK_RIGHT)) {
	//	matRotDelta = Multiply(MakeRotateZMatrix(rotateSpeed), matRotDelta);
	//}

	//if (input.IsPressKey(DIK_LEFT)) {
	//	matRotDelta = Multiply(MakeRotateZMatrix(-rotateSpeed), matRotDelta);
	//}

	if (input.IsPressKey(DIK_C)) { rotate_.z += rotateSpeed; }
	if (input.IsPressKey(DIK_Z)) { rotate_.z -= rotateSpeed; }

	if (input.IsPressKey(DIK_UP)) { rotate_.x -= rotateSpeed; }
	if (input.IsPressKey(DIK_DOWN)) { rotate_.x += rotateSpeed; }

	if (input.IsPressKey(DIK_RIGHT)) { rotate_.y += rotateSpeed; }
	if (input.IsPressKey(DIK_LEFT)) { rotate_.y -= rotateSpeed; }


	// 累積の回転行列を合成
	//matRot_ = Multiply(matRotDelta, matRot_);
	//matRot_ = Multiply(MakeRotateZMatrix(rotate_.z),
	//	Multiply(MakeRotateXMatrix(rotate_.x),
	//		MakeRotateYMatrix(rotate_.y)));


	//if (input.IsPressKey(DIK_E)) {

	//	//==================
	//	// 前入力
	//	//==================

	//	const float speed = 0.3f;

	//	// カメラ位置ベクトル
	//	Vector3 move = { 0, 0, speed };

	//	// 移動ベクトルを角度分だけ回転させる
	//	move = TransformNormal(move, matRot_);

	//	// 移動ベクトル分だけ座標を加算する
	//	translation_ = Add(translation_, move);
	//} else if (input.IsPressKey(DIK_Q)) {

	//	//==================
	//	// 後ろ入力
	//	//==================

	//	const float speed = -0.3f;

	//	// カメラ位置ベクトル
	//	Vector3 move = { 0, 0, speed };

	//	// 移動ベクトルを角度分だけ回転させる
	//	move = TransformNormal(move, matRot_);

	//	// 移動ベクトル分だけ座標を加算する
	//	translation_ = Add(translation_, move);
	//} else if (input.IsPressKey(DIK_A)) {

	//	//==================
	//	// 左入力
	//	//==================

	//	const float speed = -0.3f;

	//	// カメラ位置ベクトル
	//	Vector3 move = { speed, 0, 0 };

	//	// 移動ベクトルを角度分だけ回転させる
	//	move = TransformNormal(move, matRot_);

	//	// 移動ベクトル分だけ座標を加算する
	//	translation_ = Add(translation_, move);
	//} else if (input.IsPressKey(DIK_D)) {

	//	//==================
	//	// 右入力
	//	//==================

	//	const float speed = 0.3f;

	//	// カメラ位置ベクトル
	//	Vector3 move = { speed, 0, 0 };

	//	// 移動ベクトルを角度分だけ回転させる
	//	move = TransformNormal(move, matRot_);

	//	// 移動ベクトル分だけ座標を加算する
	//	translation_ = Add(translation_, move);
	//} else if (input.IsPressKey(DIK_W)) {

	//	//==================
	//	// 上入力
	//	//==================

	//	const float speed = 0.3f;

	//	// カメラ位置ベクトル
	//	Vector3 move = { 0, speed, 0 };

	//	// 移動ベクトルを角度分だけ回転させる
	//	move = TransformNormal(move, matRot_);

	//	// 移動ベクトル分だけ座標を加算する
	//	translation_ = Add(translation_, move);
	//} else if (input.IsPressKey(DIK_S)) {

	//	//==================
	//	// 下入力
	//	//==================

	//	const float speed = -0.3f;

	//	// カメラ位置ベクトル
	//	Vector3 move = { 0, speed, 0 };

	//	// 移動ベクトルを角度分だけ回転させる
	//	move = TransformNormal(move, matRot_);

	//	// 移動ベクトル分だけ座標を加算する
	//	translation_ = Add(translation_, move);
	//}

	const float speed = 0.3f;
	Vector3 moveLocal = { 0, 0, 0 };

	if (input.IsPressKey(DIK_E)) { moveLocal.z += speed; }
	if (input.IsPressKey(DIK_Q)) { moveLocal.z -= speed; }
	if (input.IsPressKey(DIK_D)) { moveLocal.x += speed; }
	if (input.IsPressKey(DIK_A)) { moveLocal.x -= speed; }
	if (input.IsPressKey(DIK_W)) { moveLocal.y += speed; }
	if (input.IsPressKey(DIK_S)) { moveLocal.y -= speed; }

	if (moveLocal.x != 0 || moveLocal.y != 0 || moveLocal.z != 0) {
		Vector3 moveWorld = TransformNormal(moveLocal, matRot_);
		translation_ = Add(translation_, moveWorld);
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
