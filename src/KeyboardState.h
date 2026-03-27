#pragma once
#include <cstdint>
#include <set>
#include <vector>
#include <string>

#ifdef _WIN32
    #include <windows.h>
    using NativeKeyType = DWORD;
    using NativeScanType = WORD;
#else
    using NativeKeyType = uint32_t;
    using NativeScanType = uint16_t;
#endif

struct PressedKey {
    uint32_t vkCode;
    uint16_t scanCode;
    bool extended;

    PressedKey(NativeKeyType vk, NativeScanType scan, bool ext)
        : vkCode(static_cast<uint32_t>(vk)), scanCode(static_cast<uint16_t>(scan)), extended(ext) {}
};

struct ShortcutConfig {
    bool ctrl = false;
    bool win = false;
    bool alt = false;
    bool shift = false;
    NativeKeyType key = 0;
};

class KeyboardState {
private:
    static bool g_ctrlPressed;
    static bool g_winPressed;
    static bool g_altPressed;
    static bool g_shiftPressed;
    static std::set<NativeKeyType> g_pressedKeys;
    static std::vector<PressedKey> g_pressedKeyDetails;

    static std::vector<ShortcutConfig> g_shortcuts;

public:
    static bool IsControlKey(NativeKeyType vkCode);
    static bool IsWinKey(NativeKeyType vkCode);
    static bool IsAltKey(NativeKeyType vkCode);
    static bool IsShiftKey(NativeKeyType vkCode);

    static void UpdateModifierState(NativeKeyType vkCode, bool isPressed);

    static int CheckToggleShortcut(NativeKeyType vkCode);

    static void SetToggleShortcut(const std::string& shortcut);

    static void SetToggleShortcutAt(int index, const std::string& shortcut);

    static void ResetModifiers();

    static void TrackKeyPress(NativeKeyType vkCode, NativeScanType scanCode, bool extended);
    static void TrackKeyRelease(NativeKeyType vkCode);
    static std::vector<PressedKey> GetAllPressedKeys();
    static void ClearPressedKeys();

    static void ClearShortcuts();
    static ShortcutConfig ParseShortcutString(const std::string& shortcut);
};
