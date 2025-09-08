#pragma once
#include <cstdint>
#include <set>
#include <vector>

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
    static std::set<NativeKeyType> g_pressedKeys;
    static std::vector<PressedKey> g_pressedKeyDetails;

public:
    static bool IsControlKey(NativeKeyType vkCode);
    static bool IsWinKey(NativeKeyType vkCode);
    static void UpdateModifierState(NativeKeyType vkCode, bool isPressed);
    static bool IsToggleShortcut(NativeKeyType vkCode);
    static void ResetModifiers();
    
    static void TrackKeyPress(NativeKeyType vkCode, NativeScanType scanCode, bool extended);
    static void TrackKeyRelease(NativeKeyType vkCode);
    static std::vector<PressedKey> GetAllPressedKeys();
    static void ClearPressedKeys();
};