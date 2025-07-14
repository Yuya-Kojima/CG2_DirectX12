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


PixelShaderOutput main(PixelShaderInput input) {
	float4 transformdUV = mul(float4(input.texcoord, 0.0, 1.0), gMaterial.uvTransform);
	float4 textureColor = gTexture.Sample(gSampler, input.texcoord);

	
	PixelShaderOutput output;
	
	
	output.color = gMaterial.color * textureColor * input.color;
	
	if (output.color.a == 0.0) {
		discard;
	}
	
	return output;
}