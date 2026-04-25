#pragma once
#include "Core/Dx12Core.h"
#include "Math/MathUtil.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include <vector>

class LineRenderer {
public:
    struct VertexDataLine {
        Vector4 position;
        Vector4 color;
    };

    struct ConstBufferData {
        Matrix4x4 WVP;
    };

    static LineRenderer* GetInstance() {
        static LineRenderer instance;
        return &instance;
    }

    void Initialize(Dx12Core* dx12Core);
    
    /// <summary>
    /// 線を引く（リストに登録する）
    /// </summary>
    void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color = {1.0f, 1.0f, 1.0f, 1.0f});
    
    /// <summary>
    /// 登録された線を描画し、リストをクリアする
    /// </summary>
    void Render(const Matrix4x4& viewProjectionMatrix);

private:
    LineRenderer() = default;
    ~LineRenderer() = default;
    LineRenderer(const LineRenderer&) = delete;
    LineRenderer& operator=(const LineRenderer&) = delete;

    void CreateRootSignature();
    void CreatePSO();
    void CreateBuffer();

    Dx12Core* dx12Core_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState_ = nullptr;

    // 頂点バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    
    // 定数バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_ = nullptr;
    ConstBufferData* constMap_ = nullptr;

    // 描画する頂点のリスト
    static const uint32_t kMaxLineCount = 100000; // 最大10万本
    static const uint32_t kMaxVertexCount = kMaxLineCount * 2;
    std::vector<VertexDataLine> vertices_;
};
