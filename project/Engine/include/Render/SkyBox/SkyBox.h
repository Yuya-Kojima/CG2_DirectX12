#pragma once
#include "Core/Dx12Core.h"
#include "Math/MathUtil.h"
#include "Math/VertexData.h"
#include <string>
#include <vector>
#include <wrl.h>

class ICamera;
class SkyboxRenderer;

class Skybox {

  struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
  };

  struct Material {
    Vector4 color;
  };

public:
  void Initialize(SkyboxRenderer *skyboxRenderer);
  void Update();
  void Draw();

  void SetCamera(const ICamera *camera) { camera_ = camera; }
  void SetTexture(const std::string &filePath) { textureFilePath_ = filePath; }

  void SetScale(const Vector3 &scale) { transform_.scale = scale; }
  void SetRotation(const Vector3 &rotate) { transform_.rotate = rotate; }
  void SetTranslation(const Vector3 &translate) {
    transform_.translate = translate;
  }

  Vector3 GetScale() const { return transform_.scale; }
  Vector3 GetRotation() const { return transform_.rotate; }
  Vector3 GetTranslation() const { return transform_.translate; }

private:
  void CreateVertices();
  void CreateVertexResource();
  void CreateTransformationMatrixResource();
  void CreateMaterialResource();

private:
  SkyboxRenderer *skyboxRenderer_ = nullptr;
  Dx12Core *dx12Core_ = nullptr;

  const ICamera *camera_ = nullptr;

  Transform transform_{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };

  std::string textureFilePath_;

  std::vector<VertexData> vertices_;

  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
  VertexData *vertexData_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
  TransformationMatrix *transformationMatrixData_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
  Material *materialData_ = nullptr;
};