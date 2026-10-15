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

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

// ---------------------------------------------------------------------------
// 2. Ring Buffer Physical Alias AV Write Guard (0x00000001BF9A0240)
// ---------------------------------------------------------------------------
// In Army of Two: The 40th Day, GPU command buffers write PM4 packets to the
// uncached physical alias 0xA0000000 | 0x1F9A0000 = 0xBF9A0000.
// When sub_82EAF838 allocates a new chunk or sub_82EAFAE8 writes to it,
// the host page can be write-protected or uncommitted by XMemory invalidation.
// This guard commits and marks the memory PAGE_READWRITE before writing.
REX_HOOK_RAW(sub_82EAF838)
{
  __imp__sub_82EAF838(ctx, base);
#if defined(_WIN32)
  uint32_t new_buf = ctx.r3.u32;
  if (new_buf != 0)
  {
    VirtualAlloc(base + (new_buf & ~0xFFFFull), 0x40000, MEM_COMMIT, PAGE_READWRITE);
    DWORD old_p;
    VirtualProtect(base + (new_buf & ~0xFFFFull), 0x40000, PAGE_READWRITE, &old_p);
  }
#endif
}

REX_HOOK_RAW(sub_82EAFAE8)
{
#if defined(_WIN32)
  uint32_t ring = ctx.r3.u32;
  if (ring != 0)
  {
    uint32_t cur_buf = REX_LOAD_U32(ring + 48);
    if (cur_buf != 0)
    {
      VirtualAlloc(base + (cur_buf & ~0xFFFFull), 0x40000, MEM_COMMIT, PAGE_READWRITE);
      DWORD old_p;
      VirtualProtect(base + (cur_buf & ~0xFFFFull), 0x40000, PAGE_READWRITE, &old_p);
    }
  }
#endif
  __imp__sub_82EAFAE8(ctx, base);
}

REX_HOOK_RAW(sub_82EB1658)
{
#if defined(_WIN32)
  uint32_t buf = ctx.r3.u32;
  if (buf != 0)
  {
    VirtualAlloc(base + (buf & ~0xFFFFull), 0x40000, MEM_COMMIT, PAGE_READWRITE);
    DWORD old_p;
    VirtualProtect(base + (buf & ~0xFFFFull), 0x40000, PAGE_READWRITE, &old_p);
  }
#endif
  __imp__sub_82EB1658(ctx, base);
}

// ---------------------------------------------------------------------------
// 3. Lens Flare Wall Bleeding Fix (sub_82CA7A08, sub_82C9A9F0)
// ---------------------------------------------------------------------------
// In Army of Two: The 40th Day, lens flares bleed through walls because the
// engine renders flare elements without proper depth occlusion.
// When ao2_disable_lens_flares is enabled, we prevent the lens flare manager
// from being registered/initialized by returning NULL (0) in sub_82CA7A08 and
// returning early in sub_82C9A9F0. Call sites in UE3 check for NULL pointer
// before rendering flares and cleanly skip them, completely eliminating the
// bleeding flare halos while preserving all real light sources, shadows,
// and world geometry.
REX_HOOK_RAW(sub_82CA7A08)
{
  if (REXCVAR_GET(ao2_disable_lens_flares))
  {
    ctx.r3.u64 = 0;
    return;
  }
  __imp__sub_82CA7A08(ctx, base);
}

REX_HOOK_RAW(sub_82C9A9F0)
{
  if (REXCVAR_GET(ao2_disable_lens_flares))
  {
    return;
  }
  __imp__sub_82C9A9F0(ctx, base);
}


