#ifdef _WIN32

#include "TrayIcon.h"
#include "Debug.h"
#include "Config.h"
#include "KeyboardHook.h"

extern DWORD g_mainThreadId;

HWND TrayIcon::s_hwnd = nullptr;
NOTIFYICONDATAA TrayIcon::s_nid = {};
UniqueMenu TrayIcon::s_menu;
std::function<std::vector<TrayProfile>()> TrayIcon::s_profileProvider;
std::function<void(int)>                  TrayIcon::s_profileToggleCallback;
std::function<void()>                     TrayIcon::s_reconnectAllCallback;

void TrayIcon::SetProfileProvider(std::function<std::vector<TrayProfile>()> provider) {
    s_profileProvider = std::move(provider);
}

void TrayIcon::SetProfileToggleCallback(std::function<void(int index)> callback) {
    s_profileToggleCallback = std::move(callback);
}

void TrayIcon::SetReconnectAllCallback(std::function<void()> callback) {
    s_reconnectAllCallback = std::move(callback);
}

void TrayIcon::RebuildMenu() {
    s_menu.reset(CreatePopupMenu());

    if (s_profileProvider) {
        auto profiles = s_profileProvider();

        AppendMenuA(s_menu.get(), MF_STRING, ID_TRAY_RECONNECT_ALL, "Reconnect all");

        HMENU sub = CreatePopupMenu();
        for (int i = 0; i < Config::isize(profiles); i++) {
            std::string label = (profiles[i].connected ? "Disconnect: " : "Connect: ") + profiles[i].name;
            UINT flags = MF_STRING | (profiles[i].connected ? MF_CHECKED : MF_UNCHECKED);
            AppendMenuA(sub, flags, ID_TRAY_PROFILE_BASE + i, label.c_str());
        }
        AppendMenuA(s_menu.get(), MF_POPUP, reinterpret_cast<UINT_PTR>(sub), "Profiles");

        AppendMenuA(s_menu.get(), MF_SEPARATOR, 0, nullptr);
    }

    AppendMenuA(s_menu.get(), MF_STRING, ID_TRAY_REINSTALL_HOOK, "Reinstall keyboard hook");
    AppendMenuA(s_menu.get(), MF_SEPARATOR, 0, nullptr);
    AppendMenuA(s_menu.get(), MF_STRING, ID_TRAY_EXIT, "Exit");
}

LRESULT CALLBACK TrayIcon::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            RebuildMenu();
            TrackPopupMenu(s_menu.get(), TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            PostMessage(hwnd, WM_NULL, 0, 0);
        }
        return 0;

    case WM_COMMAND: {
        UINT id = LOWORD(wParam);
        if (id == ID_TRAY_EXIT) {
            DEBUG_INFO("TRAY", "Exit requested from tray menu");
            g_shutdown = true;
            PostQuitMessage(0);
        } else if (id == ID_TRAY_REINSTALL_HOOK) {
            DEBUG_INFO("TRAY", "Reinstall keyboard hook requested from tray menu");
            if (g_mainThreadId != 0)
                PostThreadMessage(g_mainThreadId, WM_REINSTALL_HOOK, 0, 0);
        } else if (id == ID_TRAY_RECONNECT_ALL) {
            DEBUG_INFO("TRAY", "Reconnect all requested from tray menu");
            if (s_reconnectAllCallback) s_reconnectAllCallback();
        } else if (id >= ID_TRAY_PROFILE_BASE) {
            int profileIdx = static_cast<int>(id - ID_TRAY_PROFILE_BASE);
            DEBUG_INFO_F("TRAY", "Profile toggle requested for index {}", profileIdx);
            if (s_profileToggleCallback) s_profileToggleCallback(profileIdx);
        }
        return 0;
    }

    case WM_CONNECTION_LOST:
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool TrayIcon::Create() {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "NVDARemoteCompanionTray";

    if (!RegisterClassExA(&wc)) {
        DEBUG_ERROR("TRAY", "Failed to register window class");
        return false;
    }

    s_hwnd = CreateWindowExA(0, wc.lpszClassName, Config::APP_NAME,
                             0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, wc.hInstance, nullptr);
    if (!s_hwnd) {
        DEBUG_ERROR("TRAY", "Failed to create message window");
        return false;
    }

    s_menu.reset(CreatePopupMenu());

    s_nid.cbSize = sizeof(s_nid);
    s_nid.hWnd = s_hwnd;
    s_nid.uID = 1;
    s_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    s_nid.uCallbackMessage = WM_TRAYICON;
    s_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    strncpy_s(s_nid.szTip, Config::APP_NAME, _TRUNCATE);

    if (!Shell_NotifyIconA(NIM_ADD, &s_nid)) {
        DEBUG_ERROR("TRAY", "Failed to add tray icon");
        DestroyWindow(s_hwnd);
        s_hwnd = nullptr;
        return false;
    }

    DEBUG_INFO("TRAY", "System tray icon created");
    return true;
}

void TrayIcon::Destroy() {
    if (s_nid.hWnd) {
        Shell_NotifyIconA(NIM_DELETE, &s_nid);
        s_nid.hWnd = nullptr;
    }
    s_menu.reset();
    if (s_hwnd) {
        DestroyWindow(s_hwnd);
        s_hwnd = nullptr;
    }
    DEBUG_INFO("TRAY", "System tray icon destroyed");
}

void TrayIcon::SetTooltip(const std::string& text) {
    if (!s_nid.hWnd) return;
    strncpy_s(s_nid.szTip, text.c_str(), _TRUNCATE);
    Shell_NotifyIconA(NIM_MODIFY, &s_nid);
}

void TrayIcon::RunMessageLoop() {
    MSG msg;
    while (!g_shutdown && GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_QUIT) {
            g_shutdown = true;
            break;
        }
        if (msg.message == WM_CONNECTION_LOST) {
            DEBUG_INFO("TRAY", "Connection lost message received");
            break;
        }
        if (msg.message == WM_REINSTALL_HOOK) {
            DEBUG_INFO("TRAY", "Reinstall keyboard hook message received");
            if (g_mainThreadId != 0)
                PostThreadMessage(g_mainThreadId, WM_REINSTALL_HOOK, 0, 0);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

#endif
