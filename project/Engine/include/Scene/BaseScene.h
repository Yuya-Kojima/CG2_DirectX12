#pragma once
class BaseScene {

public:
  virtual ~BaseScene() = default;

  virtual void Initialize(EngineBase *engine) = 0;
  virtual void Finalize() = 0;
  virtual void Update() = 0;
  virtual void Draw2D() = 0;
  virtual void Draw3D() = 0;
};
