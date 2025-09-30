#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
	int enableLighting;
	float4x4 uvTransform;
	float shininess;
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

struct Camera {
	float3 worldPosition;
};

ConstantBuffer<Camera> gCamera : register(b2);

PixelShaderOutput main(
VertexShaderOutput input) {
	float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	PixelShaderOutput output;
	
	float3 toEye = normalize(gCamera.worldPosition - input.worldPositoin);
	
	float3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal));
	
	float RdotE = dot(reflectLight, toEye);
	float specularPow = pow(saturate(RdotE), gMaterial.shininess);
	
	
	if (textureColor.a <= 0.5) {
		discard;
	}
	
	if (textureColor.a == 0.0) {
		discard;
	}
	
	if (output.color.a == 0.0) {
		discard;
	}
	
	if (gMaterial.enableLighting != 0) {
		//Lightingする場合
		float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
		float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
		
		//==========================================
		//鏡面反射
		
		float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
		
		float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
		
		output.color.rgb = diffuse + specular;
      
		//=================================================		
		//Harflambert
		
		//output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
	
		//=================================================
		
		output.color.a = gMaterial.color.a * textureColor.a;
	} else {
		//Lightingしない場合
		output.color = gMaterial.color * textureColor;
	}
	
	return output;
}

