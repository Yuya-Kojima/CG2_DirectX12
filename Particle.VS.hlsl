#include "Particle.hlsli"

struct ParticleForGPU {
	float4x4 WVP;
	float4x4 World;
	float4 color;
};

struct VertexShaderInput {
	float4 position : POSITION0;
	float2 texcoord : TEXCOORD0;
	uint instanceId : SV_InstanceID;
};

StructuredBuffer<ParticleForGPU> gParticle : register(t0);

VertexShaderOutput main(VertexShaderInput input) {
	
	ParticleForGPU p = gParticle[input.instanceId];
	VertexShaderOutput output;
	output.position = mul(input.position, p.WVP);
	output.texcoord = input.texcoord;
	output.color = p.color;

	return output;
}