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
    // 元の画像からピクセルの色を取得
    float4 color = tex.Sample(smp, input.texcoord);
    
    // ルミナンス（輝度）を計算。RGBを人間の目の感度に合わせた重みで足し算
    float luminance = dot(color.rgb, float3(0.2126f, 0.7152f, 0.0722f));
    
    // 輝度が閾値を超えていればその色を残し、超えていなければ真っ黒にする
    if (luminance >= threshold) {
        return color;
    }
    
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}
