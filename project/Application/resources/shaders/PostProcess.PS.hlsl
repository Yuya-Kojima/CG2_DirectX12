#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gDepthTexture : register(t1);
SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);

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
    float32_t depthOutlineWeight;
    float32_t depthOutlineAttenuation;
    float32_t padding1;
    float32_t padding2;
    float32_t padding3;
    float32_t4x4 projectionInverse;
    float32_t2 radialBlurCenter;
    float32_t radialBlurWidth;
    int32_t radialBlurSamples;
    float32_t radialBlurInnerRadius;
    float32_t radialBlurOuterRadius;
    float32_t radialBlurAberration;
    float32_t padding4;
};

static const float32_t PI = 3.14159265f;

float32_t gauss(float32_t x, float32_t y, float32_t sigma) {
    float32_t exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float32_t denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}

float32_t Luminance(float32_t3 v) {
    return dot(v, float32_t3(0.2125f, 0.7154f, 0.0721f));
}

static const float32_t kPrewittHorizontalKernel[3][3] = {
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float32_t kPrewittVerticalKernel[3][3] = {
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    {  0.0f,         0.0f,         0.0f        },
    {  1.0f / 6.0f,  1.0f / 6.0f,  1.0f / 6.0f },
};

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
    } else if (postEffectType == 3) { // Luminance Outline
        uint32_t width, height;
        gTexture.GetDimensions(width, height);
        float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
        
        float32_t2 difference = float32_t2(0.0f, 0.0f);
        
        for (int32_t x = 0; x < 3; ++x) {
            for (int32_t y = 0; y < 3; ++y) {
                float32_t2 texcoord = input.texcoord + float32_t2(x - 1, y - 1) * uvStepSize;
                float32_t3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
                float32_t luminance = Luminance(fetchColor);
                
                difference.x += luminance * kPrewittHorizontalKernel[x][y];
                difference.y += luminance * kPrewittVerticalKernel[x][y];
            }
        }
        
        float32_t weight = length(difference);
        // 差が小さすぎて分かりづらいので6倍して強調
        weight = saturate(weight * 6.0f);
        
        // 元の色を取得し、エッジ（weight）が強いほど黒く（0に近づく）するように合成
        float32_t3 originalColor = gTexture.Sample(gSampler, input.texcoord).rgb;
        output.color.rgb = (1.0f - weight) * originalColor;
        output.color.a = 1.0f;
    } else if (postEffectType == 4) { // Depth Outline
        uint32_t width, height;
        gTexture.GetDimensions(width, height);
        float32_t2 uvStepSize = float32_t2(rcp(width), rcp(height));
        
        float32_t2 difference = float32_t2(0.0f, 0.0f);
        float32_t centerViewZ = 0.0f;
        
        for (int32_t x = 0; x < 3; ++x) {
            for (int32_t y = 0; y < 3; ++y) {
                float32_t2 texcoord = input.texcoord + float32_t2(x - 1, y - 1) * uvStepSize;
                
                // NDCのDepthを取得
                float32_t ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord);
                
                // View空間に戻す
                float32_t4 viewSpace = mul(float32_t4(0.0f, 0.0f, ndcDepth, 1.0f), projectionInverse);
                float32_t viewZ = viewSpace.z * rcp(viewSpace.w);
                
                if (x == 1 && y == 1) {
                    centerViewZ = viewZ;
                }
                
                difference.x += viewZ * kPrewittHorizontalKernel[x][y];
                difference.y += viewZ * kPrewittVerticalKernel[x][y];
            }
        }
        
        float32_t weight = length(difference) * depthOutlineWeight;
        // 距離による減衰（絶対値にして負数エラーを防ぐ）
        weight = weight / (1.0f + abs(centerViewZ) * depthOutlineAttenuation);
        weight = saturate(weight);
        
        float32_t3 originalColor = gTexture.Sample(gSampler, input.texcoord).rgb;
        output.color.rgb = (1.0f - weight) * originalColor;
        output.color.a = 1.0f;
    } else if (postEffectType == 5) { // Radial Blur
        // 中心から現在のUVへの方向と距離を計算
        float32_t2 direction = input.texcoord - radialBlurCenter;
        float32_t dist = length(direction);
        
        // 中心マスク（センターをクッキリさせる）
        // innerRadius 以下はブラー0、outerRadius 以上はブラー100%
        float32_t mask = smoothstep(radialBlurInnerRadius, radialBlurOuterRadius, dist);
        
        // 元画像（ブラーなし）の色
        float32_t3 originalColor = gTexture.Sample(gSampler, input.texcoord).rgb;
        
        // maskが0なら処理をスキップしてそのまま色を返す（軽量化）
        if (mask <= 0.0f) {
            output.color.rgb = originalColor;
            output.color.a = 1.0f;
            return output;
        }

        float32_t3 outputColor = float32_t3(0.0f, 0.0f, 0.0f);
        
        for (int32_t sampleIndex = 0; sampleIndex < radialBlurSamples; ++sampleIndex) {
            // 色収差のオフセット計算
            // sampleIndexが進むにつれてRGBのサンプリング位置を少しズラす
            float32_t scale = radialBlurWidth * float32_t(sampleIndex);
            
            // 赤は外側、青は内側へ少しズラす
            float32_t2 texcoordR = input.texcoord + direction * (scale * (1.0f + radialBlurAberration));
            float32_t2 texcoordG = input.texcoord + direction * scale;
            float32_t2 texcoordB = input.texcoord + direction * (scale * (1.0f - radialBlurAberration));
            
            outputColor.r += gTexture.Sample(gSampler, texcoordR).r;
            outputColor.g += gTexture.Sample(gSampler, texcoordG).g;
            outputColor.b += gTexture.Sample(gSampler, texcoordB).b;
        }
        
        // 平均化する
        outputColor.rgb *= rcp(float32_t(radialBlurSamples));
        
        // 元画像とブラー画像をマスク値でブレンドする
        output.color.rgb = lerp(originalColor, outputColor, mask);
        output.color.a = 1.0f;
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
