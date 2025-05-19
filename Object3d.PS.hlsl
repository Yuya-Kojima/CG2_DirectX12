#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
	int enableLighting;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct DirectionalLight {
	float4 color;
	float3 direction;
	float intensity;
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(
VertexShaderOutput input) {
	float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
	PixelShaderOutput output;
	output.color = gMaterial.color * textureColor;
	
	if (gMaterial.enableLighting != 0) {
		//Lightingする場合
		float cos = saturate(dot(normalize(input.normal), -gDirectionalLight.direction));
		output.color = gMaterial.color * textureColor * gDirectionalLight.color * cos * gDirectionalLight.intensity;
	} else {
		//Lightingしない場合
		output.color = gMaterial.color * textureColor;
	}
	
	return output;
}




//float4 main() : SV_TARGET
//{
//	return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}