#include "Render/TextureManager.h"
// #include "Dx12Core.h"
#include "Core/SrvManager.h"
#include "Util/StringUtil.h"

TextureManager *TextureManager::instance = nullptr;

// ImGuiで０番を使用するため、1番から使用する
// uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager *TextureManager::GetInstance() {

  if (instance == nullptr) {
    instance = new TextureManager;
  }

  return instance;
}

void TextureManager::Initialize(Dx12Core *dx12Core, SrvManager *srvManager) {

  dx12Core_ = dx12Core;

  srvManager_ = srvManager;

  // SRVの数と同数
  // textureDatas.reserve(Dx12Core::kMaxSRVCount);
}

void TextureManager::Finalize() {
  delete instance;
  instance = nullptr;
}

void TextureManager::LoadTexture(const std::string &filePath) {

  //// 読み込み済みテクスチャを検索
  // auto it = std::find_if(textureDatas.begin(), textureDatas.end(),
  //                        [&](TextureData &textureData) {
  //                          return textureData.filePath == filePath;
  //                        });

  // if (it != textureDatas.end()) {
  //   // 読み込み済みなら早期return
  //   return;
  // }

  if (textureDatas.contains(filePath)) {
    return;
  }

  // assert(textureDatas.size() + kSRVIndexTop < Dx12Core::kMaxSRVCount);

  assert(srvManager_->CanAllocate());

  // テクスチャファイルを読んでプログラムで扱えるようにする
  DirectX::ScratchImage image{};
  std::wstring filePathW = StringUtil::ConvertString(filePath);
  HRESULT hr = DirectX::LoadFromWICFile(
      filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
  assert(SUCCEEDED(hr));

  // ミニマップの作成
  DirectX::ScratchImage mipImages{};
  hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(),
                                image.GetMetadata(), DirectX::TEX_FILTER_SRGB,
                                0, mipImages);
  assert(SUCCEEDED(hr));

  // テクスチャデータを追加
  // textureDatas.resize(textureDatas.size() + 1);

  // 追加したテクスチャデータの参照を取得する
  TextureData &textureData = textureDatas[filePath];

  textureData.metadata = mipImages.GetMetadata();
  textureData.resource = dx12Core_->CreateTextureResource(textureData.metadata);

  dx12Core_->UploadTextureData(textureData.resource, mipImages);

  //   dx12Core_->UploadTextureData(textureData.resource, mipImages);
  //
  // テクスチャデータの要素番号をSRVのインデックスとする

  textureData.srvIndex = srvManager_->Allocate();
  textureData.srvHandleCPU =
      srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
  textureData.srvHandleGPU =
      srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);
  //
  //   ID3D12Device *device = dx12Core_->GetDevice();
  //
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

  // SRVの設定を行う
  srvDesc.Format = textureData.metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);

  // 設定をもとにSRVの作成
  dx12Core_->GetDevice()->CreateShaderResourceView(
      textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);
  //   device->CreateShaderResourceView(textureData.resource.Get(), &srvDesc,
  //                                    textureData.srvHandleCPU);
}

// uint32_t
// TextureManager::GetTextureIndexByFilePath(const std::string &filePath) {
//
//   auto it = std::find_if(textureDatas.begin(), textureDatas.end(),
//                          [&](TextureData &textureData) {
//                            return textureData.filePath == filePath;
//                          });
//
//   if (it != textureDatas.end()) {
//     // 読み込み済みなら要素番号を返す
//     uint32_t textureIndex =
//         static_cast<uint32_t>(std::distance(textureDatas.begin(), it));
//     return textureIndex;
//   }
//
//   assert(0);
//   return 0;
// }

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
