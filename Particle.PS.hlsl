#include "Particle.hlsli"

Texture2D<float4> gTexture : register(t1);
SamplerState gSampler : register(s0);

struct Material {
	float4 color;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct PixelShaderInput {
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float4 color : COLOR0;
};

struct PixelShaderOutput {
	float4 color : SV_TARGET0;
};

cbuffer TimeBC : register(b1) {
	float4 gTime;
}


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
	
	//セルの四隅を使って滑らかに補完する
	return lerp(lerp(value00, value01, fade.x), lerp(value10, value11, fade.x), fade.y);
}

PixelShaderOutput main(PixelShaderInput input) {
	float2 uv = input.texcoord;
	float time = gTime.x + 0.3; //+0.3はテキストより先に炎が出るようにするため
	PixelShaderOutput output;
	
	//二重ノイズでテクスチャを歪ませる
	float n1 = Noise(uv * 10 + time * 0.5);
	float n2 = Noise(uv * 30 - time * 0.8) * 0.5;
	
	//ノイズを合成
	float noise = n1 + n2;
	uv.x += (noise - 0.5f) * 0.2f;
	uv.y += (noise - 0.5f) * 0.4f;
	uv = saturate(uv);
	
	float4 textureColor = gTexture.Sample(gSampler, uv);
	
	//reveal進行度
	float reveal = saturate(time * 0.5 - uv.y);
	
	//alpha値にrevealの進行度を乗算してフェードインさせる
	textureColor.a *= reveal;
	
	//出力
	output.color = gMaterial.color * textureColor * input.color;
	if (output.color.a == 0.0) {
		discard;
	}
	
	//output.color.a = 0.0;
	
	return output;
}