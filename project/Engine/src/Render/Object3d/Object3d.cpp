#include "Object3d/Object3d.h"
#include "Camera/GameCamera.h"
#include "Debug/Logger.h"
#include "Math/MathUtil.h"
#include "Model/Model.h"
#include "Model/ModelManager.h"
#include "Renderer/Object3dRenderer.h"
#include "Texture/TextureManager.h"
#include <cassert>
#include <fstream>
#include <numbers>

void Object3d::Initialize(Object3dRenderer *object3dRenderer) {

  object3dRenderer_ = object3dRenderer;

  dx12Core_ = object3dRenderer_->GetDx12Core();

  CreateTransformationMatrixData();

  CreateCameraForGPUData();

  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  // cameraTransform_ = {
  //     {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 4.0f, -10.0f}};

  // this->camera = object3dRenderer_->GetDefaultCamera();
  this->camera_ = nullptr;
}

void Object3d::Update() {

  GameCamera *activeCamera =
      (camera_ != nullptr) ? camera_ : object3dRenderer_->GetDefaultCamera();

  transform_ = {scale_, rotate_, translate_};

  Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                           transform_.translate);

  Matrix4x4 finalWorld = worldMatrix;
  if (model_) {
    finalWorld = Multiply(model_->GetRootLocalMatrix(), worldMatrix);
  }

  Matrix4x4 worldViewProjectionMatrix;

  // Matrix4x4 cameraMatrix =
  //     MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate,
  //                      cameraTransform_.translate);
  //  Matrix4x4 viewMatrix = Inverse(cameraMatrix);
  // Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
  //    0.45f,
  //    float(WindowSystem::kClientWidth) / float(WindowSystem::kClientHeight),
  //    0.1f, 100.0f);

  // if (camera_) {
  //	const Matrix4x4& viewProjectionMatrix =
  // camera->GetViewProjectionMatrix(); 	worldViewProjectionMatrix =
  // Multiply(worldMatrix, viewProjectionMatrix); } else {
  //	worldViewProjectionMatrix = worldMatrix;
  // }

  if (activeCamera) {
    const Matrix4x4 &vp = activeCamera->GetViewProjectionMatrix();
    worldViewProjectionMatrix = Multiply(finalWorld, vp);
    cameraForGPUData->worldPosition = activeCamera->GetTranslate();
  } else {
    worldViewProjectionMatrix = finalWorld;
  }

  transformationMatrixData->WVP = worldViewProjectionMatrix;
  transformationMatrixData->World = finalWorld;
  transformationMatrixData->WorldInverseTranspose =
      Transpose(Inverse(finalWorld));
}

void Object3d::Draw() {

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
      dx12Core_->GetCommandList();

  // wvp用のCBufferの場所を設定
  commandList->SetGraphicsRootConstantBufferView(
      1, transformationMatrixResource->GetGPUVirtualAddress());

  // Lighting
  commandList->SetGraphicsRootConstantBufferView(
      3,
      object3dRenderer_->GetDirectionalLightResource()->GetGPUVirtualAddress());

  // Camera
  commandList->SetGraphicsRootConstantBufferView(
      4, cameraForGPUResource->GetGPUVirtualAddress());

  // PointLight
  commandList->SetGraphicsRootConstantBufferView(
      5, object3dRenderer_->GetPointLightResource()->GetGPUVirtualAddress());

  // SpotLight
  commandList->SetGraphicsRootConstantBufferView(
      6, object3dRenderer_->GetSpotLightResource()->GetGPUVirtualAddress());

  // 3Dモデルが割り当てられていれば描画する
  if (model_) {
    model_->Draw();
  }
}

void Object3d::SetModel(const std::string &filePath) {

  // モデルを検索し、セット
  model_ = ModelManager::GetInstance()->FindModel(filePath);

  // LoadModel と SetModel の引数不一致（スペルミス等）を即座に検知する
  if (!model_) {
    Logger::Log(std::string("[Object3d] SetModel failed. Model not found: ") +
                filePath);

    // Debug では assert、Release でも沈黙しないように abort
    assert(false && "Object3d::SetModel: model not loaded / not found");
#if defined(NDEBUG)
    std::abort();
#endif
  }
}

void Object3d::CreateTransformationMatrixData() {

  // cbvのsizeは256バイト単位で確保する
  UINT transformationMatrixSize = (sizeof(TransformationMatrix) + 255) & ~255;

  assert(transformationMatrixSize % 256 == 0);

  transformationMatrixResource;

  // データを書き込む
  transformationMatrixResource =
      dx12Core_->CreateBufferResource(transformationMatrixSize);

  // 書き込むためのアドレスを取得
  transformationMatrixResource->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData));

  // 単位行列を書き込んでおく
  transformationMatrixData->World = MakeIdentity4x4();
  transformationMatrixData->WVP = MakeIdentity4x4();
  transformationMatrixData->WorldInverseTranspose = MakeIdentity4x4();
}

void Object3d::CreateCameraForGPUData() {

  UINT cameraSize = (sizeof(CameraForGPU) + 255) & ~255;
  cameraForGPUResource = dx12Core_->CreateBufferResource(cameraSize);

  // データを書き込む
  cameraForGPUData = nullptr;

  // 書き込むためのアドレスを取得
  cameraForGPUResource->Map(0, nullptr,
                            reinterpret_cast<void **>(&cameraForGPUData));

  cameraForGPUData->worldPosition = {0.0f, 0.0f, 0.0f};
}
