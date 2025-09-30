#pragma once

#include <d3d12.h>

class ResourceObject {

public:
  /// <summary>
  /// コンストラクタ
  /// </summary>
  /// <param name="resource"></param>
  ResourceObject(ID3D12Resource *resource) : resource_(resource) {}

  /// <summary>
  /// デストラクタ
  /// </summary>
  ~ResourceObject();

  ID3D12Resource *Get() { return resource_; };

private:
  ID3D12Resource *resource_;
};
