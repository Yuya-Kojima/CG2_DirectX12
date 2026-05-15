#include "Sprite.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
	float4x4 uvTransform;
};
ConstantBuffer<Material> gMaterial : register(b0);

// 新しく追加するUIエフェクト用の定数バッファ
struct UIEffectParams {
	float time; // 経過時間
	int effectType; // 0: Normal, 1: Wave (波打ち)
	float splitY; // エフェクトの基準Y座標
	float amplitude; // 揺れ幅
	float frequency; // 波の細かさ
	float speed; // 波のスピード
	float blurWidth; // (padding1) 横方向のブラー幅
	float boundaryAmp; // (padding2) 境界線の波打ち幅
};
ConstantBuffer<UIEffectParams> gUIEffect : register(b1);

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    //  UV座標の取得
	float4 transformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
	float2 uv = transformedUV.xy;

	if (gUIEffect.effectType == 1) {
		// --- UI波打ちエフェクト ---
		float blurW = (gUIEffect.blurWidth > 0.0f) ? gUIEffect.blurWidth : 0.04f;
		float bAmp = (gUIEffect.boundaryAmp > 0.0f) ? gUIEffect.boundaryAmp : 0.04f;
		float t = gUIEffect.time;
        
		// 波の上端Y座標を計算
		float w1_top = gUIEffect.splitY + 0.02f + sin(uv.x * 2.0f + t * 2.5f) * bAmp * 1.8f;
		float w2_top = gUIEffect.splitY - 0.03f + sin(uv.x * 1.5f - t * 1.8f) * bAmp * 2.0f;
		float base_top = gUIEffect.splitY + sin(uv.x * 1.0f + t * 2.0f) * bAmp * 1.5f;

		// シルエットの境界（最も高い波を採用）
		float silhouette_top = min(min(w1_top, w2_top), base_top);
        
        // シルエット外（波より上）は描画しない
		if (uv.y < silhouette_top) {
			discard;
		}

		// 波の下端Y座標を計算
		float w1_bot = w1_top + 0.12f + sin(uv.x * 3.5f - t * 3.0f) * bAmp * 1.5f;
		float w2_bot = w2_top + 0.20f + sin(uv.x * 2.5f + t * 2.0f) * bAmp * 2.0f;

		// 各波の領域判定
		bool inW1 = (uv.y >= w1_top && uv.y <= w1_bot);
		bool inW2 = (uv.y >= w2_top && uv.y <= w2_bot);

		// ベースカラー設定（波の重なりによる加算）
		// Calculate wave tint color
		float3 waveColor = float3(0.0f, 0.15f, 0.5f);
		if (inW2) { waveColor += float3(0.0f, 0.3f, 0.4f); }
		if (inW1) { waveColor += float3(0.0f, 0.4f, 0.5f); }
        
		float4 waveTintColor = float4(saturate(waveColor), 1.0f);
        
		// Calculate refraction (X-axis distortion)
		float shiftX = cos(uv.x * 1.0f + t * 2.0f) * 1.5f;
		if (inW2) { shiftX += cos(uv.x * 1.5f - t * 1.8f) * 3.0f; }
		if (inW1) { shiftX += cos(uv.x * 2.0f + t * 2.5f) * 3.0f; }
        
		shiftX = shiftX * gUIEffect.amplitude * 0.15f;

		// --- Pixel Rendering ---
		// Apply horizontal motion blur
		float4 blurredColor = float4(0, 0, 0, 0);
		const int samples = 9;
        
		for (int i = 0; i < samples; i++) {
			float2 sampleUV = uv;
			float offset = (i / (float) (samples - 1) - 0.5f) * blurW;
			sampleUV.x += shiftX + offset;
			blurredColor += gTexture.Sample(gSampler, sampleUV);
		}
		blurredColor /= (float) samples;
        
		float4 finalColor;
        
		// 合成処理
		if (blurredColor.a > 0.05f) {
			// テクスチャ範囲内（スクリーン合成）
			float3 baseText = saturate((gMaterial.color * blurredColor).rgb);
			float3 waveColorScreen = saturate(waveTintColor.rgb);
			finalColor.rgb = 1.0f - (1.0f - baseText) * (1.0f - waveColorScreen);
			finalColor.a = blurredColor.a;
		} else {
			// テクスチャ範囲外（背景の半透明波）
			float edgeFade = smoothstep(0.0f, 0.1f, uv.x) * smoothstep(1.0f, 0.9f, uv.x);
			float3 baseBlue = waveTintColor.rgb * 0.8f;
			finalColor = float4(baseBlue, 0.6f * edgeFade);
		}

		PixelShaderOutput output;
		output.color = saturate(finalColor);
		return output;
	}

	// --- 通常描画（エフェクト無効時） ---
	float4 textureColor = gTexture.Sample(gSampler, uv);
	
	if (textureColor.a <= 0.5f) {
		discard;
	}
	
	PixelShaderOutput output;
	output.color = gMaterial.color * textureColor;
	return output;
}
