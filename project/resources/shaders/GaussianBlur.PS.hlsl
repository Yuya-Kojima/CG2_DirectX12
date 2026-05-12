Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

cbuffer BlurData : register(b0) {
    int isHorizontal; // 1 = 横方向, 0 = 縦方向
    float texWidth;
    float texHeight;
    float sigma;
};

// 1次元ガウス関数の計算
float Gaussian(float x, float s) {
    return exp(-(x * x) / (2.0f * s * s));
}

float4 main(VertexShaderOutput input) : SV_TARGET {
    float4 color = float4(0, 0, 0, 0);
    float weightSum = 0.0f;
    
    // 1ピクセルあたりのサイズ
    float2 texelSize = float2(1.0f / texWidth, 1.0f / texHeight);
    
    // シグマに基づいてサンプリング半径を決定（2.5倍程度でほぼ収束）
    int radius = max(1, (int)(sigma * 2.5f));
    
    for (int i = -radius; i <= radius; i++) {
        // 重みを計算
        float weight = Gaussian((float)i, sigma);
        weightSum += weight;
        
        // オフセット（ずらし量）を計算
        float2 offset = float2(0, 0);
        if (isHorizontal == 1) {
            offset.x = (float)i * texelSize.x;
        } else {
            offset.y = (float)i * texelSize.y;
        }
        
        // テクスチャをサンプリングして重みを掛ける
        color += tex.Sample(smp, input.texcoord + offset) * weight;
    }
    
    // 合計の重みで割って正規化
    if (weightSum > 0.0f) {
        color /= weightSum;
    }
    color.a = 1.0f;
    
    return color;
}
