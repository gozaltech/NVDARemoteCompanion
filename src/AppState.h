#pragma once
#include <chrono>

class AppState {
private:
    static bool g_sendingKeys;
    static std::chrono::steady_clock::time_point g_sendingKeysEnabledTime;
    static bool g_releasingKeys;

public:
    static bool IsSendingKeys();
    static void ToggleSendingKeys();
    static bool IsReleasingKeys();
};
