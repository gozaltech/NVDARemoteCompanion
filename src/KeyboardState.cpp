#include "KeyboardState.h"
#include "Debug.h"
#include <algorithm>
#include <sstream>
#include <map>

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
bool KeyboardState::g_altPressed = false;
bool KeyboardState::g_shiftPressed = false;

// Default: Ctrl + Win + F11
bool KeyboardState::g_targetCtrl = true;
bool KeyboardState::g_targetWin = true;
bool KeyboardState::g_targetAlt = false;
bool KeyboardState::g_targetShift = false;
NativeKeyType KeyboardState::g_targetKey = VK_F11;

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

bool KeyboardState::IsAltKey(NativeKeyType vkCode) {
    return vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU;
}

bool KeyboardState::IsShiftKey(NativeKeyType vkCode) {
    return vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT;
}

void KeyboardState::UpdateModifierState(NativeKeyType vkCode, bool isPressed) {
    if (IsControlKey(vkCode)) g_ctrlPressed = isPressed;
    if (IsWinKey(vkCode)) g_winPressed = isPressed;
    if (IsAltKey(vkCode)) g_altPressed = isPressed;
    if (IsShiftKey(vkCode)) g_shiftPressed = isPressed;
}

bool KeyboardState::IsToggleShortcut(NativeKeyType vkCode) {
#ifdef _WIN32
    // Only trigger if the non-modifier key matches target
    if (vkCode != g_targetKey) return false;
    
    return (g_ctrlPressed == g_targetCtrl) &&
           (g_winPressed == g_targetWin) &&
           (g_altPressed == g_targetAlt) &&
           (g_shiftPressed == g_targetShift);
#else
    return g_ctrlPressed && g_winPressed && vkCode == 0x4f;
#endif
}

void KeyboardState::ResetModifiers() {
    g_ctrlPressed = false;
    g_winPressed = false;
    g_altPressed = false;
    g_shiftPressed = false;
}

NativeKeyType ParseKey(const std::string& keyName) {
    std::string k = keyName;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    
    if (k.rfind("f", 0) == 0 && k.length() > 1) { // Starts with f
         try {
             int num = std::stoi(k.substr(1));
             if (num >= 1 && num <= 24) return VK_F1 + (num - 1);
         } catch(...) {}
    }
    
    if (k.length() == 1) {
        char c = k[0];
        if (c >= 'a' && c <= 'z') return toupper(c);
        if (c >= '0' && c <= '9') return c;
    }
    
    static std::map<std::string, NativeKeyType> keyMap = {
        {"space", VK_SPACE}, {"enter", VK_RETURN}, {"return", VK_RETURN},
        {"escape", VK_ESCAPE}, {"esc", VK_ESCAPE}, {"tab", VK_TAB},
        {"up", VK_UP}, {"down", VK_DOWN}, {"left", VK_LEFT}, {"right", VK_RIGHT},
        {"home", VK_HOME}, {"end", VK_END},
        {"pageup", VK_PRIOR}, {"pgup", VK_PRIOR},
        {"pagedown", VK_NEXT}, {"pgdn", VK_NEXT},
        {"insert", VK_INSERT}, {"ins", VK_INSERT},
        {"delete", VK_DELETE}, {"del", VK_DELETE},
        {"backspace", VK_BACK}, {"bs", VK_BACK},
        {"pause", VK_PAUSE}, {"printscreen", VK_SNAPSHOT},
        {"capslock", VK_CAPITAL}, {"numlock", VK_NUMLOCK}
    };
    
    if (keyMap.count(k)) return keyMap[k];
    
    return 0;
}

void KeyboardState::SetToggleShortcut(const std::string& shortcut) {
    if (shortcut.empty()) return;
    
    g_targetCtrl = false;
    g_targetWin = false;
    g_targetAlt = false;
    g_targetShift = false;
    
    std::stringstream ss(shortcut);
    std::string segment;
    
    while (std::getline(ss, segment, '+')) {
        segment.erase(0, segment.find_first_not_of(" \t"));
        segment.erase(segment.find_last_not_of(" \t") + 1);
        std::string lower = segment;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (lower == "ctrl" || lower == "control") g_targetCtrl = true;
        else if (lower == "win" || lower == "windows" || lower == "cmd") g_targetWin = true;
        else if (lower == "alt") g_targetAlt = true;
        else if (lower == "shift") g_targetShift = true;
        else {
            NativeKeyType k = ParseKey(segment);
            if (k != 0) g_targetKey = k;
            else DEBUG_WARN_F("KEYS", "Unknown key in shortcut: {}", segment);
        }
    }
    DEBUG_INFO_F("KEYS", "Shortcut set to: Ctrl={} Win={} Alt={} Shift={} Key={}", 
                 g_targetCtrl, g_targetWin, g_targetAlt, g_targetShift, g_targetKey);
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