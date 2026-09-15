#include "Renderer/TrailRenderer.h"
#include "Debug/Logger.h"
#include "Math/MathUtil.h"
#include <cassert>
#include <cmath>

void TrailRenderer::Initialize(Dx12Core* dx12Core) {
    dx12Core_ = dx12Core;

    CreateRootSignature();
    CreatePSO();
    CreateBuffer();

    vertices_.reserve(kMaxVertexCount);
}

void TrailRenderer::Finalize() {
    if (constBuffer_) {
        constBuffer_->Unmap(0, nullptr);
    }
    constBuffer_.Reset();
    vertexBuffer_.Reset();
    graphicsPipeLineState_.Reset();
    rootSignature_.Reset();
}

void TrailRenderer::AddTrail(const std::vector<TrailNode>& nodes, const Vector3& cameraPos) {
    if (nodes.size() < 2) {
        return; // 最低2点ないと帯が作れない
    }

    size_t nodeCount = nodes.size();
    // 左右頂点のリストを一時生成
    std::vector<Vector3> leftPos(nodeCount);
    std::vector<Vector3> rightPos(nodeCount);

    Vector3 prevRight = {1.0f, 0.0f, 0.0f};

    for (size_t i = 0; i < nodeCount; ++i) {
        // 進行方向ベクトルの計算
        Vector3 dir = {0.0f, 0.0f, 1.0f};
        if (i == 0) {
            dir = nodes[1].position - nodes[0].position;
        } else if (i == nodeCount - 1) {
            dir = nodes[i].position - nodes[i - 1].position;
        } else {
            dir = nodes[i + 1].position - nodes[i - 1].position;
        }

        float dirLen = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (dirLen > 0.0001f) {
            dir = {dir.x / dirLen, dir.y / dirLen, dir.z / dirLen};
        } else {
            dir = {0.0f, 0.0f, 1.0f};
        }

        // カメラへの視線ベクトル
        Vector3 toCamera = cameraPos - nodes[i].position;
        float camLen = std::sqrt(toCamera.x * toCamera.x + toCamera.y * toCamera.y + toCamera.z * toCamera.z);
        if (camLen > 0.0001f) {
            toCamera = {toCamera.x / camLen, toCamera.y / camLen, toCamera.z / camLen};
        } else {
            toCamera = {0.0f, 1.0f, 0.0f};
        }

        // 進行方向とカメラ視線の外積で、カメラに正対する「帯の横方向ベクトル（Right）」を計算
        Vector3 side = Cross(dir, toCamera);
        float sideLen = std::sqrt(side.x * side.x + side.y * side.y + side.z * side.z);
        if (sideLen > 0.0001f) {
            side = {side.x / sideLen, side.y / sideLen, side.z / sideLen};
            prevRight = side;
        } else {
            // 平行に近い場合は前回のRightを使用
            side = prevRight;
        }

        float halfW = nodes[i].width * 0.5f;
        leftPos[i] = nodes[i].position - side * halfW;
        rightPos[i] = nodes[i].position + side * halfW;
    }

    // 三角形リスト（各セグメントあたり2ポリゴン＝6頂点）を追加
    for (size_t i = 0; i < nodeCount - 1; ++i) {
        if (vertices_.size() + 6 > kMaxVertexCount) {
            break; // バッファ上限
        }

        float v0 = static_cast<float>(i) / static_cast<float>(nodeCount - 1);
        float v1 = static_cast<float>(i + 1) / static_cast<float>(nodeCount - 1);

        // 頂点情報
        VertexDataTrail tl = { Vector4(leftPos[i].x, leftPos[i].y, leftPos[i].z, 1.0f), Vector2(0.0f, v0), nodes[i].color };
        VertexDataTrail tr = { Vector4(rightPos[i].x, rightPos[i].y, rightPos[i].z, 1.0f), Vector2(1.0f, v0), nodes[i].color };
        VertexDataTrail bl = { Vector4(leftPos[i + 1].x, leftPos[i + 1].y, leftPos[i + 1].z, 1.0f), Vector2(0.0f, v1), nodes[i + 1].color };
        VertexDataTrail br = { Vector4(rightPos[i + 1].x, rightPos[i + 1].y, rightPos[i + 1].z, 1.0f), Vector2(1.0f, v1), nodes[i + 1].color };

        // 三角形1: TL -> TR -> BL
        vertices_.push_back(tl);
        vertices_.push_back(tr);
        vertices_.push_back(bl);

        // 三角形2: TR -> BR -> BL
        vertices_.push_back(tr);
        vertices_.push_back(br);
        vertices_.push_back(bl);
    }
}

void TrailRenderer::Render(const Matrix4x4& viewProjectionMatrix, D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle) {
    if (vertices_.empty()) {
        return;
    }

    auto commandList = dx12Core_->GetCommandList();

    // 定数バッファにViewProjectionを書き込む
    constMap_->ViewProjection = viewProjectionMatrix;

    // 頂点バッファに書き込む
    VertexDataTrail* vertexMap = nullptr;
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap));
    std::copy(vertices_.begin(), vertices_.end(), vertexMap);
    vertexBuffer_->Unmap(0, nullptr);

    // 描画コマンド発行
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetPipelineState(graphicsPipeLineState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // ルートパラメータ 0: 定数バッファ (b0)
    commandList->SetGraphicsRootConstantBufferView(0, constBuffer_->GetGPUVirtualAddress());

    // ルートパラメータ 1: テクスチャ (t0)
    commandList->SetGraphicsRootDescriptorTable(1, srvGpuHandle);

    // ドローコール
    commandList->DrawInstanced(static_cast<UINT>(vertices_.size()), 1, 0, 0);

    // 描画完了後にクリア
    vertices_.clear();
}

void TrailRenderer::CreateRootSignature() {
    auto device = dx12Core_->GetDevice();

    // ルートパラメータの設定
    D3D12_ROOT_PARAMETER rootParameters[2] = {};

    // 0: ViewProjection 行列 (b0: VertexShader)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    rootParameters[0].Descriptor.ShaderRegister = 0; // b0

    // 1: テクスチャ (t0: PixelShader)
    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0; // t0
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[1].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

    // 静的サンプラーの設定
    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].ShaderRegister = 0; // s0
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = _countof(rootParameters);
    rootSignatureDesc.pStaticSamplers = staticSamplers;
    rootSignatureDesc.NumStaticSamplers = _countof(staticSamplers);

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

void TrailRenderer::CreatePSO() {
    auto device = dx12Core_->GetDevice();

    // 頂点レイアウト: POSITION, TEXCOORD, COLOR
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    inputElementDescs[2].SemanticName = "COLOR";
    inputElementDescs[2].SemanticIndex = 0;
    inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    // 加算ブレンド設定（Additive Blend）: 鮮烈に光り輝く
    D3D12_BLEND_DESC blendDesc{};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // 加算合成
    blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    D3D12_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // 両面描画（リボンが反転しても消えない）
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    // シェーダーのコンパイル
    auto vsBlob = dx12Core_->CompileShader(L"resources/shaders/Trail.VS.hlsl", L"vs_6_0");
    auto psBlob = dx12Core_->CompileShader(L"resources/shaders/Trail.PS.hlsl", L"ps_6_0");

    // 深度テストは行うが、深度バッファへの書き込みはしない（半透明エフェクトの王道）
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

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
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&graphicsPipeLineState_));
    assert(SUCCEEDED(hr));
}

void TrailRenderer::CreateBuffer() {
    UINT vertexBufferSize = sizeof(VertexDataTrail) * static_cast<UINT>(kMaxVertexCount);
    vertexBuffer_ = dx12Core_->CreateBufferResource(vertexBufferSize);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = vertexBufferSize;
    vertexBufferView_.StrideInBytes = sizeof(VertexDataTrail);

    UINT constBufferSize = (sizeof(ConstBufferData) + 255) & ~255;
    constBuffer_ = dx12Core_->CreateBufferResource(constBufferSize);
    constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));
}
