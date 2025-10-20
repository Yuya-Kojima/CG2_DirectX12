#include "GameCamera.h"
#include "MathUtil.h"
#include "WindowSystem.h"

GameCamera::GameCamera()
    : transform({{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -10.0f}}),
      fov(0.45f), aspectRatio(float(WindowSystem::kClientWidth) /
                              float(WindowSystem::kClientHeight)),
      nearClip(0.1f), farClip(100.0f),
      worldMatrix(MakeAffineMatrix(transform.scale, transform.rotate,
                                   transform.translate)),
      viewMatrix(Inverse(worldMatrix)),
      projectionMatrix(
          MakePerspectiveFovMatrix(fov, aspectRatio, nearClip, farClip)),
      viewProjectionMatrix(Multiply(viewMatrix, projectionMatrix)) {}

void GameCamera::Update() {

  worldMatrix =
      MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);

  viewMatrix = Inverse(worldMatrix);

  projectionMatrix =
      MakePerspectiveFovMatrix(fov, aspectRatio, nearClip, farClip);

  viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);
}
