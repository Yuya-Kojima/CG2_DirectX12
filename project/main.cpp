#include "Math/MathUtil.h"
#include "Math/Matrix4x4.h"
#include "Math/Transform.h"
#include "Math/Vector4.h"
#include "Math/VertexData.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
// #include <Windows.h>
#include "Camera/GameCamera.h"
#include "Render/ParticleManager.h"
#include "Render/ParticleEmitter.h"
#include "Core/D3DResourceLeakChecker.h"
#include "Render/ModelManager.h"
#include "Render/TextureManager.h"
#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <format>
#include <string>
#define _USE_MATH_DEFINES
#include "Camera/DebugCamera.h"
#include "Core/Dx12Core.h"
#include "Core/Logger.h"
#include "Core/SrvManager.h"
#include "Core/WindowSystem.h"
#include "DirectionalLight.h"
#include "Input/InputKeyState.h"
#include "Material.h"
#include "Math/Field.h"
#include "Math/TransformationMatrix.h"
#include "ModelData.h"
#include "Render/Model.h"
#include "Render/ModelRenderer.h"
#include "Render/Object3d.h"
#include "Render/Object3dRenderer.h"
#include "Render/Particle.h"
#include "Render/ResourceObject.h"
#include "Render/Sprite.h"
#include "Render/SpriteRenderer.h"
#include "Util/StringUtil.h"
#include <dinput.h>
#include <fstream>
#include <math.h>
#include <numbers>
#include <random>
#include <wrl.h>
#include <xaudio2.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

// チャンクヘッダ
struct ChunkHeader {
	char id[4];   // チャンク毎のID
	int32_t size; // チャンクサイズ
};

// RIFFヘッダチャンク
struct RiffHeader {
	ChunkHeader chunk; //"RIFF"
	char type[4];      //"WAVE"
};

// FMTチャンク
struct FormatChunk {
	ChunkHeader chunk; //"fmt"
	WAVEFORMATEX fmt;  // 波形フォーマット
};

// 音声データ
struct SoundData {
	// 波形フォーマット
	WAVEFORMATEX wfex;

	// バッファの先頭アドレス
	BYTE* pBuffer;

	// バッファサイズ
	unsigned int bufferSize;
};

// GPUに転送するデータ用
struct ParticleForGPU {
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

/// <summary>
/// waveファイルを読み込む
/// </summary>
/// <param name="filename">ファイル名</param>
/// <returns></returns>
SoundData SoundLoadWave(const char* filename) {

	/* ファイルオープン
	----------------------*/

	// ファイル入力ストリームのインスタンス
	std::ifstream file;

	//.wavファイルをバイナリモードで開く
	file.open(filename, std::ios_base::binary);

	// ファイルが開けなかったら
	assert(file.is_open());

	/* .wavデータ読み込み
	----------------------*/

	// RIFFヘッダーの読み込み
	RiffHeader riff;

	file.read((char*)&riff, sizeof(riff));

	// 開いたファイルがRIFFであるかを確認する
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}

	// タイプがWAVEか確認
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	// formatチャンクの読み込み
	FormatChunk format = {};

	// チャンクヘッダーの確認
	file.read((char*)&format, sizeof(ChunkHeader));

	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}

	// チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	// Dataチャンクの読み込み
	ChunkHeader data;

	file.read((char*)&data, sizeof(data));

	// Junkチャンク(パディング？)を検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0) {

		// 読み取り位置をJunkチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);

		// 飛ばした後に再度読み込み
		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	// Dataチャンクのデータ部(波形データ)の読み込み
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	// Waveファイルを閉じる
	file.close();

	/* 読み込んだ音声データをreturn
	-------------------------------*/

	// returnするための音声データ
	SoundData soundData = {};

	soundData.wfex = format.fmt;
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	soundData.bufferSize = data.size;

	return soundData;
}

/// <summary>
/// 音声データの解放
/// </summary>
/// <param name="soundData"></param>
void SoundUnload(SoundData* soundData) {

	// バッファのメモリを解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
	soundData->wfex = {};
}

/// <summary>
/// サウンド再生
/// </summary>
/// <param name="xAudio2">再生するためのxAudio2</param>
/// <param name="soundData">音声データ(波形データ、サイズ、フォーマット)</param>
void SoundPlayerWave(IXAudio2* xAudio2, const SoundData& soundData) {

	HRESULT result;

	// 波形フォーマットを元にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	// 波形データの再生
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
}

/// <summary>
/// Vector3 正規化
/// </summary>
/// <returns>正規化された値</returns>
// Vector3 Normalize(Vector3 vector) {
//
//    長さ
//   float length;
//
//   length =
//       sqrtf(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
//
//   return {vector.x / length, vector.y / length, vector.z / length};
// }

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// hrの生成
	HRESULT hr;

	// WindowAPIの作成
	WindowSystem* windowSystem = nullptr;
	windowSystem = new WindowSystem();
	windowSystem->Initialize();

	// リソースリークチェック
	D3DResourceLeakChecker leakCheck;

	Dx12Core* dx12Core = nullptr;
	dx12Core = new Dx12Core();
	dx12Core->Initialize(windowSystem);

	ID3D12Device* device = dx12Core->GetDevice();
	ID3D12GraphicsCommandList* commandList = dx12Core->GetCommandList();

	// スプライトの共通部分
	SpriteRenderer* spriteRenderer = nullptr;
	spriteRenderer = new SpriteRenderer();
	spriteRenderer->Initialize(dx12Core);

	// オブジェクトの共通部分
	Object3dRenderer* object3dRenderer = nullptr;
	object3dRenderer = new Object3dRenderer();
	object3dRenderer->Initialize(dx12Core);

	SrvManager* srvManager = nullptr;
	srvManager = new SrvManager();
	srvManager->Initialize(dx12Core);

	// モデルの共通部分
	// ModelRenderer *modelRenderer = nullptr;
	// modelRenderer = new ModelRenderer();
	// modelRenderer->Initialize(dx12Core);

	// DirectInputの初期化
	InputKeyState* input = nullptr;
	input = new InputKeyState();
	input->Initialize(windowSystem);

	// テクスチャマネージャーの初期化
	TextureManager::GetInstance()->Initialize(dx12Core, srvManager);

	// モデルマネージャーの初期化
	ModelManager::GetInstance()->Initialize(dx12Core);

	// ParticleManagerの初期化
	ParticleManager::GetInstance()->Initialize(dx12Core, srvManager);

	//=============================
	// サウンド用
	//=============================

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice
		* masterVoice; // xAudio2が解放されると同時に無効化されるのでdeleteしない。

	hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	hr = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(hr));

	// ModelData modelData;
	// modelData.vertices.push_back({.position = {-1.0f, 1.0f, 0.0f, 1.0f},
	//                               .texcoord = {0.0f, 0.0f},
	//                               .normal = {0.0f, 0.0f, 1.0f}}); // 左上
	// modelData.vertices.push_back({.position = {1.0f, 1.0f, 0.0f, 1.0f},
	//                               .texcoord = {1.0f, 0.0f},
	//                               .normal = {0.0f, 0.0f, 1.0f}}); // 右上
	// modelData.vertices.push_back({.position = {-1.0f, -1.0f, 0.0f, 1.0f},
	//                               .texcoord = {0.0f, 1.0f},
	//                               .normal = {0.0f, 0.0f, 1.0f}}); // 左下
	// modelData.vertices.push_back({.position = {-1.0f, -1.0f, 0.0f, 1.0f},
	//                               .texcoord = {0.0f, 1.0f},
	//                               .normal = {0.0f, 0.0f, 1.0f}}); // 左下
	// modelData.vertices.push_back({.position = {1.0f, 1.0f, 0.0f, 1.0f},
	//                               .texcoord = {1.0f, 0.0f},
	//                               .normal = {0.0f, 0.0f, 1.0f}}); // 右上
	// modelData.vertices.push_back({.position = {1.0f, -1.0f, 0.0f, 1.0f},
	//                               .texcoord = {1.0f, 1.0f},
	//                               .normal = {0.0f, 0.0f, 1.0f}}); // 右下
	// modelData.material.textureFilePath = "./resources/circle.png";

	// テクスチャの読み込み
	TextureManager::GetInstance()->LoadTexture("resources/uvChecker.png");

	TextureManager::GetInstance()->LoadTexture("resources/monsterball.png");

	// モデルの読み込み
	ModelManager::GetInstance()->LoadModel("plane.obj");

	ModelManager::GetInstance()->LoadModel("axis.obj");


	// グループ登録（name と texture を紐づけ）
	ParticleManager::GetInstance()->CreateParticleGroup("test", "resources/circle.png");

	// スプライトの生成と初期化
	std::vector<Sprite*> sprites;
	const int kSpriteCount = 5;
	for (int i = 0; i < kSpriteCount; ++i) {
		Sprite* sprite = new Sprite();
		if (i % 2 == 1) {
			sprite->Initialize(spriteRenderer, "resources/uvChecker.png");
		}
		else {
			sprite->Initialize(spriteRenderer, "resources/monsterball.png");
		}
		sprites.push_back(sprite);
	}

	Sprite* sprite = new Sprite();
	sprite->Initialize(spriteRenderer, "resources/uvChecker.png");

	// モデルの生成と初期化
	// Model *model = new Model();
	// model->Initialize(modelRenderer, "resources", "plane.obj");

	// カメラの生成と初期化
	GameCamera* camera = new GameCamera();
	camera->SetRotate({ 0.3f, 0.0f, 0.0f });
	camera->SetTranslate({ 0.0f, 4.0f, -10.0f });
	object3dRenderer->SetDefaultCamera(camera);

	// オブジェクトの生成と初期化
	Object3d* object3d = new Object3d();
	object3d->Initialize(object3dRenderer);
	object3d->SetModel("plane.obj");

	Object3d* object3dA = new Object3d();
	object3dA->Initialize(object3dRenderer);
	object3dA->SetModel("axis.obj");

	// RootSignatureを作成
	// D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	// descriptionRootSignature.Flags =
	//    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Particle 用のルートシグネチャ記述子を別に作る
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignatureParticle{};
	descriptionRootSignatureParticle.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Object3d用のRootParameterを作成
	// D3D12_ROOT_PARAMETER rootParameterObject3d[4] = {};
	// rootParameterObject3d[0].ParameterType =
	//    D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	// rootParameterObject3d[0].ShaderVisibility =
	//    D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	// rootParameterObject3d[0].Descriptor.ShaderRegister =
	//    0; // レジスタ番号0とバインド

	// rootParameterObject3d[1].ParameterType =
	//     D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	// rootParameterObject3d[1].ShaderVisibility =
	//     D3D12_SHADER_VISIBILITY_VERTEX;                     //
	//     VertexShaderで使う
	// rootParameterObject3d[1].Descriptor.ShaderRegister = 0; //
	// レジスタ番号0を使う descriptionRootSignature.pParameters =
	//     rootParameterObject3d; // ルートパラメータ配列へのポインタ
	// descriptionRootSignature.NumParameters =
	//     _countof(rootParameterObject3d); // 配列の長さ

	// D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	// descriptorRange[0].BaseShaderRegister = 0; // ０から始まる
	// descriptorRange[0].NumDescriptors = 1;     // 数は1つ
	// descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; //
	// SRVを使う descriptorRange[0].OffsetInDescriptorsFromTableStart =
	//     D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // Offsetを自動計算

	// rootParameterObject3d[2].ParameterType =
	//     D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTableを使う
	// rootParameterObject3d[2].ShaderVisibility =
	//     D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	// rootParameterObject3d[2].DescriptorTable.pDescriptorRanges =
	//     descriptorRange; // Tableの中身の配列を指定
	// rootParameterObject3d[2].DescriptorTable.NumDescriptorRanges =
	//     _countof(descriptorRange); // Tableで利用する数

	// rootParameterObject3d[3].ParameterType =
	//     D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	// rootParameterObject3d[3].ShaderVisibility =
	//     D3D12_SHADER_VISIBILITY_PIXEL;                      // PSで使う
	// rootParameterObject3d[3].Descriptor.ShaderRegister = 1; //
	// レジスタ番号1を使う

	// Particle用のRootParameter作成。
	D3D12_ROOT_PARAMETER rootParameters[3] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // CBVを使う
	rootParameters[0].ShaderVisibility =
		D3D12_SHADER_VISIBILITY_PIXEL;               // PixelShaderで使う
	rootParameters[0].Descriptor.ShaderRegister = 0; // レジスタ番号0とバインド

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;

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

	// descriptionRootSignature.pStaticSamplers = staticSamplers;
	// descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	descriptionRootSignatureParticle.pParameters = rootParameters;
	descriptionRootSignatureParticle.NumParameters = _countof(rootParameters);
	descriptionRootSignatureParticle.pStaticSamplers = staticSamplers;
	descriptionRootSignatureParticle.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	// ID3DBlob *signatureBlob = nullptr;
	// ID3DBlob *errorBlob = nullptr;
	// hr = D3D12SerializeRootSignature(&descriptionRootSignature,
	//                                 D3D_ROOT_SIGNATURE_VERSION_1,
	//                                 &signatureBlob, &errorBlob);

	// if (FAILED(hr)) {
	//   Logger::Log(reinterpret_cast<char *>(errorBlob->GetBufferPointer()));
	//   assert(false);
	// }

	// バイナリを元に生成
	// Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	// hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
	//                                 signatureBlob->GetBufferSize(),
	//                                 IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	ID3DBlob* signatureBlobPart = nullptr;
	ID3DBlob* errorBlobPart = nullptr;
	hr = D3D12SerializeRootSignature(
		&descriptionRootSignatureParticle, // Particle 用記述子を渡す
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlobPart, &errorBlobPart);

	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlobPart->GetBufferPointer()));
		assert(false);
	}

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureParticle;
	hr = device->CreateRootSignature(0, signatureBlobPart->GetBufferPointer(),
		signatureBlobPart->GetBufferSize(),
		IID_PPV_ARGS(&rootSignatureParticle));
	assert(SUCCEEDED(hr));

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
	IDxcBlob* vertexShaderBlob =
		dx12Core->CompileShader(L"resources/shaders/Object3D.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	IDxcBlob* pixelShaderBlob =
		dx12Core->CompileShader(L"resources/shaders/Object3D.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	IDxcBlob* particleVertexShaderBlob =
		dx12Core->CompileShader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");

	IDxcBlob* particlePixelShaderBlob =
		dx12Core->CompileShader(L"resources/shaders/Particle.PS.hlsl", L"ps_6_0");

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};

	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;

	// 書き込み
	depthStencilDesc.DepthWriteMask =
		D3D12_DEPTH_WRITE_MASK_ZERO; // Depthの書き込みを行わない

	// 比較関数はLessEqual。近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// PSOの生成
	// D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeLineStateDesc{};
	// graphicsPipeLineStateDesc.pRootSignature =
	//    rootSignature.Get();                                 // RootSignature
	// graphicsPipeLineStateDesc.InputLayout = inputLayoutDesc; // InputLayout
	// graphicsPipeLineStateDesc.VS = {
	//    vertexShaderBlob->GetBufferPointer(),
	//    vertexShaderBlob->GetBufferSize()}; // VertexShader
	// graphicsPipeLineStateDesc.PS = {
	//    pixelShaderBlob->GetBufferPointer(),
	//    pixelShaderBlob->GetBufferSize()};                      // PixelShader
	// graphicsPipeLineStateDesc.BlendState = blendDesc;           // BlendState
	// graphicsPipeLineStateDesc.RasterizerState = rasterizerDesc; //
	// RasterizerState graphicsPipeLineStateDesc.DepthStencilState =
	// depthStencilDesc; graphicsPipeLineStateDesc.DSVFormat =
	// DXGI_FORMAT_D24_UNORM_S8_UINT;

	//// 書き込むRTVの情報
	// graphicsPipeLineStateDesc.NumRenderTargets = 1;
	// graphicsPipeLineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//// 利用するトポロジ(形状)のタイプ。三角形
	// graphicsPipeLineStateDesc.PrimitiveTopologyType =
	//     D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//// どのように画面に色を打ち込むかの設定
	// graphicsPipeLineStateDesc.SampleDesc.Count = 1;
	// graphicsPipeLineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//// 実際に生成
	// Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineState =
	// nullptr; hr = device->CreateGraphicsPipelineState(
	//     &graphicsPipeLineStateDesc, IID_PPV_ARGS(&graphicsPipeLineState));
	// assert(SUCCEEDED(hr));

	// particle用PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipeLineStateDescParticle{};
	graphicsPipeLineStateDescParticle.pRootSignature =
		rootSignatureParticle.Get(); // RootSignature
	graphicsPipeLineStateDescParticle.InputLayout =
		inputLayoutDesc; // InputLayout
	graphicsPipeLineStateDescParticle.VS = {
		particleVertexShaderBlob->GetBufferPointer(),
		particleVertexShaderBlob->GetBufferSize(),
	}; // VertexShader
	graphicsPipeLineStateDescParticle.PS = {
		particlePixelShaderBlob->GetBufferPointer(),
		particlePixelShaderBlob->GetBufferSize() };            // PixelShader
	graphicsPipeLineStateDescParticle.BlendState = blendDesc; // BlendState
	graphicsPipeLineStateDescParticle.RasterizerState =
		rasterizerDesc; // RasterizerState
	graphicsPipeLineStateDescParticle.DepthStencilState = depthStencilDesc;
	graphicsPipeLineStateDescParticle.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 書き込むRTVの情報
	graphicsPipeLineStateDescParticle.NumRenderTargets = 1;
	graphicsPipeLineStateDescParticle.RTVFormats[0] =
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// 利用するトポロジ(形状)のタイプ。三角形
	graphicsPipeLineStateDescParticle.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// どのように画面に色を打ち込むかの設定
	graphicsPipeLineStateDescParticle.SampleDesc.Count = 1;
	graphicsPipeLineStateDescParticle.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 実際に生成
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipeLineStateParticle =
		nullptr;
	hr = device->CreateGraphicsPipelineState(
		&graphicsPipeLineStateDescParticle,
		IID_PPV_ARGS(&graphicsPipeLineStateParticle));
	assert(SUCCEEDED(hr));

	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};

	// バッファリソース。テクスチャの場合はまた別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width =
		sizeof(VertexData) * 6; // リソースのサイズ。今回はVector4を三頂点分

	// バッファの場合はこれらを1にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;

	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	/*マテリアル用のリソースを作る。
	--------------------------------*/
	// Microsoft::WRL::ComPtr<ID3D12Resource> materialResource =
	//     dx12Core->CreateBufferResource(sizeof(Material));

	//// マテリアルにデータを書き込む
	// Material *materialData = nullptr;

	//// 書き込むためのアドレスを取得
	// materialResource->Map(0, nullptr, reinterpret_cast<void
	// **>(&materialData));

	// assert(SUCCEEDED(hr));

	//// 色の指定
	// materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//// Lightingさせるか
	// materialData->enableLighting = true;
	//// UVTransform 単位行列を入れておく
	// materialData->uvTransform = MakeIdentity4x4();

	/*Sprite用のマテリアルリソースを作る
	-----------------------------------*/
	// Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite =
	//     dx12Core->CreateBufferResource(sizeof(Material));

	//// データを書き込む
	// Material *materialDataSprite = nullptr;

	//// 書き込むためのアドレスを取得
	// materialResourceSprite->Map(0, nullptr,
	//                             reinterpret_cast<void
	//                             **>(&materialDataSprite));

	// assert(SUCCEEDED(hr));

	//// 色の指定
	// materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//// Lightingさせるか
	// materialDataSprite->enableLighting = false;
	//// UVTransform　単位行列を入れておく
	// materialDataSprite->uvTransform = MakeIdentity4x4();

	/*WVP用のリソースを作る。
	--------------------------------------------------*/

	//// cbvのsizeは256バイト単位で確保する(GPT調べ)
	// UINT transformationMatrixSize = (sizeof(TransformationMatrix) + 255) &
	// ~255;

	// assert(transformationMatrixSize % 256 == 0);

	// Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource[10];

	//// データを書き込む
	// TransformationMatrix *transformationMatrixData[10];

	// for (int i = 0; i < 10; i++) {

	//  transformationMatrixResource[i] =
	//      dx12Core->CreateBufferResource(transformationMatrixSize);

	//  // 書き込むためのアドレスを取得
	//  transformationMatrixResource[i]->Map(
	//      0, nullptr, reinterpret_cast<void **>(&transformationMatrixData[i]));

	//  // 単位行列を書き込んでおく
	//  transformationMatrixData[i]->World = MakeIdentity4x4();
	//  transformationMatrixData[i]->WVP = MakeIdentity4x4();
	//}

	/* Sprite用のTransformationMatrix用のリソースを作る。Matrix4x4一つ分
	----------------------------------------------------------------*/
	// Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite =
	//     dx12Core->CreateBufferResource(transformationMatrixSize);

	//// データを書き込む
	// TransformationMatrix *transformationMatrixDataSprite = nullptr;

	//// 書き込むためのアドレスを取得
	// transformationMatrixResourceSprite->Map(
	//     0, nullptr, reinterpret_cast<void
	//     **>(&transformationMatrixDataSprite));

	//// 単位行列を書き込んでおく
	// transformationMatrixDataSprite->World = MakeIdentity4x4();
	// transformationMatrixDataSprite->WVP = MakeIdentity4x4();

	/* 平行光源用のリソースを作る
	------------------------------------*/
	// Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource =
	//     dx12Core->CreateBufferResource(sizeof(DirectionalLight));

	//// データを書き込む
	// DirectionalLight *directionalLightData = nullptr;

	//// 書き込むためのアドレスを取得
	// directionalLightResource->Map(
	//     0, nullptr, reinterpret_cast<void **>(&directionalLightData));

	//// Lightingの色
	// directionalLightData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	// directionalLightData->direction = Normalize(Vector3(0.0f, -1.0f, 0.0f));
	// directionalLightData->intensity = 1.0f;

	//=============================
	// モデルの頂点
	//=============================

	//// モデル読み込み
	// ModelData modelData = LoadModelFile("resources", "plane.obj");

	//// 頂点リソースを作る
	// Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource =
	//     dx12Core->CreateBufferResource(sizeof(VertexData) *
	//                                    modelData.vertices.size());

	//// 頂点バッファビューを作成する
	// D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	//// リソースの先頭アドレスから使う
	// vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();

	//// 使用するリソースのサイズは頂点のサイズ
	// vertexBufferView.SizeInBytes =
	//     UINT(sizeof(VertexData) * modelData.vertices.size());

	//// 1頂点あたりのサイズ
	// vertexBufferView.StrideInBytes = sizeof(VertexData);

	//// Sprite用のリソースを作成する
	// Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite =
	//     dx12Core->CreateBufferResource(sizeof(VertexData) * 6);

	//// 頂点バッファビューを作成する
	// D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};

	//// リソースの先頭アドレスから使う
	// vertexBufferViewSprite.BufferLocation =
	//     vertexResourceSprite->GetGPUVirtualAddress();

	//// 使用するリソースサイズは頂点六つ分のサイズ
	// vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;

	//// 1頂点あたりのサイズ
	// vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	//// 頂点リソースにデータを書き込む
	// VertexData *vertexData = nullptr;

	//// 書き込むためのアドレスを取得
	// vertexResource->Map(0, nullptr, reinterpret_cast<void **>(&vertexData));

	//// 頂点データをリソースにコピー
	// std::memcpy(vertexData, modelData.vertices.data(),
	//             sizeof(VertexData) * modelData.vertices.size());

	//=============================
	// 頂点インデックス(球)
	//=============================
	// Microsoft::WRL::ComPtr<ID3D12Resource> indexResource =
	//    dx12Core->CreateBufferResource(sizeof(uint32_t) * 1536);

	//// インデックスバッファビュー
	// D3D12_INDEX_BUFFER_VIEW indexBufferView{};

	//// リソースの先頭アドレスから使う
	// indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();

	//// 使用するリソース
	// indexBufferView.SizeInBytes = sizeof(uint32_t) * 1536;

	//// インデックスはuint32_t
	// indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// uint32_t *indexData = nullptr;

	// indexResource->Map(0, nullptr, reinterpret_cast<void **>(&indexData));

	//=============================
	// 球
	//=============================

	// const uint32_t kSubdivision = 16; // 分割数

	// uint32_t latIndex;
	// uint32_t lonIndex;

	//// 経度分割一つ分の角度
	// const float kLonEvery =
	//     static_cast<float>(M_PI) * 2.0f / static_cast<float>(kSubdivision);

	//// 緯度
	// const float kLatEvery =
	//     static_cast<float>(M_PI) / static_cast<float>(kSubdivision);

	//// 緯度の方向に分割
	// for (latIndex = 0; latIndex < kSubdivision; ++latIndex) {

	//  // 現在の緯度
	// float lat = -static_cast<float>(M_PI) / 2.0f + kLatEvery * latIndex;

	//  // 経度の方向に分割
	//  for (lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {

	//    // 現在の経度
	//    float lon = lonIndex * kLonEvery;

	//    // 最初に書き込む場所(a)
	//    uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;

	//    // texcoord
	//    float u0 =
	//        static_cast<float>(lonIndex) / static_cast<float>(kSubdivision);

	//    float v0 =
	//        static_cast<float>(latIndex) / static_cast<float>(kSubdivision);

	//    float u1 = float(lonIndex + 1) / kSubdivision;
	//    float v1 = float(latIndex + 1) / kSubdivision;

	//    // 頂点データ入力

	//    // a 左下
	//    vertexData[start].position.x = cosf(lat) * cosf(lon);
	//    vertexData[start].position.y = sinf(lat);
	//    vertexData[start].position.z = cosf(lat) * sinf(lon);
	//    vertexData[start].position.w = 1.0f;
	//    vertexData[start].texcoord = {u0, v0};
	//    vertexData[start].normal.x = vertexData[start].position.x;
	//    vertexData[start].normal.y = vertexData[start].position.y;
	//    vertexData[start].normal.z = vertexData[start].position.z;

	//    // c 右下
	//    vertexData[start + 1].position.x = cosf(lat) * cosf(lon + kLonEvery);
	//    vertexData[start + 1].position.y = sinf(lat);
	//    vertexData[start + 1].position.z = cosf(lat) * sinf(lon + kLonEvery);
	//    vertexData[start + 1].position.w = 1.0f;
	//    vertexData[start + 1].texcoord = {u1, v0};
	//    vertexData[start + 1].normal.x = vertexData[start + 1].position.x;
	//    vertexData[start + 1].normal.y = vertexData[start + 1].position.y;
	//    vertexData[start + 1].normal.z = vertexData[start + 1].position.z;

	//    // b 左上
	//    vertexData[start + 2].position.x = cosf(lat + kLatEvery) * cosf(lon);
	//    vertexData[start + 2].position.y = sinf(lat + kLatEvery);
	//    vertexData[start + 2].position.z = cosf(lat + kLatEvery) * sinf(lon);
	//    vertexData[start + 2].position.w = 1.0f;
	//    vertexData[start + 2].texcoord = {u0, v1};
	//    vertexData[start + 2].normal.x = vertexData[start + 2].position.x;
	//    vertexData[start + 2].normal.y = vertexData[start + 2].position.y;
	//    vertexData[start + 2].normal.z = vertexData[start + 2].position.z;

	//    // d 右上
	//    vertexData[start + 3].position.x =
	//        cosf(lat + kLatEvery) * cosf(lon + kLonEvery);
	//    vertexData[start + 3].position.y = sinf(lat + kLatEvery);
	//    vertexData[start + 3].position.z =
	//        cosf(lat + kLatEvery) * sinf(lon + kLonEvery);
	//    vertexData[start + 3].position.w = 1.0f;
	//    vertexData[start + 3].texcoord = {u1, v1};
	//    vertexData[start + 3].normal.x = vertexData[start + 3].position.x;
	//    vertexData[start + 3].normal.y = vertexData[start + 3].position.y;
	//    vertexData[start + 3].normal.z = vertexData[start + 3].position.z;

	//    // 頂点インデックスデータに入力
	//    indexData[start] = start + 0;
	//    indexData[start + 1] = start + 1;
	//    indexData[start + 2] = start + 2;
	//    indexData[start + 3] = start + 1;
	//    indexData[start + 4] = start + 3;
	//    indexData[start + 5] = start + 2;
	//  }
	//}

	//========================
	// 音声読み込み
	//========================

	SoundData soundData1 = SoundLoadWave("resources/fanfare.wav");
	// SoundData soundData1 = SoundLoadWave("resources/Alarm01.wav");

	//SoundPlayerWave(xAudio2.Get(), soundData1);

	//=============================
	// Instancing用リソース
	//=============================

	// const uint32_t kNumMaxInstance = 100; // インスタンス数
	//// Instancing用のTransformationMatrixリソースを作る
	// Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource =
	//     dx12Core->CreateBufferResource(sizeof(ParticleForGPU) *
	//     kNumMaxInstance);

	//// 書き込むためのアドレスを取得
	// ParticleForGPU *instancingData = nullptr;

	// instancingResource->Map(0, nullptr,
	//                         reinterpret_cast<void **>(&instancingData));

	// for (uint32_t index = 0; index < kNumMaxInstance; ++index) {
	//   instancingData[index].WVP = MakeIdentity4x4();
	//   instancingData[index].World = MakeIdentity4x4();
	//   instancingData[index].color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	// }

	//=============================
	// SRVの作成
	//=============================

	// D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	// instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	// instancingSrvDesc.Shader4ComponentMapping =
	//     D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	// instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	// instancingSrvDesc.Buffer.FirstElement = 0;
	// instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	// instancingSrvDesc.Buffer.NumElements = kNumMaxInstance;
	// instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

	//D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU =
	//    srvManager->GetCPUDescriptorHandle(
	//        3); // Heapの三番目に作成(空いているのであればどこでもOK)
	//D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU =
	//    srvManager->GetGPUDescriptorHandle(3);

	//device->CreateShaderResourceView(instancingResource.Get(), &instancingSrvDesc,
	//                                 instancingSrvHandleCPU);

	MSG msg{};

	// 乱数生成器
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	std::uniform_real_distribution<float> distribution(-1.0f,
		1.0f); //-1.0f~1.0f

	// Transform変数を作る
	// Particle particles[kNumMaxInstance];

	// for (uint32_t index = 0; index < kNumMaxInstance; ++index) {
	//   particles[index] = MakeNewParticle(randomEngine);
	// }

	// パーティクルエミッタ
	Emitter emitter{};
	emitter.transform.translate = { 0.0f, 0.0f, 0.0f };
	emitter.transform.rotate = { 0.0f, 0.0f, 0.0f };
	emitter.transform.scale = { 1.0f, 1.0f, 1.0f };
	emitter.count = 3;
	emitter.frequency = 0.5f;
	emitter.frequencyTime = 0.0f;

	std::list<Particle> particles;

	particles.push_back(
		MakeNewParticle(randomEngine, emitter.transform.translate));
	particles.push_back(
		MakeNewParticle(randomEngine, emitter.transform.translate));
	particles.push_back(
		MakeNewParticle(randomEngine, emitter.transform.translate));

	// カメラ行列
	// Transform cameraTransform{
	//    {1.0f, 1.0f, 1.0f},
	//    {std::numbers::pi_v<float> / 3.0f, std::numbers::pi_v<float>, 0.0f},
	//    {0.0f, 18.0f, 10.0f},
	//};

	// Sprite用のTransform
	Transform transformSprite{
		{1.0f, 1.0f, 1.0},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
	};

	// UVTransfotm用
	Transform uvTransformSprite{
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f},
	};

	// texture切り替え用
	bool useMonsterBall = true;

	// Lighting
	bool enableLighting = true;

	// LightingDirection用
	// Vector3 tempDirection = directionalLightData->direction;

	// デバッグカメラ
	DebugCamera* debugCamera = new DebugCamera();
	debugCamera->Initialize();

	// デルタタイム 一旦60fpsで固定
	const float kDeltaTime = 1.0f / 60.0f;

	bool isUpdate = false;

	bool useBillboard = true;

	bool set60FPS = false;

	// スプライトのTransform
	Vector2 spritePosition{
		640.0f,
		360.0f,
	};

	float spriteRotation = 0.0f;

	Vector4 spriteColor{ 1.0f, 1.0f, 1.0f, 1.0f };

	Vector2 spriteSize{ 640.0f, 360.0f };

	Vector2 spriteAnchorPoint{ 0.5f, 0.5f };

	bool isFlipX = false;
	bool isFlipY = false;

	sprite->SetPosition(spritePosition);
	sprite->SetRotation(spriteRotation);
	sprite->SetColor(spriteColor);
	sprite->SetSize(spriteSize);
	sprite->SetAnchorPoint(spriteAnchorPoint);
	sprite->SetIsFlipX(isFlipX);
	sprite->SetIsFlipY(isFlipY);

	float rotate = 0.0f;

	Transform cameraTransform{
		{1.0f, 1.0f, 1.0f},
		{0.3f, 0.0f, 0.0f},
		{0.0f, 4.0f, -10.0f},
	};

	// Vector2 spritePositions[kSpriteCount];

	// Vector2 spriteSizes[kSpriteCount];

	// Vector2 spriteAnchorPoints[kSpriteCount];

	// for (uint32_t i = 0; i < kSpriteCount; ++i) {
	//   spritePositions[i] = {i * 200.0f, 0.0f};
	//   spriteSizes[i] = {150.0f, 150.0f};
	// }

	// 板ポリをカメラに向ける回転行列
   // Matrix4x4 backToFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);

	// 風を出す範囲
	AccelerationField accelerationField;
	accelerationField.acceleration = { 15.0f, 0.0f, 0.0f };
	accelerationField.area.min = { -1.0f, -1.0f, -1.0f };
	accelerationField.area.max = { 1.0f, 1.0f, 1.0f };

	Transform emitterTransform{
  {1.0f, 1.0f, 1.0f},
  {0.0f, 0.0f, 0.0f},
  {0.0f, 0.0f, 5.0f} , };

	ParticleEmitter particleEmitter("test", emitterTransform, 3, 1.0f, 0.0f);

	// ウィンドウのxボタンが押されるまでループ
	while (true) {
		// Windowにメッセージが来てたら最優先で処理させる
		if (windowSystem->ProcessMessage()) {
			break;
		}

		// ImGui_ImplDX12_NewFrame();
		// ImGui_ImplWin32_NewFrame();
		// ImGui::NewFrame();

		// キー入力
		input->Update();

		// 確認用
		if (input->IsTriggerKey(DIK_0)) {
			OutputDebugStringA("Trigger 0\n");
		}

		// ゲームの処理

		//for (std::list<Particle>::iterator particleIterator = particles.begin();
		//	particleIterator != particles.end(); ++particleIterator) {
		//	// partic.transform.translate += particles.velocity * kDeltaTime;
		//	// particles.currentTime += kDeltaTime; // 経過時間を足す

		//	if (isUpdate) {
		//		if (IsCollision(accelerationField.area,
		//			(*particleIterator).transform.translate)) {
		//			(*particleIterator).velocity +=
		//				accelerationField.acceleration * kDeltaTime;
		//		}
		//	}

		//	(*particleIterator).transform.translate +=
		//		(*particleIterator).velocity * kDeltaTime;
		//	(*particleIterator).currentTime += kDeltaTime;
		//}

		//emitter.frequencyTime += kDeltaTime;

		//if (emitter.frequency <= emitter.frequencyTime) {
		//	particles.splice(particles.end(), Emit(emitter, randomEngine));
		//	emitter.frequencyTime -= emitter.frequency;
		//}

		// エミッタ更新
		particleEmitter.Update();

		// view / projection を作って ParticleManager 更新
		Matrix4x4 cameraMatrix = MakeAffineMatrix(
			cameraTransform.scale,
			cameraTransform.rotate,
			cameraTransform.translate
		);

		Matrix4x4 viewMatrix = Inverse(cameraMatrix);

		Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
			0.45f,
			float(WindowSystem::kClientWidth) / float(WindowSystem::kClientHeight),
			0.1f,
			100.0f
		);

		ParticleManager::GetInstance()->Update(viewMatrix, projectionMatrix);

		debugCamera->Update(*input);

		// 開発用UIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
		// ImGui::ColorEdit4("materialColor", &materialData->color.x);
		// ImGui::DragFloat3("translate", &transform.translate.x, 0.01f);
		// ImGui::SliderAngle("rotateX", &transform.rotate.x);
		// ImGui::SliderAngle("rotateY", &transform.rotate.y);
		// ImGui::SliderAngle("rotateZ", &transform.rotate.z);
		//  ImGui::Checkbox("useMonsterBall", &useMonsterBall);
		// ImGui::Checkbox("enableLighting", &enableLighting);
		//  ImGui::Checkbox("Update", &isUpdate);
		//  ImGui::Checkbox("useBillboard", &useBillboard);
		// ImGui::Checkbox("set60FPS", &set60FPS);

		// FPSをセット
		dx12Core->SetFPS(set60FPS);

		// スプライトの情報を呼び出す
		spritePosition = sprite->GetPosition();
		spriteRotation = sprite->GetRotation();
		spriteColor = sprite->GetColor();
		spriteSize = sprite->GetSize();
		spriteAnchorPoint = sprite->GetAnchorPoint();

		// if (ImGui::Button("Add Particle")) {
		//   particles.splice(particles.end(), Emit(emitter, randomEngine));
		//  }

		// ImGui::DragFloat3("EmitterTranslate", &emitter.transform.translate.x,
		// 0.01f,
		//            -100.0f, 100.0f);

		// ImGui::ColorEdit3("LightingColor", &directionalLightData->color.x);
		// ImGui::DragFloat3("Lighting Direction", &tempDirection.x, 0.01f, -1.0f,
		//      1.0f);
		// ImGui::DragFloat("Intensity", &directionalLightData->intensity, 0.01f,
		// 0.0f,
		//    10.0f);
		// ImGui::DragFloat3("CameraTranslate", &cameraTransform.translate.x,
		// 0.01f); ImGui::DragFloat3("CameraRotate", &cameraTransform.rotate.x,
		// 0.001f);

		// ImGui::ColorEdit4("SpriteColor", &spriteColor.x);

		// ImGui::DragFloat2("SpritePosition", &spritePosition.x, 1.0f);
		// ImGui::DragFloat("SpriteRotation", &spriteRotation, 0.01f);
		// ImGui::DragFloat2("SpriteSize", &spriteSize.x, 1.0f);
		// ImGui::DragFloat2("SpriteAnchorPoint", &spriteAnchorPoint.x, 0.01f);
		// ImGui::Checkbox("isFlipX", &isFlipX);
		// ImGui::Checkbox("isFlipY", &isFlipY);

		// ImGui::DragFloat3("UVTranslate", &uvTransformSprite.translate.x, 0.01f,
		//                   -10.0f, 10.0f);
		// ImGui::DragFloat3("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f,
		//                   10.0f);
		// ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);

		// materialData->enableLighting = enableLighting;
		// directionalLightData->direction = Normalize(tempDirection);

		// スプライトに設定
		sprite->SetPosition(spritePosition);
		sprite->SetRotation(spriteRotation);
		sprite->SetColor(spriteColor);
		sprite->SetSize(spriteSize);
		sprite->SetAnchorPoint(spriteAnchorPoint);
		sprite->SetIsFlipX(isFlipX);
		sprite->SetIsFlipY(isFlipY);

		// テクスチャ差し替え
		if (input->IsTriggerKey(DIK_SPACE)) {
			sprites[0]->ChangeTexture("resources/uvChecker.png");
		}

		rotate += 0.01f;

		object3d->SetRotation({ 0.0f, rotate, 0.0f });

		object3dA->SetRotation({ 0.0f, rotate, 0.0f });
		object3dA->SetTranslation({ 1.0f, 1.0f, 0.0f });

		// for (uint32_t i = 0; i < kSpriteCount; ++i) {
		//   // spritePosition[i].x++;
		//   sprites[i]->SetPosition(spritePositions[i]);
		//   sprites[i]->SetSize(spriteSizes[i]);
		// }

		// ImGuiの内部コマンドを生成する
		// ImGui::Render();

		// for (int i = 0; i < 10; i++) {
		/*Matrix4x4 worldMatrix = MakeAffineMatrix(
			transforms[i].scale, transforms[i].rotate, transforms[i].translate);*/
			// Matrix4x4 cameraMatrix =
			//     MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate,
			//                      cameraTransform.translate);
			// Matrix4x4 viewMatrix = Inverse(cameraMatrix);
			//  Matrix4x4 viewMatrix = debugCamera->GetViewMatrix();
			// Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(
			//     0.45f,
			//     float(WindowSystem::kClientWidth) /
			//     float(WindowSystem::kClientHeight), 0.1f, 100.0f);
			/*Matrix4x4 worldViewProjectionMatrix =
				Multiply(Multiply(worldMatrix, viewMatrix), projectionMatrix);*/

				/*  transformationMatrixData[i]->WVP = worldViewProjectionMatrix;
				  transformationMatrixData[i]->World = worldMatrix;
				}*/

				// Matrix4x4 billboardMatrix = Multiply(backToFrontMatrix, cameraMatrix);
				// billboardMatrix.m[3][0] = 0.0f; // 平行移動成分は無し
				// billboardMatrix.m[3][1] = 0.0f;
				// billboardMatrix.m[3][2] = 0.0f;

				// uint32_t numInstance = 0; // 描画すべきインスタンス数
				// for (std::list<Particle>::iterator particleIterator = particles.begin();
				//      particleIterator != particles.end();) {

				//  if ((*particleIterator).lifeTime <= (*particleIterator).currentTime) {
				//    particleIterator = particles.erase(
				//        particleIterator); //
				//        生存期間が過ぎたParticleはListから消す。戻り値が次のイテレータとなる
				//    continue;
				//  }

				//  float alpha =
				//      1.0f - (particleIterator->currentTime / particleIterator->lifeTime);

				//  Matrix4x4 worldMatrix = MakeIdentity4x4();

				//  if (useBillboard) {

				//    Matrix4x4 scaleMatrix =
				//        MakeScaleMatrix(particleIterator->transform.scale);

				//    Matrix4x4 translateMatrix =
				//        MakeTranslateMatrix(particleIterator->transform.translate);

				//    worldMatrix =
				//        Multiply(Multiply(scaleMatrix, billboardMatrix), translateMatrix);
				//  } else {
				//    worldMatrix = MakeAffineMatrix(particleIterator->transform.scale,
				//                                   particleIterator->transform.rotate,
				//                                   particleIterator->transform.translate);
				//  }

				//  Matrix4x4 worldViewProjectionMatrix =
				//      Multiply(Multiply(worldMatrix, viewMatrix), projectionMatrix);

				//  if (numInstance < kNumMaxInstance) {

				//    instancingData[numInstance].WVP = worldViewProjectionMatrix;
				//    instancingData[numInstance].World = worldMatrix;
				//    instancingData[numInstance].color = (*particleIterator).color;
				//    instancingData[numInstance].color.w = alpha;

				//    ++numInstance;
				//  }

				//  ++particleIterator; // 生きているParticleの数を1つカウントする
				//}

				// cameraTransform = {
				//     {1.0f, 1.0f, 1.0f}, camera->GetRotate(), camera->GetTranslate()};
				// cameraTransform.translate.x += 0.01f;

		camera->SetRotate(cameraTransform.rotate);
		camera->SetTranslate(cameraTransform.translate);

		// カメラの更新処理
		camera->Update();

		// ３Dオブジェクトの更新処理
		object3d->Update();
		object3dA->Update();

		// スプライトの更新処理
		sprite->Update(uvTransformSprite);

		for (uint32_t i = 0; i < kSpriteCount; ++i) {
			sprites[i]->Update(uvTransformSprite);
		}

		// DirectXの描画準備。すべてに共通のグラフィックスコマンドを積む
		dx12Core->BeginFrame();

		// SRVをコマンドリストにセット
		srvManager->PreDraw();

		// RootSignatureを設定。PSOに設定しているけど別途設定が必要
		// commandList->SetGraphicsRootSignature(rootSignatureParticle.Get());
		// commandList->SetPipelineState(
		// graphicsPipeLineStateParticle.Get());                 // PSOを設定
		// commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定

		// 形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけばいい
		// commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// commandList->IASetIndexBuffer(&indexBufferView);

		//// マテリアルCBufferの場所を設定
		// commandList->SetGraphicsRootConstantBufferView(
		//     0, materialResource->GetGPUVirtualAddress());

		//// wvp用のCBufferの場所を設定
		// commandList->SetGraphicsRootDescriptorTable(1, instancingSrvHandleGPU);

		//// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
		// commandList->SetGraphicsRootDescriptorTable(
		//     2, useMonsterBall ? TextureManager::GetInstance()->GetSrvHandleGPU(1)
		//                       :
		//                       TextureManager::GetInstance()->GetSrvHandleGPU(0));

		// Lighting
			  // commandList->SetGraphicsRootConstantBufferView(
			//	   3, directionalLightResource->GetGPUVirtualAddress());
				   // }
				   // 描画(DrawCall/ドローコール)。3頂点で1つのインスタンス
				   // commandList->DrawInstanced(UINT(modelData.vertices.size()), numInstance,
				   // 0,
				   //          0);
				   // commandList->DrawIndexedInstanced(1536, 1, 0, 0, 0);

				   // 3DObjectの描画準備。3DObjectの描画に共通のグラフィックスコマンドを積む
		object3dRenderer->Begin();

		// ３Dオブジェクトの描画
		object3d->Draw();
		object3dA->Draw();

		ParticleManager::GetInstance()->Draw();

		// Spriteの描画準備。Spriteの描画に共通のグラフィックスコマンドを積む
		spriteRenderer->Begin();

		// スプライトの描画
		// sprite->Draw();

		// for (uint32_t i = 0; i < kSpriteCount; ++i) {
		//   sprites[i]->Draw();
		// }

		// 実際のcommandListのImGuiの描画コマンドを積む
		// ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
		//                              dx12Core->GetCommandList());

		dx12Core->EndFrame();
	}

	// ImGuiの終了処理
	// ImGui_ImplDX12_Shutdown();
	// ImGui_ImplWin32_Shutdown();
	// ImGui::DestroyContext();

	// 解放処理
	// CloseHandle(fenceEvent);
	// signatureBlob->Release();
	// signatureBlobPart->Release();
	// if (errorBlob) {
	//  errorBlob->Release();
	//}
	if (errorBlobPart) {
		errorBlobPart->Release();
	}
	pixelShaderBlob->Release();
	vertexShaderBlob->Release();
	particleVertexShaderBlob->Release();
	particlePixelShaderBlob->Release();

	xAudio2.Reset();

	delete debugCamera;

	ModelManager::GetInstance()->Finalize();

	TextureManager::GetInstance()->Finalize();

	ParticleManager::GetInstance()->Finalize();

	delete input;

	delete object3dA;

	delete object3d;

	delete sprite;

	for (uint32_t i = 0; i < kSpriteCount; ++i) {
		delete sprites[i];
	}

	delete srvManager;
	srvManager = nullptr;

	delete object3dRenderer;

	delete spriteRenderer;

	delete dx12Core;

	windowSystem->Finalize();

	delete windowSystem;
	windowSystem = nullptr;

	SoundUnload(&soundData1);

	return 0;
}
