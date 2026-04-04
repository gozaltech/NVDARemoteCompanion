#pragma once
#include <cstdint>
#include <vector>
#include <string>

#ifdef _WIN32
    #include <windows.h>
    using NativeKeyType = DWORD;
    using NativeScanType = WORD;
#else
    using NativeKeyType = uint32_t;
    using NativeScanType = uint16_t;

    constexpr uint32_t VK_BACK      = 0x08;
    constexpr uint32_t VK_TAB       = 0x09;
    constexpr uint32_t VK_RETURN    = 0x0D;
    constexpr uint32_t VK_SHIFT     = 0x10;
    constexpr uint32_t VK_CONTROL   = 0x11;
    constexpr uint32_t VK_MENU      = 0x12;
    constexpr uint32_t VK_PAUSE     = 0x13;
    constexpr uint32_t VK_CAPITAL   = 0x14;
    constexpr uint32_t VK_ESCAPE    = 0x1B;
    constexpr uint32_t VK_SPACE     = 0x20;
    constexpr uint32_t VK_PRIOR     = 0x21;
    constexpr uint32_t VK_NEXT      = 0x22;
    constexpr uint32_t VK_END       = 0x23;
    constexpr uint32_t VK_HOME      = 0x24;
    constexpr uint32_t VK_LEFT      = 0x25;
    constexpr uint32_t VK_UP        = 0x26;
    constexpr uint32_t VK_RIGHT     = 0x27;
    constexpr uint32_t VK_DOWN      = 0x28;
    constexpr uint32_t VK_SNAPSHOT  = 0x2C;
    constexpr uint32_t VK_INSERT    = 0x2D;
    constexpr uint32_t VK_DELETE    = 0x2E;
    constexpr uint32_t VK_LWIN      = 0x5B;
    constexpr uint32_t VK_RWIN      = 0x5C;
    constexpr uint32_t VK_NUMPAD0   = 0x60;
    constexpr uint32_t VK_NUMPAD1   = 0x61;
    constexpr uint32_t VK_NUMPAD2   = 0x62;
    constexpr uint32_t VK_NUMPAD3   = 0x63;
    constexpr uint32_t VK_NUMPAD4   = 0x64;
    constexpr uint32_t VK_NUMPAD5   = 0x65;
    constexpr uint32_t VK_NUMPAD6   = 0x66;
    constexpr uint32_t VK_NUMPAD7   = 0x67;
    constexpr uint32_t VK_NUMPAD8   = 0x68;
    constexpr uint32_t VK_NUMPAD9   = 0x69;
    constexpr uint32_t VK_MULTIPLY  = 0x6A;
    constexpr uint32_t VK_ADD       = 0x6B;
    constexpr uint32_t VK_SUBTRACT  = 0x6D;
    constexpr uint32_t VK_DECIMAL   = 0x6E;
    constexpr uint32_t VK_DIVIDE    = 0x6F;
    constexpr uint32_t VK_F1        = 0x70;
    constexpr uint32_t VK_F2        = 0x71;
    constexpr uint32_t VK_F3        = 0x72;
    constexpr uint32_t VK_F4        = 0x73;
    constexpr uint32_t VK_F5        = 0x74;
    constexpr uint32_t VK_F6        = 0x75;
    constexpr uint32_t VK_F7        = 0x76;
    constexpr uint32_t VK_F8        = 0x77;
    constexpr uint32_t VK_F9        = 0x78;
    constexpr uint32_t VK_F10       = 0x79;
    constexpr uint32_t VK_F11       = 0x7A;
    constexpr uint32_t VK_F12       = 0x7B;
    constexpr uint32_t VK_F13       = 0x7C;
    constexpr uint32_t VK_F14       = 0x7D;
    constexpr uint32_t VK_F15       = 0x7E;
    constexpr uint32_t VK_F16       = 0x7F;
    constexpr uint32_t VK_F17       = 0x80;
    constexpr uint32_t VK_F18       = 0x81;
    constexpr uint32_t VK_F19       = 0x82;
    constexpr uint32_t VK_F20       = 0x83;
    constexpr uint32_t VK_F21       = 0x84;
    constexpr uint32_t VK_F22       = 0x85;
    constexpr uint32_t VK_F23       = 0x86;
    constexpr uint32_t VK_F24       = 0x87;
    constexpr uint32_t VK_NUMLOCK   = 0x90;
    constexpr uint32_t VK_SCROLL    = 0x91;
    constexpr uint32_t VK_LSHIFT    = 0xA0;
    constexpr uint32_t VK_RSHIFT    = 0xA1;
    constexpr uint32_t VK_LCONTROL  = 0xA2;
    constexpr uint32_t VK_RCONTROL  = 0xA3;
    constexpr uint32_t VK_LMENU     = 0xA4;
    constexpr uint32_t VK_RMENU     = 0xA5;
    constexpr uint32_t VK_OEM_1     = 0xBA;
    constexpr uint32_t VK_OEM_PLUS  = 0xBB;
    constexpr uint32_t VK_OEM_COMMA = 0xBC;
    constexpr uint32_t VK_OEM_MINUS = 0xBD;
    constexpr uint32_t VK_OEM_PERIOD= 0xBE;
    constexpr uint32_t VK_OEM_2     = 0xBF;
    constexpr uint32_t VK_OEM_3     = 0xC0;
    constexpr uint32_t VK_OEM_4     = 0xDB;
    constexpr uint32_t VK_OEM_5     = 0xDC;
    constexpr uint32_t VK_OEM_6     = 0xDD;
    constexpr uint32_t VK_OEM_7     = 0xDE;
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
    static std::vector<PressedKey> g_pressedKeyDetails;

    static std::vector<ShortcutConfig> g_shortcuts;
    static ShortcutConfig g_cycleShortcut;
    static ShortcutConfig g_exitShortcut;
    static ShortcutConfig g_reinstallHookShortcut;
static ShortcutConfig g_reconnectShortcut;
    static ShortcutConfig g_clipboardShortcut;
    static ShortcutConfig g_forwardKeysShortcut;

    static void ApplyGlobalShortcut(ShortcutConfig& sc, const std::string& shortcut, const char* name);
    static bool CheckGlobalShortcut(const ShortcutConfig& sc, NativeKeyType vkCode);

public:
    static bool IsControlKey(NativeKeyType vkCode);
    static bool IsWinKey(NativeKeyType vkCode);
    static bool IsAltKey(NativeKeyType vkCode);
    static bool IsShiftKey(NativeKeyType vkCode);

    static void UpdateModifierState(NativeKeyType vkCode, bool isPressed);

    static int CheckToggleShortcut(NativeKeyType vkCode);
    static bool CheckCycleShortcut(NativeKeyType vkCode);
    static void SetCycleShortcut(const std::string& shortcut);
    static void SetExitShortcut(const std::string& shortcut);
    static void SetReinstallHookShortcut(const std::string& shortcut);
    static bool CheckExitShortcut(NativeKeyType vkCode);
    static bool CheckReinstallHookShortcut(NativeKeyType vkCode);
static void SetReconnectShortcut(const std::string& shortcut);
    static bool CheckReconnectShortcut(NativeKeyType vkCode);
    static void SetClipboardShortcut(const std::string& shortcut);
    static bool CheckClipboardShortcut(NativeKeyType vkCode);
    static void SetForwardKeysShortcut(const std::string& shortcut);
    static bool CheckForwardKeysShortcut(NativeKeyType vkCode);


    static void SetToggleShortcut(const std::string& shortcut);

    static void SetToggleShortcutAt(int index, const std::string& shortcut);

    static void ResetModifiers();

    static void TrackKeyPress(NativeKeyType vkCode, NativeScanType scanCode, bool extended);
    static bool TrackKeyRelease(NativeKeyType vkCode);
    static const std::vector<PressedKey>& GetAllPressedKeys();
    static void ClearPressedKeys();

    static void ClearShortcuts();
    static ShortcutConfig ParseShortcutString(const std::string& shortcut);
};
