Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderInput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

float4 main(PixelShaderInput input) : SV_TARGET0 {
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);
    float4 outputColor = input.color * texColor;
    return outputColor;
}
