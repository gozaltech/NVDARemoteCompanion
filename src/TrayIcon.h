#pragma once
#ifdef _WIN32

#include <windows.h>
#include <shellapi.h>
#include <atomic>
#include <string>

extern std::atomic<bool> g_shutdown;

class TrayIcon {
public:
    static bool Create();
    static void Destroy();
    static void SetTooltip(const std::string& text);
    static void RunMessageLoop();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static HWND s_hwnd;
    static NOTIFYICONDATAA s_nid;
    static HMENU s_menu;

    static constexpr UINT WM_TRAYICON = WM_USER + 100;
    static constexpr UINT ID_TRAY_EXIT = 1001;
    static constexpr UINT ID_TRAY_REINSTALL_HOOK = 1002;
};

#endif
