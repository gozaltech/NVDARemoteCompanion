#pragma once
#include <string>
#include <optional>
#include <vector>

struct ProfileConfig {
    std::string name;
    std::string host;
    int port = 6837;
    std::string key;
    std::string shortcut;
    bool autoConnect = true;
};

struct ConfigFileData {
    std::optional<std::string> debugLevel;
    std::optional<bool> speech;
    std::optional<bool> background;
    std::optional<std::string> cycleShortcut;

    std::vector<ProfileConfig> profiles;

    std::optional<std::string> host;
    std::optional<int> port;
    std::optional<std::string> key;
    std::optional<std::string> shortcut;
};

namespace ConfigFile {
    std::string FindConfigFile(const std::string& explicitPath = "");
    ConfigFileData Load(const std::string& path);
    bool CreateDefault(const std::string& path);
    bool Save(const std::string& path, const ConfigFileData& data);
}
