#include "Renderer/Object3dRenderer.h"
#include "Debug/Logger.h"
#include "Util/StringUtil.h"

void Object3dRenderer::Initialize(Dx12Core *dx12Core) {
  dx12Core_ = dx12Core;

  CreateRootSignature();

  CreatePSO();
}

void Object3dRenderer::CreateRootSignature() {

  HRESULT hr;

  ID3D12Device *device = dx12Core_->GetDevice();

  // RootSignatureを作成
  D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
  descriptionRootSignature.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  // Object3d用のRootParameterを作成
  D3D12_ROOT_PARAMETER rootParameterObject3d[4] = {};
  rootParameterObject3d[0].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  rootParameterObject3d[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
  rootParameterObject3d[0].Descriptor.ShaderRegister =
      0; // レジスタ番号0とバインド

  rootParameterObject3d[1].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  rootParameterObject3d[1].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_VERTEX;                     // VertexShaderで使う
  rootParameterObject3d[1].Descriptor.ShaderRegister = 0; // レジスタ番号0を使う

  D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
  descriptorRange[0].BaseShaderRegister = 0; // ０から始まる
  descriptorRange[0].NumDescriptors = 1;     // 数は1つ
  descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
  descriptorRange[0].OffsetInDescriptorsFromTableStart =
      D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

  rootParameterObject3d[2].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
  rootParameterObject3d[2].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
  rootParameterObject3d[2].DescriptorTable.pDescriptorRanges =
      descriptorRange; // Tableの中身の配列を指定
  rootParameterObject3d[2].DescriptorTable.NumDescriptorRanges =
      _countof(descriptorRange); // Tableで利用する数

  rootParameterObject3d[3].ParameterType =
      D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
  rootParameterObject3d[3].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL;                      // PSで使う
  rootParameterObject3d[3].Descriptor.ShaderRegister = 1; // レジスタ番号1を使う

  D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
  staticSamplers[0].Filter =
      D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
  staticSamplers[0].AddressU =
      D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 0~1の範囲外をリピート
  staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較しない
  staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
  staticSamplers[0].ShaderRegister = 0; // レジスタ番号0
  staticSamplers[0].ShaderVisibility =
      D3D12_SHADER_VISIBILITY_PIXEL; // pixelShaderを使う

  descriptionRootSignature.pParameters =
      rootParameterObject3d; // ルートパラメータ配列へのポインタ
  descriptionRootSignature.NumParameters =
      _countof(rootParameterObject3d); // 配列の長さ
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
  hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                   signatureBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&rootSignature_));
  assert(SUCCEEDED(hr));
}

void Object3dRenderer::CreatePSO() {
  HRESULT hr;

  ID3D12Device *device = dx12Core_->GetDevice();

  CreateRootSignature();

  // inputLayout
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

  // BlendStateの設定
  D3D12_BLEND_DESC blendDesc{};

  // すべての色要素を書き込む
  blendDesc.RenderTarget[0].RenderTargetWriteMask =
      D3D12_COLOR_WRITE_ENABLE_ALL;

  blendDesc.RenderTarget[0].BlendEnable = true; // ブレンドを有効化

  blendDesc.RenderTarget[0].SrcBlend =
      D3D12_BLEND_SRC_ALPHA; // 元画像の透明度を使う

  blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 合成方法　(加算)

  blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;

  blendDesc.RenderTarget[0].SrcBlendAlpha =
      D3D12_BLEND_ONE; // 元画像の透明度をブレンドに反映

  blendDesc.RenderTarget[0].BlendOpAlpha =
      D3D12_BLEND_OP_ADD; // 上二つを合成(加算)

  blendDesc.RenderTarget[0].DestBlendAlpha =
      D3D12_BLEND_INV_SRC_ALPHA; // 背景の透明度は無視

  // RasiterzerStateの設定
  D3D12_RASTERIZER_DESC rasterizerDesc{};

  // 裏面(時計回り)を表示しない
  rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
  rasterizerDesc.FrontCounterClockwise = FALSE;

  // 三角形の中を塗りつぶす
  rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

  // shaderをcompileする
  IDxcBlob *vertexShaderBlob =
      dx12Core_->CompileShader(L"resources/shaders/Object3D.VS.hlsl", L"vs_6_0");
  assert(vertexShaderBlob != nullptr);

  IDxcBlob *pixelShaderBlob =
      dx12Core_->CompileShader(L"resources/shaders/Object3D.PS.hlsl", L"ps_6_0");
  assert(pixelShaderBlob != nullptr);

  // DepthStencilStateの設定
  D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

  // Depthの機能を有効化する
  depthStencilDesc.DepthEnable = true;

  // 書き込み
  depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

  // 比較関数はLessEqual。近ければ描画される
  depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

  // PSOの生成
  D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeLineStateDesc{};
  graphicsPipeLineStateDesc.pRootSignature =
      rootSignature_.Get();                                // RootSignature
  graphicsPipeLineStateDesc.InputLayout = inputLayoutDesc; // InputLayout
  graphicsPipeLineStateDesc.VS = {
      vertexShaderBlob->GetBufferPointer(),
      vertexShaderBlob->GetBufferSize()}; // VertexShader
  graphicsPipeLineStateDesc.PS = {
      pixelShaderBlob->GetBufferPointer(),
      pixelShaderBlob->GetBufferSize()};                      // PixelShader
  graphicsPipeLineStateDesc.BlendState = blendDesc;           // BlendState
  graphicsPipeLineStateDesc.RasterizerState = rasterizerDesc; // RasterizerState
  graphicsPipeLineStateDesc.DepthStencilState = depthStencilDesc;
  graphicsPipeLineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

  // 書き込むRTVの情報
  graphicsPipeLineStateDesc.NumRenderTargets = 1;
  graphicsPipeLineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

  // 利用するトポロジ(形状)のタイプ。三角形
  graphicsPipeLineStateDesc.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  // どのように画面に色を打ち込むかの設定
  graphicsPipeLineStateDesc.SampleDesc.Count = 1;
  graphicsPipeLineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

  // 実際に生成
  graphicsPipeLineState_ = nullptr;
  hr = device->CreateGraphicsPipelineState(
      &graphicsPipeLineStateDesc, IID_PPV_ARGS(&graphicsPipeLineState_));
  assert(SUCCEEDED(hr));
}

void Object3dRenderer::Begin() {
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
      dx12Core_->GetCommandList();

  // RootSignatureをセット
  commandList->SetGraphicsRootSignature(rootSignature_.Get());

  // PSOをセット
  commandList->SetPipelineState(graphicsPipeLineState_.Get());

  // プリミティブトポロジー(形状）をセット
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}