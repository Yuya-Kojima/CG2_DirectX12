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

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

//擬似乱数を生成
float hash(float2 p) {
	
	//入力された座標をシード値に変換
	float seed = dot(p, float2(12.9898, 78.233));
	
	//sinを使用して非線形変換で擬似乱数に
	float warped = sin(seed);
	
	//振れ幅を拡大
	float scaled = warped * 43758.5453;
	
	//小数部分だけ取り出す
	float result = frac(scaled);
	
	return result;
}

//バリューノイズ
float Noise(float2 uv) {
	float2 cellCorrds = floor(uv); //セルの位置
	float2 cellFrac = frac(uv); //セル内での位置
	
	float value00 = hash(cellCorrds); //(i,j)
	float value01 = hash(cellCorrds + float2(1.0, 0.0)); //(i+1,j  )
	float value10 = hash(cellCorrds + float2(0.0, 1.0)); //(i  ,j+1)
	float value11 = hash(cellCorrds + float2(1.0, 1.0)); //(i+1,j+1)
	
	//セル内の補間係数をイージングで滑らかにする
	//境界のつなぎ目を滑らかにする
	float2 fade = cellFrac * cellFrac * (3.0 - 2.0 * cellFrac);
	
	
	return lerp(lerp(value00, value01, fade.x), lerp(value10, value11, fade.x), fade.y);
}

PixelShaderOutput main(VertexShaderOutput input) {
	float2 uv = input.texcoord;
	
	//経過時間をとる
	float time = gTime.x;
	
	   // レイヤー判別
	float w = gMaterial.color.w;
	
	PixelShaderOutput output;
	
	/* 浮かび上がる時の炎
	----------------------------*/
	if (w >= 0.85) {
		output.color.w = 0.0;
		return output;
	}
	
	/* 浮かび上がる文字
	----------------------------*/
	if (gMaterial.color.w >= 0.75 && gMaterial.color.w < 0.85) {
		
		//reveal進行度
		//上から下へフェードインする
		float reveal = saturate((time * 0.5) - (uv.y));
		
		// テクスチャ読み込み
		float4 textureColor = gTexture.Sample(gSampler, uv);
		
		//alpha値にrevealの進行度を乗算してフェードインさせる
		textureColor.a *= reveal;
		
		//出力
		output.color = textureColor;
		return output;
	}
	
	/* 浮かび上がる時の炎のグロー
	----------------------------*/
	if (gMaterial.color.w >= 0.65 && gMaterial.color.w < 0.75) {
		output.color.w = 0.0;
		return output;
	}
	
	/* テキストエフェクト
	-------------------------*/
	if (gMaterial.color.w >= 0.5 && gMaterial.color.w < 0.65) {
		
		//ノイズによってテクスチャを歪ませる(炎)
		float noise = Noise(uv * 10.0f + time * 0.5f);
		
		//振れ幅
		uv.x += (noise - 0.5f) * 0.02f;
		uv.y += (noise - 0.5f) * 0.04f;
		uv = saturate(uv);
		
		//テクスチャ読み込み
		float4 textureColor = gTexture.Sample(gSampler, uv);
		
		//炎の色
		//縦方向に波打つグラデーション
		float fireShift = saturate(sin(gTime.x * 3.0 + uv.y * 10.0) * 0.5f + 0.5f);
		float3 color1 = float3(1.0, 0.1, 0.0); // 赤
		float3 color2 = float3(1.0, 1.0, 0.0); // 黄
		float3 color3 = float3(1.0, 1.0, 1.0); // 白
		
		// 2段階で色を補間
		float3 midColor = lerp(color1, color2, fireShift);
		float3 fireColor = lerp(midColor, color3, fireShift * 0.5f);
	
		//テクスチャに炎の色を乗算
		textureColor.rgb *= fireColor;
		
		//輪郭をはっきりさせる
		textureColor.a = pow(textureColor.a, 1.5f);
		
		//出力
		output.color = textureColor;
		return output;
	}
	
	/* グロー
	---------------------------*/
	if (gMaterial.color.w < 0.5f) {
		
		//炎と同じノイズをかける
		float noise = Noise(uv * 10.0f + time * 0.5f);
		uv.x += (noise - 0.5f) * 0.02f;
		uv.y += (noise - 0.5f) * 0.04f;
		uv = saturate(uv);
		
		float2 texelSize = float2(1.0 / 256.0, 1.0 / 256.0);
		float alpha = gTexture.Sample(gSampler, uv).a;

        // 周囲のα値を合計して光の強さにする
 	    // 7x7
		float glowSum = 0.0f;
		for (int x = -3; x <= 3; ++x) {
			for (int y = -3; y <= 3; ++y) {
				float2 offset = texelSize * float2(x, y);
				glowSum += gTexture.Sample(gSampler, uv + offset).a;
			}
		}

		glowSum /= 49.0f; // 7x7 = 49サンプルの平均を取る

        // 元のαが小さい(透明)なら、周囲の光を描く
		float glow = glowSum * (1.0 - alpha);
		
		//黄色
		float3 glowColor = float3(1.2, 1.0, 0.4);
		
		//出力
		output.color = float4(glowColor, glow /** 0.9f*/);
		return output;
	}
	
	output.color = gTexture.Sample(gSampler, uv);
	return output;
}