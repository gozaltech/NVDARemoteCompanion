#include "ConfigFile.h"
#include "Config.h"
#include "Debug.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <cstdlib>
#endif

namespace fs = std::filesystem;

static std::string GetPlatformConfigDir() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        return (fs::path(path) / "NVDARemoteCompanion").string();
    }
    return "";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0] != '\0') {
        return (fs::path(xdg) / "NVDARemoteCompanion").string();
    }
    const char* home = std::getenv("HOME");
    if (home) {
        return (fs::path(home) / ".config" / "NVDARemoteCompanion").string();
    }
    return "";
#endif
}

static constexpr const char* CONFIG_FILENAME = "nvdaremote.json";

std::string ConfigFile::FindConfigFile(const std::string& explicitPath) {
    if (!explicitPath.empty()) {
        if (fs::exists(explicitPath)) {
            return explicitPath;
        }
        return "";
    }

    if (fs::exists(CONFIG_FILENAME)) {
        return CONFIG_FILENAME;
    }

    std::string configDir = GetPlatformConfigDir();
    if (!configDir.empty()) {
        fs::path configPath = fs::path(configDir) / CONFIG_FILENAME;
        if (fs::exists(configPath)) {
            return configPath.string();
        }
    }

    return "";
}

static ProfileConfig ParseProfile(const nlohmann::json& j) {
    ProfileConfig p;
    if (j.contains("name") && j["name"].is_string())
        p.name = j["name"].get<std::string>();
    if (j.contains("host") && j["host"].is_string())
        p.host = j["host"].get<std::string>();
    if (j.contains("port") && j["port"].is_number_integer())
        p.port = j["port"].get<int>();
    if (j.contains("key") && j["key"].is_string())
        p.key = j["key"].get<std::string>();
    if (j.contains("shortcut") && j["shortcut"].is_string())
        p.shortcut = j["shortcut"].get<std::string>();
    return p;
}

ConfigFileData ConfigFile::Load(const std::string& path) {
    ConfigFileData data;
    if (path.empty()) return data;

    std::ifstream file(path);
    if (!file.is_open()) {
        DEBUG_WARN("CONFIG", "Could not open config file: " + path);
        return data;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(file);

        if (j.contains("debug_level") && j["debug_level"].is_string()) {
            data.debugLevel = j["debug_level"].get<std::string>();
        }
        if (j.contains("speech") && j["speech"].is_boolean()) {
            data.speech = j["speech"].get<bool>();
        }
        if (j.contains("background") && j["background"].is_boolean()) {
            data.background = j["background"].get<bool>();
        }

        if (j.contains("profiles") && j["profiles"].is_array()) {
            for (const auto& pj : j["profiles"]) {
                if (pj.is_object()) {
                    auto profile = ParseProfile(pj);
                    if (!profile.host.empty() && !profile.key.empty()) {
                        if (profile.name.empty()) {
                            profile.name = profile.host;
                        }
                        data.profiles.push_back(std::move(profile));
                    } else if (!profile.name.empty() || !profile.host.empty() || !profile.key.empty()) {
                        std::string name = profile.name.empty() ? "(unnamed)" : profile.name;
                        DEBUG_WARN_F("CONFIG", "Skipping profile '{}': host and key are both required", name);
                    }
                }
            }
        }

        if (j.contains("host") && j["host"].is_string()) {
            data.host = j["host"].get<std::string>();
        }
        if (j.contains("port") && j["port"].is_number_integer()) {
            data.port = j["port"].get<int>();
        }
        if (j.contains("key") && j["key"].is_string()) {
            data.key = j["key"].get<std::string>();
        }
        if (j.contains("shortcut") && j["shortcut"].is_string()) {
            data.shortcut = j["shortcut"].get<std::string>();
        }

        DEBUG_INFO("CONFIG", "Loaded config from: " + path);
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error: Failed to parse config file '" << path << "': " << e.what() << std::endl;
        DEBUG_WARN("CONFIG", "Failed to parse config file: " + std::string(e.what()));
    }

    return data;
}

bool ConfigFile::CreateDefault(const std::string& path) {
    fs::path dir = fs::path(path).parent_path();
    if (!dir.empty() && !fs::exists(dir)) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) return false;
    }

    nlohmann::ordered_json j = {
        {"debug_level", "warning"},
        {"speech", true},
        {"background", false},
        {"profiles", nlohmann::ordered_json::array({
            nlohmann::ordered_json({
                {"name", "default"},
                {"host", ""},
                {"port", Config::DEFAULT_PORT},
                {"key", ""},
                {"shortcut", "ctrl+win+f11"}
            })
        })}
    };

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << j.dump(4) << std::endl;
    return file.good();
}
