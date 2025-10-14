#pragma once
#include "Dx12Core.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

class TextureManager {

public:
  /// <summary>
  /// シングルトンインスタンスの取得
  /// </summary>
  /// <returns></returns>
  static TextureManager *GetInstance();

  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(Dx12Core *dx12Core);

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

  /// <summary>
  /// テクスチャファイルの読み込み
  /// </summary>
  /// <param name="filePath">テクスチャファイルパス</param>
  /// <returns>画像イメージデータ</returns>
  void LoadTexture(const std::string &filePath);

private:
  static TextureManager *instance;

  TextureManager() = default;
  ~TextureManager() = default;
  TextureManager(TextureManager &) = delete;
  TextureManager &operator=(TextureManager &) = delete;

  // テクスチャ一枚分のデータ
  struct TextureData {
    std::string filePath;
    DirectX::TexMetadata metadata;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
  };

  // テクスチャデータ
  std::vector<TextureData> textureDatas;

  Dx12Core *dx12Core_ = nullptr;

  // SRVインデックスの開始番号
  static uint32_t kSRVIndexTop;

public:
  /// <summary>
  /// SRVインデックスの開始番号
  /// </summary>
  /// <param name="filePath"></param>
  /// <returns></returns>
  uint32_t GetTextureIndexByFilePath(const std::string &filePath);

  /// <summary>
  /// テクスチャ番号からGPUハンドルを取得z
  /// </summary>
  /// <param name="textureIndex"></param>
  /// <returns></returns>
  D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);
};
