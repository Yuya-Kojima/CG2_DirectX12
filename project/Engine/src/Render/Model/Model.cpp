#include "Model/Model.h"
#include "Debug/Logger.h"
#include "Math/MathUtil.h"
#include "Renderer/ModelRenderer.h"
#include "Texture/TextureManager.h"
#include <fstream>

void Model::Initialize(ModelRenderer* modelRenderer,
	const std::string& directorypath,
	const std::string& filename) {

	modelRenderer_ = modelRenderer;

	dx12Core_ = modelRenderer_->GetDx12Core();

	LoadModelFile(directorypath, filename);

	CreateVertexData();

	defaultMaterial_.color = Vector4(1, 1, 1, 1);
	defaultMaterial_.enableLighting = true;
	defaultMaterial_.uvTransform = MakeIdentity4x4();
	defaultMaterial_.shininess = 30.0f;

	// objデータが参照しているテクスチャ読み込み
	TextureManager::GetInstance()->LoadTexture(
		modelData_.material.textureFilePath);

	// 読み込んだテクスチャの番号を取得
	// modelData_.material.textureIndex =
	//    TextureManager::GetInstance()->GetTextureIndexByFilePath(
	//        modelData_.material.textureFilePath);
}
void Model::Draw() {

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList =
		dx12Core_->GetCommandList();

	commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // VBVを設定

	// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
	commandList->SetGraphicsRootDescriptorTable(
		2, TextureManager::GetInstance()->GetSrvHandleGPU(
			modelData_.material.textureFilePath));

	// 描画(DrawCall/ドローコール)。3頂点で1つのインスタンス
	commandList->DrawInstanced(UINT(modelData_.vertices.size()), 1, 0, 0);
}

Model::MaterialData
Model::LoadMaterialTemplateFile(const std::string& directoryPath,
	const std::string& filename) {
	// 1. 中で必要となる変数の宣言
	MaterialData materialData; // 構築するMaterialData
	std::string line;          // ファイルから読んだ一行を格納する

	// 2. ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open()); // とりあえず開けなかったら止める

	// 3. 実際にファイルを読み、MaterialDataを構築していく
	while (std::getline(file, line)) {

		std::string identifier;
		std::istringstream s(line);

		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	// 4. MaterialDataを返す
	return materialData;
}

void Model::LoadModelFile(const std::string& directoryPath,
	const std::string& filename) {

	// assimp読み込み
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;

	// 1. 中で必要となる変数の宣言
	ModelData modelData; // 構築するModelData

	const aiScene* scene = importer.ReadFile(
		filePath.c_str(),
		aiProcess_Triangulate | aiProcess_FlipWindingOrder | aiProcess_FlipUVs);

	assert(scene);
	assert(scene->HasMeshes());

	modelData.rootNode = ReadNode(scene->mRootNode);

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];

		assert(mesh->HasNormals());        // 法線がないMeshは今回は非対応
		assert(mesh->HasTextureCoords(0)); // TexcoordがないMeshは今回は非対応

		// ここから Mesh の中身（Face）を解析していく
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // 三角形のみサポート

			// ここから Face の中身（Vertex）の解析を行っていく
			for (uint32_t element = 0; element < face.mNumIndices; ++element) {
				uint32_t vertexIndex = face.mIndices[element];

				aiVector3D position = mesh->mVertices[vertexIndex];
				aiVector3D normal = mesh->mNormals[vertexIndex];
				aiVector3D texcoord = mesh->mTextureCoords[0][vertexIndex];

				VertexData vertex;
				vertex.position = { position.x, position.y, position.z, 1.0f };
				vertex.normal = { normal.x, normal.y, normal.z };
				vertex.texcoord = { texcoord.x, texcoord.y };

				// aiProcess_MakeLeftHanded は z *= -1
				// で、右手->左手に変換するので手動で対処
				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;

				modelData.vertices.push_back(vertex);
			}
		}
	}

	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials;
		++materialIndex) {
		aiMaterial* material = scene->mMaterials[materialIndex];

		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);

			modelData.material.textureFilePath =
				directoryPath + "/" + textureFilePath.C_Str();
		}
	}

	// std::vector<Vector4> positions; // 位置
	// std::vector<Vector3> normals;   // 法線
	// std::vector<Vector2> texcoords; // テクスチャ座標
	// std::string line;               // ファイルから読んだ一行を格納する

	// VertexData triangle[3];

	//// 2. ファイルを開く
	// std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	// assert(file.is_open()); // とりあえず開けなかったら止める

	//// 3. 実際にファイルを読み、ModelDataを構築していく
	// while (std::getline(file, line)) {
	//   std::string identifier;
	//   std::istringstream s(line);
	//   s >> identifier; // 先頭の識別子を読む

	//  if (identifier == "v") {
	//    Vector4 position;

	//    s >> position.x >> position.y >> position.z;

	//    position.w = 1.0f;
	//    positions.push_back(position);
	//  } else if (identifier == "vt") {
	//    Vector2 texcoord;

	//    s >> texcoord.x >> texcoord.y;

	//    texcoords.push_back(texcoord);
	//  } else if (identifier == "vn") {
	//    Vector3 normal;

	//    s >> normal.x >> normal.y >> normal.z;

	//    normals.push_back(normal);
	//  } else if (identifier == "f") {

	//    // 面は三角形限定
	//    for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
	//      std::string vertexDefinition;
	//      s >> vertexDefinition;

	//      //
	//      頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得する
	//      std::istringstream v(vertexDefinition);

	//      uint32_t elementIndices[3];

	//      for (int32_t element = 0; element < 3; ++element) {
	//        std::string index;
	//        std::getline(v, index, '/'); // 区切りでインデックスを読んでいく
	//        elementIndices[element] = std::stoi(index);
	//      }

	//      // 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
	//      Vector4 position = positions[elementIndices[0] - 1];
	//      Vector2 texcoord = texcoords[elementIndices[1] - 1];
	//      Vector3 normal = normals[elementIndices[2] - 1];

	//      // 右手座標なので反転
	//      position.x *= -1.0f;
	//      normal.x *= -1.0f;
	//      texcoord.y = 1.0f - texcoord.y;

	//      triangle[faceVertex] = {position, texcoord, normal};
	//    }

	//    // 頂点を逆順で登録することで、回り順を逆にする
	//    modelData.vertices.push_back(triangle[2]);
	//    modelData.vertices.push_back(triangle[1]);
	//    modelData.vertices.push_back(triangle[0]);
	//  } else if (identifier == "mtllib") {

	//    // materialTemplateLibraryファイルの名前を取得
	//    std::string materialFilename;

	//    s >> materialFilename;

	//    //
	//    基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
	//    modelData.material =
	//        LoadMaterialTemplateFile(directoryPath, materialFilename);
	//  }
	//}

	// 4. ModelDataを返す
	modelData_ = modelData;
}

void Model::CreateVertexData() {
	// 頂点リソースを作る
	vertexResource = dx12Core_->CreateBufferResource(sizeof(VertexData) *
		modelData_.vertices.size());

	// リソースの先頭アドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();

	// 使用するリソースのサイズは頂点のサイズ
	vertexBufferView.SizeInBytes =
		UINT(sizeof(VertexData) * modelData_.vertices.size());

	// 1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	std::memcpy(vertexData, modelData_.vertices.data(),
		sizeof(VertexData) * modelData_.vertices.size());
}

Model::Node Model::ReadNode(aiNode* node) {

	Node result{};

	// nodeのLocalMatrixを取得
	aiMatrix4x4 aiLocalMatrix = node->mTransformation;

	// 列ベクトル形式 → 行ベクトル形式（スライド通り）
	aiLocalMatrix.Transpose();

	// aiMatrix4x4 → Matrix4x4 にコピー
	result.localMatrix.m[0][0] = aiLocalMatrix[0][0];
	result.localMatrix.m[0][1] = aiLocalMatrix[0][1];
	result.localMatrix.m[0][2] = aiLocalMatrix[0][2];
	result.localMatrix.m[0][3] = aiLocalMatrix[0][3];

	result.localMatrix.m[1][0] = aiLocalMatrix[1][0];
	result.localMatrix.m[1][1] = aiLocalMatrix[1][1];
	result.localMatrix.m[1][2] = aiLocalMatrix[1][2];
	result.localMatrix.m[1][3] = aiLocalMatrix[1][3];

	result.localMatrix.m[2][0] = aiLocalMatrix[2][0];
	result.localMatrix.m[2][1] = aiLocalMatrix[2][1];
	result.localMatrix.m[2][2] = aiLocalMatrix[2][2];
	result.localMatrix.m[2][3] = aiLocalMatrix[2][3];

	result.localMatrix.m[3][0] = aiLocalMatrix[3][0];
	result.localMatrix.m[3][1] = aiLocalMatrix[3][1];
	result.localMatrix.m[3][2] = aiLocalMatrix[3][2];
	result.localMatrix.m[3][3] = aiLocalMatrix[3][3];

	// Node名を格納
	result.name = node->mName.C_Str();

	// 子供の数だけ確保
	result.children.resize(node->mNumChildren);

	// 再帰的に読んで階層構造を作る
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}

	return result;
}
