/**
 * @file        hardware_overlay.h
 * @brief       Hardware telemetry HUD overlay for Army of Two: The 40th Day (FPS, Frame Time, CPU %, GPU/VRAM).
 */

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dxgi1_4.h>

#include <imgui.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

#include "game_cvars.h"

namespace rex::ui {

class HardwareMonitorOverlay : public ImGuiDialog {
public:
  explicit HardwareMonitorOverlay(ImGuiDrawer* drawer)
      : ImGuiDialog(drawer), enabled_(true) {}

  void SetEnabled(bool enabled) { enabled_ = enabled; }
  bool IsEnabled() const { return enabled_; }

protected:
  void OnDraw(ImGuiIO& io) override {
    // High-precision authentic 30 FPS console frame limiter
    int32_t target_fps = REXCVAR_GET(ao2_framerate_limit);
    if (target_fps > 0) {
      static auto last_present_time = std::chrono::steady_clock::now();
      const double target_duration_us = 1000000.0 / static_cast<double>(target_fps);
      auto now = std::chrono::steady_clock::now();
      auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(now - last_present_time).count();
      if (elapsed_us < target_duration_us) {
        auto sleep_us = target_duration_us - elapsed_us;
        if (sleep_us > 2000) {
          std::this_thread::sleep_for(std::chrono::microseconds(sleep_us - 1500));
        }
        while (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - last_present_time).count() < target_duration_us) {
          YieldProcessor();
        }
      }
      last_present_time = std::chrono::steady_clock::now();
    }

    // Check toggle hotkey (F2)
    if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
      enabled_ = !enabled_;
    }

    if (!enabled_) {
      return;
    }

    // Fixed overlay in top-left corner
    ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.70f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("HardwareMonitor##HUD", nullptr, flags)) {
      // Header
      ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "[ Army of Two: 40th Day - D3D12 ROV ]");
      ImGui::Separator();

      // FPS & Frame Time
      float fps = io.Framerate;
      float frame_time = io.DeltaTime * 1000.0f;
      ImVec4 fps_color = (fps >= 55.0f) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f)
                       : (fps >= 28.0f) ? ImVec4(1.0f, 0.9f, 0.2f, 1.0f)
                                        : ImVec4(1.0f, 0.2f, 0.2f, 1.0f);

      ImGui::TextColored(fps_color, "FPS: %.1f", fps);
      ImGui::SameLine();
      ImGui::TextDisabled("(%.1f ms)", frame_time);

      // CPU Process Usage
      double cpu_percent = GetProcessCpuUsage();
      ImGui::Text("CPU: %.1f%%", cpu_percent);
      ImGui::SameLine();
      ImGui::ProgressBar(static_cast<float>(cpu_percent / 100.0), ImVec2(70.0f, 0.0f), "");

      // GPU & VRAM Usage
      std::string gpu_name;
      uint64_t vram_used_mb = 0;
      uint64_t vram_budget_mb = 0;
      bool has_vram = GetGpuMemoryInfo(gpu_name, vram_used_mb, vram_budget_mb);

      ImGui::Text("GPU: %s", gpu_name.c_str());
      if (has_vram && vram_budget_mb > 0) {
        float vram_fraction = static_cast<float>(vram_used_mb) / static_cast<float>(vram_budget_mb);
        ImGui::Text("VRAM: %llu / %llu MB", vram_used_mb, vram_budget_mb);
        ImGui::SameLine();
        ImGui::ProgressBar(vram_fraction, ImVec2(70.0f, 0.0f), "");
      } else {
        ImGui::TextDisabled("VRAM: Direct3D 12 Native (AMD ROV)");
      }

      ImGui::Separator();
      ImGui::TextDisabled("[F2] Toggle HUD  |  [F3] Debug");
    }
    ImGui::End();
  }

private:
  bool enabled_ = true;

  static double GetProcessCpuUsage() {
    static ULONGLONG last_system_time = 0;
    static ULONGLONG last_process_time = 0;
    static double last_cpu_percent = 0.0;
    static DWORD last_tick = 0;

    DWORD current_tick = GetTickCount();
    if (current_tick - last_tick < 500 && last_system_time != 0) {
      return last_cpu_percent;
    }

    FILETIME idle_time, kernel_time, user_time;
    if (!GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
      return last_cpu_percent;
    }

    FILETIME creation_time, exit_time, proc_kernel_time, proc_user_time;
    if (!GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &proc_kernel_time, &proc_user_time)) {
      return last_cpu_percent;
    }

    auto to_ull = [](const FILETIME& ft) -> ULONGLONG {
      return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };

    ULONGLONG system_time = to_ull(kernel_time) + to_ull(user_time);
    ULONGLONG process_time = to_ull(proc_kernel_time) + to_ull(proc_user_time);

    if (last_system_time != 0 && system_time > last_system_time) {
      ULONGLONG system_delta = system_time - last_system_time;
      ULONGLONG process_delta = process_time - last_process_time;
      last_cpu_percent = (100.0 * process_delta) / system_delta;
      if (last_cpu_percent > 100.0) last_cpu_percent = 100.0;
      last_tick = current_tick;
    }

    last_system_time = system_time;
    last_process_time = process_time;
    return last_cpu_percent;
  }

  static bool GetGpuMemoryInfo(std::string& out_gpu_name, uint64_t& out_used_mb, uint64_t& out_budget_mb) {
    static IDXGIFactory4* factory = nullptr;
    static IDXGIAdapter3* adapter = nullptr;
    static std::string cached_gpu_name = "AMD Radeon RX 6600";
    static bool initialized = false;

    if (!initialized) {
      initialized = true;
      HMODULE dxgi_mod = LoadLibraryA("dxgi.dll");
      if (dxgi_mod) {
        typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY2)(UINT Flags, REFIID riid, void** ppFactory);
        auto pCreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgi_mod, "CreateDXGIFactory2");
        if (pCreateDXGIFactory2) {
          pCreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
          if (factory) {
            IDXGIAdapter1* adapter1 = nullptr;
            if (SUCCEEDED(factory->EnumAdapters1(0, &adapter1)) && adapter1) {
              DXGI_ADAPTER_DESC1 desc;
              if (SUCCEEDED(adapter1->GetDesc1(&desc))) {
                char name[128];
                WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name, sizeof(name), NULL, NULL);
                cached_gpu_name = name;
              }
              adapter1->QueryInterface(IID_PPV_ARGS(&adapter));
              adapter1->Release();
            }
          }
        }
      }
    }

    out_gpu_name = cached_gpu_name;
    if (adapter) {
      DXGI_QUERY_VIDEO_MEMORY_INFO mem_info{};
      if (SUCCEEDED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &mem_info))) {
        out_used_mb = mem_info.CurrentUsage / (1024 * 1024);
        out_budget_mb = mem_info.Budget / (1024 * 1024);
        return true;
      }
    }
    return false;
  }
};

}  // namespace rex::ui
