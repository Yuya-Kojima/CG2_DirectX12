#include "Render/Renderer/Bloom.h"
#include <cassert>

void Bloom::Initialize(Dx12Core *dx12Core, SrvManager *srvManager,
                       uint32_t width, uint32_t height) {
  width_ = width;
  height_ = height;

  // 中間テクスチャの生成（今回はフル解像度で作成）
  Vector4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
  luminanceTexture_ = std::make_unique<RenderTexture>();
  luminanceTexture_->Initialize(dx12Core, srvManager, width, height, clearColor,
                                DXGI_FORMAT_R16G16B16A16_FLOAT);

  blurXTexture_ = std::make_unique<RenderTexture>();
  blurXTexture_->Initialize(dx12Core, srvManager, width, height, clearColor,
                            DXGI_FORMAT_R16G16B16A16_FLOAT);

  blurYTexture_ = std::make_unique<RenderTexture>();
  blurYTexture_->Initialize(dx12Core, srvManager, width, height, clearColor,
                            DXGI_FORMAT_R16G16B16A16_FLOAT);

  CreateExtractPipeline(dx12Core);
  CreateBlurPipeline(dx12Core);
}

void Bloom::CreateExtractPipeline(Dx12Core *dx12Core) {
  HRESULT hr;
  extractConstBuffer_ = dx12Core->CreateBufferResource(sizeof(ExtractData));

  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0; // t0
  descriptorRange[0].NumDescriptors = 1;
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER rootParameter[2] = {};
  rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[0].Descriptor.ShaderRegister = 0; // b0

  rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[1].DescriptorTable.pDescriptorRanges = descriptorRange;
  rootParameter[1].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange);

  D3D12_STATIC_SAMPLER_DESC staticSampler = {};
  staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
  staticSampler.ShaderRegister = 0; // s0
  staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
  rootSignatureDesc.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  rootSignatureDesc.pParameters = rootParameter;
  rootSignatureDesc.NumParameters = _countof(rootParameter);
  rootSignatureDesc.pStaticSamplers = &staticSampler;
  rootSignatureDesc.NumStaticSamplers = 1;

  Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
  Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
  hr = D3D12SerializeRootSignature(&rootSignatureDesc,
                                   D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob,
                                   &errorBlob);
  assert(SUCCEEDED(hr));

  hr = dx12Core->GetDevice()->CreateRootSignature(
      0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
      IID_PPV_ARGS(&extractRootSignature_));
  assert(SUCCEEDED(hr));

  IDxcBlob *vsBlob = dx12Core->CompileShader(
      L"resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
  IDxcBlob *psBlob = dx12Core->CompileShader(
      L"resources/shaders/LuminanceExtract.PS.hlsl", L"ps_6_0");

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.pRootSignature = extractRootSignature_.Get();
  psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
  psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};

  D3D12_BLEND_DESC blendDesc{};
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;
  psoDesc.BlendState = blendDesc;

  psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
  psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  psoDesc.DepthStencilState.DepthEnable = false;

  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.SampleDesc.Count = 1;
  psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  hr = dx12Core->GetDevice()->CreateGraphicsPipelineState(
      &psoDesc, IID_PPV_ARGS(&extractPipelineState_));
  assert(SUCCEEDED(hr));
}

void Bloom::CreateBlurPipeline(Dx12Core *dx12Core) {
  HRESULT hr;
  blurConstBuffer_ = dx12Core->CreateBufferResource(sizeof(BlurData));

  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0; // t0
  descriptorRange[0].NumDescriptors = 1;
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER rootParameter[2] = {};
  rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[0].Descriptor.ShaderRegister = 0; // b0

  rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[1].DescriptorTable.pDescriptorRanges = descriptorRange;
  rootParameter[1].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange);

  D3D12_STATIC_SAMPLER_DESC staticSampler = {};
  staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
  staticSampler.ShaderRegister = 0; // s0
  staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
  rootSignatureDesc.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  rootSignatureDesc.pParameters = rootParameter;
  rootSignatureDesc.NumParameters = _countof(rootParameter);
  rootSignatureDesc.pStaticSamplers = &staticSampler;
  rootSignatureDesc.NumStaticSamplers = 1;

  Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
  Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
  hr = D3D12SerializeRootSignature(&rootSignatureDesc,
                                   D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob,
                                   &errorBlob);
  assert(SUCCEEDED(hr));

  hr = dx12Core->GetDevice()->CreateRootSignature(
      0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
      IID_PPV_ARGS(&blurRootSignature_));
  assert(SUCCEEDED(hr));

  IDxcBlob *vsBlob = dx12Core->CompileShader(
      L"resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
  IDxcBlob *psBlob = dx12Core->CompileShader(
      L"resources/shaders/GaussianBlur.PS.hlsl", L"ps_6_0");

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.pRootSignature = blurRootSignature_.Get();
  psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
  psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};

  D3D12_BLEND_DESC blendDesc{};
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;
  psoDesc.BlendState = blendDesc;

  psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
  psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  psoDesc.DepthStencilState.DepthEnable = false;

  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.SampleDesc.Count = 1;
  psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  hr = dx12Core->GetDevice()->CreateGraphicsPipelineState(
      &psoDesc, IID_PPV_ARGS(&blurPipelineState_));
  assert(SUCCEEDED(hr));
}

uint32_t Bloom::Draw(Dx12Core *dx12Core, SrvManager *srvManager,
                     uint32_t srcSrvIndex) {
  auto commandList = dx12Core->GetCommandList();

  // ----- 1. 高輝度抽出 (srcSrvIndex -> luminanceTexture_) -----
  luminanceTexture_->TransitionToRenderTarget(dx12Core);
  D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = luminanceTexture_->GetRtvHandle();
  commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
  luminanceTexture_->Clear(dx12Core);

  ExtractData *eData = nullptr;
  extractConstBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&eData));
  eData->threshold = threshold_;
  extractConstBuffer_->Unmap(0, nullptr);

  commandList->SetGraphicsRootSignature(extractRootSignature_.Get());
  commandList->SetPipelineState(extractPipelineState_.Get());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->SetGraphicsRootConstantBufferView(
      0, extractConstBuffer_->GetGPUVirtualAddress());
  srvManager->SetGraphicsRootDescriptorTable(1, srcSrvIndex);

  commandList->DrawInstanced(3, 1, 0, 0);
  luminanceTexture_->TransitionToShaderResource(dx12Core);

  // ----- 2. 横ボカシ (luminanceTexture_ -> blurXTexture_) -----
  blurXTexture_->TransitionToRenderTarget(dx12Core);
  rtvHandle = blurXTexture_->GetRtvHandle();
  commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
  blurXTexture_->Clear(dx12Core);

  BlurData *bData = nullptr;
  blurConstBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&bData));
  bData->isHorizontal = 1;
  bData->texWidth = (float)width_;
  bData->texHeight = (float)height_;
  bData->sigma = sigma_;
  blurConstBuffer_->Unmap(0, nullptr);

  commandList->SetGraphicsRootSignature(blurRootSignature_.Get());
  commandList->SetPipelineState(blurPipelineState_.Get());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->SetGraphicsRootConstantBufferView(
      0, blurConstBuffer_->GetGPUVirtualAddress());
  srvManager->SetGraphicsRootDescriptorTable(1,
                                             luminanceTexture_->GetSrvIndex());

  commandList->DrawInstanced(3, 1, 0, 0);
  blurXTexture_->TransitionToShaderResource(dx12Core);

  // ----- 3. 縦ボカシ (blurXTexture_ -> blurYTexture_) -----
  blurYTexture_->TransitionToRenderTarget(dx12Core);
  rtvHandle = blurYTexture_->GetRtvHandle();
  commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
  blurYTexture_->Clear(dx12Core);

  blurConstBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&bData));
  bData->isHorizontal = 0;
  bData->texWidth = (float)width_;
  bData->texHeight = (float)height_;
  bData->sigma = sigma_;
  blurConstBuffer_->Unmap(0, nullptr);

  srvManager->SetGraphicsRootDescriptorTable(1, blurXTexture_->GetSrvIndex());
  commandList->DrawInstanced(3, 1, 0, 0);
  blurYTexture_->TransitionToShaderResource(dx12Core);

  // 最終的なボカシ結果のSRVを返す
  return blurYTexture_->GetSrvIndex();
}
