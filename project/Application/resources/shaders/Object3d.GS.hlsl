#include "Object3d.hlsli"

[maxvertexcount(3)]
void main(
    triangle VertexShaderOutput input[3],
    inout TriangleStream<GeometryShaderOutput> output
) {
	for (uint i = 0; i < 3; i++) {
		GeometryShaderOutput element;
		element.position = input[i].position;
		element.texcoord = input[i].texcoord;
		element.normal = input[i].normal;
		element.worldPosition = input[i].worldPosition;
		output.Append(element);
	}
}
