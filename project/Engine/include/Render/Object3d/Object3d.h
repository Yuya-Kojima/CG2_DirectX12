#pragma once
#include "Camera/CameraForGPU.h"
#include "Core/Dx12Core.h"
#include "Math/MathUtil.h"
#include <d3d12.h>
#include <stdint.h>
#include <wrl.h>
#include"Model/Model.h"

class Dx12Core;

class Object3dRenderer;

class ICamera;

class Object3d {

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInverseTranspose;
	};

public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Object3dRenderer* object3dRenderer);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

private:
	Object3dRenderer* object3dRenderer_ = nullptr;

	Dx12Core* dx12Core_ = nullptr;

	Model* model_ = nullptr;

public:
	Model* GetModel() const { return model_; }

	void SetModel(const std::string& filePath);

private:
	/* 座標変換行列データ
	-----------------------------*/
	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource = nullptr;

	// バッファリソース内のデータを指すポインタ
	TransformationMatrix* transformationMatrixData = nullptr;

	/// <summary>
	/// 座標変換行列データを生成
	/// </summary>
	void CreateTransformationMatrixData();

	/* 鏡面反射用データ
	-----------------------------*/
	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraForGPUResource = nullptr;

	// バッファリソース内のデータを指すポインタ
	CameraForGPU* cameraForGPUData = nullptr;

	/// <summary>
	/// カメラデータを生成
	/// </summary>
	void CreateCameraForGPUData();

	/* マテリアルデータ
----------------------------*/

// マテリアルバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;

	// バッファリソース内のデータを指すポインタ
	Model::Material* materialData = nullptr;

	/// <summary>
/// マテリアルデータを作成
/// </summary>
	void CreateMaterialData();

private:
	Vector3 scale_{ 1.0f, 1.0f, 1.0f };

	Vector3 rotate_{};

	Vector3 translate_{};

	Transform transform_{};

	// Transform cameraTransform_{};

public:
	// scale
	Vector3 GetScale() const { return scale_; }

	void SetScale(const Vector3& scale) { scale_ = scale; }

	// rotation
	Vector3 GetRotation() const { return rotate_; }

	void SetRotation(const Vector3& rotate) { rotate_ = rotate; }

	// translation
	Vector3 GetTranslation() const { return translate_; }

	void SetTranslation(const Vector3& translate) { translate_ = translate; }

	Vector4 GetColor() const { return materialData ? materialData->color : Vector4(1, 1, 1, 1); }

	void SetColor(const Vector4& color) { if (materialData) { materialData->color = color; } }

	void SetAlpha(float a) {
		if (materialData) { materialData->color.w = a; }
	}

	// カメラ
	// void SetCameraMatrix(Transform cameraTransform) {
	//  cameraTransform_ = cameraTransform;
	//}

private:
	// uint32_t numInstance_{};

private:
	const ICamera* camera_ = nullptr;

public:
	void SetCamera(const ICamera* camera) { this->camera_ = camera; }
};
