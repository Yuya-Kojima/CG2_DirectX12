#include "Sprite.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
	float4x4 uvTransform;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(
VertexShaderOutput input) {
	
	//=========================
    // UV transform & sampling
    //=========================
	float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	
	//アルファテスト
	if (textureColor.a <= 0.5) {
		discard;
	}
	
	//=========================
	// Output
	//=========================
	PixelShaderOutput output;
	output.color = gMaterial.color * textureColor;
	
	return output;
}

