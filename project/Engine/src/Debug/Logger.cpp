#include "Debug/Logger.h"
#include <Windows.h>

namespace Logger {
void Log(const std::string &message) {

#if defined(_DEBUG) || defined(DEVELOPMENT)

  OutputDebugStringA(message.c_str());

#else

  (void)message;

#endif
}
} // namespace Logger
