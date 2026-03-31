#include "AppState.h"
#include "KeyboardState.h"
#include "MessageSender.h"
#include "KeyEvent.h"
#include "Audio.h"
#include "Speech.h"

int AppState::g_activeProfile = -1;
bool AppState::g_releasingKeys = false;
std::vector<int> AppState::g_connectedProfiles;
std::vector<std::string> AppState::g_profileNames;

bool AppState::IsSendingKeys() {
    return g_activeProfile >= 0;
}

void AppState::ReleaseAllKeys() {
    g_releasingKeys = true;
    auto pressedKeys = KeyboardState::GetAllPressedKeys();
    for (const auto& key : pressedKeys) {
        KeyEvent keyEvent(key.vkCode, false, key.scanCode, key.extended);
        MessageSender::SendKeyEvent(keyEvent);
    }
    KeyboardState::ClearPressedKeys();
    g_releasingKeys = false;
}

void AppState::ToggleSendingKeys(int profileIndex) {
    if (g_activeProfile == profileIndex) {
        ReleaseAllKeys();
        g_activeProfile = -1;
        Audio::PlayTone(440, 100);
        Speech::Speak("Local", true);
    } else {
        if (g_activeProfile >= 0) {
            ReleaseAllKeys();
        }
        g_activeProfile = profileIndex;
        MessageSender::SetActiveProfile(profileIndex);
        Audio::PlayTone(880, 100);
        for (size_t i = 0; i < g_connectedProfiles.size(); i++) {
            if (g_connectedProfiles[i] == profileIndex && i < g_profileNames.size()) {
                Speech::Speak(g_profileNames[i], true);
                break;
            }
        }
    }
}

void AppState::CycleProfile() {
    if (g_connectedProfiles.empty()) {
        return;
    }

    int currentIdx = -1;
    if (g_activeProfile >= 0) {
        for (int i = 0; i < static_cast<int>(g_connectedProfiles.size()); i++) {
            if (g_connectedProfiles[i] == g_activeProfile) {
                currentIdx = i;
                break;
            }
        }
    }

    if (g_activeProfile >= 0) {
        ReleaseAllKeys();
    }

    int nextIdx = currentIdx + 1;
    if (nextIdx >= static_cast<int>(g_connectedProfiles.size())) {
        g_activeProfile = -1;
        Audio::PlayTone(440, 100);
        Speech::Speak("Local", true);
    } else {
        g_activeProfile = g_connectedProfiles[nextIdx];
        MessageSender::SetActiveProfile(g_activeProfile);
        Audio::PlayTone(880, 100);
        if (nextIdx < static_cast<int>(g_profileNames.size())) {
            Speech::Speak(g_profileNames[nextIdx], true);
        }
    }
}

void AppState::SetConnectedProfiles(const std::vector<int>& indices, const std::vector<std::string>& names) {
    g_connectedProfiles = indices;
    g_profileNames = names;
}

bool AppState::IsReleasingKeys() {
    return g_releasingKeys;
}

int AppState::GetActiveProfile() {
    return g_activeProfile;
}

void AppState::GoLocal() {
    if (g_activeProfile >= 0) {
        ReleaseAllKeys();
        g_activeProfile = -1;
        Audio::PlayTone(440, 100);
        Speech::Speak("Local", true);
    }
}
