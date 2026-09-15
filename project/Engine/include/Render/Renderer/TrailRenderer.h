#pragma once
#include "Core/Dx12Core.h"
#include "Math/Matrix4x4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <vector>
#include <memory>
#include <wrl.h>

class TrailRenderer {
public:
    struct VertexDataTrail {
        Vector4 position;
        Vector2 texcoord;
        Vector4 color;
    };

    struct ConstBufferData {
        Matrix4x4 ViewProjection;
    };

    struct TrailNode {
        Vector3 position;
        float width;
        Vector4 color;
    };

    static TrailRenderer* GetInstance() {
        static TrailRenderer instance;
        return &instance;
    }

    void Initialize(Dx12Core* dx12Core);
    void Finalize();

    /// <summary>
    /// 一連のノード（点列）からリボントレイルを登録する
    /// </summary>
    /// <param name=nodes>弾が通った座標・太さ・色の履歴リスト</param>
    /// <param name=cameraPos>現在のカメラ座標（ビルボード計算用）</param>
    void AddTrail(const std::vector<TrailNode>& nodes, const Vector3& cameraPos);

    /// <summary>
    /// 登録されたトレイルを一括レンダリングする
    /// </summary>
    /// <param name=viewProjectionMatrix>カメラのViewProjection行列</param>
    /// <param name=srvGpuHandle>テクスチャのGPUハンドル（0の場合はデフォルトの白）</param>
    void Render(const Matrix4x4& viewProjectionMatrix, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle);

private:
    TrailRenderer() = default;
    ~TrailRenderer() = default;
    TrailRenderer(const TrailRenderer&) = delete;
    TrailRenderer& operator=(const TrailRenderer&) = delete;

    void CreateRootSignature();
    void CreatePSO();
    void CreateBuffer();

    static constexpr size_t kMaxVertexCount = 16384; // 最大頂点数

    Dx12Core* dx12Core_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState_ = nullptr;

    // 頂点バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_ = nullptr;
    ConstBufferData* constMap_ = nullptr;

    // 毎フレーム動的に構築される頂点リスト
    std::vector<VertexDataTrail> vertices_;
};
