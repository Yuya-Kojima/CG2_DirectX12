#pragma once
#include "Render/Model/Model.h"
#include <vector>

namespace RC {

/// @brief 基本的なプリミティブ形状のメッシュデータを生成する静的クラス
class MeshGenerator {
public:
  /// @brief 平面メッシュ（XZ平面）を生成
  static Model::ModelData GeneratePlane(float width = 1.0f, float height = 1.0f,
                                        uint32_t segmentsW = 1, uint32_t segmentsH = 1);

  /// @brief 直方体メッシュを生成
  static Model::ModelData GenerateBox(float width = 1.0f, float height = 1.0f,
                                      float depth = 1.0f);

  /// @brief 円盤メッシュ（XZ平面）を生成
  static Model::ModelData GenerateCircle(float radius = 1.0f, uint32_t segments = 32);

  /// @brief リング状（ドーナツ状）の円板メッシュ（XZ平面）を生成
  static Model::ModelData GenerateRing(float innerRadius = 0.5f,
                                       float outerRadius = 1.0f,
                                       uint32_t segments = 32);

  /// @brief 球体メッシュを生成
  static Model::ModelData GenerateSphere(float radius = 1.0f, uint32_t slices = 32,
                                         uint32_t stacks = 16);

  /// @brief 円柱メッシュを生成（上下のフタ付き、Y軸中心）
  static Model::ModelData GenerateCylinder(float radius = 0.5f, float height = 1.0f,
                                           uint32_t segments = 32);

  /// @brief カプセルメッシュを生成（Y軸中心）
  static Model::ModelData GenerateCapsule(float radius = 0.5f, float height = 2.0f,
                                          uint32_t slices = 32, uint32_t stacks = 16);

  /// @brief 円錐メッシュを生成
  static Model::ModelData GenerateCone(float radius = 0.5f, float height = 1.0f,
                                       uint32_t segments = 32);

  /// @brief トーラス（円環）メッシュを生成
  static Model::ModelData GenerateTorus(float majorRadius = 1.0f,
                                        float minorRadius = 0.2f,
                                        uint32_t majorSegments = 32,
                                        uint32_t minorSegments = 16);
};

} // namespace RC
