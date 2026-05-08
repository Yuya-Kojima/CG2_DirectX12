#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer PostProcessData : register(b0) {
    int32_t useGrayscale;
    float32_t3 monotoneColor;
    int32_t useVignette;
    float32_t vignetteScale;
    float32_t vignetteExponent;
    float32_t padding;
};

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);

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
