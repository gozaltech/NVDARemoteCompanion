#pragma once
#include <vector>
#include <string>

class AppState {
private:
    static int g_activeProfile;
    static bool g_releasingKeys;
    static std::vector<int> g_connectedProfiles;
    static std::vector<std::string> g_profileNames;

    static void ReleaseAllKeys();

public:
    static bool IsSendingKeys();
    static void ToggleSendingKeys(int profileIndex);
    static void CycleProfile();
    static void SetConnectedProfiles(const std::vector<int>& indices, const std::vector<std::string>& names);
    static bool IsReleasingKeys();
    static int GetActiveProfile();
    static void GoLocal();
};
