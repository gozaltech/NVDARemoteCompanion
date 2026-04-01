#pragma once
#include "KeyboardHandler.h"
#include <windows.h>

constexpr UINT WM_CONNECTION_LOST = WM_USER + 1;
constexpr UINT WM_REINSTALL_HOOK  = WM_USER + 2;
constexpr UINT WM_RECONNECT_ALL   = WM_USER + 3;

class KeyboardHook : public KeyboardHandler {
public:
    bool Install() override;
    void Uninstall() override;
    void Reinstall() override;
    void RunMessageLoop() override;
    void NotifyConnectionLost() override;

protected:
    void OnExit() override;
    void OnReinstallHook() override;

private:
    static HHOOK s_hook;
    static KeyboardHook* s_instance;

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT ProcessKeyEvent(WPARAM wParam, DWORD vkCode, WORD scanCode, bool isExtended);
};
