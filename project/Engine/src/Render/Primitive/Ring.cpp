#include "Render/Primitive/Ring.h"
#include <cassert>

void Ring::Build(uint32_t divide, float outerRadius, float innerRadius) {
	divide_ = divide;
	outerRadius_ = outerRadius;
	innerRadius_ = innerRadius;

	assert(divide_ >= 3 && "Ring::Build: divide must be >= 3");
	assert(outerRadius_ > 0.0f && "Ring::Build: outerRadius must be > 0");
	assert(innerRadius_ >= 0.0f && "Ring::Build: innerRadius must be >= 0");
	assert(outerRadius_ > innerRadius_ && "Ring::Build: outerRadius must be > innerRadius");

	vertices_.clear();
	vertices_.reserve(static_cast<size_t>(divide_) * 6u);

	const float radPer = 2.0f * std::numbers::pi_v<float> / float(divide_);

	for (uint32_t i = 0; i < divide_; ++i) {
		const float a0 = float(i) * radPer;
		const float a1 = float(i + 1) * radPer;

		const float sin0 = std::sin(a0);
		const float cos0 = std::cos(a0);
		const float sin1 = std::sin(a1);
		const float cos1 = std::cos(a1);

		const float u0 = float(i) / float(divide_);
		const float u1 = float(i + 1) / float(divide_);

		VertexData v1{}, v2{}, v3{}, v4{};

		// 外周
		v1.position = { -sin0 * outerRadius_,  cos0 * outerRadius_, 0.0f, 1.0f };
		v2.position = { -sin1 * outerRadius_,  cos1 * outerRadius_, 0.0f, 1.0f };
		// 内周
		v3.position = { -sin0 * innerRadius_,  cos0 * innerRadius_, 0.0f, 1.0f };
		v4.position = { -sin1 * innerRadius_,  cos1 * innerRadius_, 0.0f, 1.0f };

		v1.texcoord = { u0, 0.0f }; v2.texcoord = { u1, 0.0f };
		v3.texcoord = { u0, 1.0f }; v4.texcoord = { u1, 1.0f };

		v1.normal = v2.normal = v3.normal = v4.normal = { 0.0f, 0.0f, 1.0f };

		// Index無しで三角形2枚に展開
		vertices_.push_back(v1);
		vertices_.push_back(v2);
		vertices_.push_back(v3);

		vertices_.push_back(v3);
		vertices_.push_back(v2);
		vertices_.push_back(v4);
	}
}