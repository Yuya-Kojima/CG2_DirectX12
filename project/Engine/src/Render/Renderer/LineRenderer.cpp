#include "Renderer/LineRenderer.h"
#include "Debug/Logger.h"
#include <cassert>

void LineRenderer::Initialize(Dx12Core* dx12Core) {
    dx12Core_ = dx12Core;

    CreateRootSignature();
    CreatePSO();
    CreateBuffer();

    vertices_.reserve(kMaxVertexCount);
}

void LineRenderer::DrawLine(const Vector3& start, const Vector3& end, const Vector4& color) {
    if (vertices_.size() + 2 > kMaxVertexCount) {
        return; // 上限超過時は追加しない
    }

    vertices_.push_back({ Vector4(start.x, start.y, start.z, 1.0f), color });
    vertices_.push_back({ Vector4(end.x, end.y, end.z, 1.0f), color });
}

void LineRenderer::Render(const Matrix4x4& viewProjectionMatrix) {
    if (vertices_.empty()) {
        return;
    }

    auto commandList = dx12Core_->GetCommandList();

    // 定数バッファにWVPを書き込む
    // 線を描画する際は、すでにワールド座標になっている頂点を渡すため、
    // ここでの WVP 行列は「ViewProjection 行列」そのものになる
    constMap_->WVP = viewProjectionMatrix;

    // 頂点バッファに書き込む
    VertexDataLine* vertexMap = nullptr;
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap));
    std::copy(vertices_.begin(), vertices_.end(), vertexMap);
    vertexBuffer_->Unmap(0, nullptr);

    // 描画設定
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetPipelineState(graphicsPipeLineState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    
    // ルートパラメータの設定
    commandList->SetGraphicsRootConstantBufferView(0, constBuffer_->GetGPUVirtualAddress());

    // 描画
    commandList->DrawInstanced(static_cast<UINT>(vertices_.size()), 1, 0, 0);

    // リストをクリア
    vertices_.clear();
}

void LineRenderer::CreateRootSignature() {
    auto device = dx12Core_->GetDevice();

    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Descriptor.ShaderRegister = 0; // b0

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = _countof(rootParameters);

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}

void LineRenderer::CreatePSO() {
    auto device = dx12Core_->GetDevice();

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[1].SemanticName = "COLOR";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // カリングなし
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // シェーダーコンパイル
    auto vsBlob = dx12Core_->CompileShader(L"resources/shaders/Line.VS.hlsl", L"vs_6_0");
    auto psBlob = dx12Core_->CompileShader(L"resources/shaders/Line.PS.hlsl", L"ps_6_0");

    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = false; // デバッグ用なので常に手前に描画する
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込みもしない
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
    psoDesc.BlendState = blendDesc;
    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipeLineState_));
    assert(SUCCEEDED(hr));
}

void LineRenderer::CreateBuffer() {
    UINT vertexBufferSize = sizeof(VertexDataLine) * kMaxVertexCount;
    vertexBuffer_ = dx12Core_->CreateBufferResource(vertexBufferSize);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = vertexBufferSize;
    vertexBufferView_.StrideInBytes = sizeof(VertexDataLine);

    UINT constBufferSize = (sizeof(ConstBufferData) + 255) & ~255;
    constBuffer_ = dx12Core_->CreateBufferResource(constBufferSize);
    constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));
}
