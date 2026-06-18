#include "Render/Text/FontManager.h"
#include "Debug/Logger.h"
#include "Render/Texture/TextureManager.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

FontManager *FontManager::GetInstance() {
  static FontManager instance;
  return &instance;
}

void FontManager::Initialize() {
  fontDatas_.clear();

  std::string directoryPath = "resources/Fonts";
  if (!std::filesystem::exists(directoryPath)) {
    return;
  }

  for (const auto &entry : std::filesystem::directory_iterator(directoryPath)) {
    if (entry.is_regular_file()) {
      std::string extension = entry.path().extension().string();
      // 拡張子を小文字にして比較
      std::transform(extension.begin(), extension.end(), extension.begin(),
                     [](unsigned char c) { return std::tolower(c); });

      if (extension == ".ttf" || extension == ".ttc" || extension == ".otf") {
        std::string fontName = entry.path().stem().string();
        std::string filePath = entry.path().string();

        // パスのセパレータを統一 (Windows対応)
        std::replace(filePath.begin(), filePath.end(), '\\', '/');

        LoadFont(fontName, filePath, 64.0f);
      }
    }
  }
}

void FontManager::Finalize() { fontDatas_.clear(); }

void FontManager::LoadFont(const std::string &fontName,
                           const std::string &filePath, float pixelHeight) {
  if (fontDatas_.contains(fontName)) {
    return; // Already loaded
  }

  // ファイル読み込み
  std::ifstream file(filePath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    std::ofstream out("font_debug.txt");
    out << "Failed to open font file: " << filePath << std::endl;
    out.close();
    Logger::Log("Failed to open font file: " + filePath + "\n");
    return;
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<unsigned char> ttf_buffer(size);
  if (!file.read(reinterpret_cast<char *>(ttf_buffer.data()), size)) {
    std::ofstream out("font_debug.txt");
    out << "Failed to read font file: " << filePath << std::endl;
    out.close();
    Logger::Log("Failed to read font file: " + filePath + "\n");
    return;
  }

  // テクスチャサイズの決定（解像度と文字数によるが、一旦固定で十分大きく取る）
  int texWidth = 2048;
  int texHeight = 2048;
  std::vector<unsigned char> temp_bitmap(texWidth * texHeight);

  // ベイク処理（ASCII文字 32～126 の96文字分）
  FontData fontData;
  fontData.name = fontName;
  fontData.cdata.resize(96);
  fontData.textureWidth = texWidth;
  fontData.textureHeight = texHeight;
  fontData.pixelHeight = pixelHeight;

  int result = stbtt_BakeFontBitmap(ttf_buffer.data(), 0, pixelHeight,
                                    temp_bitmap.data(), texWidth, texHeight, 32,
                                    96, fontData.cdata.data());
  if (result <= 0) {
    std::ofstream out("font_debug.txt");
    out << "Failed to bake font: " << fontName << " (result: " << result << ")"
        << std::endl;
    out.close();
    Logger::Log("Failed to bake font: " + fontName +
                " (result: " + std::to_string(result) + ")\n");
    return;
  }
  Logger::Log("Bake font success: " + fontName +
              " (result: " + std::to_string(result) + ")\n");

  // stb_truetypeは1ピクセル1バイト（アルファ値のみ）のビットマップを返すため、
  // DXGI_FORMAT_R8G8B8A8_UNORM用の4バイト（RGBA）配列に変換する
  std::vector<uint8_t> rgba_bitmap(texWidth * texHeight * 4);
  for (int i = 0; i < texWidth * texHeight; ++i) {
    rgba_bitmap[i * 4 + 0] = 255;            // R
    rgba_bitmap[i * 4 + 1] = 255;            // G
    rgba_bitmap[i * 4 + 2] = 255;            // B
    rgba_bitmap[i * 4 + 3] = temp_bitmap[i]; // A
  }

  // TextureManagerに「メモリからロードしたテクスチャ」として登録
  TextureManager::GetInstance()->LoadTextureFromMemory(
      fontName, rgba_bitmap.data(), texWidth, texHeight);
  Logger::Log("LoadTextureFromMemory called for: " + fontName + "\n");

  fontDatas_[fontName] = fontData;
}

const FontData *FontManager::GetFontData(const std::string &fontName) const {
  if (fontDatas_.contains(fontName)) {
    return &fontDatas_.at(fontName);
  }
  return nullptr;
}

std::vector<std::string> FontManager::GetLoadedFontNames() const {
  std::vector<std::string> names;
  for (const auto &pair : fontDatas_) {
    names.push_back(pair.first);
  }
  return names;
}
