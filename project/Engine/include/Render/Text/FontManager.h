#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "stb_truetype.h"

struct FontData {
  std::string name;
  std::vector<stbtt_bakedchar> cdata; // ASCII文字用データ(96文字)
  int textureWidth;
  int textureHeight;
  float pixelHeight;
};

class FontManager {
public:
  static FontManager *GetInstance();

  void Initialize();
  void Finalize();

  /// <summary>
  /// フォントファイルを読み込み、アトラス画像を生成してTextureManagerに登録する
  /// </summary>
  void LoadFont(const std::string &fontName, const std::string &filePath, float pixelHeight = 32.0f);

  /// <summary>
  /// 読み込み済みのフォントデータを取得する
  /// </summary>
  const FontData *GetFontData(const std::string &fontName) const;

  /// <summary>
  /// ロードされているすべてのフォント名を取得する
  /// </summary>
  std::vector<std::string> GetLoadedFontNames() const;

private:
  FontManager() = default;
  ~FontManager() = default;
  FontManager(FontManager &) = delete;
  FontManager &operator=(FontManager &) = delete;

  std::unordered_map<std::string, FontData> fontDatas_;
};
