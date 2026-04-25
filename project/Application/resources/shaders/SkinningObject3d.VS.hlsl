#include "Object3d.hlsli"

struct TransformationMatrix {
	float4x4 WVP;
	float4x4 World;
	float4x4 WorldInverseTranspose;
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// GPU側のWellの定義と、MatrixPaletteを追加
struct Well {
	float4x4 skeletonSpaceMatrix;
	float4x4 skeletonSpaceInverseTransposeMatrix;
};

StructuredBuffer<Well> gMatrixPalette : register(t0);

// WeightとJointIndexを受け取れるように入力頂点を拡張
struct VertexShaderInput {
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
	float3 normal : NORMAL0;
	float4 weight : WEIGHT0;
	int4 index : INDEX0;
};

// Skinningの結果を受け取るための構造体を作成
struct Skinned {
	float4 position;
	float3 normal;
};

// Skinningを行う関数
Skinned Skinning(VertexShaderInput input) {
	Skinned skinned;
	
	// 位置の変換
	skinned.position  = mul(input.position, gMatrixPalette[input.index.x].skeletonSpaceMatrix) * input.weight.x;
	skinned.position += mul(input.position, gMatrixPalette[input.index.y].skeletonSpaceMatrix) * input.weight.y;
	skinned.position += mul(input.position, gMatrixPalette[input.index.z].skeletonSpaceMatrix) * input.weight.z;
	skinned.position += mul(input.position, gMatrixPalette[input.index.w].skeletonSpaceMatrix) * input.weight.w;
	skinned.position.w = 1.0f; // 確実に1を入れる
	
	// 法線の変換
	skinned.normal  = mul(input.normal, (float3x3)gMatrixPalette[input.index.x].skeletonSpaceInverseTransposeMatrix) * input.weight.x;
	skinned.normal += mul(input.normal, (float3x3)gMatrixPalette[input.index.y].skeletonSpaceInverseTransposeMatrix) * input.weight.y;
	skinned.normal += mul(input.normal, (float3x3)gMatrixPalette[input.index.z].skeletonSpaceInverseTransposeMatrix) * input.weight.z;
	skinned.normal += mul(input.normal, (float3x3)gMatrixPalette[input.index.w].skeletonSpaceInverseTransposeMatrix) * input.weight.w;
	skinned.normal = normalize(skinned.normal); // 正規化して戻してあげる
	
	return skinned;
}

VertexShaderOutput main(VertexShaderInput input) {
	VertexShaderOutput output;
	
	// まずSkinning計算を行って、Skinning後の頂点情報を手に入れる。ここでの頂点もSkeletonSpace
	Skinned skinned = Skinning(input);
	
	// Skinning結果を使って変換
	output.position = mul(skinned.position, gTransformationMatrix.WVP);
	output.worldPosition = mul(skinned.position, gTransformationMatrix.World).xyz;
	output.texcoord = input.texcoord;
	output.normal = normalize(mul(skinned.normal, (float3x3)gTransformationMatrix.WorldInverseTranspose));
	
	return output;
}
