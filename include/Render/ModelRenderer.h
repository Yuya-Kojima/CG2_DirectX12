#pragma once
#include "Core/Dx12Core.h"

class ModelRenderer {

public:
  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(Dx12Core *dx12Core);

  Dx12Core *GetDx12Core() const { return dx12Core_; }

private:
  Dx12Core *dx12Core_;
};
