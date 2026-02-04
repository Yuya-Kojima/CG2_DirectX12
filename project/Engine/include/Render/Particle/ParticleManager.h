#pragma once
#include "Core/Dx12Core.h"
#include "Math/Matrix4x4.h"
#include "Math/VertexData.h"
#include "Particle/Particle.h"
#include <list>
#include <random>
#include <string>
#include <unordered_map>

class SrvManager;

class ParticleManager {

public:
  struct ParticleEmitDesc {
    std::string name;   // グループ名
    Vector3 position{}; // center
    uint32_t count = 1; // 発生数

    Vector3 baseVelocity{};   // ベース速度
    Vector3 velocityRandom{}; // 速度ばらつき(±)
    Vector3 spawnRandom{};    // 生成位置ばらつき(±)

    float lifeMin = 1.0f;
    float lifeMax = 1.0f;

    Vector4 color = Vector4{1.0f, 1.0f, 1.0f, 1.0f}; // 白

    Vector3 baseScale{1.0f, 1.0f, 1.0f};
    Vector3 scaleRandom{};
    Vector3 baseRotate{};
    Vector3 rotateRandom{};
    // Vector3 acceleration{};
  };

  /// <summary>
  /// シングルトンインスタンスの取得
  /// </summary>
  static ParticleManager *GetInstance();

  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(Dx12Core *dx12Core, SrvManager *srvManager);

  /// <summary>
  /// 更新処理
  /// </summary>
  void Update(const Matrix4x4 &viewMatrix, const Matrix4x4 &projectionMatrix);

  /// <summary>
  /// 描画
  /// </summary>
  void Draw(const std::string &name);

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

  // 生存しているパーティクルのみをクリア
  void ClearParticles(const std::string &groupName);

  // 全グループの生存パーティクルをクリア
  void ClearAllParticles();

  void Emit(const ParticleEmitDesc &desc);

  /// <summary>
  /// パーティクルの発生
  /// </summary>
  /// <param name="name">グループ名</param>
  /// <param name="position">発生中心</param>
  /// <param name="count">発生数</param>
  /// <param name="baseVelocity">ベース速度</param>
  /// <param name="velocityRandom">速度ばらつき幅（各成分の±）</param>
  /// <param name="spawnRandom">発生位置ばらつき幅（各成分の±）</param>
  /// <param name="lifeMin">寿命最小</param>
  /// <param name="lifeMax">寿命最大</param>
  void Emit(const std::string &name, const Vector3 &position, uint32_t count,
            const Vector3 &baseVelocity, const Vector3 &velocityRandom,
            const Vector3 &spawnRandom, float lifeMin, float lifeMax,
            const Vector4 &color = Vector4{1.0f, 1.0f, 1.0f, 1.0f},
            const Vector3 &baseScale = Vector3{1.0f, 1.0f, 1.0f},
            const Vector3 &scaleRandom = Vector3{},
            const Vector3 &baseRotate = Vector3{},
            const Vector3 &rotateRandom = Vector3{});

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

  struct Material {
    Vector4 color;
    int32_t enableLighting;
    float padding[3];
    Matrix4x4 uvTransform;
  };

  Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
  Material *materialData_ = nullptr;

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
  // std::uniform_real_distribution<float> distribution_;

  // バッファリソース
  Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_ = nullptr;

  // バッファリソースの使い道を補足するバッファビュー
  D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

  // パーティクルグループコンテナ
  std::unordered_map<std::string, ParticleGroup> particleGroups_;

  // インスタンシング用SRVの空き番号管理用インデックス
  // uint32_t nextInstancingSrvIndex_ = 0;

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

public:
  /// <summary>
  /// パーティクルグループの生成
  /// </summary>
  /// <param name="name"></param>
  /// <param name="textureFilePath"></param>
  void CreateParticleGroup(const std::string &name,
                           const std::string textureFilePath);

  bool HasGroup(const std::string &name) const;

private:
  Matrix4x4 viewMatrix_;
  Matrix4x4 projectionMatrix_;

private:
  const uint32_t kNumMaxInstance_ = 100;
};
