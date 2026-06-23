#include "Particle.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

ConstantBuffer<Material> gMaterial : register(b1);

struct DirectionalLight {
	float4 color;
	float3 direction;
	float intensity;
};

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(
VertexShaderOutput input) {
	float4 textureColor = gTexture.Sample(gSampler, input.texcoord);
	PixelShaderOutput output;
	output.color = gMaterial.color * textureColor * input.color;
	
	if (gMaterial.isRingMode != 0) {
		// UV座標の中心からの距離を計算
		float2 uv = input.texcoord - float2(0.5f, 0.5f);
		float dist = length(uv);
		
		// 距離0.4の位置をピークとするリング形状を作成
		float ring = 1.0f - abs(dist - 0.4f) * 20.0f;
		
		 // パーティクルの寿命(アルファ値)に応じてリングの線を細く鋭くする
		float power = lerp(1.0f, 4.0f, input.color.a);
		ring = saturate(ring);
		ring = pow(ring, power);
		
		output.color = gMaterial.color * input.color;
		output.color.a *= ring;
	}
	
	// 計算結果がNaNやInfになったピクセルは描画を破棄する
	if (any(isnan(output.color)) || any(isinf(output.color))) {
		discard;
	}
	
	if (output.color.a == 0.0) {
		discard;
	}
	return output;
}

