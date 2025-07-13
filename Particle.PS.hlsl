#include "Particle.hlsli"

Texture2D<float4> gTexture : register(t1);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
	int enableLighting;
	float4x4 uvTransform;
};



//ConstantBuffer<Material> gMaterial : register(b0);

cbuffer FXParams : register(b0) {
	float gTime;
	float flameSpeed;
	float flameNoiseScale;
	float flameIntensity;
	float revealSpeed;
	float revealWidth;
	float flameDuration;
};

struct PSOutput {
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


struct DirectionalLight {
	float4 color;
	float3 direction;
	float intensity;
};

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(
VertexShaderOutput input) {
	//float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	//float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	//PixelShaderOutput output;
	//output.color = gMaterial.color * textureColor * input.color;
	
	//if (output.color.a == 0.0) {
	//	discard;
	//}
	
	PSOutput output;
	float2 uv = input.texcoord;
	float w = input.color.w; // インスタンスごとの「レイヤー判別値」
	float t = gTime;

    // 1) リビール炎
	if (w >= 0.85f) {
        // ノイズベースの色変化
		float n = noise(uv * flameNoiseScale + float2(0, t * flameSpeed));
		float3 fireCol = lerp(float3(1, 0.4, 0), float3(1, 1, 0), n) * flameIntensity;

        // フェードアウト
		float fade = saturate((flameDuration - t) / flameDuration);

		float4 tex = gTexture.Sample(gSampler, uv);
		tex.rgb *= fireCol;
		tex.a *= fade;

		output.color = tex;
		return output;
	}
    // 2) 炎のグロー
	else if (w >= 0.65f) {
        // リビール量
		float reveal = smoothstep(0.0f, revealWidth, t * revealSpeed - uv.y);

        // 5×5 サンプルでぼかし
		float2 texel = 1.0f / 256.0f;
		float sumA = 0;
        [unroll]
		for (int y = -2; y <= 2; ++y)
			for (int x = -2; x <= 2; ++x)
				sumA += gTexture.Sample(gSampler, uv + float2(x, y) * texel).a;

		float glow = (sumA / 25.0f) * reveal;
		output.color = float4(1.2, 1.0, 0.4, glow * flameIntensity);
		return output;
	}

    // 3) それ以外は透明
	output.color = float4(0, 0, 0, 0);
	return output;
}