#include "KeyboardHook.h"
#include "KeyboardState.h"
#include "AppState.h"
#include "EventChecker.h"
#include "KeyEvent.h"
#include "MessageSender.h"
#include "Clipboard.h"
#include "Speech.h"
#include "Debug.h"

extern DWORD g_mainThreadId;

HHOOK KeyboardHook::s_hook = nullptr;
KeyboardHook* KeyboardHook::s_instance = nullptr;

void KeyboardHook::OnExit() {
    g_shutdown = true;
    PostQuitMessage(0);
}

void KeyboardHook::OnReinstallHook() {
    if (g_mainThreadId != 0)
        PostThreadMessage(g_mainThreadId, WM_REINSTALL_HOOK, 0, 0);
}

void KeyboardHook::OnClipboardShortcut() {
    std::string text = Clipboard::GetText();
    if (text.empty()) {
        Speech::Speak("Clipboard is empty", false);
        return;
    }
    MessageSender::SendClipboardText(text);
    Speech::Speak("Clipboard sent", false);
}

LRESULT KeyboardHook::ProcessKeyEvent(WPARAM wParam, DWORD vkCode, WORD scanCode, bool isExtended) {
    if (EventChecker::IsKeyDownEvent(wParam)) {
        KeyboardState::UpdateModifierState(vkCode, true);

        if (s_instance && s_instance->HandleShortcut(vkCode))
            return 1;

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
            if (KeyboardState::TrackKeyRelease(vkCode)) {
                KeyEvent keyEvent(static_cast<uint32_t>(vkCode), false, static_cast<uint16_t>(scanCode), isExtended);
                MessageSender::SendKeyEvent(keyEvent);
                return 1;
            }
        }
        return 0;
    }

    return 0;
}

LRESULT CALLBACK KeyboardHook::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0)
        return CallNextHookEx(s_hook, nCode, wParam, lParam);

    KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    LRESULT result = ProcessKeyEvent(wParam, kb->vkCode,
                                     static_cast<WORD>(kb->scanCode),
                                     (kb->flags & LLKHF_EXTENDED) != 0);
    return result ? result : CallNextHookEx(s_hook, nCode, wParam, lParam);
}

bool KeyboardHook::Install() {
    s_instance = this;
    s_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc,
                               GetModuleHandle(nullptr), 0);
    return s_hook != nullptr;
}

void KeyboardHook::Uninstall() {
    if (s_hook) {
        UnhookWindowsHookEx(s_hook);
        s_hook = nullptr;
    }
    s_instance = nullptr;
}

void KeyboardHook::Reinstall() {
    DEBUG_INFO("HOOK", "Reinstalling keyboard hook");
    Uninstall();
    if (Install()) {
        DEBUG_INFO("HOOK", "Keyboard hook reinstalled successfully");
        Speech::Speak("Hook reinstalled", false);
    } else {
        DEBUG_ERROR("HOOK", "Failed to reinstall keyboard hook");
        Speech::Speak("Hook reinstall failed", false);
    }
}

void KeyboardHook::NotifyConnectionLost() {
    if (g_mainThreadId != 0)
        PostThreadMessage(g_mainThreadId, WM_CONNECTION_LOST, 0, 0);
}

void KeyboardHook::RunMessageLoop() {
    MSG msg;
    DEBUG_INFO("HOOK", "Starting message loop");

    while (!g_shutdown) {
        BOOL hasMessage = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        if (hasMessage) {
            if (msg.message == WM_QUIT) {
                DEBUG_INFO("HOOK", "Received WM_QUIT");
                break;
            }
            if (msg.message == WM_CONNECTION_LOST) {
                DEBUG_INFO("HOOK", "Received WM_CONNECTION_LOST, breaking loop");
                break;
            }
            if (msg.message == WM_REINSTALL_HOOK) {
                DEBUG_INFO("HOOK", "Received WM_REINSTALL_HOOK");
                Reinstall();
                continue;
            }
            if (msg.message == WM_RECONNECT_ALL) {
                DEBUG_INFO("HOOK", "Received WM_RECONNECT_ALL");
                if (m_reconnectCallback) m_reconnectCallback();
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            Sleep(1);
        }
    }
    DEBUG_INFO("HOOK", "Message loop ended");
}
