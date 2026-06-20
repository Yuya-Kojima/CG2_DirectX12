struct VertexShaderOutput {
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR0;
};

struct PerView {
    float4x4 viewProjection;
    float4x4 billboardMatrix;
    float3 cameraPosition;
    float padding;
};

struct Material {
    float4 color;
    int enableLighting;
    int isBillboard;
    int isRingMode;
    float padding;
    float4x4 uvTransform;
};