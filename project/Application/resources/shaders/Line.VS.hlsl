struct VertexShaderInput {
    float4 position : POSITION;
    float4 color : COLOR;
};

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

// 定数バッファ (Root Parameter 0)
cbuffer TransformationMatrix : register(b0) {
    matrix WVP; // World View Projection 行列
};

VertexShaderOutput main(VertexShaderInput input) {
    VertexShaderOutput output;
    
    // WVP行列を掛けてクリップ空間に変換
    output.position = mul(input.position, WVP);
    output.color = input.color;
    
    return output;
}
