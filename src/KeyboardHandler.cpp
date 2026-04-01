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
    if (KeyboardState::CheckLocalShortcut(vkCode)) {
        KeyboardState::ResetModifiers();
        AppState::GoLocal();
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
        AppState::ToggleSendingKeys(profileIndex);
        return true;
    }
    return false;
}
