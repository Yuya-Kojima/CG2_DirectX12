#pragma once

#include <d3d12.h>
#include <wrl.h>

class ResourceObject {

public:
  /// <summary>
  /// コンストラクタ
  /// </summary>
  /// <param name="resource"></param>
  ResourceObject(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
      : resource_(resource) {}

  ID3D12Resource *Get() { return resource_.Get(); };

private:
  Microsoft::WRL::ComPtr<ID3D12Resource> resource_;
};
