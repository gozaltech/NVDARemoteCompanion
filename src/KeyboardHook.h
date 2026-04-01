#pragma once
#include <windows.h>
#include <atomic>
#include <functional>

constexpr UINT WM_CONNECTION_LOST = WM_USER + 1;
constexpr UINT WM_REINSTALL_HOOK  = WM_USER + 2;
constexpr UINT WM_RECONNECT_ALL   = WM_USER + 3;

extern std::atomic<bool> g_shutdown;

class KeyboardHook {
private:
    static HHOOK g_keyboardHook;
    static bool HandleToggleShortcut(DWORD vkCode);
    static LRESULT ProcessKeyEvent(WPARAM wParam, DWORD vkCode, WORD scanCode, bool isExtended);
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

public:
    static bool Install();
    static void Uninstall();
    static void Reinstall();
    static void RunMessageLoop();
    static void SetReconnectAllCallback(std::function<void()> callback);
};