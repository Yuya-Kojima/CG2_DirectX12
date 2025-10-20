#pragma once
#include <string>

namespace StringUtil {

// std::stringからstd::wstringに変換する関数
std::wstring ConvertString(const std::string &str);

// std::wstringからstd::stringに変換する関数
std::string ConvertString(const std::wstring &str);

}; // namespace StringUtil
