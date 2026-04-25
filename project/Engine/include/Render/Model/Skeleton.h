#pragma once
#include "Math/MathUtil.h"
#include "Math/Transform.h"
#include "Render/Model/Model.h"
#include <map>
#include <optional>
#include <string>
#include <vector>

struct Joint {
    QuaternionTransform transform; // Transform情報
    Matrix4x4 localMatrix; // localMatrix
    Matrix4x4 skeletonSpaceMatrix; // skeletonSpaceでの変換行列
    std::string name; // 名前
    std::vector<int32_t> children; // 子JointのIndexのリスト。いなければ空
    int32_t index; // 自身のIndex
    std::optional<int32_t> parent; // 親JointのIndex。いなければnull
};

struct Skeleton {
    int32_t root; // RootJointのIndex
    std::map<std::string, int32_t> jointMap; // Joint名とIndexとの辞書
    std::vector<Joint> joints; // 所属しているジョイント
};

void Update(Skeleton& skeleton);
void ApplyAnimation(Skeleton& skeleton, const Animation& animation, float animationTime);

Skeleton CreateSkeleton(const Model::Node& rootNode);
int32_t CreateJoint(const Model::Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);
