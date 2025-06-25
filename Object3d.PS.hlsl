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

float hash(float2 p) {
	return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float noise(float2 uv) {
	float2 i = floor(uv);
	float2 f = frac(uv);
	float a = hash(i);
	float b = hash(i + float2(1.0, 0.0));
	float c = hash(i + float2(0.0, 1.0));
	float d = hash(i + float2(1.0, 1.0));
	float2 u = f * f * (3.0 - 2.0 * f);
	return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

PixelShaderOutput main(VertexShaderOutput input) {
	float2 uv = input.texcoord;
	
	// UVにゆらぎを追加（例：炎風）
	float time = gTime.x; // float4からx成分だけ使う
	float n = noise(uv * 10.0f + time * 0.5f);
	uv.x += (n - 0.5f) * 0.05f;
	uv.y += (n - 0.5f) * 0.05f;
	//uv.x += sin(uv.y * 10.0f + gTime.x * 8.0f) * 0.08f;
	//uv.y += cos(uv.x * 8.0f + gTime.x * 4.0f) * 0.05f;
	uv = saturate(uv);
	
	float4 transformedUV = mul(float4(uv, 0.0f, 1.0f), gMaterial.uvTransform);
	
	float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	//textureColor.a = 1.0f;
	
	
	float fireColorRate = sin(gTime.x * 5.0f + uv.y * 5.0f) * 0.5f + 0.5f;
	float3 fireColor = lerp(float3(1.0, 0.4, 0.0), float3(1.0, 1.0, 0.0), fireColorRate); // 赤→黄
	
	textureColor.rgb *= fireColor;
	//textureColor.a *= 0.6 + 0.4 * sin(gTime.x * 6.0f + uv.y * 10.0f);
	
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