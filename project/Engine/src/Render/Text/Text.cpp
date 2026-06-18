#include "Render/Text/Text.h"
#include "Core/Dx12Core.h"
#include "Debug/Logger.h"
#include "Math/MathUtil.h"
#include "Render/Renderer/SpriteRenderer.h"
#include "Render/Text/FontManager.h"
#include "Render/Texture/TextureManager.h"
#include "stb_truetype.h"
#include <algorithm>

void Text::Initialize(SpriteRenderer *spriteRenderer,
                      const std::string &fontName) {
  spriteRenderer_ = spriteRenderer;
  dx12Core_ = spriteRenderer->GetDx12Core();
  fontName_ = fontName;

  CreateBuffers();
}

void Text::CreateBuffers() {
  vertexResource =
      dx12Core_->CreateBufferResource(sizeof(VertexData) * kMaxCharacters * 4);
  vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

  indexResource =
      dx12Core_->CreateBufferResource(sizeof(uint32_t) * kMaxCharacters * 6);
  indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));

  for (int i = 0; i < kMaxCharacters; ++i) {
    indexData[i * 6 + 0] = i * 4 + 0;
    indexData[i * 6 + 1] = i * 4 + 1;
    indexData[i * 6 + 2] = i * 4 + 2;
    indexData[i * 6 + 3] = i * 4 + 1;
    indexData[i * 6 + 4] = i * 4 + 3;
    indexData[i * 6 + 5] = i * 4 + 2;
  }

  vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
  vertexBufferView.SizeInBytes = sizeof(VertexData) * kMaxCharacters * 4;
  vertexBufferView.StrideInBytes = sizeof(VertexData);

  indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
  indexBufferView.SizeInBytes = sizeof(uint32_t) * kMaxCharacters * 6;
  indexBufferView.Format = DXGI_FORMAT_R32_UINT;

  materialResource = dx12Core_->CreateBufferResource(sizeof(Material));
  materialResource->Map(0, nullptr, reinterpret_cast<void **>(&materialData));
  materialData->color = color_;
  materialData->uvTransform = MakeIdentity4x4();

  transformationMatrixResource =
      dx12Core_->CreateBufferResource(sizeof(TransformationMatrix));
  transformationMatrixResource->Map(
      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData));

  uiEffectParamsResource_ =
      dx12Core_->CreateBufferResource(sizeof(UIEffectParams));
  uiEffectParamsResource_->Map(0, nullptr,
                               reinterpret_cast<void **>(&uiEffectParamsData_));
  *uiEffectParamsData_ = {0};
}

void Text::UpdateVertices() {
  const FontData *fontData = FontManager::GetInstance()->GetFontData(fontName_);
  if (!fontData)
    return;

  float x = 0.0f;
  float y = fontData->pixelHeight; // ベースラインを下げて画面内に収める

  int charCount = (std::min)((int)text_.size(), kMaxCharacters);

  for (int i = 0; i < charCount; ++i) {
    char c = text_[i];
    if (c >= 32 && c < 128) {
      stbtt_aligned_quad q;
      stbtt_GetBakedQuad(const_cast<stbtt_bakedchar *>(fontData->cdata.data()),
                         fontData->textureWidth, fontData->textureHeight,
                         c - 32, &x, &y, &q, 1);

      int vIdx = i * 4;

      // Bottom-Left
      vertexData[vIdx + 0].position = {q.x0, q.y1, 0.0f, 1.0f};
      vertexData[vIdx + 0].texcoord = {q.s0, q.t1};

      // Top-Left
      vertexData[vIdx + 1].position = {q.x0, q.y0, 0.0f, 1.0f};
      vertexData[vIdx + 1].texcoord = {q.s0, q.t0};

      // Bottom-Right
      vertexData[vIdx + 2].position = {q.x1, q.y1, 0.0f, 1.0f};
      vertexData[vIdx + 2].texcoord = {q.s1, q.t1};

      // Top-Right
      vertexData[vIdx + 3].position = {q.x1, q.y0, 0.0f, 1.0f};
      vertexData[vIdx + 3].texcoord = {q.s1, q.t0};
    } else {
      int vIdx = i * 4;
      for (int j = 0; j < 4; ++j) {
        vertexData[vIdx + j].position = {0, 0, 0, 1};
        vertexData[vIdx + j].texcoord = {0, 0};
      }
    }
  }
  size_ = {x, fontData->pixelHeight};

  // アンカーポイントの適用
  float offsetX = size_.x * anchorPoint_.x;
  float offsetY = size_.y * anchorPoint_.y;
  for (int i = 0; i < charCount * 4; ++i) {
    vertexData[i].position.x -= offsetX;
    vertexData[i].position.y -= offsetY;
  }
}

void Text::Update() {
  UpdateVertices();

  materialData->color = color_;
  materialData->uvTransform = MakeIdentity4x4();

  Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate,
                                           transform_.translate);
  Matrix4x4 viewMatrix = MakeIdentity4x4();
  Matrix4x4 projectionMatrix =
      MakeOrthographicMatrix(0.0f, 0.0f, (float)WindowSystem::kClientWidth,
                             (float)WindowSystem::kClientHeight, 0.0f, 100.0f);
  Matrix4x4 wvpMatrix =
      Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

  transformationMatrixData->WVP = wvpMatrix;
}

void Text::Draw() {
  if (text_.empty())
    return;

  ID3D12GraphicsCommandList *commandList = dx12Core_->GetCommandList();

  commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
  commandList->IASetIndexBuffer(&indexBufferView);

  commandList->SetGraphicsRootConstantBufferView(
      0, materialResource->GetGPUVirtualAddress());
  commandList->SetGraphicsRootConstantBufferView(
      1, transformationMatrixResource->GetGPUVirtualAddress());

  commandList->SetGraphicsRootDescriptorTable(
      2, TextureManager::GetInstance()->GetSrvHandleGPU(fontName_));

  int charCount = (std::min)((int)text_.size(), kMaxCharacters);
  commandList->DrawIndexedInstanced(charCount * 6, 1, 0, 0, 0);
}
