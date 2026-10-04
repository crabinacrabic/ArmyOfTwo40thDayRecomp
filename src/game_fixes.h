// game_fixes.h - guest-function overrides for Army of Two: The 40th Day
//
// Included once, from src/main.cpp, after the generated init header. Every
// definition here is a strong `extern "C"` symbol that replaces the weak alias
// codegen emits for the same name (DEFINE_REX_FUNC in generated pch):
//
//   __imp__sub_82XXXXXX  the recompiled body (always callable)
//   sub_82XXXXXX         weak alias -> __imp__ (what call sites and the
//                        dispatch table reference)
//
// generated/ is never touched. One override per guest function.
#pragma once

#include <atomic>
#include <mutex>

#include <rex/hook.h>
#include <rex/logging.h>

namespace game_fixes
{
  // ---------------------------------------------------------------------------
  // 1. Serialize XInput calls
  // ---------------------------------------------------------------------------
  // rex::input::InputSystem has no internal lock across multiple threads.
  // We keep a thread-safe guard here to serialize concurrent guest queries.
  inline std::mutex g_input_mutex;
  inline std::atomic<uint32_t> g_input_contention{0};

  class InputLock
  {
  public:
    InputLock()
    {
      if (!g_input_mutex.try_lock())
      {
        if (g_input_contention.fetch_add(1, std::memory_order_relaxed) == 0)
        {
          REXLOG_WARN("[fix] XamInput called from two guest threads simultaneously; serialized");
        }
        g_input_mutex.lock();
      }
    }
    ~InputLock() { g_input_mutex.unlock(); }
    InputLock(const InputLock &) = delete;
    InputLock &operator=(const InputLock &) = delete;
  };
} // namespace game_fixes
