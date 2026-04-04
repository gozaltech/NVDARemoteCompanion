#include "KeyboardHandler.h"
#include "KeyboardState.h"
#include "AppState.h"

void KeyboardHandler::Reinstall() {
    Uninstall();
    Install();
}

void KeyboardHandler::SetReconnectCallback(std::function<void()> callback) {
    m_reconnectCallback = std::move(callback);
}

bool KeyboardHandler::HandleShortcut(uint32_t vkCode) {
    if (KeyboardState::CheckForwardKeysShortcut(vkCode)) {
        KeyboardState::ResetModifiers();
        AppState::ToggleForwarding();
        return true;
    }

    if (AppState::IsSendingKeys()) return false;

    if (KeyboardState::CheckExitShortcut(vkCode)) {
        KeyboardState::ResetModifiers();
        OnExit();
        return true;
    }
    if (KeyboardState::CheckReinstallHookShortcut(vkCode)) {
        KeyboardState::ResetModifiers();
        OnReinstallHook();
        return true;
    }
    if (KeyboardState::CheckClipboardShortcut(vkCode)) {
        KeyboardState::ResetModifiers();
        OnClipboardShortcut();
        return true;
    }
    if (KeyboardState::CheckReconnectShortcut(vkCode)) {
        KeyboardState::ResetModifiers();
        if (m_reconnectCallback) m_reconnectCallback();
        return true;
    }
    if (KeyboardState::CheckCycleShortcut(vkCode)) {
        KeyboardState::ResetModifiers();
        AppState::CycleProfile();
        return true;
    }
    int profileIndex = KeyboardState::CheckToggleShortcut(vkCode);
    if (profileIndex >= 0) {
        KeyboardState::ResetModifiers();
        AppState::SetActiveProfile(profileIndex);
        return true;
    }
    return false;
}
