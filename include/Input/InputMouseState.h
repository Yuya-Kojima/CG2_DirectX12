#pragma once


class InputMouseState {

private:
  char mouseButtons_[2]{};
  char prevMouseButtons_[2]{};

public:
  void Update(const char current[2]); // 状態更新
  bool IsPress(int button) const;   // 押されている
  bool IsTrigger(int button) const; // 押した瞬間
  bool IsRelease(int button) const; // 離した瞬間
};
