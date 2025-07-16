#include "Particle.hlsli"

Texture2D<float4> gTexture : register(t1);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderInput {
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR0;
};

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

cbuffer TimeBC : register(b1) {
	float4 gTime;
}


float hash(float2 p) {
	return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float Noise(float2 uv) {
	float2 i = floor(uv);
	float2 f = frac(uv);
	float a = hash(i);
	float b = hash(i + float2(1.0, 0.0));
	float c = hash(i + float2(0.0, 1.0));
	float d = hash(i + float2(1.0, 1.0));
	float2 u = f * f * (3.0 - 2.0 * f);
	return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

PixelShaderOutput main(PixelShaderInput input) {
	float2 uv = input.texcoord;
	float time = gTime.x + 0.3; //+0.3はテキストより先に炎が出るようにするため
	PixelShaderOutput output;
	
	//二重ノイズでテクスチャを歪ませる
	float n1 = Noise(uv * 10 + time * 0.5);
	float n2 = Noise(uv * 30 - time * 0.8) * 0.5;
	
	//ノイズを合成
	float noise = n1 + n2;
	uv.x += (noise - 0.5f) * 0.2f;
	uv.y += (noise - 0.5f) * 0.4f;
	uv = saturate(uv);
	
	float4 textureColor = gTexture.Sample(gSampler, uv);
	
	//reveal進行度
	float reveal = saturate((time * 0.5) - (uv.y));
	
	//alpha値にrevealの進行度を乗算してフェードインさせる
	textureColor.a *= reveal;
	
	//出力
	output.color = gMaterial.color * textureColor * input.color;
	if (output.color.a == 0.0) {
		discard;
	}
	
	//output.color.a = 0.0;
	
	return output;
}