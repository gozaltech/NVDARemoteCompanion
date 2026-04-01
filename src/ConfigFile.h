#pragma once
#include <string>
#include <optional>
#include <vector>

namespace ProfileFields {
    constexpr const char* NAME                  = "name";
    constexpr const char* HOST                  = "host";
    constexpr const char* PORT                  = "port";
    constexpr const char* KEY                   = "key";
    constexpr const char* SHORTCUT              = "shortcut";
    constexpr const char* AUTO_CONNECT          = "auto_connect";
    constexpr const char* SPEECH                = "speech";
    constexpr const char* MUTE_ON_LOCAL_CONTROL = "mute_on_local_control";
    constexpr const char* FORWARD_AUDIO         = "forward_nvda_sounds";
}

struct ProfileConfig {
    std::string name;
    std::string host;
    int port = 6837;
    std::string key;
    std::string shortcut;
    bool autoConnect = true;
    bool speech = true;
    bool muteOnLocalControl = false;
    bool forwardAudio = true;
};

struct ConfigFileData {
    std::optional<std::string> debugLevel;
    std::optional<bool> background;
    std::optional<bool> audio;
    std::optional<std::string> cycleShortcut;
    std::optional<std::string> exitShortcut;
    std::optional<std::string> reinstallHookShortcut;
    std::optional<std::string> localShortcut;
    std::optional<std::string> reconnectShortcut;

    std::vector<ProfileConfig> profiles;

    std::optional<std::string> host;
    std::optional<int> port;
    std::optional<std::string> key;
    std::optional<std::string> shortcut;
};

namespace ConfigFile {
    std::string DefaultPath();
    std::string FindConfigFile(const std::string& explicitPath = "");
    bool Migrate(const std::string& path);
    ConfigFileData Load(const std::string& path);
    bool CreateDefault(const std::string& path);
    bool Save(const std::string& path, const ConfigFileData& data);

#ifdef __ANDROID__
    void SetAndroidDataDir(const std::string& dir);
#endif
}
