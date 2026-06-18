#pragma once
#include "Core/WindowSystem.h"
#include "Math/Matrix4x4.h"
#include "Math/Transform.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <cassert>
#include <d3d12.h>
#include <string>
#include <wrl.h>

class Dx12Core;
class SpriteRenderer;

class Text {
  struct VertexData {
    Vector4 position;
    Vector2 texcoord;
  };

  struct Material {
    Vector4 color;
    Matrix4x4 uvTransform;
  };

  struct TransformationMatrix {
    Matrix4x4 WVP;
  };

  struct UIEffectParams {
    float time;
    int effectType;
    float splitY;
    float amplitude;
    float frequency;
    float speed;
    float padding1;
    float padding2;
  };

public:
  void Initialize(SpriteRenderer *spriteRenderer, const std::string &fontName);
  void Update();
  void Draw();

  void SetText(const std::string &text) { text_ = text; }
  std::string GetText() const { return text_; }

  void SetPosition(const Vector2 &position) {
    transform_.translate.x = position.x;
    transform_.translate.y = position.y;
  }
  Vector2 GetPosition() const {
    return {transform_.translate.x, transform_.translate.y};
  }

  void SetScale(const Vector2 &scale) {
    transform_.scale = {scale.x, scale.y, 1.0f};
  }
  Vector2 GetScale() const { return {transform_.scale.x, transform_.scale.y}; }

  void SetAnchorPoint(const Vector2 &anchor) { anchorPoint_ = anchor; }
  Vector2 GetAnchorPoint() const { return anchorPoint_; }

  Vector2 GetSize() const { return size_; }

  void SetColor(const Vector4 &color) { color_ = color; }
  Vector4 GetColor() const { return color_; }

private:
  void CreateBuffers();
  void UpdateVertices();

  SpriteRenderer *spriteRenderer_ = nullptr;
  Dx12Core *dx12Core_ = nullptr;
  std::string fontName_;
  std::string text_;
  Vector2 size_ = {0.0f, 0.0f};

  // 1回の描画でサポートする最大文字数
  static const int kMaxCharacters = 256;

  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Resource> indexResource = nullptr;
  VertexData *vertexData = nullptr;
  uint32_t *indexData = nullptr;

  D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
  D3D12_INDEX_BUFFER_VIEW indexBufferView{};

  Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = nullptr;
  Material *materialData = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource = nullptr;
  TransformationMatrix *transformationMatrixData = nullptr;

  Microsoft::WRL::ComPtr<ID3D12Resource> uiEffectParamsResource_ = nullptr;
  UIEffectParams *uiEffectParamsData_ = nullptr;

  Transform transform_{
      {1.0f, 1.0f, 1.0f},
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
  };
  Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};
  Vector2 anchorPoint_ = {0.0f, 0.0f};
};
