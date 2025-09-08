#include "AppState.h"
#include "KeyboardState.h"
#include "MessageSender.h"
#include "KeyEvent.h"

bool AppState::g_sendingKeys = false;
std::chrono::steady_clock::time_point AppState::g_sendingKeysEnabledTime;
bool AppState::g_releasingKeys = false;

bool AppState::IsSendingKeys() {
    if (!g_sendingKeys) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_sendingKeysEnabledTime).count();
    return elapsed >= 500;
}

void AppState::ToggleSendingKeys() {
    if (g_sendingKeys) {
        g_releasingKeys = true;
        auto pressedKeys = KeyboardState::GetAllPressedKeys();
        for (const auto& key : pressedKeys) {
            KeyEvent keyEvent(key.vkCode, false, key.scanCode, key.extended);
            MessageSender::SendKeyEvent(keyEvent);
        }
        KeyboardState::ClearPressedKeys();
        g_releasingKeys = false;
        g_sendingKeys = false;
    } else {
        g_sendingKeys = true;
        g_sendingKeysEnabledTime = std::chrono::steady_clock::now();
    }
}

bool AppState::IsReleasingKeys() {
    return g_releasingKeys;
}
