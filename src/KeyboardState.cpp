#include "KeyboardState.h"
#include <algorithm>

#ifdef _WIN32
    #define CTRL_KEY_1 VK_CONTROL
    #define CTRL_KEY_2 VK_LCONTROL  
    #define CTRL_KEY_3 VK_RCONTROL
    #define WIN_KEY_1  VK_LWIN
    #define WIN_KEY_2  VK_RWIN
#else
    #define CTRL_KEY_1 0x25
    #define CTRL_KEY_2 0x69
    #define WIN_KEY_1  0x85
    #define WIN_KEY_2  0x86
#endif

bool KeyboardState::g_ctrlPressed = false;
bool KeyboardState::g_winPressed = false;
std::set<NativeKeyType> KeyboardState::g_pressedKeys;
std::vector<PressedKey> KeyboardState::g_pressedKeyDetails;

bool KeyboardState::IsControlKey(NativeKeyType vkCode) {
#ifdef _WIN32
    return vkCode == CTRL_KEY_1 || vkCode == CTRL_KEY_2 || vkCode == CTRL_KEY_3;
#else
    return vkCode == CTRL_KEY_1 || vkCode == CTRL_KEY_2;
#endif
}

bool KeyboardState::IsWinKey(NativeKeyType vkCode) {
    return vkCode == WIN_KEY_1 || vkCode == WIN_KEY_2;
}

void KeyboardState::UpdateModifierState(NativeKeyType vkCode, bool isPressed) {
    if (IsControlKey(vkCode)) {
        g_ctrlPressed = isPressed;
    }
    if (IsWinKey(vkCode)) {
        g_winPressed = isPressed;
    }
}

bool KeyboardState::IsToggleShortcut(NativeKeyType vkCode) {
#ifdef _WIN32
    return g_ctrlPressed && g_winPressed && vkCode == VK_F11;
#else
    return g_ctrlPressed && g_winPressed && vkCode == 0x4f;
#endif
}

void KeyboardState::ResetModifiers() {
    g_ctrlPressed = false;
    g_winPressed = false;
}

void KeyboardState::TrackKeyPress(NativeKeyType vkCode, NativeScanType scanCode, bool extended) {
    if (g_pressedKeys.find(vkCode) == g_pressedKeys.end()) {
        g_pressedKeys.insert(vkCode);
        g_pressedKeyDetails.push_back({vkCode, scanCode, extended});
    }
}

void KeyboardState::TrackKeyRelease(NativeKeyType vkCode) {
    g_pressedKeys.erase(vkCode);
    g_pressedKeyDetails.erase(
        std::remove_if(g_pressedKeyDetails.begin(), g_pressedKeyDetails.end(),
                      [vkCode](const PressedKey& key) { return key.vkCode == vkCode; }),
        g_pressedKeyDetails.end());
}

std::vector<PressedKey> KeyboardState::GetAllPressedKeys() {
    return g_pressedKeyDetails;
}

void KeyboardState::ClearPressedKeys() {
    g_pressedKeys.clear();
    g_pressedKeyDetails.clear();
}