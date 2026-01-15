#pragma once
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <xaudio2.h>

class SoundManager {

public:
  // 音声データ
  struct SoundData {
    // 波形フォーマット
    WAVEFORMATEX wfex;

    // バッファの先頭アドレス
    BYTE *pBuffer;

    // バッファサイズ
    unsigned int bufferSize;
  };

  /// <summary>
  /// インスタンス取得
  /// </summary>
  static SoundManager *GetInstance();

  /// <summary>
  /// 初期化
  /// </summary>
  void Initialize(IXAudio2 *xAudio2);

  /// <summary>
  /// 終了
  /// </summary>
  void Finalize();

  /// <summary>
  /// 更新
  /// </summary>
  void Update();

  void Load(const std::string &key, const std::string &filename);
  void Unload(const std::string &key);

  void PlaySE(const std::string &key);

  void PlayBGM(const std::string &key);
  void StopBGM();

  bool Exists(const std::string &key) const;

  // ↓ここからは旧サウンド用関数

  /// <summary>
  /// waveファイルを読み込む
  /// </summary>
  /// <param name="filename">ファイル名</param>
  /// <returns></returns>
  SoundData SoundLoadWave(const char *filename);

  /// <summary>
  /// 音声データの解放
  /// </summary>
  /// <param name="soundData"></param>
  void SoundUnload(SoundData *soundData);

  /// <summary>
  /// サウンド再生
  /// </summary>
  /// <param name="xAudio2">再生するためのxAudio2</param>
  /// <param
  /// name="soundData">音声データ(波形データ、サイズ、フォーマット)</param>
  void SoundPlayWave(IXAudio2 *xAudio2, const SoundData &soundData);

  void SoundPlayWave(const SoundData &soundData);

private:
  SoundManager() = default;
  ~SoundManager() = default;
  SoundManager(const SoundManager &) = delete;
  SoundManager &operator=(const SoundManager &) = delete;

private:
  IXAudio2 *xAudio2_ = nullptr;

  // std::vector<IXAudio2SourceVoice *> activeVoices_;

  std::unordered_map<std::string, SoundData> sounds_;

  // BGM（1本だけ）
  IXAudio2SourceVoice *bgmVoice_ = nullptr;
  std::string bgmKey_;

  // SE（使い捨てvoice。Updateで回収）
  std::vector<IXAudio2SourceVoice *> seVoices_;
};
