#pragma once
#include <windows.h>
#include <atomic>

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
    static void RunMessageLoop();
};