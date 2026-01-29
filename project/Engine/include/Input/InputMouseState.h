#pragma once

class InputMouseState {

public:
  void Update(const char current[2], int x, int y); // 状態更新

  bool IsPress(int button) const;   // 押されている
  bool IsTrigger(int button) const; // 押した瞬間
  bool IsRelease(int button) const; // 離した瞬間

  int GetX() const { return x_; }
  int GetY() const { return y_; }
  int GetDeltaX() const { return x_ - prevX_; }
  int GetDeltaY() const { return y_ - prevY_; }

private:
  char mouseButtons_[2]{};
  char prevMouseButtons_[2]{};

  int x_ = 0;
  int y_ = 0;
  int prevX_ = 0;
  int prevY_ = 0;
};
