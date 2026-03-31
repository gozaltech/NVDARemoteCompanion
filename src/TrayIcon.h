#pragma once
#ifdef _WIN32

#include <windows.h>
#include <shellapi.h>
#include <atomic>
#include <string>
#include <vector>
#include <functional>
#include <memory>

extern std::atomic<bool> g_shutdown;

struct HMenuDeleter {
    using pointer = HMENU;
    void operator()(HMENU h) const noexcept { if (h) DestroyMenu(h); }
};
using UniqueMenu = std::unique_ptr<HMENU, HMenuDeleter>;

struct TrayProfile {
    std::string name;
    bool connected;
};

class TrayIcon {
public:
    static bool Create();
    static void Destroy();
    static void SetTooltip(const std::string& text);
    static void RunMessageLoop();

    static void SetProfileProvider(std::function<std::vector<TrayProfile>()> provider);
    static void SetProfileToggleCallback(std::function<void(int index)> callback);
    static void SetReconnectAllCallback(std::function<void()> callback);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void RebuildMenu();

    static HWND s_hwnd;
    static NOTIFYICONDATAA s_nid;
    static UniqueMenu s_menu;

    static std::function<std::vector<TrayProfile>()> s_profileProvider;
    static std::function<void(int)>                  s_profileToggleCallback;
    static std::function<void()>                     s_reconnectAllCallback;

    static constexpr UINT WM_TRAYICON           = WM_USER + 100;
    static constexpr UINT ID_TRAY_EXIT          = 1001;
    static constexpr UINT ID_TRAY_REINSTALL_HOOK = 1002;
    static constexpr UINT ID_TRAY_RECONNECT_ALL = 1003;
    static constexpr UINT ID_TRAY_PROFILE_BASE  = 2000;
};

#endif
