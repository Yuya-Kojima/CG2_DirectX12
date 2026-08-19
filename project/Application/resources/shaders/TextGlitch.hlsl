// ==========================================
// AI Generated HLSL Sprite Shader
// ==========================================
#include "Sprite.hlsli"
#include "AiEffectLibrary.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material {
    float4 color;
    float4x4 uvTransform;
};
ConstantBuffer<Material> gMaterial : register(b0);
cbuffer TimeBuffer : register(b1) { float u_time; }


static const float u_intensity = 0.17;
static const float u_speed = 2.0;
static const float3 u_colorA = float3(1.0, 0.0, 0.5); // ネオンピンク
static const float3 u_colorB = float3(0.0, 1.0, 1.0); // シアン

float4 ApplyAiEffect(float2 uv, float u_time, Texture2D<float4> u_texture, SamplerState u_sampler) {
    float time = u_time * u_speed;
    float2 distortedUV = applyVHSGlitch(uv, time, u_intensity * 0.5);
    distortedUV = applyDigitalGlitch(distortedUV, time, float2(15.0, 25.0), u_intensity * 0.2, float2(0.1, 0.0) * u_intensity);
    float4 col = sampleWithAberration(u_texture, u_sampler, distortedUV, float2(1.0, 0.0), u_intensity * 0.08);
    float luma = luminance(col);
    col.rgb = lerp(col.rgb, lerp(u_colorA, u_colorB, uv.x), luma * u_intensity * 0.8);
    return col;
}

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    PixelShaderOutput output;
    output.color = ApplyAiEffect(transformedUV.xy, u_time, gTexture, gSampler);
    output.color *= gMaterial.color;
    if (output.color.a <= 0.5f) { discard; }
    return output;
}