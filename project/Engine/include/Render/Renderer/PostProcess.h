#pragma once
#include "Core/Dx12Core.h"
#include "Math/Matrix4x4.h"
#include <wrl.h>

class SrvManager;

class PostProcess {
public:
  void Initialize(Dx12Core *dx12Core);
  void Draw(uint32_t renderSrvIndex, uint32_t depthSrvIndex,
            SrvManager *srvManager);

  /// <summary>
  /// ImGuiを使用したデバッグ用の設定UIを描画する
  /// </summary>
  void DrawDebugUI(const char* windowName = "PostEffect Settings");

  /// <summary>
  /// グレースケールの有効/無効を切り替える
  /// </summary>
  /// <param name="useGrayscale">trueで有効、falseで無効</param>
  void SetUseGrayscale(bool useGrayscale) { useGrayscale_ = useGrayscale; }

  /// <summary>
  /// グレースケールする際の色合い（セピア調など）を設定する
  /// </summary>
  /// <param name="r"></param>
  /// <param name="g"></param>
  /// <param name="b"></param>
  void SetMonotoneColor(float r, float g, float b) {
    monotoneColor_[0] = r;
    monotoneColor_[1] = g;
    monotoneColor_[2] = b;
  }

  /// <summary>
  /// ビネット（画面四隅を暗くする効果）の有効/無効を切り替える
  /// </summary>
  /// <param name="useVignette">trueで有効、falseで無効</param>
  void SetUseVignette(bool useVignette) { useVignette_ = useVignette; }

  /// <summary>
  /// ビネットのスケール（暗くなる範囲の強さ）を設定する
  /// </summary>
  /// <param name="scale">スケール値（大きいほど暗い領域が狭まる）</param>
  void SetVignetteScale(float scale) { vignetteScale_ = scale; }

  /// <summary>
  /// ビネットの減衰率を設定する
  /// </summary>
  /// <param name="exponent">減衰率（大きいほど境界がクッキリする）</param>
  void SetVignetteExponent(float exponent) { vignetteExponent_ = exponent; }

  /// <summary>
  /// ポストエフェクトの種類を切り替える
  /// </summary>
  /// <param name="type">0: None, 1: BoxFilter, 2: GaussianFilter, etc.</param>
  void SetPostEffectType(int type) { postEffectType_ = type; }
  
  /// <summary>
  /// 現在のポストエフェクトのタイプを取得する
  /// </summary>
  int GetPostEffectType() const { return postEffectType_; }

  /// <summary>
  /// ボックスフィルターのカーネルサイズを設定する
  /// </summary>
  /// <param name="k">サンプリング半径（1で3x3、2で5x5）</param>
  void SetBoxFilterK(int k) { boxFilterK_ = k; }

  /// <summary>
  /// ガウシアンフィルターのカーネルサイズを設定する
  /// </summary>
  /// <param name="k">サンプリング半径（1で3x3、2で5x5）</param>
  void SetGaussianFilterK(int k) { gaussianFilterK_ = k; }

  /// <summary>
  /// ガウシアンフィルターの標準偏差（ぼかしの強さ）を設定する
  /// </summary>
  /// <param name="sigma">シグマ値（大きいほど強くぼやける）</param>
  void SetGaussianSigma(float sigma) { gaussianSigma_ = sigma; }

  /// <summary>
  /// アウトラインの太さ・重みを設定する
  /// </summary>
  /// <param name="weight">輪郭線の重み</param>
  void SetDepthOutlineWeight(float weight) { depthOutlineWeight_ = weight; }

  /// <summary>
  /// アウトラインの減衰率を設定する
  /// </summary>
  /// <param name="attenuation">距離による減衰度合い</param>
  void SetDepthOutlineAttenuation(float attenuation) {
    depthOutlineAttenuation_ = attenuation;
  }

  /// <summary>
  /// ラジアルブラーの中心座標を設定する
  /// </summary>
  /// <param name="x">画面内X座標（0.0～1.0）</param>
  /// <param name="y">画面内Y座標（0.0～1.0）</param>
  void SetRadialBlurCenter(float x, float y) {
    radialBlurCenter_[0] = x;
    radialBlurCenter_[1] = y;
  }

  /// <summary>
  /// ラジアルブラーの幅（ぼかしの広がり具合）を設定する
  /// </summary>
  /// <param name="width">ぼかしの強さ</param>
  void SetRadialBlurWidth(float width) { radialBlurWidth_ = width; }

  /// <summary>
  /// ラジアルブラーのサンプリング回数を設定する
  /// </summary>
  /// <param name="samples">サンプリング数（多いほど綺麗だが重い）</param>
  void SetRadialBlurSamples(int samples) { radialBlurSamples_ = samples; }

  /// <summary>
  /// ラジアルブラーがかからない内側の半径を設定する
  /// </summary>
  /// <param name="radius">内側の半径（0.0～1.0）</param>
  void SetRadialBlurInnerRadius(float radius) {
    radialBlurInnerRadius_ = radius;
  }

  /// <summary>
  /// ラジアルブラーがかかる外側の半径を設定する
  /// </summary>
  /// <param name="radius">外側の半径（0.0～1.0）</param>
  void SetRadialBlurOuterRadius(float radius) {
    radialBlurOuterRadius_ = radius;
  }

  /// <summary>
  /// ラジアルブラーの色収差（RGBのズレ）の強さを設定する
  /// </summary>
  /// <param name="aberration">色収差の強さ</param>
  void SetRadialBlurAberration(float aberration) {
    radialBlurAberration_ = aberration;
  }

  /// <summary>
  /// 画面全体ディゾルブの進行度を設定する
  /// </summary>
  /// <param name="threshold">0.0（完全表示）～ 1.0（完全消失）</param>
  void SetDissolveThreshold(float threshold) { dissolveThreshold_ = threshold; }

  /// <summary>
  /// 画面全体ディゾルブのエッジの太さを設定する
  /// </summary>
  /// <param name="range">エッジの太さ（例: 0.05）</param>
  void SetDissolveEdgeRange(float range) { dissolveEdgeRange_ = range; }

  /// <summary>
  /// 画面全体ディゾルブのエッジの色を設定する
  /// </summary>
  /// <param name="r">赤成分</param>
  /// <param name="g">緑成分</param>
  /// <param name="b">青成分</param>
  void SetDissolveEdgeColor(float r, float g, float b) {
    dissolveEdgeColor_[0] = r;
    dissolveEdgeColor_[1] = g;
    dissolveEdgeColor_[2] = b;
  }

  /// <summary>
  /// シェーダーに渡す経過時間（乱数シードなどのアニメーション用）を設定する
  /// </summary>
  /// <param name="time">経過時間</param>
  void SetTime(float time) { time_ = time; }

  /// <summary>
  /// プロジェクション行列の逆行列を設定する
  /// </summary>
  /// <param name="m">逆プロジェクション行列</param>
  void SetProjectionInverse(const Matrix4x4 &m) { projectionInverse_ = m; }

  /// <summary>
  /// Dissolveなどに使用するマスク画像のSRVインデックスを設定する
  /// </summary>
  /// <param name="index">SRVインデックス</param>
  void SetMaskSrvIndex(uint32_t index) { maskSrvIndex_ = index; }

private:
  Dx12Core *dx12Core_ = nullptr;
  int postEffectType_ = 0;
  int boxFilterK_ = 1;
  int gaussianFilterK_ = 1;
  float gaussianSigma_ = 4.0f;
  bool useGrayscale_ = false;
  float monotoneColor_[3] = {1.0f, 1.0f, 1.0f};

  bool useVignette_ = false;
  float vignetteScale_ = 16.0f;
  float vignetteExponent_ = 0.8f;

  float depthOutlineWeight_ = 10.0f;
  float depthOutlineAttenuation_ = 0.05f;

  float radialBlurCenter_[2] = {0.5f, 0.5f};
  float radialBlurWidth_ = 0.01f;
  int32_t radialBlurSamples_ = 10;
  float radialBlurInnerRadius_ = 0.0f;
  float radialBlurOuterRadius_ = 0.5f;
  float radialBlurAberration_ = 0.1f;

  float dissolveThreshold_ = 0.5f;
  float dissolveEdgeRange_ = 0.05f;
  float dissolveEdgeColor_[3] = {1.0f, 0.4f, 0.3f};

  float time_ = 0.0f;

  uint32_t maskSrvIndex_ = 0;

  Matrix4x4 projectionInverse_;

  Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_ = nullptr;

  Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_ = nullptr;

  void CreateRootSignature();
  void CreatePSO();
};
