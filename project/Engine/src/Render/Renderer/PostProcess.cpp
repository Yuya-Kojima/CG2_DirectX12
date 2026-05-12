#include "Renderer/PostProcess.h"
#include "Core/SrvManager.h"
#include "Debug/Logger.h"
#include "Math/MathUtil.h"
#ifdef USE_IMGUI
#include <imgui.h>
#endif

void PostProcess::Initialize(Dx12Core *dx12Core) {
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
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constBuffer_));

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
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // DescriptorTable (Depthテクスチャ用)
  D3D12_DESCRIPTOR_RANGE descriptorRangeDepth[1] = {};
  descriptorRangeDepth[0].BaseShaderRegister = 1; // t1
  descriptorRangeDepth[0].NumDescriptors = 1;
  descriptorRangeDepth[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  descriptorRangeDepth[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // DescriptorTable (Maskテクスチャ用)
  D3D12_DESCRIPTOR_RANGE descriptorRangeMask[1] = {};
  descriptorRangeMask[0].BaseShaderRegister = 2; // t2
  descriptorRangeMask[0].NumDescriptors = 1;
  descriptorRangeMask[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  descriptorRangeMask[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  // DescriptorTable (Bloomテクスチャ用)
  D3D12_DESCRIPTOR_RANGE descriptorRangeBloom[1] = {};
  descriptorRangeBloom[0].BaseShaderRegister = 3; // t3
  descriptorRangeBloom[0].NumDescriptors = 1;
  descriptorRangeBloom[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  descriptorRangeBloom[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  D3D12_ROOT_PARAMETER rootParameter[5] = {};

  // b0: 定数バッファ (Grayscale切り替え用)
  rootParameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[0].Descriptor.ShaderRegister = 0; // b0

  // t0: テクスチャ用デスクリプタテーブル
  rootParameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[1].DescriptorTable.pDescriptorRanges = descriptorRange;
  rootParameter[1].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange);

  // t1: Depthテクスチャ用デスクリプタテーブル
  rootParameter[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameter[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[2].DescriptorTable.pDescriptorRanges = descriptorRangeDepth;
  rootParameter[2].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRangeDepth);

  // t2: Maskテクスチャ用デスクリプタテーブル
  rootParameter[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameter[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[3].DescriptorTable.pDescriptorRanges = descriptorRangeMask;
  rootParameter[3].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRangeMask);

  // t3: Bloomテクスチャ用デスクリプタテーブル
  rootParameter[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameter[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameter[4].DescriptorTable.pDescriptorRanges = descriptorRangeBloom;
  rootParameter[4].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRangeBloom);

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
      Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
    }
    assert(false);
  }

  // バイナリを元に生成
  rootSignature_ = nullptr;
  hr = dx12Core_->GetDevice()->CreateRootSignature(
      0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
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
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  // RasterizerStateの設定
  D3D12_RASTERIZER_DESC rasterizerDesc{};
  rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

  // shaderをcompileする
  IDxcBlob *vertexShaderBlob = dx12Core_->CompileShader(
      L"resources/shaders/Fullscreen.VS.hlsl", L"vs_6_0");
  assert(vertexShaderBlob != nullptr);

  IDxcBlob *pixelShaderBlob = dx12Core_->CompileShader(
      L"resources/shaders/PostProcess.PS.hlsl", L"ps_6_0");
  assert(pixelShaderBlob != nullptr);

  // DepthStencilStateの設定
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  depthStencilDesc.DepthEnable = false; // 深度不要

  // PSOの生成
  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeLineStateDesc{};
  graphicsPipeLineStateDesc.pRootSignature = rootSignature_.Get();
  graphicsPipeLineStateDesc.InputLayout = inputLayoutDesc;
  graphicsPipeLineStateDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                                  vertexShaderBlob->GetBufferSize()};
  graphicsPipeLineStateDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                                  pixelShaderBlob->GetBufferSize()};
  graphicsPipeLineStateDesc.BlendState = blendDesc;
  graphicsPipeLineStateDesc.RasterizerState = rasterizerDesc;
  graphicsPipeLineStateDesc.DepthStencilState = depthStencilDesc;
  graphicsPipeLineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  graphicsPipeLineStateDesc.NumRenderTargets = 1;
  graphicsPipeLineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  graphicsPipeLineStateDesc.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  graphicsPipeLineStateDesc.SampleDesc.Count = 1;
  graphicsPipeLineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  graphicsPipelineState_ = nullptr;
  hr = dx12Core_->GetDevice()->CreateGraphicsPipelineState(
      &graphicsPipeLineStateDesc, IID_PPV_ARGS(&graphicsPipelineState_));
  assert(SUCCEEDED(hr));
}

void PostProcess::Draw(uint32_t renderSrvIndex, uint32_t depthSrvIndex,
                       SrvManager *srvManager) {

  auto commandList = dx12Core_->GetCommandList();
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
    float radialBlurCenter[2];
    float radialBlurWidth;
    int32_t radialBlurSamples;
    float radialBlurInnerRadius;
    float radialBlurOuterRadius;
    float radialBlurAberration;
    float padding4;
    float dissolveThreshold;
    float dissolveEdgeRange;
    float padding5[2];
    float dissolveEdgeColor[3];
    float time;
    float hsvFilterHue;
    float hsvFilterSaturation;
    float hsvFilterValue;
    float padding6;
    float bloomIntensity;
    int32_t useBloom;
    int32_t toneMappingType;
    float exposure;
  };
  PostProcessData *data = nullptr;
  constBuffer_->Map(0, nullptr, reinterpret_cast<void **>(&data));
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
  data->radialBlurCenter[0] = radialBlurCenter_[0];
  data->radialBlurCenter[1] = radialBlurCenter_[1];
  data->radialBlurWidth = radialBlurWidth_;
  data->radialBlurSamples = radialBlurSamples_;
  data->radialBlurInnerRadius = radialBlurInnerRadius_;
  data->radialBlurOuterRadius = radialBlurOuterRadius_;
  data->radialBlurAberration = radialBlurAberration_;
  data->padding4 = 0.0f;
  data->dissolveThreshold = dissolveThreshold_;
  data->dissolveEdgeRange = dissolveEdgeRange_;
  data->padding5[0] = 0.0f;
  data->padding5[1] = 0.0f;
  data->dissolveEdgeColor[0] = dissolveEdgeColor_[0];
  data->dissolveEdgeColor[1] = dissolveEdgeColor_[1];
  data->dissolveEdgeColor[2] = dissolveEdgeColor_[2];
  data->time = time_;
  data->hsvFilterHue = hsvFilterHue_;
  data->hsvFilterSaturation = hsvFilterSaturation_;
  data->hsvFilterValue = hsvFilterValue_;
  data->padding6 = 0.0f;
  data->bloomIntensity = bloomIntensity_;
  data->useBloom = useBloom_ ? 1 : 0;
  data->toneMappingType = toneMappingType_;
  data->exposure = exposure_;
  constBuffer_->Unmap(0, nullptr);

  // Barrier: DEPTH_WRITE -> PIXEL_SHADER_RESOURCE
  D3D12_RESOURCE_BARRIER depthBarrier1{};
  depthBarrier1.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  depthBarrier1.Transition.pResource = depthStencilResource;
  depthBarrier1.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
  depthBarrier1.Transition.StateAfter =
      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  depthBarrier1.Transition.Subresource =
      D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  commandList->ResourceBarrier(1, &depthBarrier1);

  // パイプライン設定
  commandList->SetGraphicsRootSignature(rootSignature_.Get());
  commandList->SetPipelineState(graphicsPipelineState_.Get());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // CBVセット (rootParameter[0])
  commandList->SetGraphicsRootConstantBufferView(
      0, constBuffer_->GetGPUVirtualAddress());

  // DescriptorTableをセット
  srvManager->SetGraphicsRootDescriptorTable(1, renderSrvIndex);
  srvManager->SetGraphicsRootDescriptorTable(2, depthSrvIndex);
  srvManager->SetGraphicsRootDescriptorTable(3, maskSrvIndex_);
  srvManager->SetGraphicsRootDescriptorTable(4, bloomSrvIndex_);

  // 描画コマンド
  commandList->DrawInstanced(3, 1, 0, 0);

  // Barrier: PIXEL_SHADER_RESOURCE -> DEPTH_WRITE
  D3D12_RESOURCE_BARRIER depthBarrier2{};
  depthBarrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  depthBarrier2.Transition.pResource = depthStencilResource;
  depthBarrier2.Transition.StateBefore =
      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  depthBarrier2.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
  depthBarrier2.Transition.Subresource =
      D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  commandList->ResourceBarrier(1, &depthBarrier2);
}

void PostProcess::DrawDebugUI(const char *windowName) {
#ifdef USE_IMGUI
  ImGui::Begin(windowName);

  if (ImGui::CollapsingHeader("PostEffect", ImGuiTreeNodeFlags_DefaultOpen)) {

    // --- Base Effect ---
    ImGui::Separator();
    ImGui::Text("Base Effect");
    const char *effectTypes[] = {
        "None",          "BoxFilter",   "GaussianFilter", "Luminance Outline",
        "Depth Outline", "Radial Blur", "Dissolve",       "Random Noise",
        "HSV Filter"};
    ImGui::Combo("Effect Type", &postEffectType_, effectTypes,
                 IM_ARRAYSIZE(effectTypes));

    if (postEffectType_ == 1) { // BoxFilter
      ImGui::DragInt("BoxFilter K (Radius)", &boxFilterK_, 0.1f, 1, 10);
    } else if (postEffectType_ == 2) { // GaussianFilter
      ImGui::DragInt("Gaussian K", &gaussianFilterK_, 0.1f, 1, 10);
      ImGui::DragFloat("Gaussian Sigma", &gaussianSigma_, 0.1f, 0.1f, 20.0f);
    } else if (postEffectType_ == 4) { // Depth Outline
      ImGui::DragFloat("Outline Weight", &depthOutlineWeight_, 0.1f, 1.0f,
                       100.0f);
      ImGui::DragFloat("Distance Attenuation", &depthOutlineAttenuation_,
                       0.001f, 0.0f, 1.0f, "%.3f");
    } else if (postEffectType_ == 5) { // Radial Blur
      ImGui::DragFloat2("Center X/Y", radialBlurCenter_, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Blur Width", &radialBlurWidth_, 0.001f, 0.0f, 0.1f,
                       "%.3f");
      ImGui::DragInt("Num Samples", &radialBlurSamples_, 1.0f, 1, 50);
      ImGui::DragFloat("Inner Radius (Sharp)", &radialBlurInnerRadius_, 0.01f,
                       0.0f, 1.0f);
      ImGui::DragFloat("Outer Radius", &radialBlurOuterRadius_, 0.01f, 0.0f,
                       1.0f);
      ImGui::DragFloat("Aberration (RGB Shift)", &radialBlurAberration_, 0.01f,
                       -0.5f, 0.5f);
      if (radialBlurInnerRadius_ > radialBlurOuterRadius_) {
        radialBlurInnerRadius_ = radialBlurOuterRadius_;
      }
    } else if (postEffectType_ == 6) { // Dissolve
      ImGui::DragFloat("Threshold", &dissolveThreshold_, 0.01f, 0.0f, 1.0f);
      ImGui::DragFloat("Edge Range", &dissolveEdgeRange_, 0.001f, 0.0f, 0.2f,
                       "%.3f");
      ImGui::ColorEdit3("Edge Color", dissolveEdgeColor_);
    } else if (postEffectType_ == 7) { // Random Noise
      ImGui::Text("Generating animated GPU random noise.");
    } else if (postEffectType_ == 8) { // HSV Filter
      ImGui::DragFloat("Hue", &hsvFilterHue_, 0.01f, -1.0f, 1.0f);
      ImGui::DragFloat("Saturation", &hsvFilterSaturation_, 0.01f, -1.0f, 1.0f);
      ImGui::DragFloat("Value", &hsvFilterValue_, 0.01f, -1.0f, 1.0f);
    }

    // --- Monotone ---
    ImGui::Separator();
    ImGui::Text("Color Grading");

    // Grayscale
    int monotoneType = useGrayscale_ ? 1 : 0;
    if (monotoneColor_[0] == 1.0f && monotoneColor_[1] == 1.0f &&
        monotoneColor_[2] == 1.0f)
      monotoneType = 1;
    else if (monotoneColor_[0] == 1.0f && monotoneColor_[1] == 74.0f / 107.0f &&
             monotoneColor_[2] == 43.0f / 107.0f)
      monotoneType = 2;
    else
      monotoneType = 3;
    if (!useGrayscale_)
      monotoneType = 0;

    const char *monotoneItems[] = {"Normal (Off)", "Grayscale", "Sepia",
                                   "Custom"};
    if (ImGui::Combo("Monotone", &monotoneType, monotoneItems,
                     IM_ARRAYSIZE(monotoneItems))) {
      useGrayscale_ = (monotoneType != 0);
      if (monotoneType == 1) {
        monotoneColor_[0] = 1.0f;
        monotoneColor_[1] = 1.0f;
        monotoneColor_[2] = 1.0f;
      } else if (monotoneType == 2) {
        monotoneColor_[0] = 1.0f;
        monotoneColor_[1] = 74.0f / 107.0f;
        monotoneColor_[2] = 43.0f / 107.0f;
      }
    }
    if (monotoneType == 3) {
      ImGui::ColorEdit3("Custom Color", monotoneColor_);
    }

    // --- Vignette ---
    ImGui::Separator();
    ImGui::Text("Vignette");
    int vignetteType = useVignette_ ? 4 : 0;
    if (useVignette_) {
      if (vignetteScale_ == 16.0f && vignetteExponent_ == 0.8f)
        vignetteType = 1;
      else if (vignetteScale_ == 16.0f && vignetteExponent_ == 1.5f)
        vignetteType = 2;
      else if (vignetteScale_ == 5.0f && vignetteExponent_ == 1.2f)
        vignetteType = 3;
    }

    const char *vignetteItems[] = {"Normal (Off)", "Mild", "Strong", "Pinhole",
                                   "Custom"};
    if (ImGui::Combo("Vignette", &vignetteType, vignetteItems,
                     IM_ARRAYSIZE(vignetteItems))) {
      useVignette_ = (vignetteType != 0);
      if (vignetteType == 1) {
        vignetteScale_ = 16.0f;
        vignetteExponent_ = 0.8f;
      } else if (vignetteType == 2) {
        vignetteScale_ = 16.0f;
        vignetteExponent_ = 1.5f;
      } else if (vignetteType == 3) {
        vignetteScale_ = 5.0f;
        vignetteExponent_ = 1.2f;
      }
    }
    if (vignetteType == 4) {
      ImGui::DragFloat("Scale", &vignetteScale_, 0.1f, 1.0f, 100.0f);
      ImGui::DragFloat("Exponent", &vignetteExponent_, 0.05f, 0.1f, 5.0f);
    }

    // --- Bloom ---
    ImGui::Separator();
    ImGui::Text("Bloom (Multi-pass)");
    bool bloomEnabled = useBloom_;
    if (ImGui::Checkbox("Enable Bloom", &bloomEnabled)) {
      useBloom_ = bloomEnabled;
    }
    if (useBloom_) {
      ImGui::DragFloat("Bloom Intensity", &bloomIntensity_, 0.05f, 0.0f, 10.0f);
      ImGui::DragFloat("Extract Threshold", &bloomThreshold_, 0.01f, 0.0f,
                       1.0f);
      ImGui::DragFloat("Blur Sigma (Radius)", &bloomSigma_, 0.1f, 0.1f, 30.0f);
    }

    // --- Tone Mapping ---
    ImGui::Separator();
    ImGui::Text("Tone Mapping & HDR");
    const char *toneMappingItems[] = {"None (Linear/Clamp)", "ACES Filmic",
                                      "Reinhard"};
    ImGui::Combo("Tone Mapping", &toneMappingType_, toneMappingItems,
                 IM_ARRAYSIZE(toneMappingItems));
    ImGui::DragFloat("Exposure", &exposure_, 0.05f, 0.1f, 10.0f);
  }

  ImGui::End();
#endif
}
