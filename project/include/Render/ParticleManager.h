#pragma once
#include "Core/Dx12Core.h"
#include "Math/Matrix4x4.h"
#include "Math/VertexData.h"
#include "Render/Particle.h"
#include <list>
#include <random>
#include <string>
#include <unordered_map>

class SrvManager;

class ParticleManager {

public:
  /// <summary>
  /// シングルトンインスタンスの取得
  /// </summary>
  static ParticleManager *GetInstance();

  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(Dx12Core *dx12Core, SrvManager *srvManager);

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

private:
  struct MaterialData {
    std::string filePath;
    uint32_t textureSrvIndex;
  };

  struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Vector4 color;
  };

  struct ParticleGroup {
    MaterialData materialData;
    std::list<Particle> particles;
    uint32_t instancingSrvIndex = 0;
    Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;
    uint32_t numInstance = 0;
    ParticleForGPU *instancingData = nullptr;
  };

  ParticleManager() = default;
  ~ParticleManager() = default;
  ParticleManager(ParticleManager &) = delete;
  ParticleManager &operator=(ParticleManager &) = delete;

private:
  Dx12Core *dx12Core_ = nullptr;

  SrvManager *srvManager_ = nullptr;

  // ルートシグネチャ
  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

  // PSO
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState_ = nullptr;

  // ランダムエンジン
  std::random_device seedGenerator_;
  std::mt19937 randomEngine_;
  std::uniform_real_distribution<float> distribution_;

  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;

  // バッファリソースの使い道を補足するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

  // パーティクルグループコンテナ
  std::unordered_map<std::string, ParticleGroup> particleGroups_;

private:
  /// <summary>
  /// ルートシグネチャを作成
  /// </summary>
  void CreateRootSignature();

  /// <summary>
  /// オブジェクト用のPSOを作成
  /// </summary>
  void CreatePSO();

  /// <summary>
  /// 頂点データの作成
  /// </summary>
  void CreateVertexData();

  void CreateParticleGroup(const std::string name,
                           const std::string textureFilePath);
};
