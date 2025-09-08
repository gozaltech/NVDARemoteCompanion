#include "KeyboardHook.h"
#include "KeyboardState.h"
#include "AppState.h"
#include "EventChecker.h"
#include "KeyEvent.h"
#include "MessageSender.h"
#include "Debug.h"
#include <thread>
#include <chrono>


HHOOK KeyboardHook::g_keyboardHook = nullptr;

bool KeyboardHook::HandleToggleShortcut(DWORD vkCode) {
    if (KeyboardState::IsToggleShortcut(vkCode)) {
        AppState::ToggleSendingKeys();
        KeyboardState::ResetModifiers();
        return true;
    }
    return false;
}

LRESULT KeyboardHook::ProcessKeyEvent(WPARAM wParam, DWORD vkCode, WORD scanCode, bool isExtended) {
    if (EventChecker::IsKeyDownEvent(wParam)) {
        KeyboardState::UpdateModifierState(vkCode, true);
        
        if (HandleToggleShortcut(vkCode)) {
            return 1;
        }
        
        
        if (AppState::IsSendingKeys() || AppState::IsReleasingKeys()) {
            if (AppState::IsSendingKeys()) {
                KeyboardState::TrackKeyPress(vkCode, scanCode, isExtended);
                KeyEvent keyEvent(static_cast<uint32_t>(vkCode), true, static_cast<uint16_t>(scanCode), isExtended);
                MessageSender::SendKeyEvent(keyEvent);
            }
            return 1;
        }
        return 0;
    }
    
    if (EventChecker::IsKeyUpEvent(wParam)) {
        KeyboardState::UpdateModifierState(vkCode, false);
        
        if (AppState::IsSendingKeys() || AppState::IsReleasingKeys()) {
            if (AppState::IsSendingKeys()) {
                KeyboardState::TrackKeyRelease(vkCode);
                KeyEvent keyEvent(static_cast<uint32_t>(vkCode), false, static_cast<uint16_t>(scanCode), isExtended);
                MessageSender::SendKeyEvent(keyEvent);
            }
            return 1;
        }
        return 0;
    }
    
    return 0;
}

LRESULT CALLBACK KeyboardHook::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) {
        return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
    }
    
    KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    DWORD vkCode = pKeyboard->vkCode;
    WORD scanCode = static_cast<WORD>(pKeyboard->scanCode);
    bool isExtended = (pKeyboard->flags & LLKHF_EXTENDED) != 0;
    
    LRESULT result = ProcessKeyEvent(wParam, vkCode, scanCode, isExtended);
    return result ? result : CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

bool KeyboardHook::Install() {
    g_keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        GetModuleHandle(nullptr),
        0
    );
    return g_keyboardHook != nullptr;
}

void KeyboardHook::Uninstall() {
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
}

void KeyboardHook::RunMessageLoop() {
    MSG msg;
    DEBUG_INFO("HOOK", "Starting message loop");
    
    while (!g_shutdown) {
        BOOL hasMessage = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        
        if (hasMessage) {
            if (msg.message == WM_QUIT) {
                DEBUG_INFO("HOOK", "Received WM_QUIT message");
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            Sleep(1);
        }
    }
    DEBUG_INFO("HOOK", "Message loop terminated due to shutdown flag");
}