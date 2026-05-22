#pragma once
#include <string>

namespace Logger {

void Log(const std::string &message);

// Editor Console UI用のログ取得・クリア関数
std::string GetConsoleLog();
void ClearConsoleLog();

};
