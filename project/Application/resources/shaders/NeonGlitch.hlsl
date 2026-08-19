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

static const float u_intensity = 0.396;
static const float u_speed = 1.659;
static const float3 u_color1 = float3(1, 0, 0.903475);
static const float3 u_color2 = float3(0, 1, 1);
// ==========================================

float4 ApplyAiEffect(float2 uv, float u_time, Texture2D<float4> u_texture, SamplerState u_sampler) {
    float t = u_time * u_speed;
    
    // ツールのバグった座標系（Yが下から上）をシミュレート
    float2 buggyUV = float2(uv.x, 1.0 - uv.y);
    
    float2 vhsUV = applyVHSGlitch(buggyUV, t, u_intensity * 0.5);
    float2 glitchUV = applyDigitalGlitch(vhsUV, t, float2(15.0, 30.0), u_intensity * 0.2, float2(u_intensity * 0.1, 0.0));
    
    // 画像サンプリング前に正しい座標系（Yが上から下）に戻す
    float2 finalSampleUV = float2(glitchUV.x, 1.0 - glitchUV.y);
    
    float4 col = sampleWithAberration(u_texture, u_sampler, finalSampleUV, float2(1.0, 0.5), u_intensity * 0.03);
    float mask = luminance(col);
    
    float3 tint = lerp(u_color1, u_color2, sin(t + buggyUV.y * 10.0) * 0.5 + 0.5);
    col.rgb = lerp(col.rgb, col.rgb * tint * 1.5, mask * u_intensity);
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
