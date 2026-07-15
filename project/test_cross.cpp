#include <iostream>

struct Vector3 { float x, y, z; };

Vector3 Cross(const Vector3 &a, const Vector3 &b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

int main() {
    Vector3 up = {0.0f, 1.0f, 0.0f};
    Vector3 fwd = {0.0f, 0.0f, 1.0f};
    
    Vector3 r1 = Cross(up, fwd);
    Vector3 r2 = Cross(fwd, up);
    
    std::cout << "Cross(up, fwd) = " << r1.x << ", " << r1.y << ", " << r1.z << std::endl;
    std::cout << "Cross(fwd, up) = " << r2.x << ", " << r2.y << ", " << r2.z << std::endl;
    return 0;
}
