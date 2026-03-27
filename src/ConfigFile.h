#pragma once
#include <string>
#include <optional>

struct ConfigFileData {
    std::optional<std::string> host;
    std::optional<int> port;
    std::optional<std::string> key;
    std::optional<std::string> shortcut;
    std::optional<std::string> debugLevel;
    std::optional<bool> speech;
};

namespace ConfigFile {
    std::string FindConfigFile(const std::string& explicitPath = "");
    ConfigFileData Load(const std::string& path);
    bool CreateDefault(const std::string& path);
}
