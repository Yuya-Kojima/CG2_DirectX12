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

struct PointLight {
	float4 color;
	float3 position;
	float intensity;
};

ConstantBuffer<PointLight> gPointLight : register(b3);

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
    // Base color
    //=========================
	float4 base = gMaterial.color * textureColor;
	
	PixelShaderOutput output;
	
    //=========================
    // No lighting
    //=========================
	if (gMaterial.enableLighting == 0) {
		output.color = base;
		return output;
	}
	
	//=========================
	// Common vectors
	//=========================
	float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);

	float3 normal = normalize(input.normal);
		
	//float3 lightDir = normalize(-gDirectionalLight.direction); // 表面→光
	//float3 reflectLight = reflect(gDirectionalLight.direction, normalize(input.normal));
	
	//float RdotE = dot(reflectLight, toEye);
		
	//=========================
	// Diffuse
	//=========================
	float NdotL = dot(normal, -gDirectionalLight.direction);
	float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
	
	// Lambert
	float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
		
	//Harflambert
		
	//output.color.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
	
	//=========================
	// Specular
	//=========================
	float specularPow = 0.0f;
	
	//Phong Reflection
	//float3 reflectLight = reflect(-lightDir, normal);
	
	//float RdotE = dot(reflectLight, toEye);
	
	//specularPow = pow(saturate(RdotE), gMaterial.shininess);
      
	//Bllin-Phong 
	float3 halfVector = normalize(-gDirectionalLight.direction + toEye);
		
	float NDotH = dot(normal, halfVector);
		
	specularPow = pow(saturate(NDotH), gMaterial.shininess);
		
	//計算
	float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow;
		
	//=========================
	// PointLight
	//=========================
	
	float3 pointLightDirection = normalize(input.worldPosition - gPointLight.position);
	
	float NdotLPoint = dot(normal, -pointLightDirection);
	float cosPoint = pow(NdotLPoint * 0.5f + 0.5f, 2.0f);
	
	float3 pointLightColor = gPointLight.color.rgb * gPointLight.intensity;
	
	//diffuse
	float3 diffusePointLight = gPointLight.color.rgb * gPointLight.intensity * pointLightColor * cosPoint;
	
	//specular
	float3 halfVectorPoint = normalize(-pointLightDirection + toEye);
	float NdotH_Point = dot(normal, halfVectorPoint);
	float specularPowPoint = pow(saturate(NdotH_Point), gMaterial.shininess);

	float3 specularPointLight = pointLightColor * specularPowPoint;
	
	
	//=========================
	// Output
	//=========================
	output.color.rgb = diffuse + specular + diffusePointLight + specularPointLight;
	output.color.a = gMaterial.color.a * textureColor.a;
	
	return output;
}

