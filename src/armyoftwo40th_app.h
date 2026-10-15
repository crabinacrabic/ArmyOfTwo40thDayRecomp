// armyoftwo40threcomp - Army of Two: The 40th Day Recompiled Project
// Based on ReXGlue SDK v0.10.0+

#pragma once

#include <cstdlib>
#include <memory>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/system/gpu_plugin.h>

#include "generated/default/armyoftwo40threcomp_pch.h"

#if defined(_WIN32)
static LONG WINAPI GuestMemoryCrashHandler(EXCEPTION_POINTERS *ep)
{
  if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
  {
    uintptr_t fault_addr = ep->ExceptionRecord->ExceptionInformation[1];
    uint8_t *base = rex::Runtime::instance() ? rex::Runtime::instance()->virtual_membase() : nullptr;
    if (base && fault_addr >= (uintptr_t)base && fault_addr < (uintptr_t)base + 0x100000000ull)
    {
      uint32_t guest_addr = (uint32_t)(fault_addr - (uintptr_t)base);
      if (guest_addr >= 0x7F000000u && guest_addr <= 0x7FFFFFFFu)
      {
        return EXCEPTION_CONTINUE_SEARCH;
      }
      MEMORY_BASIC_INFORMATION mbi{};
      if (VirtualQuery((void *)fault_addr, &mbi, sizeof(mbi)))
      {
        if (mbi.State == MEM_COMMIT)
        {
          DWORD old_protect = 0;
          DWORD new_protect = (mbi.AllocationProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
                                  ? PAGE_EXECUTE_READWRITE
                                  : PAGE_READWRITE;
          if (VirtualProtect(mbi.BaseAddress, mbi.RegionSize, new_protect, &old_protect))
          {
            return EXCEPTION_CONTINUE_EXECUTION;
          }
          if (new_protect != PAGE_READWRITE && VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &old_protect))
          {
            return EXCEPTION_CONTINUE_EXECUTION;
          }
          void *page_4k = (void *)(fault_addr & ~0xFFFull);
          if (VirtualProtect(page_4k, 0x1000, PAGE_READWRITE, &old_protect))
          {
            return EXCEPTION_CONTINUE_EXECUTION;
          }
        }
        else if (mbi.State == MEM_RESERVE)
        {
          void *page_base = (void *)(fault_addr & ~0xFFFFull);
          if (VirtualAlloc(page_base, 0x10000, MEM_COMMIT, PAGE_READWRITE))
          {
            return EXCEPTION_CONTINUE_EXECUTION;
          }
          void *page_4k = (void *)(fault_addr & ~0xFFFull);
          if (VirtualAlloc(page_4k, 0x1000, MEM_COMMIT, PAGE_READWRITE))
          {
            return EXCEPTION_CONTINUE_EXECUTION;
          }
        }
        else if (mbi.State == MEM_FREE)
        {
          void *page_base = (void *)(fault_addr & ~0xFFFFull);
          if (VirtualAlloc(page_base, 0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))
          {
            return EXCEPTION_CONTINUE_EXECUTION;
          }
        }
      }
    }
  }
  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

#include "debug_tools.h"
#include "game_cvars.h"
#include "game_patches.h"
#include "hardware_overlay.h"
#include "utils.h"

#ifdef REXGLUE_ENABLE_PERF_COUNTERS
#include <rex/perf/counter.h>
#endif

extern const rex::PPCImageInfo PPCImageConfig;

class Armyoftwo40thApp : public rex::ReXApp
{
public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext &ctx)
  {
#if defined(_WIN32)
    AddVectoredExceptionHandler(0, GuestMemoryCrashHandler);
#endif
    return std::unique_ptr<Armyoftwo40thApp>(new Armyoftwo40thApp(ctx, "armyoftwo40threcomp",
                                                                 PPCImageConfig));
  }

  void PrecommitMemoryAliases()
  {
#if defined(_WIN32)
    uint8_t *base = rex::Runtime::instance() ? rex::Runtime::instance()->virtual_membase() : nullptr;
    if (!base)
    {
      return;
    }
    VirtualAlloc(base + 0x00000000ull, 0x40000000ull, MEM_COMMIT, PAGE_READWRITE);
    VirtualAlloc(base + 0x40000000ull, 0x40000000ull, MEM_COMMIT, PAGE_READWRITE);
    VirtualAlloc(base + 0x80000000ull, 0x20000000ull, MEM_COMMIT, PAGE_READWRITE);
    VirtualAlloc(base + 0xA0000000ull, 0x20000000ull, MEM_COMMIT, PAGE_READWRITE);
    VirtualAlloc(base + 0xC0000000ull, 0x20000000ull, MEM_COMMIT, PAGE_READWRITE);
    VirtualAlloc(base + 0xE0000000ull, 0x1FD00000ull, MEM_COMMIT, PAGE_READWRITE);
#endif
  }

  void OnPreSetup(rex::RuntimeConfig &config) override
  {
    std::string backend = REXCVAR_GET(graphics_backend);
    if (backend != "any" && !config.gpu_plugin.empty())
    {
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin, backend);
      if (!config.graphics)
      {
        REXLOG_WARN("graphics_backend '{}' unavailable, falling back to automatic selection",
                    backend);
      }
    }
  }

  static void UnlockImageMemoryProtection()
  {
#if defined(_WIN32)
    uint8_t *base = rex::Runtime::instance() ? rex::Runtime::instance()->virtual_membase() : nullptr;
    if (!base)
    {
      return;
    }
    // Xbox 360 does not enforce W^X. Walk every committed section in the 4GB guest memory arena
    // and make it PAGE_EXECUTE_READWRITE or PAGE_READWRITE, handling section boundaries properly.
    uint8_t *curr = base;
    uint8_t *end = base + 0x100000000ull;
    while (curr < end)
    {
      MEMORY_BASIC_INFORMATION mbi{};
      if (VirtualQuery(curr, &mbi, sizeof(mbi)))
      {
        if (mbi.State == MEM_COMMIT && mbi.BaseAddress != nullptr &&
            (mbi.Protect == PAGE_READONLY || mbi.Protect == PAGE_NOACCESS))
        {
          DWORD old_protect = 0;
          DWORD new_protect = (mbi.AllocationProtect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))
                                  ? PAGE_EXECUTE_READWRITE
                                  : PAGE_READWRITE;
          if (!VirtualProtect(mbi.BaseAddress, mbi.RegionSize, new_protect, &old_protect))
          {
            VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &old_protect);
          }
        }
        size_t step = mbi.RegionSize ? mbi.RegionSize : 0x1000;
        curr = (uint8_t *)mbi.BaseAddress + step;
      }
      else
      {
        curr += 0x1000;
      }
    }
    // Explicitly guarantee the page containing 0x834E8B10 is writable
    DWORD old_p;
    VirtualProtect(base + (0x834E8B10ull & ~0xFFFFull), 0x20000ull, PAGE_EXECUTE_READWRITE, &old_p);
#endif
  }

  void OnPostLoadXexImage() override
  {
    PrecommitMemoryAliases();
    UnlockImageMemoryProtection();

    game_patches::FpsUnlock();
    game_patches::DisableMsaa();
    game_patches::AnisotropicFiltering16x();
  }

  void OnPostSetup() override
  {
    PrecommitMemoryAliases();
    UnlockImageMemoryProtection();
    utils::LoadSettingsFiles();

    game_patches::FpsUnlock();
    game_patches::DisableMsaa();
    game_patches::AnisotropicFiltering16x();

#ifdef REXGLUE_ENABLE_PERF_COUNTERS
    rex::perf::SetCsvLogPath(rex::cvar::GetFlagByName("perf_log_csv"));
#endif

    if (REXCVAR_GET(dev_debug_runtime))
    {
      debug_tools::PerformMissingFunctionScan();
      debug_tools::PerformStubSweep();
    }
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer *drawer) override
  {
    if (drawer)
    {
      hardware_overlay_ = std::make_unique<rex::ui::HardwareMonitorOverlay>(drawer);
      drawer->AddDialog(hardware_overlay_.get());
    }
  }

  void OnShutdown() override
  {
    if (hardware_overlay_ && imgui_drawer())
    {
      imgui_drawer()->RemoveDialog(hardware_overlay_.get());
      hardware_overlay_.reset();
    }
#ifdef REXGLUE_ENABLE_PERF_COUNTERS
    rex::perf::FlushCsv();
#endif
  }

  void OnConfigurePaths(rex::PathConfig &paths) override
  {
    if (paths.game_data_root.empty())
    {
      paths.game_data_root = utils::RepoRoot() / "assets";
    }

    paths.config_path = utils::SettingsDir() / "hardware.toml";
    utils::LoadSettingsFiles();
  }

private:
  std::unique_ptr<rex::ui::HardwareMonitorOverlay> hardware_overlay_;
};
