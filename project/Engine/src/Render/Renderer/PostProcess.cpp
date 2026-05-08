#include "Renderer/PostProcess.h"
#include "Core/SrvManager.h"
#include "Debug/Logger.h"
#include "Math/MathUtil.h"

void PostProcess::Initialize(Dx12Core* dx12Core) {
  dx12Core_ = dx12Core;

  // デフォルトで単位行列にする
  projectionInverse_ = MakeIdentity4x4();

  // 定数バッファの作成
  D3D12_HEAP_PROPERTIES heapProps{};
  heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

  D3D12_RESOURCE_DESC resourceDesc{};
  resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resourceDesc.Width = 256; // 256バイトアラインメント
  resourceDesc.Height = 1;
  resourceDesc.DepthOrArraySize = 1;
  resourceDesc.MipLevels = 1;
  resourceDesc.SampleDesc.Count = 1;
  resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  dx12Core_->GetDevice()->CreateCommittedResource(
      &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&constBuffer_));

  CreateRootSignature();
  CreatePSO();
}

void PostProcess::CreateRootSignature() {
  HRESULT hr;

  // RootSignatureを作成
  D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
  descriptionRootSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  // DescriptorTable (テクスチャ用)
  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0; // t0
  descriptorRange[0].NumDescriptors = 1;
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // DescriptorTable (Depthテクスチャ用)
  D3D12_DESCRIPTOR_RANGE descriptorRangeDepth[1] = {};
  descriptorRangeDepth[0].BaseShaderRegister = 1; // t1
  descriptorRangeDepth[0].NumDescriptors = 1;
  descriptorRangeDepth[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  descriptorRangeDepth[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER rootParameter[3] = {};

  // b0: 定数バッファ (Grayscale切り替え用)
  rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[0].Descriptor.ShaderRegister = 0; // b0

  // t0: テクスチャ用デスクリプタテーブル
  rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[1].DescriptorTable.pDescriptorRanges = descriptorRange;
  rootParameter[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

  // t1: Depthテクスチャ用デスクリプタテーブル
  rootParameter[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameter[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[2].DescriptorTable.pDescriptorRanges = descriptorRangeDepth;
  rootParameter[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeDepth);

  // Sampler
  D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
  staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
  staticSamplers[0].ShaderRegister = 0; // s0
  staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  staticSamplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
  staticSamplers[1].ShaderRegister = 1; // s1
  staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  descriptionRootSignature.pParameters = rootParameter;
  descriptionRootSignature.NumParameters = _countof(rootParameter);
  descriptionRootSignature.pStaticSamplers = staticSamplers;
  descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

  // シリアライズしてバイナリにする
  Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
  Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
  hr = D3D12SerializeRootSignature(
      &descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1,
      signatureBlob.GetAddressOf(), errorBlob.GetAddressOf());

  if (FAILED(hr)) {
    if (errorBlob) {
      Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
    }
    assert(false);
  }

  // バイナリを元に生成
  rootSignature_ = nullptr;
  hr = dx12Core_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                    signatureBlob->GetBufferSize(),
                                    IID_PPV_ARGS(&rootSignature_));
  assert(SUCCEEDED(hr));
}

void PostProcess::CreatePSO() {
  HRESULT hr;

  // inputLayout (頂点データ不要のため空)
  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
  inputLayoutDesc.pInputElementDescs = nullptr;
  inputLayoutDesc.NumElements = 0;

  // BlendStateの設定
  D3D12_BLEND_DESC blendDesc{};
  blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

  // RasterizerStateの設定
  D3D12_RASTERIZER_DESC rasterizerDesc{};
  rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

  // shaderをcompileする
  IDxcBlob* vertexShaderBlob =
      dx12Core_->CompileShader(L"resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
  assert(vertexShaderBlob != nullptr);

  IDxcBlob* pixelShaderBlob =
      dx12Core_->CompileShader(L"resources/shaders/PostProcess.PS.hlsl", L"ps_6_0");
  assert(pixelShaderBlob != nullptr);

  // DepthStencilStateの設定
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  depthStencilDesc.DepthEnable = false; // 深度不要

  // PSOの生成
  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeLineStateDesc{};
  graphicsPipeLineStateDesc.pRootSignature = rootSignature_.Get();
  graphicsPipeLineStateDesc.InputLayout = inputLayoutDesc;
  graphicsPipeLineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
  graphicsPipeLineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
  graphicsPipeLineStateDesc.BlendState = blendDesc;
  graphicsPipeLineStateDesc.RasterizerState = rasterizerDesc;
  graphicsPipeLineStateDesc.DepthStencilState = depthStencilDesc;
  graphicsPipeLineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  graphicsPipeLineStateDesc.NumRenderTargets = 1;
  graphicsPipeLineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  graphicsPipeLineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  graphicsPipeLineStateDesc.SampleDesc.Count = 1;
  graphicsPipeLineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  graphicsPipelineState_ = nullptr;
  hr = dx12Core_->GetDevice()->CreateGraphicsPipelineState(
      &graphicsPipeLineStateDesc, IID_PPV_ARGS(&graphicsPipelineState_));
  assert(SUCCEEDED(hr));
}

void PostProcess::Draw(uint32_t renderSrvIndex, uint32_t depthSrvIndex, SrvManager* srvManager) {
  auto commandList = dx12Core_->GetCommandList();
  auto renderTextureResource = dx12Core_->GetRenderTextureResource();
  auto depthStencilResource = dx12Core_->GetDepthStencilResource();

  // 定数バッファへのデータ転送
  struct PostProcessData {
    int32_t postEffectType; // 0: None, 1: BoxFilter, 2: GaussianFilter
    int32_t useGrayscale;
    int32_t useVignette;
    int32_t boxFilterK;
    float monotoneColor[3];
    float vignetteScale;
    float vignetteExponent;
    int32_t gaussianFilterK;
    float gaussianSigma;
    float depthOutlineWeight;
    float depthOutlineAttenuation;
    float padding1;
    float padding2;
    float padding3;
    Matrix4x4 projectionInverse;
  };
  PostProcessData* data = nullptr;
  constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&data));
  data->postEffectType = postEffectType_;
  data->useGrayscale = useGrayscale_ ? 1 : 0;
  data->useVignette = useVignette_ ? 1 : 0;
  data->boxFilterK = boxFilterK_;
  data->monotoneColor[0] = monotoneColor_[0];
  data->monotoneColor[1] = monotoneColor_[1];
  data->monotoneColor[2] = monotoneColor_[2];
  data->vignetteScale = vignetteScale_;
  data->vignetteExponent = vignetteExponent_;
  data->gaussianFilterK = gaussianFilterK_;
  data->gaussianSigma = gaussianSigma_;
  data->depthOutlineWeight = depthOutlineWeight_;
  data->depthOutlineAttenuation = depthOutlineAttenuation_;
  data->padding1 = 0.0f;
  data->padding2 = 0.0f;
  data->padding3 = 0.0f;
  data->projectionInverse = projectionInverse_;
  constBuffer_->Unmap(0, nullptr);

  // Barrier: RENDER_TARGET -> PIXEL_SHADER_RESOURCE
  D3D12_RESOURCE_BARRIER barrier1{};
  barrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier1.Transition.pResource = renderTextureResource;
  barrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier1.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  barrier1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  D3D12_RESOURCE_BARRIER depthBarrier1{};
  depthBarrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  depthBarrier1.Transition.pResource = depthStencilResource;
  depthBarrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
  depthBarrier1.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  depthBarrier1.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  D3D12_RESOURCE_BARRIER barriers[2] = { barrier1, depthBarrier1 };
  commandList->ResourceBarrier(2, barriers);

  // パイプライン設定
  commandList->SetGraphicsRootSignature(rootSignature_.Get());
  commandList->SetPipelineState(graphicsPipelineState_.Get());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // CBVセット (rootParameter[0])
  commandList->SetGraphicsRootConstantBufferView(0, constBuffer_->GetGPUVirtualAddress());

  // SRVセット (rootParameter[1])
  srvManager->SetGraphicsRootDescriptorTable(1, renderSrvIndex);

  // SRVセット (rootParameter[2])
  srvManager->SetGraphicsRootDescriptorTable(2, depthSrvIndex);

  // 描画
  commandList->DrawInstanced(3, 1, 0, 0);

  // Barrier: PIXEL_SHADER_RESOURCE -> RENDER_TARGET
  D3D12_RESOURCE_BARRIER barrier2{};
  barrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier2.Transition.pResource = renderTextureResource;
  barrier2.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  barrier2.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
  barrier2.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  D3D12_RESOURCE_BARRIER depthBarrier2{};
  depthBarrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  depthBarrier2.Transition.pResource = depthStencilResource;
  depthBarrier2.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  depthBarrier2.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
  depthBarrier2.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  D3D12_RESOURCE_BARRIER barriersEnd[2] = { barrier2, depthBarrier2 };
  commandList->ResourceBarrier(2, barriersEnd);
}
