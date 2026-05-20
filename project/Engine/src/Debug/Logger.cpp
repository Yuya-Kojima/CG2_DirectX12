#include "Debug/Logger.h"
#include <Windows.h>

#if defined(_DEBUG) || defined(DEVELOPMENT)

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>

namespace Logger {
namespace {

// ---- 設定 ----
constexpr size_t kMaxBytesBuffered = 256 * 1024; // 256KB
constexpr size_t kNotifyThreshold = kMaxBytesBuffered / 2;
constexpr int kFlushIntervalMs = 50;

// ---- 状態 ----
std::mutex gMutex;
std::condition_variable gCv;
std::string gBuffer;
std::string gConsoleBuffer;

std::atomic<bool> gRunning{false};
std::atomic<bool> gStarted{false};
std::thread gWorker;

void WorkerLoop() {
  std::unique_lock<std::mutex> lock(gMutex);

  while (gRunning.load(std::memory_order_acquire)) {
    gCv.wait_for(lock, std::chrono::milliseconds(kFlushIntervalMs));

    if (!gBuffer.empty()) {
      std::string out;
      out.swap(gBuffer);
      lock.unlock();
      OutputDebugStringA(out.c_str());
      lock.lock();
    }
  }

  // 終了時 flush
  if (!gBuffer.empty()) {
    std::string out;
    out.swap(gBuffer);
    lock.unlock();
    OutputDebugStringA(out.c_str());
    lock.lock();
  }
}

void Shutdown() {
  if (!gStarted.load(std::memory_order_acquire)) {
    return;
  }
  gRunning.store(false, std::memory_order_release);
  gCv.notify_all();
  if (gWorker.joinable()) {
    gWorker.join();
  }
}

void EnsureStarted() {
  bool expected = false;
  if (gStarted.compare_exchange_strong(expected, true,
                                       std::memory_order_acq_rel)) {
    gRunning.store(true, std::memory_order_release);
    gWorker = std::thread(WorkerLoop);
    std::atexit(Shutdown);
  }
}

} // namespace

void Log(const std::string &message) {
  EnsureStarted();

  bool needNotify = false;

  {
    std::lock_guard<std::mutex> lock(gMutex);

    if (gBuffer.size() >= kMaxBytesBuffered) {
      needNotify = true; // もう溜まりすぎ → とにかく吐かせる
    } else {
      const size_t remain = kMaxBytesBuffered - gBuffer.size();
      if (message.size() <= remain) {
        gBuffer += message;
        gConsoleBuffer += message;
      } else {
        gBuffer.append(message.data(), remain);
        gBuffer += "\n[Logger] (log truncated: too much output)\n";
        gConsoleBuffer.append(message.data(), remain);
        gConsoleBuffer += "\n[Logger] (log truncated: too much output)\n";
      }

      // ConsoleBufferが大きすぎたら古いものを消す (簡易的に後半を残す)
      if (gConsoleBuffer.size() > kMaxBytesBuffered) {
          gConsoleBuffer = gConsoleBuffer.substr(gConsoleBuffer.size() - (kMaxBytesBuffered / 2));
      }

      if (gBuffer.size() >= kNotifyThreshold) {
        needNotify = true;
      }
    }
  }

  if (needNotify) {
    gCv.notify_one();
  }
}

std::string GetConsoleLog() {
    std::lock_guard<std::mutex> lock(gMutex);
    return gConsoleBuffer;
}

void ClearConsoleLog() {
    std::lock_guard<std::mutex> lock(gMutex);
    gConsoleBuffer.clear();
}

} // namespace Logger

#else

namespace Logger {
void Log(const std::string &message) { (void)message; }
std::string GetConsoleLog() { return ""; }
void ClearConsoleLog() {}
} // namespace Logger

#endif
