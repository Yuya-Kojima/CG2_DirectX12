#include "Render/ResourceObject.h"

ResourceObject::~ResourceObject() {

  if (resource_) {
    resource_->Release();
  }
}
