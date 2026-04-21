#include "Texture/TextureManager.h"
// #include "Dx12Core.h"
#include "Core/SrvManager.h"
#include "Debug/Logger.h"
#include "Util/StringUtil.h"
#include <filesystem>

// ImGuiで０番を使用するため、1番から使用する
// uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager *TextureManager::GetInstance() {
  static TextureManager instance;
  return &instance;
}

void TextureManager::Initialize(Dx12Core *dx12Core, SrvManager *srvManager) {

  dx12Core_ = dx12Core;

  srvManager_ = srvManager;

  // SRVの数と同数
  // textureDatas.reserve(Dx12Core::kMaxSRVCount);
}

void TextureManager::Finalize() {
  TextureManager *instance = GetInstance();
  instance->textureDatas.clear();
  instance->intermediateResources_.clear();
  instance->dx12Core_ = nullptr;
  instance->srvManager_ = nullptr;
}

void TextureManager::LoadTexture(const std::string &filePath) {

  if (textureDatas.contains(filePath)) {
    return;
  }

  assert(srvManager_->CanAllocate());

  DirectX::ScratchImage image{};
  DirectX::ScratchImage mipImages{};
  std::wstring filePathW = StringUtil::ConvertString(filePath);

  HRESULT hr = S_OK;

  //========================
  // 読み込み
  //========================
  if (filePathW.size() >= 4 &&
      filePathW.substr(filePathW.size() - 4) == L".dds") {
    hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE,
                                  nullptr, image);
  } else {
    hr = DirectX::LoadFromWICFile(
        filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
  }
  assert(SUCCEEDED(hr));

  const auto &meta = image.GetMetadata();

  //========================
  // MipMap
  //========================
  if (DirectX::IsCompressed(meta.format)) {
    mipImages = std::move(image);
  } else {
    bool needMip = (meta.mipLevels <= 1) && (meta.width > 1 || meta.height > 1);

    if (needMip) {
      hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                    meta, DirectX::TEX_FILTER_DEFAULT, 0,
                                    mipImages);
      assert(SUCCEEDED(hr));
    } else {
      mipImages = std::move(image);
    }
  }

  TextureData &textureData = textureDatas[filePath];

  textureData.metadata = mipImages.GetMetadata();
  textureData.resource = dx12Core_->CreateTextureResource(textureData.metadata);

  intermediateResources_.push_back(
      dx12Core_->UploadTextureData(textureData.resource, mipImages));

  textureData.srvIndex = srvManager_->Allocate();
  textureData.srvHandleCPU =
      srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
  textureData.srvHandleGPU =
      srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = textureData.metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

  if (textureData.metadata.IsCubemap()) {
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = textureData.metadata.mipLevels
                                        ? UINT(textureData.metadata.mipLevels)
                                        : 1u;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
  } else {
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = textureData.metadata.mipLevels
                                      ? UINT(textureData.metadata.mipLevels)
                                      : 1u;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
  }

  dx12Core_->GetDevice()->CreateShaderResourceView(
      textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);
}

D3D12_GPU_DESCRIPTOR_HANDLE
TextureManager::GetSrvHandleGPU(const std::string &filePath) const {

  // 範囲外指定違反チェック
  assert(textureDatas.contains(filePath));

  const TextureData &textureData = textureDatas.at(filePath);
  return textureData.srvHandleGPU;
}

const DirectX::TexMetadata &
TextureManager::GetMetaData(const std::string &filePath) const {
  // 範囲外指定違反チェック
  assert(textureDatas.contains(filePath));

  const TextureData &textureData = textureDatas.at(filePath);
  return textureData.metadata;
}

uint32_t TextureManager::GetSrvIndex(const std::string &filePath) const {

  assert(textureDatas.contains(filePath));

  const TextureData &textureData = textureDatas.at(filePath);
  return textureData.srvIndex;
}
