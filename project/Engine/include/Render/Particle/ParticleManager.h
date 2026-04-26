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
// ParticleEmitDesc removed (moved to IParticleEmitter.h)

	/// <summary>
	/// シングルトンインスタンスの取得
	/// </summary>
	static ParticleManager* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Dx12Core* dx12Core, SrvManager* srvManager);

	/// <summary>
	/// 終了
	/// </summary>
	void Finalize();

	// Methods related to individual groups are removed.

private:
	// Structs moved to IParticleEmitter.h

	ParticleManager() = default;
	~ParticleManager() = default;
	ParticleManager(ParticleManager&) = delete;
	ParticleManager& operator=(ParticleManager&) = delete;

	// Material moved to IParticleEmitter.h

private:
	Dx12Core* dx12Core_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// ルートシグネチャ (Graphics)
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
	// PSO (Graphics)
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState_ = nullptr;

	// Compute用 ルートシグネチャとPSO (初期化用)
	Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> initializeComputePipelineState_ = nullptr;

	// Compute用 ルートシグネチャとPSO (Emit用)
	Microsoft::WRL::ComPtr<ID3D12RootSignature> emitComputeRootSignature_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> emitComputePipelineState_ = nullptr;

	// GPU Particle用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> gpuParticleResource_ = nullptr;
	uint32_t gpuParticleSrvIndex_ = 0;
	uint32_t gpuParticleUavIndex_ = 0;

	// PerView 用構造体とリソース
	struct PerView {
		Matrix4x4 viewProjection;
		Matrix4x4 billboardMatrix;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> perViewResource_ = nullptr;
	PerView* perViewData_ = nullptr;

	// EmitterSphere用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> emitterSphereResource_ = nullptr;
	EmitterSphere* emitterSphereData_ = nullptr;

	// PerFrame用構造体とリソース
	struct PerFrame {
		float time;
		float deltaTime;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> perFrameResource_ = nullptr;
	PerFrame* perFrameData_ = nullptr;

	// フリーカウンター用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> freeCounterResource_ = nullptr;

	const uint32_t kMaxParticles = 1024;

public:
	Dx12Core* GetDx12Core() const { return dx12Core_; }
	SrvManager* GetSrvManager() const { return srvManager_; }
	ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
	ID3D12PipelineState* GetPipelineState() const { return graphicsPipeLineState_.Get(); }

	uint32_t GetGPUParticleSrvIndex() const { return gpuParticleSrvIndex_; }
	ID3D12Resource* GetPerViewResource() const { return perViewResource_.Get(); }
	PerView* GetPerViewData() const { return perViewData_; }
	ID3D12Resource* GetEmitterSphereResource() const { return emitterSphereResource_.Get(); }

	/// <summary>
	/// EmitterなどのCPUでの更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// GPU側でParticleの射出を行う
	/// </summary>
	void Emit();

private:
	/// <summary>
	/// GPUパーティクルのリソース生成と初期化(CSの実行)
	/// </summary>
	void InitializeGPUParticles();

	/// <summary>
	/// 初期化CS用のルートシグネチャとPSOを作成
	/// </summary>
	void CreateComputePipeline();

	/// <summary>
	/// 射出CS用のルートシグネチャとPSOを作成
	/// </summary>
	void CreateEmitComputePipeline();

private:
	/// <summary>
	/// ルートシグネチャを作成
	/// </summary>
	void CreateRootSignature();

	/// <summary>
	/// オブジェクト用のPSOを作成
	/// </summary>
	void CreatePSO();

};
