// game_constants.h - Game-specific constants for Army of Two: The 40th Day (454108D8)
#pragma once

#include <cstdint>

namespace GameConstants
{
  constexpr uint32_t kCodeBase = 0x82310000;
  constexpr uint32_t kCodeEnd  = 0x82310000 + 0x13C4120; // 0x836D4120
}

namespace GameConstants::PatchConstants
{
  struct Patch16
  {
    std::uintptr_t address;
    std::uint16_t value;
  };

  struct Patch8
  {
    std::uintptr_t address;
    std::uint8_t value;
  };

  // Ported from xenia-canary game-patches for Army of Two: The 40th Day (454108D8)
  // Branch around the frame-limiter check (be16 0x4800).
  constexpr Patch16 FpsUnlockBranch()
  {
    return Patch16{
        0x8275F382,
        0x4800};
  }

  // Frame-rate target byte: 0x00=unlimited, 0x01=60FPS, 0x02=30FPS.
  constexpr Patch8 FpsUnlockMode()
  {
    return Patch8{
        0x82EB05D7,
        0x02}; // 30 FPS by default for retro stability
  }
}