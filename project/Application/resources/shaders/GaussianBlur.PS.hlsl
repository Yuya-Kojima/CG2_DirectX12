Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

cbuffer BlurData : register(b0) {
    int isHorizontal;
    float texWidth;
    float texHeight;
    float sigma;
};

float Gaussian(float x, float s) {
    return exp(-(x * x) / (2.0f * s * s));
}

float4 main(VertexShaderOutput input) : SV_TARGET {
    float4 color = float4(0, 0, 0, 0);
    float weightSum = 0.0f;
    float2 texelSize = float2(1.0f / texWidth, 1.0f / texHeight);
    int radius = max(1, (int)(sigma * 2.5f));
    
    for (int i = -radius; i <= radius; i++) {
        float weight = Gaussian((float)i, sigma);
        weightSum += weight;
        
        float2 offset = float2(0, 0);
        if (isHorizontal == 1) {
            offset.x = (float)i * texelSize.x;
        } else {
            offset.y = (float)i * texelSize.y;
        }
        
        color += tex.Sample(smp, input.texcoord + offset) * weight;
    }
    
    if (weightSum > 0.0f) {
        color /= weightSum;
    }
    color.a = 1.0f;
    
    return color;
}
