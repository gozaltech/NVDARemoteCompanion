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

class KeyboardState {
private:
    static bool g_ctrlPressed;
    static bool g_winPressed;
    static bool g_altPressed;
    static bool g_shiftPressed;
    static std::set<NativeKeyType> g_pressedKeys;
    static std::vector<PressedKey> g_pressedKeyDetails;
    
    // Configured shortcut
    static bool g_targetCtrl;
    static bool g_targetWin;
    static bool g_targetAlt;
    static bool g_targetShift;
    static NativeKeyType g_targetKey;

public:
    static bool IsControlKey(NativeKeyType vkCode);
    static bool IsWinKey(NativeKeyType vkCode);
    static bool IsAltKey(NativeKeyType vkCode);
    static bool IsShiftKey(NativeKeyType vkCode);
    
    static void UpdateModifierState(NativeKeyType vkCode, bool isPressed);
    static bool IsToggleShortcut(NativeKeyType vkCode);
    static void SetToggleShortcut(const std::string& shortcut);
    static void ResetModifiers();
    
    static void TrackKeyPress(NativeKeyType vkCode, NativeScanType scanCode, bool extended);
    static void TrackKeyRelease(NativeKeyType vkCode);
    static std::vector<PressedKey> GetAllPressedKeys();
    static void ClearPressedKeys();
};