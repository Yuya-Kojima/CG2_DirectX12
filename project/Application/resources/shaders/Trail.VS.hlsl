struct VertexShaderInput {
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

// 定数バッファ (Root Parameter 0)
cbuffer TransformationMatrix : register(b0) {
    matrix ViewProjection;
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    
    // ワールド座標にViewProjection行列を掛けてクリップ空間に変換
    output.position = mul(input.position, ViewProjection);
    output.texcoord = input.texcoord;
    output.color = input.color;
    
    return output;
}
