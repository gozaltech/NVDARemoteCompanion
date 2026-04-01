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

template<typename T>
static void ReadJson(const nlohmann::json& j, const char* key, std::optional<T>& out) {
    if (j.contains(key)) {
        try { out = j[key].get<T>(); }
        catch (const std::exception& e) {
            DEBUG_WARN_F("CONFIG", "Failed to read field '{}': {}", key, e.what());
        }
    }
}

template<typename T>
static void ReadJson(const nlohmann::json& j, const char* key, T& out) {
    if (j.contains(key)) {
        try { out = j[key].get<T>(); }
        catch (const std::exception& e) {
            DEBUG_WARN_F("CONFIG", "Failed to read field '{}': {}", key, e.what());
        }
    }
}

std::string ConfigFile::DefaultPath() {
    std::string configDir = GetPlatformConfigDir();
    if (!configDir.empty()) {
        return (fs::path(configDir) / CONFIG_FILENAME).string();
    }
    return CONFIG_FILENAME;
}

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
    ReadJson(j, ProfileFields::NAME,                 p.name);
    ReadJson(j, ProfileFields::HOST,                 p.host);
    ReadJson(j, ProfileFields::PORT,                 p.port);
    ReadJson(j, ProfileFields::KEY,                  p.key);
    ReadJson(j, ProfileFields::SHORTCUT,             p.shortcut);
    ReadJson(j, ProfileFields::AUTO_CONNECT,         p.autoConnect);
    ReadJson(j, ProfileFields::SPEECH,               p.speech);
    ReadJson(j, ProfileFields::MUTE_ON_LOCAL_CONTROL, p.muteOnLocalControl);
    ReadJson(j, ProfileFields::FORWARD_AUDIO,         p.forwardAudio);
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

        ReadJson(j, "debug_level",              data.debugLevel);
        ReadJson(j, "background",               data.background);
        ReadJson(j, "audio",                    data.audio);
        ReadJson(j, "cycle_shortcut",           data.cycleShortcut);
        ReadJson(j, "exit_shortcut",            data.exitShortcut);
        ReadJson(j, "reinstall_hook_shortcut",  data.reinstallHookShortcut);
        ReadJson(j, "local_shortcut",           data.localShortcut);

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

        ReadJson(j, "host",     data.host);
        ReadJson(j, "port",     data.port);
        ReadJson(j, "key",      data.key);
        ReadJson(j, "shortcut", data.shortcut);

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
        {"background", false},
        {"audio", true},
        {"cycle_shortcut", "ctrl+alt+f11"},
        {"profiles", nlohmann::ordered_json::array({
            nlohmann::ordered_json({
                {"name", "default"},
                {"host", ""},
                {"port", Config::DEFAULT_PORT},
                {"key", ""},
                {"shortcut", "ctrl+win+f11"},
                {"auto_connect", true},
                {"speech", true},
                {"mute_on_local_control", false},
                {"forward_audio", true}
            })
        })}
    };

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << j.dump(4) << std::endl;
    return file.good();
}

bool ConfigFile::Save(const std::string& path, const ConfigFileData& data) {
    nlohmann::ordered_json j;

    j["debug_level"] = data.debugLevel.value_or("warning");
    j["background"] = data.background.value_or(false);
    j["audio"] = data.audio.value_or(true);
    j["cycle_shortcut"] = data.cycleShortcut.value_or("ctrl+alt+f11");
    if (data.exitShortcut) j["exit_shortcut"] = *data.exitShortcut;
    if (data.reinstallHookShortcut) j["reinstall_hook_shortcut"] = *data.reinstallHookShortcut;
    if (data.localShortcut) j["local_shortcut"] = *data.localShortcut;

    auto profilesArr = nlohmann::ordered_json::array();
    for (const auto& p : data.profiles) {
        nlohmann::ordered_json pj;
        pj[ProfileFields::NAME]                 = p.name;
        pj[ProfileFields::HOST]                 = p.host;
        pj[ProfileFields::PORT]                 = p.port;
        pj[ProfileFields::KEY]                  = p.key;
        pj[ProfileFields::SHORTCUT]             = p.shortcut;
        pj[ProfileFields::AUTO_CONNECT]         = p.autoConnect;
        pj[ProfileFields::SPEECH]               = p.speech;
        pj[ProfileFields::MUTE_ON_LOCAL_CONTROL] = p.muteOnLocalControl;
        pj[ProfileFields::FORWARD_AUDIO]         = p.forwardAudio;
        profilesArr.push_back(std::move(pj));
    }
    j["profiles"] = std::move(profilesArr);

    fs::path filePath(path);
    if (filePath.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(filePath.parent_path(), ec);
    }

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << j.dump(4) << std::endl;
    return file.good();
}
