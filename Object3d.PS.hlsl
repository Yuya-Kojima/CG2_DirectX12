#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
	float4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderOutput
{
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(
VertexShaderOutput input)
{
	float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
	PixelShaderOutput output;
	output.color = gMaterial.color * textureColor;
	
	return output;
}




//float4 main() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}