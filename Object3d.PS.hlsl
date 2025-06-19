#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
	int enableLighting;
	float4x4 uvTransform;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct DirectionalLight {
	float4 color;
	float3 direction;
	float intensity;
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

cbuffer TimeBC : register(b2) {
	float4 gTime;
}

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
	float2 uv = input.texcoord;
	
	// UVにゆらぎを追加（例：炎風）
	float time = gTime.x; // float4からx成分だけ使う
	uv.x += sin(uv.y * 10.0f + gTime.x * 8.0f) * 0.08f;
	uv.y += cos(uv.x * 8.0f + gTime.x * 4.0f) * 0.05f;
	uv = saturate(uv);
	
	float4 transformedUV = mul(float4(uv, 0.0f, 1.0f), gMaterial.uvTransform);
	
	float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	textureColor.a = 1.0f;
	
	PixelShaderOutput output;
	
	if (gMaterial.enableLighting != 0) {
		// half-lambert 簡易ライティング
		float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
		float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
		output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
	} else {
		output.color = gMaterial.color * textureColor;
	}
	
	return output;
	
	//PixelShaderOutput output;
	//float t = gTime.x;
	//output.color = float4(sin(t), 0.0f, 0.0f, 1.0f); // 時間経過で赤が変わる
	//return output;
}