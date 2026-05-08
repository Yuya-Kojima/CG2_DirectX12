#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer PostProcessData : register(b0) {
    int32_t postEffectType; // 0: None, 1: BoxFilter, 2: GaussianFilter
    int32_t useGrayscale;
    int32_t useVignette;
    int32_t boxFilterK; // K radius for BoxFilter
    float32_t3 monotoneColor;
    float32_t vignetteScale;
    float32_t vignetteExponent;
    int32_t gaussianFilterK;
    float32_t gaussianSigma;
    float32_t padding;
};

static const float32_t PI = 3.14159265f;

float32_t gauss(float32_t x, float32_t y, float32_t sigma) {
    float32_t exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float32_t denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    if (postEffectType == 1) { // BoxFilter
        uint32_t width, height;
        gTexture.GetDimensions(width, height);
        float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
        
        output.color.rgb = float32_t3(0.0f, 0.0f, 0.0f);
        output.color.a = 1.0f;

        float32_t kSize = (float32_t)boxFilterK;
        float32_t weight = 1.0f / ((2.0f * kSize + 1.0f) * (2.0f * kSize + 1.0f));

        for (int32_t x = -boxFilterK; x <= boxFilterK; ++x) {
            for (int32_t y = -boxFilterK; y <= boxFilterK; ++y) {
                float32_t2 texcoord = input.texcoord + float32_t2(x, y) * uvStepSize;
                float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
                output.color.rgb += fetchColor * weight;
            }
        }
    } else if (postEffectType == 2) { // GaussianFilter
        uint32_t width, height;
        gTexture.GetDimensions(width, height);
        float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
        
        output.color.rgb = float32_t3(0.0f, 0.0f, 0.0f);
        output.color.a = 1.0f;

        float32_t totalWeight = 0.0f;
        for (int32_t x = -gaussianFilterK; x <= gaussianFilterK; ++x) {
            for (int32_t y = -gaussianFilterK; y <= gaussianFilterK; ++y) {
                float32_t weight = gauss((float32_t)x, (float32_t)y, gaussianSigma);
                float32_t2 texcoord = input.texcoord + float32_t2(x, y) * uvStepSize;
                float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
                
                output.color.rgb += fetchColor * weight;
                totalWeight += weight;
            }
        }
        
        output.color.rgb *= rcp(totalWeight);
    } else {
        output.color = gTexture.Sample(gSampler, input.texcoord);
    }

    if (useGrayscale != 0) {
        float32_t value = dot(output.color.rgb, float32_t3(0.2125f, 0.7154f, 0.0721f));
        output.color.rgb = value * monotoneColor;
    }

    if (useVignette != 0) {
        // 周囲を0に、中心になるほど明るくなるように計算で調整
        float32_t2 correct = input.texcoord * (1.0f - input.texcoord.yx);
        // correctだけで計算すると中心の最大値が0.0625で暗すぎるのでScaleで調整。この例では16倍して1にしている
        float vignette = correct.x * correct.y * vignetteScale;
        // とりあえず0.8乗でそれっぽくしてみた
        vignette = saturate(pow(vignette, vignetteExponent));
        // 係数として乗算
        output.color.rgb *= vignette;
    }

    return output;
}
