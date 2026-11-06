#include "thread.h"
#include "util_likely.h"
#include "util_string.h"

#include "log/log.h"

#include <atomic>

#ifdef _WIN32

namespace dxvk::this_thread {

  bool isInModuleDetachment() {
    using PFN_RtlDllShutdownInProgress = BOOLEAN (WINAPI *)();

    static auto RtlDllShutdownInProgress = reinterpret_cast<PFN_RtlDllShutdownInProgress>(
      ::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "RtlDllShutdownInProgress"));

    return RtlDllShutdownInProgress();
  }


  void reportUnhandledException() noexcept {
    // We are inside a catch block, so the exception that is being handled
    // can be thrown again here to find out what it is.
    const uint32_t id = uint32_t(::GetCurrentThreadId());

    try {
      throw;
    } catch (const DxvkError& e) {
      Logger::err(str::format("DXVK: Unhandled DxvkError on worker thread ", id, ":"));
      Logger::err(e.message());
    } catch (const std::exception& e) {
      Logger::err(str::format("DXVK: Unhandled exception on worker thread ", id, ": ", e.what()));
    } catch (...) {
      Logger::err(str::format("DXVK: Unhandled exception of unknown type on worker thread ", id));
    }
  }

}

#else

namespace dxvk::this_thread {

  static std::atomic<uint32_t> g_threadCtr = { 0u };
  static thread_local uint32_t g_threadId  = 0u;

  // This implementation returns thread ids unique to the current instance.
  // ie. if you use this across multiple .so's then you might get conflicting ids.
  //
  // This isn't an issue for us, as it is only used by the spinlock implementation,
  // but may be for you if you use this elsewhere.
  uint32_t get_id() {
    if (unlikely(!g_threadId))
      g_threadId = ++g_threadCtr;

    return g_threadId;
  }

}

#endif
