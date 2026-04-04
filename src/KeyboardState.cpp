#include "KeyboardState.h"
#include "Config.h"
#include "Debug.h"
#include <algorithm>
#include <sstream>
#include <unordered_map>

#define CTRL_KEY_1 VK_CONTROL
#define CTRL_KEY_2 VK_LCONTROL
#define CTRL_KEY_3 VK_RCONTROL
#define WIN_KEY_1  VK_LWIN
#define WIN_KEY_2  VK_RWIN

bool KeyboardState::g_ctrlPressed = false;
bool KeyboardState::g_winPressed = false;
bool KeyboardState::g_altPressed = false;
bool KeyboardState::g_shiftPressed = false;

std::vector<ShortcutConfig> KeyboardState::g_shortcuts;
ShortcutConfig KeyboardState::g_cycleShortcut;
ShortcutConfig KeyboardState::g_exitShortcut;
ShortcutConfig KeyboardState::g_reinstallHookShortcut;
ShortcutConfig KeyboardState::g_localShortcut;
ShortcutConfig KeyboardState::g_reconnectShortcut;
std::vector<PressedKey> KeyboardState::g_pressedKeyDetails;

bool KeyboardState::IsControlKey(NativeKeyType vkCode) {
    return vkCode == CTRL_KEY_1 || vkCode == CTRL_KEY_2 || vkCode == CTRL_KEY_3;
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

static std::string ShortcutToString(const ShortcutConfig& sc) {
    std::string result;
    if (sc.ctrl)  result += "Ctrl+";
    if (sc.win)   result += "Win+";
    if (sc.alt)   result += "Alt+";
    if (sc.shift) result += "Shift+";

    if (sc.key >= VK_F1 && sc.key <= VK_F24) {
        result += "F" + std::to_string(sc.key - VK_F1 + 1);
    } else {
        static std::unordered_map<NativeKeyType, std::string> nameMap = {
            {VK_SPACE, "Space"}, {VK_RETURN, "Enter"}, {VK_ESCAPE, "Escape"},
            {VK_TAB, "Tab"}, {VK_UP, "Up"}, {VK_DOWN, "Down"},
            {VK_LEFT, "Left"}, {VK_RIGHT, "Right"}, {VK_HOME, "Home"},
            {VK_END, "End"}, {VK_PRIOR, "PageUp"}, {VK_NEXT, "PageDown"},
            {VK_INSERT, "Insert"}, {VK_DELETE, "Delete"}, {VK_BACK, "Backspace"},
            {VK_PAUSE, "Pause"}, {VK_SNAPSHOT, "PrintScreen"},
            {VK_CAPITAL, "CapsLock"}, {VK_NUMLOCK, "NumLock"}
        };
        auto it = nameMap.find(sc.key);
        if (it != nameMap.end()) {
            result += it->second;
        } else if (sc.key >= 'A' && sc.key <= 'Z') {
            result += static_cast<char>(sc.key);
        } else if (sc.key >= '0' && sc.key <= '9') {
            result += static_cast<char>(sc.key);
        } else {
            result += "0x" + [&]{ std::ostringstream o; o << std::hex << sc.key; return o.str(); }();
        }
    }
    return result;
}

void KeyboardState::ApplyGlobalShortcut(ShortcutConfig& sc, const std::string& shortcut, const char* name) {
    sc = ParseShortcutString(shortcut);
    if (sc.key != 0) {
        DEBUG_INFO_F("KEYS", "{} shortcut set to: {}", name, ShortcutToString(sc));
    }
}

bool KeyboardState::CheckGlobalShortcut(const ShortcutConfig& sc, NativeKeyType vkCode) {
    return MatchesShortcut(sc, vkCode, g_ctrlPressed, g_winPressed, g_altPressed, g_shiftPressed);
}

int KeyboardState::CheckToggleShortcut(NativeKeyType vkCode) {
    for (int i = 0; i < static_cast<int>(g_shortcuts.size()); i++) {
        if (MatchesShortcut(g_shortcuts[i], vkCode,
                            g_ctrlPressed, g_winPressed, g_altPressed, g_shiftPressed)) {
            return i;
        }
    }
    return -1;
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
    DEBUG_INFO_F("KEYS", "Shortcut[{}] set to: {}", index, ShortcutToString(sc));
}

void KeyboardState::SetCycleShortcut(const std::string& shortcut) {
    ApplyGlobalShortcut(g_cycleShortcut, shortcut, "Cycle");
}

bool KeyboardState::CheckCycleShortcut(NativeKeyType vkCode) {
    return CheckGlobalShortcut(g_cycleShortcut, vkCode);
}

void KeyboardState::SetExitShortcut(const std::string& shortcut) {
    ApplyGlobalShortcut(g_exitShortcut, shortcut, "Exit");
}

bool KeyboardState::CheckExitShortcut(NativeKeyType vkCode) {
    return CheckGlobalShortcut(g_exitShortcut, vkCode);
}

void KeyboardState::SetReinstallHookShortcut(const std::string& shortcut) {
    ApplyGlobalShortcut(g_reinstallHookShortcut, shortcut, "Reinstall hook");
}

bool KeyboardState::CheckReinstallHookShortcut(NativeKeyType vkCode) {
    return CheckGlobalShortcut(g_reinstallHookShortcut, vkCode);
}

void KeyboardState::SetLocalShortcut(const std::string& shortcut) {
    ApplyGlobalShortcut(g_localShortcut, shortcut, "Local");
}

bool KeyboardState::CheckLocalShortcut(NativeKeyType vkCode) {
    return CheckGlobalShortcut(g_localShortcut, vkCode);
}

void KeyboardState::SetReconnectShortcut(const std::string& shortcut) {
    ApplyGlobalShortcut(g_reconnectShortcut, shortcut, "Reconnect");
}

bool KeyboardState::CheckReconnectShortcut(NativeKeyType vkCode) {
    return CheckGlobalShortcut(g_reconnectShortcut, vkCode);
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

    static std::unordered_map<std::string, NativeKeyType> keyMap = {
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
        segment = Config::TrimWhitespace(segment);
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

void KeyboardState::ResetModifiers() {
    g_ctrlPressed = false;
    g_winPressed = false;
    g_altPressed = false;
    g_shiftPressed = false;
}

void KeyboardState::TrackKeyPress(NativeKeyType vkCode, NativeScanType scanCode, bool extended) {
    for (const auto& pk : g_pressedKeyDetails) {
        if (pk.vkCode == vkCode) return;
    }
    g_pressedKeyDetails.push_back({vkCode, scanCode, extended});
}

bool KeyboardState::TrackKeyRelease(NativeKeyType vkCode) {
    auto it = std::remove_if(g_pressedKeyDetails.begin(), g_pressedKeyDetails.end(),
                             [vkCode](const PressedKey& key) { return key.vkCode == vkCode; });
    if (it == g_pressedKeyDetails.end()) return false;
    g_pressedKeyDetails.erase(it, g_pressedKeyDetails.end());
    return true;
}

const std::vector<PressedKey>& KeyboardState::GetAllPressedKeys() {
    return g_pressedKeyDetails;
}

void KeyboardState::ClearPressedKeys() {
    g_pressedKeyDetails.clear();
}
