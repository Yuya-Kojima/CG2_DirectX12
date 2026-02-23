#pragma once
#include"Math/MathUtil.h"
#include<wrl.h>
#include "Math/VertexData.h"
#include <cstdint>
#include <vector>

class Cylinder {

public:
	const std::vector<VertexData>& GetVertices() const { return vertices_; }
	uint32_t GetVertexCount() const {
		return static_cast<uint32_t>(vertices_.size());
	}

	uint32_t GetDivide() const { return divide_; }
	float GetTopRadius() const { return topRadius_; }
	float GetBottomRadius() const { return bottomRadius_; }
	float GetHeight() const { return height_; }

	void Build(uint32_t divide, float topRadius, float bottomRadius, float height);

private:
	uint32_t divide_ = 0;
	float topRadius_ = 1.0f;
	float bottomRadius_ = 1.0f;
	float height_ = 1.0f;

	std::vector<VertexData> vertices_;
};