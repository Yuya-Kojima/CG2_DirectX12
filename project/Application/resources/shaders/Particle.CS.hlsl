// GPU Particle 用の構造体
struct Particle {
    float32_t3 translate;
    float32_t3 scale;
    float32_t lifeTime;
    float32_t3 velocity;
    float32_t currentTime;
    float32_t4 color;
};

// --- 今後のステップでここにComputeShaderのメイン処理（[numthreads(X, Y, Z)] や main()）を追加していきます ---
