#pragma once
#ifdef _WIN32

#include <windows.h>
#include <shellapi.h>
#include <atomic>
#include <string>
#include <memory>

extern std::atomic<bool> g_shutdown;

struct HMenuDeleter {
    using pointer = HMENU;
    void operator()(HMENU h) const noexcept { if (h) DestroyMenu(h); }
};
using UniqueMenu = std::unique_ptr<HMENU, HMenuDeleter>;

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
    static UniqueMenu s_menu;

    static constexpr UINT WM_TRAYICON = WM_USER + 100;
    static constexpr UINT ID_TRAY_EXIT = 1001;
    static constexpr UINT ID_TRAY_REINSTALL_HOOK = 1002;
};

#endif
