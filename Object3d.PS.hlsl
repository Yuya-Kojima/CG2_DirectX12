#include "Object3d.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
	float4x4 uvTransform;
};

ConstantBuffer<Material> gMaterial : register(b0);


cbuffer TimeBC : register(b1) {
	float4 gTime;
}

cbuffer FXParamsCB : register(b2) {
	float flameSpeed;
	float flameNoiseScale;
	float flameIntensity;
	float revealSpeed;
	float revealWidth;
	float flameDuration;
};

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

float hash(float2 p) {
	return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float noise(float2 uv) {
	float2 i = floor(uv);
	float2 f = frac(uv);
	float a = hash(i);
	float b = hash(i + float2(1.0, 0.0));
	float c = hash(i + float2(0.0, 1.0));
	float d = hash(i + float2(1.0, 1.0));
	float2 u = f * f * (3.0 - 2.0 * f);
	return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

PixelShaderOutput main(VertexShaderOutput input) {
	float2 uv = input.texcoord;
	
	// UVにゆらぎを追加（例：炎風）
	float time = gTime.x; // float4からx成分だけ使う
	
	    // 0) ベーステクスチャ読み込み
	float4 tex = gTexture.Sample(gSampler, uv);

    // 共通の「リビール量」
	float reveal = smoothstep(0.0, revealWidth,
                          time * revealSpeed - uv.y);
	
	   // レイヤー判定スイッチ
	float w = gMaterial.color.w;
	
	PixelShaderOutput output;
	
	//float n = noise(uv * 10.0f + time * 0.5f);
	//uv.x += (n - 0.5f) * 0.02f;
	//uv.y += (n - 0.5f) * 0.04f;
	//uv.x += sin(uv.y * 10.0f + gTime.x * 8.0f) * 0.08f;
	//uv.y += cos(uv.x * 8.0f + gTime.x * 4.0f) * 0.05f;
	//uv = saturate(uv);
	
	//float4 transformedUV = mul(float4(uv, 0.0f, 1.0f), gMaterial.uvTransform);
	
	//textureColor.a = 1.0f;
	//float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
	
	//float4 textureColor = gTexture.Sample(gSampler, uv);
	
	//float fireColorRate = sin(gTime.x * 5.0f + uv.y * 5.0f) * 0.5f + 0.5f;
	//float3 fireColor = lerp(float3(1.0, 0.4, 0.0), float3(1.0, 1.0, 0.0), fireColorRate); // 赤→黄
	
	//float fireShift = saturate(sin(gTime.x * 3.0 + uv.y * 10.0) * 0.5f + 0.5f);
	//float3 color1 = float3(1.0, 0.1, 0.0); // 赤
	//float3 color2 = float3(1.0, 1.0, 0.0); // 黄
	//float3 color3 = float3(1.0, 1.0, 1.0); // 白

// 2段階Lerp
//	float3 midColor = lerp(color1, color2, fireShift);
//	float3 fireColor = lerp(midColor, color3, fireShift * 0.5f); // 白は軽めに
	
//	textureColor.rgb *= fireColor;
//	textureColor.a = pow(textureColor.a, 1.5f);
	//textureColor.a *= 0.6 + 0.4 * sin(gTime.x * 6.0f + uv.y * 10.0f);
	
	//円形に描画
	//float2 center = float2(0.5f, 0.5f);
	//float dist = distance(uv, center);
	//float fade = saturate(1.0f - dist * 2.0f); // 端にいくほど透明
	//textureColor.a *= fade;
	
	//float fireShift = 0.5f; // ← 一旦固定
	//float3 fireColor = lerp(float3(1.0, 0.5, 0.0), float3(1.0, 1.0, 0.0), fireShift);
	//textureColor.rgb *= fireColor;
	
	//float2 center = float2(0.5f, 0.5f);
	//float dist = distance(uv, center);
	//float fade = smoothstep(0.4f, 0.5f, 1.0f - dist); // エッジのぼけを滑らかに
	//textureColor.a *= fade;
	
	
	
	///* 浮かび上がる時の炎
	//----------------------------*/
	//if (w >= 0.85) {
 //   // ノイズで揺らす
	//	float n = noise(uv * flameNoiseScale + float2(0, time * flameSpeed));
 //       // ノイズ強度で色を赤→黄に
	//	float3 fireCol = lerp(float3(1, 0.4, 0), float3(1, 1, 0), n)
 //                      * flameIntensity;
	//	tex.rgb *= fireCol;

 //       // リビールタイミングが来たら徐々にアルファを乗算
	//	float flameFade = saturate((flameDuration - time) / flameDuration);
	//	tex.a *= reveal * flameFade;

	//	output.color = tex;
	//	return output;
	//}
	
	/* 浮かび上がる文字
	----------------------------*/
	if (gMaterial.color.w >= 0.75 && gMaterial.color.w < 0.85) {
		
		//reveal値
		float reveal = saturate((time * 0.5) - (uv.y));
		
		
		float4 textureColor = gTexture.Sample(gSampler, uv);
		textureColor.a *= reveal;
		
		output.color = textureColor;
		//output.color.w = 0.0;
		return output;
	}
	
	///* 浮かび上がる時の炎のグロー
	//----------------------------*/
	//if (gMaterial.color.w >= 0.65 && gMaterial.color.w < 0.75) {
	//// リビール量
	//	float reveal = smoothstep(0.0, revealWidth,
 //                         time * revealSpeed - uv.y);

 //   // テクセルサイズを固定 (256×256)
	//	float2 texelSize = float2(1.0 / 256.0, 1.0 / 256.0);

 //   // 周囲 5×5 サンプルでグローを作る
	//	float sumA = 0.0f;
	//	for (int oy = -2; oy <= 2; ++oy) {
	//		for (int ox = -2; ox <= 2; ++ox) {
	//			sumA += gTexture.Sample(gSampler, uv + float2(ox, oy) * texelSize).a;
	//		}
	//	}
	//	float glow = (sumA / 25.0f) * reveal;

	//	float3 glowColor = float3(1.2, 1.0, 0.4);
	//	output.color = float4(glowColor, glow * flameIntensity);
	//	//output.color.w = 0.0;
	//	return output;
	//}
	
	/* テキストエフェクト
	-------------------------*/
	if (gMaterial.color.w >= 0.5 && gMaterial.color.w < 0.65) {
		
		float n = noise(uv * 10.0f + time * 0.5f);
		uv.x += (n - 0.5f) * 0.02f;
		uv.y += (n - 0.5f) * 0.04f;
		uv = saturate(uv);
		
		float4 textureColor = gTexture.Sample(gSampler, uv);
		
		float fireShift = saturate(sin(gTime.x * 3.0 + uv.y * 10.0) * 0.5f + 0.5f);
		float3 color1 = float3(1.0, 0.1, 0.0); // 赤
		float3 color2 = float3(1.0, 1.0, 0.0); // 黄
		float3 color3 = float3(1.0, 1.0, 1.0); // 白
		
		// 2段階Lerp
		float3 midColor = lerp(color1, color2, fireShift);
		float3 fireColor = lerp(midColor, color3, fireShift * 0.5f); // 白は軽めに
	
		textureColor.rgb *= fireColor;
		textureColor.a = pow(textureColor.a, 1.5f);
		
		output.color = textureColor;
		return output;
	}
	
	/* グロー
	---------------------------*/
	if (gMaterial.color.w < 0.5f) {
		float n = noise(uv * 10.0f + time * 0.5f);
		uv.x += (n - 0.5f) * 0.02f;
		uv.y += (n - 0.5f) * 0.04f;
		uv = saturate(uv);
		
		float2 texelSize = float2(1.0 / 256.0, 1.0 / 256.0);
		float alpha = gTexture.Sample(gSampler, uv).a;

    // 周囲のα値を合計して光の強さにする（放射っぽく）
		float glowSum = 0.0f;
		for (int x = -3; x <= 3; ++x) {
			for (int y = -3; y <= 3; ++y) {
				float2 offset = texelSize * float2(x, y);
				glowSum += gTexture.Sample(gSampler, uv + offset).a;
			}
		}

		glowSum /= 49.0f; // 7x7 = 49サンプル

    // 元のαが小さい（透明）なら、周囲から漏れる光として描く
		float glow = glowSum * (1.0 - alpha);

		//float3 glowColor = float3(1.0, 0.8, 0.2); // やわらかい光色
		//output.color = float4(glowColor, glow * 0.6f); // αも弱めに
		
		float3 glowColor = float3(1.2, 1.0, 0.4); // ちょっと強めの黄
		output.color = float4(glowColor, glow * 0.9f);
		return output;
	}
	
	output.color = gTexture.Sample(gSampler, uv);
	return output;
	
	//PixelShaderOutput output;
	//float t = gTime.x;
	//output.color = float4(sin(t), 0.0f, 0.0f, 1.0f); // 時間経過で赤が変わる
	//return output;
}