#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

//=========================
// ライト構造体の定義
//=========================
struct DirectionalLight {
	float4 color;
	float3 direction;
	float intensity;
};

struct PointLight {
	float4 color;
	float3 position;
	float intensity;
	float radius;
	float decay;
};

struct SpotLight {
	float4 color;
	float3 position;
	float intensity;
	float3 direction;
	float distance;
	float decay;
	float cosAngle;
};

struct LightResult {
    float3 diffuse;
    float3 specular;
};

//=========================
// ライティング計算関数
//=========================

// 平行光源の計算
LightResult CalculateDirectionalLight(DirectionalLight light, float3 normal, float3 toEye, float shininess, float3 baseColor) {
    LightResult result;
    
    // ハーフランバート (Diffuse)
    float NdotL = dot(normal, -light.direction);
    float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
    result.diffuse = baseColor * light.color.rgb * cos * light.intensity;
    
    // Blinn-Phong (Specular)
    float3 halfVector = normalize(-light.direction + toEye);
    float NDotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NDotH), shininess);
    result.specular = light.color.rgb * light.intensity * specularPow;
    
    return result;
}

// 点光源の計算
LightResult CalculatePointLight(PointLight light, float3 worldPosition, float3 normal, float3 toEye, float shininess, float3 baseColor) {
    LightResult result;
    result.diffuse = float3(0.0f, 0.0f, 0.0f);
    result.specular = float3(0.0f, 0.0f, 0.0f);

    float distance = length(light.position - worldPosition);
    float factor = pow(saturate(-distance / light.radius + 1.0f), light.decay);
    if (factor <= 0.0f) return result;

    float3 lightDir = normalize(worldPosition - light.position);
    float3 lightColor = light.color.rgb * light.intensity * factor;

    // Diffuse
    float NdotL = dot(normal, -lightDir);
    float cosPoint = pow(NdotL * 0.5f + 0.5f, 2.0f);
    result.diffuse = baseColor * lightColor * cosPoint;

    // Specular
    float3 halfVector = normalize(-lightDir + toEye);
    float NdotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NdotH), shininess);
    result.specular = lightColor * specularPow;

    return result;
}

// スポットライトの計算
LightResult CalculateSpotLight(SpotLight light, float3 worldPosition, float3 normal, float3 toEye, float shininess, float3 baseColor) {
    LightResult result;
    result.diffuse = float3(0.0f, 0.0f, 0.0f);
    result.specular = float3(0.0f, 0.0f, 0.0f);

    float3 lightDir = normalize(worldPosition - light.position);
    float cosAngle = dot(lightDir, light.direction);
    float falloffFactor = saturate((cosAngle - light.cosAngle) / (1.0f - light.cosAngle));
    if (falloffFactor <= 0.0f) return result;

    float spotDistance = length(light.position - worldPosition);
    float attenuationFactor = pow(saturate(-spotDistance / light.distance + 1.0f), light.decay);
    if (attenuationFactor <= 0.0f) return result;

    float3 lightColor = light.color.rgb * light.intensity * attenuationFactor * falloffFactor;

    // Diffuse
    float NdotL = dot(normal, -lightDir);
    float cosSpot = pow(NdotL * 0.5f + 0.5f, 2.0f);
    result.diffuse = baseColor * lightColor * cosSpot;

    // Specular
    float3 halfVector = normalize(-lightDir + toEye);
    float NdotH = dot(normal, halfVector);
    float specularPow = pow(saturate(NdotH), shininess);
    result.specular = lightColor * specularPow;

    return result;
}

#endif
