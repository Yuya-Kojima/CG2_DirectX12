Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

cbuffer ExtractData : register(b0) {
    float threshold;
    float3 padding;
};

float4 main(VertexShaderOutput input) : SV_TARGET {
    float4 color = tex.Sample(smp, input.texcoord);
    float luminance = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));
    
    if (luminance >= threshold) {
        return color;
    }
    
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}
