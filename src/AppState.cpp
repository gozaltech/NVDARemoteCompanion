#include "AppState.h"
#include "KeyboardState.h"
#include "MessageSender.h"
#include "KeyEvent.h"
#include "Audio.h"

int AppState::g_activeProfile = -1;
bool AppState::g_releasingKeys = false;

bool AppState::IsSendingKeys() {
    return g_activeProfile >= 0;
}

void AppState::ToggleSendingKeys(int profileIndex) {
    if (g_activeProfile == profileIndex) {
        g_releasingKeys = true;
        auto pressedKeys = KeyboardState::GetAllPressedKeys();
        for (const auto& key : pressedKeys) {
            KeyEvent keyEvent(key.vkCode, false, key.scanCode, key.extended);
            MessageSender::SendKeyEvent(keyEvent);
        }
        KeyboardState::ClearPressedKeys();
        g_releasingKeys = false;
        g_activeProfile = -1;
        Audio::PlayTone(440, 100);
    } else {
        if (g_activeProfile >= 0) {
            g_releasingKeys = true;
            auto pressedKeys = KeyboardState::GetAllPressedKeys();
            for (const auto& key : pressedKeys) {
                KeyEvent keyEvent(key.vkCode, false, key.scanCode, key.extended);
                MessageSender::SendKeyEvent(keyEvent);
            }
            KeyboardState::ClearPressedKeys();
            g_releasingKeys = false;
        }
        g_activeProfile = profileIndex;
        MessageSender::SetActiveProfile(profileIndex);
        Audio::PlayTone(880, 100);
    }
}

bool AppState::IsReleasingKeys() {
    return g_releasingKeys;
}

int AppState::GetActiveProfile() {
    return g_activeProfile;
}
