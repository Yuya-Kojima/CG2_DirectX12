#pragma once
#include"Math/MathUtil.h"
#include<wrl.h>
#include "Math/VertexData.h"
#include <cstdint>
#include <vector>

class Ring {

public:
	const std::vector<VertexData>& GetVertices() const { return vertices_; }
	uint32_t GetVertexCount() const { return static_cast<uint32_t>(vertices_.size()); }

	uint32_t GetDivide() const { return divide_; }
	float GetOuterRadius() const { return outerRadius_; }
	float GetInnerRadius() const { return innerRadius_; }

	void Build(uint32_t divide, float outerRadius, float innerRadius);

private:
	uint32_t divide_ = 0;
	float outerRadius_ = 1.0f;
	float innerRadius_ = 0.5f;

	std::vector<VertexData> vertices_;
};

