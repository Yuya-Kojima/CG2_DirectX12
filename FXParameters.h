#pragma once

struct FXParams {
  // Flame
  float flameSpeed;      // 下方向に流れる速さ
  float flameNoiseScale; // ノイズの細かさ
  float flameIntensity;  // 炎の強さ
  // Reveal
  float revealSpeed;   // revealの進行速度
  float revealWidth;   // revealの幅調整
  float flameDuration; // Flameが通過するまでの時間
  float padding;
};