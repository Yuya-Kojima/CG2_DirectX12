#include "Particle/ParticleManager.h"
#include "Debug/Logger.h"
#include "Core/SrvManager.h"
#include "Math/MathUtil.h"
#include "Texture/TextureManager.h"
#include <cassert>
#include<numbers>

ParticleManager* ParticleManager::GetInstance() {
	static ParticleManager instance;
	return &instance;
}

void ParticleManager::Initialize(Dx12Core* dx12Core, SrvManager* srvManager) {

	// ポインタを受け取ってメンバ変数に記録
	dx12Core_ = dx12Core;

	srvManager_ = srvManager;

	// ランダムエンジンの初期化
	randomEngine_ = std::mt19937(seedGenerator_());
	distribution_ = std::uniform_real_distribution<float>(-1.0f, 1.0f);

	// Material用のリソース作成（CBV）
	materialResource_ = dx12Core_->CreateBufferResource(sizeof(Material));
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

	// 初期値
	materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData_->enableLighting = 0;
	materialData_->uvTransform = MakeIdentity4x4();

	nextInstancingSrvIndex_ = 0;

	// パイプライン生成
	CreatePSO();

	CreateVertexData();
}

void ParticleManager::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {



	const float deltaTime = 1.0f / 60.0f;

	//ビルボード回転行列
	Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
	Matrix4x4 cameraMatrix = Inverse(viewMatrix);
	Matrix4x4 billboardMatrix = Multiply(backToFrontMatrix, cameraMatrix);
	billboardMatrix.m[3][0] = 0;
	billboardMatrix.m[3][1] = 0;
	billboardMatrix.m[3][2] = 0;
	billboardMatrix.m[0][3] = 0.0f;
	billboardMatrix.m[1][3] = 0.0f;
	billboardMatrix.m[2][3] = 0.0f;
	billboardMatrix.m[3][3] = 1.0f;

	//ビュー行列とプロジェクション行列をカメラから取得
	viewMatrix_ = viewMatrix;
	projectionMatrix_ = projectionMatrix;

	//すべてのパーティクルグループについて処理する
	for (auto& [name, group] : particleGroups_) {

		group.numInstance = 0;

		//グループ内のすべてのパーティクルについて処理する
		for (auto it = group.particles.begin();
			it != group.particles.end(); ) {

			if (group.numInstance >= kNumMaxInstance_) {
				break;
			}


			//寿命チェック
			if (it->currentTime >= it->lifeTime) {
				it = group.particles.erase(it);
				continue;
			}

			//場の影響を計算

			//移動処理
			it->transform.translate += it->velocity * deltaTime;

			//経過時間を加算
			it->currentTime += deltaTime;


			// スケール行列
			Matrix4x4 scaleMatrix = MakeScaleMatrix(it->transform.scale);

			// 平行移動行列
			Matrix4x4 translateMatrix = MakeTranslateMatrix(it->transform.translate);

			//ワールド行列を計算
			Matrix4x4 worldMatrix = Multiply(Multiply(scaleMatrix, billboardMatrix), translateMatrix);

			//ワールドビュープロジェクション行列を合成
			Matrix4x4 worldViewProjectionMatrix = Multiply(Multiply(worldMatrix, viewMatrix_), projectionMatrix_);

			//インスタンシング用データ一個分の書き込み
			const uint32_t instanceIndex = group.numInstance;

			group.instancingData[instanceIndex].World = worldMatrix;
			group.instancingData[instanceIndex].WVP = worldViewProjectionMatrix;
			group.instancingData[instanceIndex].color = { 1.0f,1.0f,1.0f,1.0f };

			group.numInstance++;

			++it;
		}
	}
}

void ParticleManager::Draw() {

	auto* commandList = dx12Core_->GetCommandList();

	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(
		graphicsPipeLineState_.Get());                 // PSOを設定
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_); // VBVを設定

	commandList->SetGraphicsRootConstantBufferView(
		0, materialResource_->GetGPUVirtualAddress());

	for (auto& [name, group] : particleGroups_) {

		if (group.numInstance == 0) { continue; }

		D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU =
			srvManager_->GetGPUDescriptorHandle(group.instancingSrvIndex);
		commandList->SetGraphicsRootDescriptorTable(1, instancingSrvHandleGPU);

		// (2) テクスチャSRV（rootParameter[2]）
		D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU =
			srvManager_->GetGPUDescriptorHandle(group.materialData.textureSrvIndex);
		commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);

		// (3) DrawCall（6頂点 = 板ポリ、instance数 = group.numInstance）
		commandList->DrawInstanced(6, group.numInstance, 0, 0);

	}
}

void ParticleManager::Finalize() {

	particleGroups_.clear();

	// GPUリソース類を解放
	vertexResource_.Reset();
	materialResource_.Reset();
	graphicsPipeLineState_.Reset();
	rootSignature_.Reset();

	// Mapしてるやつはポインタ無効化（Unmap必須かは設計次第だが、最低これ）
	materialData_ = nullptr;

	dx12Core_ = nullptr;
	srvManager_ = nullptr;
}

void ParticleManager::CreateRootSignature() {

	HRESULT hr;

	// RootSignatureを作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Particle用のRootParameter作成。
	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[0].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;               // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド

	D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1] = {};
	descriptorRangeForInstancing[0].BaseShaderRegister = 0; // 0から始まる
	descriptorRangeForInstancing[0].NumDescriptors = 1;     // 数は一つ
	descriptorRangeForInstancing[0].RangeType =
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[1].ParameterType =
		D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	rootParameters[1].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_VERTEX; // VertexShaderで使う
	rootParameters[1].DescriptorTable.pDescriptorRanges =
		descriptorRangeForInstancing; // Tableの中身の配列を指定
	rootParameters[1].DescriptorTable.NumDescriptorRanges =
		_countof(descriptorRangeForInstancing); // Tableで利用する数

	D3D12_DESCRIPTOR_RANGE descriptorRangeParticle[1] = {};
	descriptorRangeParticle[0].BaseShaderRegister = 0; // ０から始まる
	descriptorRangeParticle[0].NumDescriptors = 1;     // 数は1つ
	descriptorRangeParticle[0].RangeType =
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRangeParticle[0].OffsetInDescriptorsFromTableStart =
		D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

	rootParameters[2].ParameterType =
		D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DiscriptorTableを使う
	rootParameters[2].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges =
		descriptorRangeParticle; // Tableの中身の配列を指定
	rootParameters[2].DescriptorTable.NumDescriptorRanges =
		_countof(descriptorRangeParticle); // Tableで利用する数

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

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	ID3DBlob* signatureBlobPart = nullptr;
	ID3DBlob* errorBlobPart = nullptr;
	hr = D3D12SerializeRootSignature(
		&descriptionRootSignature, // Particle 用記述子を渡す
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlobPart, &errorBlobPart);

	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlobPart->GetBufferPointer()));
		assert(false);
	}

	rootSignature_ = nullptr;
	hr = dx12Core_->GetDevice()->CreateRootSignature(
		0, signatureBlobPart->GetBufferPointer(),
		signatureBlobPart->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));

	assert(SUCCEEDED(hr));
}

void ParticleManager::CreatePSO() {

	HRESULT hr;

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

	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;

	blendDesc.RenderTarget[0].SrcBlendAlpha =
		D3D12_BLEND_ONE; // 元画像の透明度をブレンドに反映

	blendDesc.RenderTarget[0].BlendOpAlpha =
		D3D12_BLEND_OP_ADD; // 上二つを合成(加算)

	blendDesc.RenderTarget[0].DestBlendAlpha =
		D3D12_BLEND_ZERO; // 背景の透明度は無視

	// RasiterzerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// 裏面(時計回り)を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// shaderをcompileする
	IDxcBlob* particleVertexShaderBlob = dx12Core_->CompileShader(
		L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");

	IDxcBlob* particlePixelShaderBlob = dx12Core_->CompileShader(
		L"resources/shaders/Particle.PS.hlsl", L"ps_6_0");

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;

	// 書き込み
	depthStencilDesc.DepthWriteMask =
		D3D12_DEPTH_WRITE_MASK_ZERO; // Depthの書き込みを行わない

	// 比較関数はLessEqual。近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// particle用PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeLineStateDesc{};
	graphicsPipeLineStateDesc.pRootSignature =
		rootSignature_.Get();                                // RootSignature
	graphicsPipeLineStateDesc.InputLayout = inputLayoutDesc; // InputLayout
	graphicsPipeLineStateDesc.VS = {
		particleVertexShaderBlob->GetBufferPointer(),
		particleVertexShaderBlob->GetBufferSize(),
	}; // VertexShader
	graphicsPipeLineStateDesc.PS = {
		particlePixelShaderBlob->GetBufferPointer(),
		particlePixelShaderBlob->GetBufferSize() };              // PixelShader
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
	hr = dx12Core_->GetDevice()->CreateGraphicsPipelineState(
		&graphicsPipeLineStateDesc, IID_PPV_ARGS(&graphicsPipeLineState_));
	assert(SUCCEEDED(hr));
}

void ParticleManager::CreateVertexData() {

	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;

	// VertexResourceを作る
	//=========================
	vertexResource_ = dx12Core_->CreateBufferResource(sizeof(VertexData) * 6);

	// VertexBufferViewを作る(値を設定するだけ)
	//=======================================
	// リソースの先頭アドレスから使う
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();

	// 使用するリソースサイズは頂点六つ分のサイズ
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;

	// 1頂点あたりのサイズ
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// VertexResourceにデータを書き込むためのアドレスを取得してvertexDataに割り当てる
	//==========================================================================

	// 頂点データ設定
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// 0: 左上
	vertexData[0].position = { -1.0f, 1.0f, 0.0f, 1.0f };
	vertexData[0].texcoord = { 0.0f, 0.0f };
	vertexData[0].normal = { 0.0f, 0.0f, 1.0f };

	// 1: 右上
	vertexData[1].position = { 1.0f, 1.0f, 0.0f, 1.0f };
	vertexData[1].texcoord = { 1.0f, 0.0f };
	vertexData[1].normal = { 0.0f, 0.0f, 1.0f };

	// 2: 左下
	vertexData[2].position = { -1.0f, -1.0f, 0.0f, 1.0f };
	vertexData[2].texcoord = { 0.0f, 1.0f };
	vertexData[2].normal = { 0.0f, 0.0f, 1.0f };

	// 3: 左下（2枚目の三角形）
	vertexData[3].position = { -1.0f, -1.0f, 0.0f, 1.0f };
	vertexData[3].texcoord = { 0.0f, 1.0f };
	vertexData[3].normal = { 0.0f, 0.0f, 1.0f };

	// 4: 右上（2枚目の三角形）
	vertexData[4].position = { 1.0f, 1.0f, 0.0f, 1.0f };
	vertexData[4].texcoord = { 1.0f, 0.0f };
	vertexData[4].normal = { 0.0f, 0.0f, 1.0f };

	// 5: 右下
	vertexData[5].position = { 1.0f, -1.0f, 0.0f, 1.0f };
	vertexData[5].texcoord = { 1.0f, 1.0f };
	vertexData[5].normal = { 0.0f, 0.0f, 1.0f };
}

void ParticleManager::CreateParticleGroup(const std::string name,
	const std::string textureFilePath) {

	// 登録済みかチェック
	assert(!particleGroups_.contains(name));

	// 新たなパーティクルグループを作成し、コンテナに登録
	ParticleGroup& particleGroup = particleGroups_[name];

	// マテリアルデータにテクスチャファイルパスを設定
	particleGroup.materialData.filePath = textureFilePath;

	// テクスチャ読み込み
	TextureManager::GetInstance()->LoadTexture(
		particleGroup.materialData.filePath);

	// マテリアルデータにテクスチャのSRVインデックスを記録
	particleGroup.materialData.textureSrvIndex =
		TextureManager::GetInstance()->GetSrvIndex(
			particleGroup.materialData.filePath);

	const uint32_t kNumMaxInstance = 100; // インスタンス数

	// インスタンシング用リソースの生成
	particleGroup.instancingResource =
		dx12Core_->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);

	// 書き込むためのアドレスを取得
	ParticleForGPU* instancingData = nullptr;

	particleGroup.instancingResource->Map(
		0, nullptr, reinterpret_cast<void**>(&instancingData));

	particleGroup.instancingData = instancingData;

	for (uint32_t index = 0; index < kNumMaxInstance; ++index) {
		instancingData[index].WVP = MakeIdentity4x4();
		instancingData[index].World = MakeIdentity4x4();
		instancingData[index].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	}

	//// インスタンシングようにSRVを確保して記録
	//D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	//instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	//instancingSrvDesc.Shader4ComponentMapping =
	//	D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	//instancingSrvDesc.Buffer.FirstElement = 0;
	//instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	//instancingSrvDesc.Buffer.NumElements = kNumMaxInstance;
	//instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

	//particleGroup.instancingSrvIndex = nextInstancingSrvIndex_++;

	//D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU =
	//	srvManager_->GetCPUDescriptorHandle(
	//		particleGroup
	//		.instancingSrvIndex); // Heapの三番目に作成(空いているのであればどこでもOK)
	//D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU =
	//	srvManager_->GetGPUDescriptorHandle(particleGroup.instancingSrvIndex);

	//dx12Core_->GetDevice()->CreateShaderResourceView(
	//	particleGroup.instancingResource.Get(), &instancingSrvDesc,
	//	instancingSrvHandleCPU);

	particleGroup.instancingSrvIndex = srvManager_->Allocate();

	// できればSrvManagerの関数を使う（あなたのSrvManagerにある）
	srvManager_->CreateSRVforStructuredBuffer(
		particleGroup.instancingSrvIndex,
		particleGroup.instancingResource.Get(),
		kNumMaxInstance,
		sizeof(ParticleForGPU));
}

void ParticleManager::Emit(
	const std::string& name,
	const Vector3& position,
	uint32_t count) {

	Logger::Log(std::format(
		"Emit: name={}, count={}\n",
		name,
		count
	));

	assert(particleGroups_.contains(name));

	ParticleGroup& group = particleGroups_.at(name);

	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distLife(1.0f, 3.0f);

	//秒速スケール
	const float speed = 1.0f;

	for (uint32_t i = 0; i < count; ++i) {
		Particle particle{};

		Vector3 randomOffset{ dist(randomEngine_), dist(randomEngine_), dist(randomEngine_) };
		particle.transform.translate = position + randomOffset;

		particle.transform.scale = { 1.0f, 1.0f, 1.0f };

		particle.velocity = {
			distribution_(randomEngine_),
			distribution_(randomEngine_),
			distribution_(randomEngine_)
		};


		particle.lifeTime = distLife(randomEngine_);
		particle.currentTime = 0.0f;

		// 指定グループに登録
		group.particles.push_back(particle);
	}
}
