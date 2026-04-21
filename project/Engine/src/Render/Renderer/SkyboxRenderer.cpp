#include "Renderer/SkyboxRenderer.h"
#include "Debug/Logger.h"

void SkyboxRenderer::Initialize(Dx12Core *dx12Core) {
  dx12Core_ = dx12Core;
  CreatePSO();
}

void SkyboxRenderer::CreateRootSignature() {

  HRESULT hr;
  ID3D12Device *device = dx12Core_->GetDevice();

  D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
  descriptionRootSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  //========================
  // RootParameter
  // 0 : Material (PS b0 or b1 は後でシェーダーと合わせる)
  // 1 : TransformationMatrix (VS b0)
  // 2 : TextureCube SRV (t0)
  //========================
  D3D12_ROOT_PARAMETER rootParameters[3] = {};

  // Material
  rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameters[0].Descriptor.ShaderRegister = 0;

  // Transformation
  rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
  rootParameters[1].Descriptor.ShaderRegister = 0;

  // TextureCube SRV
  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0;
  descriptorRange[0].NumDescriptors = 1;
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
  rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
  rootParameters[2].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange);

  //========================
  // Sampler
  //========================
  D3D12_STATIC_SAMPLER_DESC staticSampler{};
  staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
  staticSampler.ShaderRegister = 0;
  staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  descriptionRootSignature.pParameters = rootParameters;
  descriptionRootSignature.NumParameters = _countof(rootParameters);
  descriptionRootSignature.pStaticSamplers = &staticSampler;
  descriptionRootSignature.NumStaticSamplers = 1;

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

  hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                   signatureBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&rootSignature_));
  assert(SUCCEEDED(hr));
}

void SkyboxRenderer::CreatePSO() {

  HRESULT hr;
  ID3D12Device *device = dx12Core_->GetDevice();

  CreateRootSignature();

  //========================
  // InputLayout
  // いまの VertexData に合わせる
  //========================
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};

  inputElementDescs[0].SemanticName = "POSITION";
  inputElementDescs[0].SemanticIndex = 0;
  inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
  inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

  inputElementDescs[1].SemanticName = "TEXCOORD";
  inputElementDescs[1].SemanticIndex = 0;
  inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
  inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

  inputElementDescs[2].SemanticName = "NORMAL";
  inputElementDescs[2].SemanticIndex = 0;
  inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
  inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

  D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
  inputLayoutDesc.pInputElementDescs = inputElementDescs;
  inputLayoutDesc.NumElements = _countof(inputElementDescs);

  //========================
  // Blend
  //========================
  D3D12_BLEND_DESC blendDesc{};
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;
  blendDesc.RenderTarget[0].BlendEnable = true;
  blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
  blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
  blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
  blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

  //========================
  // Rasterizer
  // 箱を内側から見るのでまずは NONE
  //========================
  D3D12_RASTERIZER_DESC rasterizerDesc{};
  rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
  rasterizerDesc.FrontCounterClockwise = FALSE;
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

  //========================
  // Shader
  //========================
  IDxcBlob *vertexShaderBlob =
      dx12Core_->CompileShader(L"resources/shaders/Skybox.VS.hlsl", L"vs_6_0");
  assert(vertexShaderBlob != nullptr);

  IDxcBlob *pixelShaderBlob =
      dx12Core_->CompileShader(L"resources/shaders/Skybox.PS.hlsl", L"ps_6_0");
  assert(pixelShaderBlob != nullptr);

  //========================
  // DepthStencil
  // Skybox特有
  //========================
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
  depthStencilDesc.DepthEnable = true;
  depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
  depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  //========================
  // PSO
  //========================
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
  psoDesc.pRootSignature = rootSignature_.Get();
  psoDesc.InputLayout = inputLayoutDesc;

  psoDesc.VS = {vertexShaderBlob->GetBufferPointer(),
                vertexShaderBlob->GetBufferSize()};

  psoDesc.PS = {pixelShaderBlob->GetBufferPointer(),
                pixelShaderBlob->GetBufferSize()};

  psoDesc.BlendState = blendDesc;
  psoDesc.RasterizerState = rasterizerDesc;
  psoDesc.DepthStencilState = depthStencilDesc;
  psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  psoDesc.NumRenderTargets = 1;
  psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

  psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.SampleDesc.Count = 1;
  psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  hr = device->CreateGraphicsPipelineState(
      &psoDesc, IID_PPV_ARGS(&graphicsPipelineState_));
  assert(SUCCEEDED(hr));
}

void SkyboxRenderer::Begin() {
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
      dx12Core_->GetCommandList();

  commandList->SetGraphicsRootSignature(rootSignature_.Get());
  commandList->SetPipelineState(graphicsPipelineState_.Get());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}