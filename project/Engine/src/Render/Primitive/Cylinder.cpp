#include "Render/Primitive/Cylinder.h"
#include <cassert>
#include <cmath>
#include <numbers>

void Cylinder::Build(uint32_t divide, float topRadius, float bottomRadius,
	float height) {
	assert(divide >= 3);
	assert(topRadius >= 0.0f);
	assert(bottomRadius >= 0.0f);

	divide_ = divide;
	topRadius_ = topRadius;
	bottomRadius_ = bottomRadius;
	height_ = height;

	vertices_.clear();
	vertices_.reserve(static_cast<size_t>(divide_) * 6);

	const float radPer =
		2.0f * std::numbers::pi_v<float> / static_cast<float>(divide_);

	for (uint32_t i = 0; i < divide_; ++i) {
		const float a0 = static_cast<float>(i) * radPer;
		const float a1 = static_cast<float>(i + 1) * radPer;

		const float s0 = std::sin(a0);
		const float c0 = std::cos(a0);
		const float s1 = std::sin(a1);
		const float c1 = std::cos(a1);

		const float u0 = static_cast<float>(i) / static_cast<float>(divide_);
		const float u1 =
			static_cast<float>(i + 1) / static_cast<float>(divide_);

		const Vector4 top0 = Vector4{ -s0 * topRadius_,    height_,  c0 * topRadius_,    1.0f };
		const Vector4 top1 = Vector4{ -s1 * topRadius_,    height_,  c1 * topRadius_,    1.0f };
		const Vector4 bottom0 = Vector4{ -s0 * bottomRadius_, 0.0f,     c0 * bottomRadius_, 1.0f };
		const Vector4 bottom1 = Vector4{ -s1 * bottomRadius_, 0.0f,     c1 * bottomRadius_, 1.0f };

		const Vector3 n0 = Vector3{ -s0, 0.0f, c0 };
		const Vector3 n1 = Vector3{ -s1, 0.0f, c1 };

		const Vector2 uvTop0 = Vector2{ u0, 0.0f };
		const Vector2 uvTop1 = Vector2{ u1, 0.0f };
		const Vector2 uvBottom0 = Vector2{ u0, 1.0f };
		const Vector2 uvBottom1 = Vector2{ u1, 1.0f };

		vertices_.push_back(VertexData{ top0,    uvTop0,    n0 });
		vertices_.push_back(VertexData{ bottom0, uvBottom0, n0 });
		vertices_.push_back(VertexData{ top1,    uvTop1,    n1 });

		vertices_.push_back(VertexData{ bottom0, uvBottom0, n0 });
		vertices_.push_back(VertexData{ bottom1, uvBottom1, n1 });
		vertices_.push_back(VertexData{ top1,    uvTop1,    n1 });
	}
}