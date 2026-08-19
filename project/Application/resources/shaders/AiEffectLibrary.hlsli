// --- Standard Pro Shader Library (HLSL) ---
float3 mod289(float3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
float4 mod289(float4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
float4 permute(float4 x) { return mod289(((x*34.0)+1.0)*x); }
float4 taylorInvSqrt(float4 r) { return 1.79284291400159 - 0.85373472095314 * r; }
float snoise(float3 v) {
  const float2 C = float2(1.0/6.0, 1.0/3.0);
  const float4 D = float4(0.0, 0.5, 1.0, 2.0);
  float3 i  = floor(v + dot(v, C.yyy));
  float3 x0 = v - i + dot(i, C.xxx);
  float3 g = step(x0.yzx, x0.xyz);
  float3 l = 1.0 - g;
  float3 i1 = min(g.xyz, l.zxy);
  float3 i2 = max(g.xyz, l.zxy);
  float3 x1 = x0 - i1 + C.xxx;
  float3 x2 = x0 - i2 + C.yyy;
  float3 x3 = x0 - D.yyy;
  i = mod289(i);
  float4 p = permute( permute( permute(
             i.z + float4(0.0, i1.z, i2.z, 1.0))
           + i.y + float4(0.0, i1.y, i2.y, 1.0))
           + i.x + float4(0.0, i1.x, i2.x, 1.0));
  float4 j = p - 49.0 * floor(p * (1.0 / 49.0));
  float4 x_ = floor(j * (1.0 / 7.0));
  float4 y_ = floor(j - 7.0 * x_);
  float4 x = x_ * (1.0 / 7.0);
  float4 y = y_ * (1.0 / 7.0);
  float4 h = 1.0 - abs(x) - abs(y);
  float4 b0 = float4( x.xy, y.xy );
  float4 b1 = float4( x.zw, y.zw );
  float4 s0 = floor(b0)*2.0 + 1.0;
  float4 s1 = floor(b1)*2.0 + 1.0;
  float4 sh = -step(h, float4(0.0,0.0,0.0,0.0));
  float4 a0 = b0.xzyw + s0.xzyw*sh.xxyy;
  float4 a1 = b1.xzyw + s1.xzyw*sh.zzww;
  float3 p0 = float3(a0.xy,h.x);
  float3 p1 = float3(a0.zw,h.y);
  float3 p2 = float3(a1.xy,h.z);
  float3 p3 = float3(a1.zw,h.w);
  float4 norm = taylorInvSqrt(float4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x; p1 *= norm.y; p2 *= norm.z; p3 *= norm.w;
  float4 m = max(0.6 - float4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m; return 42.0 * dot( m*m, float4( dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3) ) );
}
float snoise(float2 v) { return snoise(float3(v.x, v.y, 0.0)); }
float fbm(float3 x) {
    float v = 0.0; float a = 0.5; float3 shift = float3(100.0, 100.0, 100.0);
    for (int i = 0; i < 5; ++i) { v += a * snoise(x); x = x * 2.0 + shift; a *= 0.5; }
    return v;
}
float fbm(float2 x) { return fbm(float3(x.x, x.y, 0.0)); }
float3 firePalette(float t) {
    t = max(0.0, min(1.0, t));
    float3 c1 = float3(0.0, 0.0, 0.0); float3 c2 = float3(1.0, 0.1, 0.0); float3 c3 = float3(1.0, 0.6, 0.0); float3 c4 = float3(1.0, 1.0, 0.6); float3 c5 = float3(1.0, 1.0, 1.0);
    if(t < 0.25) return lerp(c1, c2, t / 0.25);
    if(t < 0.50) return lerp(c2, c3, (t - 0.25) / 0.25);
    if(t < 0.75) return lerp(c3, c4, (t - 0.50) / 0.25);
    return lerp(c4, c5, (t - 0.75) / 0.25);
}
float hash(float n) { return frac(sin(n) * 43758.5453123); }
float hash2(float2 p) { return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453); }
float2 hash22(float2 p) { p = float2(dot(p,float2(127.1,311.7)), dot(p,float2(269.5,183.3))); return frac(sin(p)*43758.5453); }

// --- Color & Sampling ---
float4 sampleWithAberration(Texture2D tex, SamplerState samp, float2 uv, float2 dir, float amount) {
    float4 colR = tex.Sample(samp, uv + dir * amount);
    float4 colG = tex.Sample(samp, uv);
    float4 colB = tex.Sample(samp, uv - dir * amount);
    return float4(colR.r, colG.g, colB.b, max(colR.a, max(colG.a, colB.a)));
}
float3 rgb2hsv(float3 c) {
    float4 K = float4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}
float3 hsv2rgb(float3 c) {
    float4 K = float4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
float luminance(float4 col) { return dot(col.rgb, float3(0.299, 0.587, 0.114)); }
float luminance(float3 col) { return dot(col, float3(0.299, 0.587, 0.114)); }

// --- Distortion & UV Math ---
float2 applyPixelate(float2 uv, float pixels) { return floor(uv * pixels) / pixels; }
float2 polarCoords(float2 uv, float2 center) {
    float2 p = uv - center;
    return float2(length(p), atan2(p.y, p.x));
}
float2 applyVHSGlitch(float2 uv, float time, float intensity) {
    float tear = snoise(float2(0.0, uv.y * 10.0 + time * 5.0));
    float threshold = 0.8 - intensity * 0.5;
    if (tear > threshold) { return uv + float2((tear - threshold) * 0.2, 0.0); }
    return uv;
}
float2 applyDigitalGlitch(float2 uv, float time, float2 gridSize, float probability, float2 amount) {
    float2 block = floor(uv * gridSize);
    float t = fmod(floor(time * 15.0), 1000.0);
    float rand = hash2(block + t);
    if (rand < probability) {
        float2 offset = hash22(block + t) * 2.0 - 1.0;
        return uv + offset * amount;
    }
    return uv;
}

// --- Advanced Noise ---
float valueNoise(float2 p) {
    float2 i = floor(p); float2 f = frac(p); float2 u = f*f*(3.0-2.0*f);
    return lerp(lerp(hash2(i + float2(0.0,0.0)), hash2(i + float2(1.0,0.0)), u.x),
                lerp(hash2(i + float2(0.0,1.0)), hash2(i + float2(1.0,1.0)), u.x), u.y);
}
float voronoi(float2 x) {
    float2 n = floor(x); float2 f = frac(x); float m = 8.0;
    for(int j=-1; j<=1; j++) {
        for(int i=-1; i<=1; i++) {
            float2 g = float2(float(i),float(j));
            float2 o = hash22(n + g);
            float2 r = g - f + o;
            m = min(m, dot(r,r));
        }
    }
    return sqrt(m);
}
// --- End Standard Pro Shader Library ---
