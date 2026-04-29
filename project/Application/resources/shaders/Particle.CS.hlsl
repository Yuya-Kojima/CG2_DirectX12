// GPUパーティクル用の構造体
struct Particle {
    float32_t3 translate;
    float32_t padding1;
    float32_t3 scale;
    float32_t padding2;
    float32_t3 rotate;
    float32_t lifeTime;
    float32_t3 velocity;
    float32_t currentTime;
    float32_t4 color;
    float32_t3 scaleVelocity;
    float32_t padding3;
};
