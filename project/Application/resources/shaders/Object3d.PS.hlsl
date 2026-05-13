#include "Object3d.hlsli"
#include "Common.hlsli"
#include "Lighting.hlsli"

Texture2D<float4> gTexture : register(t0);
TextureCube<float4> gEnvironmentTexture : register(t1);
Texture2D<float32_t> gMaskTexture : register(t2);
SamplerState gSampler : register(s0);

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);

PixelShaderOutput main(
GeometryShaderOutput input) {
	
	//=========================
    // UV transform & sampling
    //=========================
	float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	
	//アルファテスト
	if (textureColor.a <= 0.5) {
		discard;
	}

	// ディゾルブ
	float edge = 0.0f;
	if (gMaterial.enableDissolve != 0) {
		float mask = gMaskTexture.Sample(gSampler, input.texcoord + gMaterial.maskTransform).r;
		if (mask <= gMaterial.dissolveThreshold) {
			discard;
		}
		// Edgeっぽさを算出
		edge = 1.0f - smoothstep(gMaterial.dissolveThreshold, gMaterial.dissolveThreshold + gMaterial.dissolveEdgeRange, mask);
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
	float3 baseColor = gMaterial.color.rgb * textureColor.rgb;
		
	//=========================
	// Lighting Calculations
	//=========================
	LightResult dirLight = CalculateDirectionalLight(gDirectionalLight, normal, toEye, gMaterial.shininess, baseColor);
	LightResult pointLight = CalculatePointLight(gPointLight, input.worldPosition, normal, toEye, gMaterial.shininess, baseColor);
	LightResult spotLight = CalculateSpotLight(gSpotLight, input.worldPosition, normal, toEye, gMaterial.shininess, baseColor);

	//=========================
	// Output
	//=========================
	output.color.rgb = dirLight.diffuse + dirLight.specular + 
                       pointLight.diffuse + pointLight.specular + 
                       spotLight.diffuse + spotLight.specular;
	
	// Environment Map
	float3 cameraToPosition = normalize(input.worldPosition - gCamera.worldPosition);
	float3 reflectedVector = reflect(cameraToPosition, normalize(input.normal));
	float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
	output.color.rgb += environmentColor.rgb * gMaterial.environmentCoefficient;

	// Dissolve Edge 発光
	if (gMaterial.enableDissolve != 0) {
		output.color.rgb += edge * gMaterial.dissolveEdgeColor.rgb;
	}

	output.color.a = gMaterial.color.a * textureColor.a;
	
	// 計算結果がNaNやInfになったピクセルは描画を破棄する
	if (any(isnan(output.color)) || any(isinf(output.color))) {
		discard;
	}
	
	return output;
}

