// game_patches.h - Game-specific memory patches for Army of Two: The 40th Day
#pragma once

#include <algorithm>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/memory/utils.h>
#include <rex/runtime.h>

#include "game_cvars.h"
#include "game_constants.h"

namespace game_patches
{
  // Ported from xenia-canary game-patches for Army of Two: The 40th Day (454108D8)
  static void FpsUnlock()
  {
    if (!REXCVAR_GET(ao2_fps_unlock))
    {
      return;
    }

    auto *rt = rex::Runtime::instance();
    uint8_t *base = rt ? rt->virtual_membase() : nullptr;
    if (!base)
    {
      return;
    }

    // Branch around limiter check (be16)
    {
      auto patch = GameConstants::PatchConstants::FpsUnlockBranch();
      if (patch.address != 0)
      {
        uint8_t *p = base + patch.address;
        rex::memory::PageAccess old_access{};
        rex::memory::Protect(p, sizeof(patch.value), rex::memory::PageAccess::kReadWrite, &old_access);
        p[0] = static_cast<uint8_t>(patch.value >> 8);
        p[1] = static_cast<uint8_t>(patch.value);
        rex::memory::Protect(p, sizeof(patch.value), old_access, nullptr);
      }
    }

    // Target mode byte
    {
      auto patch = GameConstants::PatchConstants::FpsUnlockMode();
      if (patch.address != 0)
      {
        int32_t mode = REXCVAR_GET(ao2_fps_unlock_mode);
        uint8_t memoryValue = static_cast<uint8_t>(std::clamp(mode, 0, 2));
        uint8_t *p = base + patch.address;
        rex::memory::PageAccess old_access{};
        rex::memory::Protect(p, 1, rex::memory::PageAccess::kReadWrite, &old_access);
        p[0] = memoryValue;
        rex::memory::Protect(p, 1, old_access, nullptr);
      }
    }
  }

  static void DisableMsaa()
  {
    // Will be populated once exact MSAA patch address for 40th Day is verified
  }

  static void AnisotropicFiltering16x()
  {
    // Will be populated once exact AF address for 40th Day is verified
  }
} // namespace game_patches
