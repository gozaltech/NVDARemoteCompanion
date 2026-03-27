#pragma once

class AppState {
private:
    static int g_activeProfile;
    static bool g_releasingKeys;

public:
    static bool IsSendingKeys();
    static void ToggleSendingKeys(int profileIndex);
    static bool IsReleasingKeys();
    static int GetActiveProfile();
};
