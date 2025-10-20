#include "Object3d.h"
#include "GameCamera.h"
#include "MathUtil.h"
#include "Model.h"
#include "ModelManager.h"
#include "Object3dRenderer.h"
#include "TextureManager.h"
#include <cassert>
#include <fstream>
#include <numbers>

void Object3d::Initialize(Object3dRenderer *object3dRenderer) {

  object3dRenderer_ = object3dRenderer;

  dx12Core_ = object3dRenderer_->GetDx12Core();

  CreateTransformationMatrixData();

  CreateDirectionalLightData();

  transform_ = {{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
  // cameraTransform_ = {
  //     {1.0f, 1.0f, 1.0f}, {0.3f, 0.0f, 0.0f}, {0.0f, 4.0f, -10.0f}};

  this->camera = object3dRenderer_->GetDefaultCamera();
}

void Object3d::Update() {

  transform_ = {scale_, rotate_, translate_};

  Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                           transform_.translate);
  Matrix4x4 worldViewProjectionMatrix;

  // Matrix4x4 cameraMatrix =
  //     MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate,
  //                      cameraTransform_.translate);
  //  Matrix4x4 viewMatrix = Inverse(cameraMatrix);
  // Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
  //    0.45f,
  //    float(WindowSystem::kClientWidth) / float(WindowSystem::kClientHeight),
  //    0.1f, 100.0f);

  if (camera) {
    const Matrix4x4 &viewProjectionMatrix = camera->GetViewProjectionMatrix();
    worldViewProjectionMatrix = Multiply(worldMatrix,viewProjectionMatrix);
  } else {
    worldViewProjectionMatrix = worldMatrix;
  }

  transformationMatrixData->WVP = worldViewProjectionMatrix;
  transformationMatrixData->World = worldMatrix;
}

void Object3d::Draw() {

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
      dx12Core_->GetCommandList();

  // wvp用のCBufferの場所を設定
  commandList->SetGraphicsRootConstantBufferView(
      1, transformationMatrixResource->GetGPUVirtualAddress());

  // Lighting
  commandList->SetGraphicsRootConstantBufferView(
      3, directionalLightResource->GetGPUVirtualAddress());

  // 3Dモデルが割り当てられていれば描画する
  if (model_) {
    model_->Draw();
  }
}

void Object3d::SetModel(const std::string &filePath) {

  // モデルを検索し、セット
  model_ = ModelManager::GetInstance()->FindModel(filePath);
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
}

void Object3d::CreateDirectionalLightData() {

  directionalLightResource =
      dx12Core_->CreateBufferResource(sizeof(DirectionalLight));

  // データを書き込む
  directionalLightData = nullptr;

  // 書き込むためのアドレスを取得
  directionalLightResource->Map(
      0, nullptr, reinterpret_cast<void **>(&directionalLightData));

  // Lightingの色
  directionalLightData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  directionalLightData->direction = Normalize(Vector3(0.0f, -1.0f, 0.0f));
  directionalLightData->intensity = 1.0f;
}
