struct VertexShaderOutput {
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	//float3 normal : NORMAL0;
	float4 color : COLOR0;
};

struct Material {
    float4 color;
    int enableLighting;
    int isBillboard;
    float2 padding;
    float4x4 uvTransform;
};