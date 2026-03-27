#ifdef _WIN32

#include "TrayIcon.h"
#include "Debug.h"
#include "Config.h"
#include "KeyboardHook.h"

HWND TrayIcon::s_hwnd = nullptr;
NOTIFYICONDATAA TrayIcon::s_nid = {};
HMENU TrayIcon::s_menu = nullptr;

LRESULT CALLBACK TrayIcon::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(s_menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
            PostMessage(hwnd, WM_NULL, 0, 0);
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_EXIT) {
            DEBUG_INFO("TRAY", "Exit requested from tray menu");
            g_shutdown = true;
            PostQuitMessage(0);
        }
        return 0;

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

    s_menu = CreatePopupMenu();
    AppendMenuA(s_menu, MF_STRING, ID_TRAY_EXIT, "Exit");

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
    if (s_menu) {
        DestroyMenu(s_menu);
        s_menu = nullptr;
    }
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
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

#endif
