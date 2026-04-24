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

	// ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;

	// PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState_ = nullptr;

public:
	Dx12Core* GetDx12Core() const { return dx12Core_; }
	SrvManager* GetSrvManager() const { return srvManager_; }
	ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
	ID3D12PipelineState* GetPipelineState() const { return graphicsPipeLineState_.Get(); }

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
