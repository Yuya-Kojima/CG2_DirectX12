#include "Particle.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

ConstantBuffer<Material> gMaterial : register(b1);

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
	float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
	PixelShaderOutput output;
	output.color = gMaterial.color * textureColor * input.color;
	
	// 計算結果がNaNやInfになったピクセルは描画を破棄する
	if (any(isnan(output.color)) || any(isinf(output.color))) {
		discard;
	}
	
	if (output.color.a == 0.0) {
		discard;
	}
	return output;
}

