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

std::vector<ShortcutConfig> KeyboardState::g_shortcuts;
ShortcutConfig KeyboardState::g_cycleShortcut;
ShortcutConfig KeyboardState::g_exitShortcut;
bool KeyboardState::g_exitShortcutSet = false;
ShortcutConfig KeyboardState::g_reinstallHookShortcut;
bool KeyboardState::g_reinstallHookShortcutSet = false;
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

static bool MatchesShortcut(const ShortcutConfig& sc, NativeKeyType vkCode,
                            bool ctrl, bool win, bool alt, bool shift) {
    if (sc.key == 0) return false;
    if (vkCode != sc.key) return false;
    return (ctrl == sc.ctrl) && (win == sc.win) && (alt == sc.alt) && (shift == sc.shift);
}

int KeyboardState::CheckToggleShortcut(NativeKeyType vkCode) {
#ifdef _WIN32
    for (int i = 0; i < static_cast<int>(g_shortcuts.size()); i++) {
        if (MatchesShortcut(g_shortcuts[i], vkCode,
                            g_ctrlPressed, g_winPressed, g_altPressed, g_shiftPressed)) {
            return i;
        }
    }
#endif
    return -1;
}

static NativeKeyType ParseKey(const std::string& keyName) {
    std::string k = keyName;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);

    if (k.rfind("f", 0) == 0 && k.length() > 1) {
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

ShortcutConfig KeyboardState::ParseShortcutString(const std::string& shortcut) {
    ShortcutConfig sc;
    if (shortcut.empty()) return sc;

    std::stringstream ss(shortcut);
    std::string segment;

    while (std::getline(ss, segment, '+')) {
        segment.erase(0, segment.find_first_not_of(" \t"));
        segment.erase(segment.find_last_not_of(" \t") + 1);
        std::string lower = segment;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower == "ctrl" || lower == "control") sc.ctrl = true;
        else if (lower == "win" || lower == "windows" || lower == "cmd") sc.win = true;
        else if (lower == "alt") sc.alt = true;
        else if (lower == "shift") sc.shift = true;
        else {
            NativeKeyType k = ParseKey(segment);
            if (k != 0) sc.key = k;
            else DEBUG_WARN_F("KEYS", "Unknown key in shortcut: {}", segment);
        }
    }
    return sc;
}

bool KeyboardState::CheckCycleShortcut(NativeKeyType vkCode) {
    return MatchesShortcut(g_cycleShortcut, vkCode,
                           g_ctrlPressed, g_winPressed, g_altPressed, g_shiftPressed);
}

void KeyboardState::SetCycleShortcut(const std::string& shortcut) {
    g_cycleShortcut = ParseShortcutString(shortcut);
    if (g_cycleShortcut.key != 0) {
        DEBUG_INFO_F("KEYS", "Cycle shortcut set to: Ctrl={} Win={} Alt={} Shift={} Key={}",
                     g_cycleShortcut.ctrl, g_cycleShortcut.win, g_cycleShortcut.alt,
                     g_cycleShortcut.shift, g_cycleShortcut.key);
    }
}

void KeyboardState::SetExitShortcut(const std::string& shortcut) {
    g_exitShortcut = ParseShortcutString(shortcut);
    g_exitShortcutSet = (g_exitShortcut.key != 0);
    if (g_exitShortcutSet) {
        DEBUG_INFO_F("KEYS", "Exit shortcut set to: Ctrl={} Win={} Alt={} Shift={} Key={}",
                     g_exitShortcut.ctrl, g_exitShortcut.win, g_exitShortcut.alt,
                     g_exitShortcut.shift, g_exitShortcut.key);
    }
}

void KeyboardState::SetReinstallHookShortcut(const std::string& shortcut) {
    g_reinstallHookShortcut = ParseShortcutString(shortcut);
    g_reinstallHookShortcutSet = (g_reinstallHookShortcut.key != 0);
    if (g_reinstallHookShortcutSet) {
        DEBUG_INFO_F("KEYS", "Reinstall hook shortcut set to: Ctrl={} Win={} Alt={} Shift={} Key={}",
                     g_reinstallHookShortcut.ctrl, g_reinstallHookShortcut.win,
                     g_reinstallHookShortcut.alt, g_reinstallHookShortcut.shift,
                     g_reinstallHookShortcut.key);
    }
}

bool KeyboardState::CheckExitShortcut(NativeKeyType vkCode) {
    return g_exitShortcutSet && MatchesShortcut(g_exitShortcut, vkCode,
                                                g_ctrlPressed, g_winPressed, g_altPressed, g_shiftPressed);
}

bool KeyboardState::CheckReinstallHookShortcut(NativeKeyType vkCode) {
    return g_reinstallHookShortcutSet && MatchesShortcut(g_reinstallHookShortcut, vkCode,
                                                         g_ctrlPressed, g_winPressed, g_altPressed, g_shiftPressed);
}

void KeyboardState::ClearShortcuts() {
    g_shortcuts.clear();
    DEBUG_VERBOSE("KEYS", "All shortcuts cleared");
}

void KeyboardState::SetToggleShortcut(const std::string& shortcut) {
    SetToggleShortcutAt(0, shortcut);
}

void KeyboardState::SetToggleShortcutAt(int index, const std::string& shortcut) {
    auto sc = ParseShortcutString(shortcut);
    if (sc.key == 0) return;

    while (static_cast<int>(g_shortcuts.size()) <= index) {
        g_shortcuts.push_back({});
    }
    g_shortcuts[index] = sc;
    DEBUG_INFO_F("KEYS", "Shortcut[{}] set to: Ctrl={} Win={} Alt={} Shift={} Key={}",
                 index, sc.ctrl, sc.win, sc.alt, sc.shift, sc.key);
}

void KeyboardState::ResetModifiers() {
    g_ctrlPressed = false;
    g_winPressed = false;
    g_altPressed = false;
    g_shiftPressed = false;
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
