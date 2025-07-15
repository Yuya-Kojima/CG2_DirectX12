#include "Particle.hlsli"

Texture2D<float4> gTexture : register(t1);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
	int enableLighting;
	float4x4 uvTransform;
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

float fnoise(float2 uv) {
	float v = 0, amp = 1, freq = 1;
	for (int i = 0; i < 4; ++i) {
		v += noise(uv * freq) * amp;
		freq *= 2;
		amp *= 0.5;
	}
	return v;
}

PixelShaderOutput main(PixelShaderInput input) {
	float2 uv = input.texcoord;
	float time = gTime.x + 0.5;
	PixelShaderOutput output;
	
	float n1 = noise(uv * 10 + time * 0.5);
	float n2 = noise(uv * 30 - time * 0.8) * 0.5;
	float n = n1 + n2;
	uv.x += (n - 0.5f) * 0.2f;
	uv.y += (n - 0.5f) * 0.4f;
	uv = saturate(uv);

	//新しい
	//float4 textureColor = gTexture.Sample(gSampler, uv) * input.color;
	
	//float2 ncoord = uv * float2(1.0, 2.0) + float2(0.0, time * 0.8);
	//float m = fnoise(ncoord);
	
	//float mask = smoothstep(0.4, 0.6, m);
	//textureColor.a *= mask;
	
	//float reveal = saturate(time * 0.5 - uv.y);
	//textureColor.a *= reveal;
	//if (textureColor.a < 0.01) {
	//	discard;
	//}
	//output.color = gMaterial.color * textureColor;
	
	//既存の
	float4 textureColor = gTexture.Sample(gSampler, uv);
	float reveal = saturate((time * 0.5) - (uv.y));
	
	textureColor.a *= reveal;
	output.color = gMaterial.color * textureColor * input.color;
	if (output.color.a == 0.0) {
		discard;
	}
	
	return output;
}