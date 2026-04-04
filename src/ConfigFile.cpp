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

#ifdef __ANDROID__
static std::string g_androidDataDir;

void ConfigFile::SetAndroidDataDir(const std::string& dir) {
    g_androidDataDir = dir;
}
#endif

static std::string GetPlatformConfigDir() {
#ifdef __ANDROID__
    return g_androidDataDir;
#elif defined(_WIN32)
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
static constexpr int CURRENT_SCHEMA_VERSION = 1;

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

static nlohmann::ordered_json SerializeProfile(const ProfileConfig& p) {
    nlohmann::ordered_json j;
    j[ProfileFields::NAME]                  = p.name;
    j[ProfileFields::HOST]                  = p.host;
    j[ProfileFields::PORT]                  = p.port;
    j[ProfileFields::KEY]                   = p.key;
    j[ProfileFields::SHORTCUT]              = p.shortcut;
    j[ProfileFields::AUTO_CONNECT]          = p.autoConnect;
    j[ProfileFields::SPEECH]               = p.speech;
    j[ProfileFields::MUTE_ON_LOCAL_CONTROL] = p.muteOnLocalControl;
    j[ProfileFields::FORWARD_AUDIO]         = p.forwardAudio;
    return j;
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

static void Migrate_0_to_1(nlohmann::json& j) {
    nlohmann::json sc = nlohmann::json::object();
    auto move = [&](const char* oldKey, const char* newKey) {
        if (j.contains(oldKey)) {
            sc[newKey] = j[oldKey];
            j.erase(oldKey);
        }
    };
    move("cycle_shortcut",          "cycle");
    move("exit_shortcut",           "exit");
    move("reinstall_hook_shortcut", "reinstall_hook");
    move("local_shortcut",          "local");
    move("reconnect_shortcut",      "reconnect");
    if (!sc.empty() || !j.contains("shortcuts")) {
        j["shortcuts"] = std::move(sc);
    }
}

bool ConfigFile::Migrate(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return false;

    nlohmann::json j;
    try { j = nlohmann::json::parse(in); }
    catch (...) { return false; }
    in.close();

    int version = j.value("schema_version", 0);
    if (version >= CURRENT_SCHEMA_VERSION) return false;

    if (version < 1) { Migrate_0_to_1(j); version = 1; }

    j["schema_version"] = CURRENT_SCHEMA_VERSION;

    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << j.dump(4) << std::endl;

    DEBUG_INFO_F("CONFIG", "Config migrated to schema v{}", CURRENT_SCHEMA_VERSION);
    return true;
}

static ConfigFileData ParseConfigJson(const nlohmann::json& j) {
    ConfigFileData data;

    ReadJson(j, "debug_level", data.debugLevel);
    ReadJson(j, "background",  data.background);
    ReadJson(j, "audio",       data.audio);

    if (j.contains("shortcuts") && j["shortcuts"].is_object()) {
        const auto& sc = j["shortcuts"];
        ReadJson(sc, "cycle",          data.cycleShortcut);
        ReadJson(sc, "exit",           data.exitShortcut);
        ReadJson(sc, "reinstall_hook", data.reinstallHookShortcut);
        ReadJson(sc, "reconnect",      data.reconnectShortcut);
        ReadJson(sc, "clipboard",      data.clipboardShortcut);
        ReadJson(sc, "forward_keys",   data.forwardKeysShortcut);
    }

    if (j.contains("profiles") && j["profiles"].is_array()) {
        for (const auto& pj : j["profiles"]) {
            if (pj.is_object()) {
                auto profile = ParseProfile(pj);
                if (!profile.host.empty() && !profile.key.empty()) {
                    if (profile.name.empty()) profile.name = profile.host;
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

    return data;
}

ConfigFileData ConfigFile::Load(const std::string& path) {
    if (path.empty()) return {};

    std::ifstream file(path);
    if (!file.is_open()) {
        DEBUG_WARN("CONFIG", "Could not open config file: " + path);
        return {};
    }

    try {
        ConfigFileData data = ParseConfigJson(nlohmann::json::parse(file));
        DEBUG_INFO("CONFIG", "Loaded config from: " + path);
        return data;
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Error: Failed to parse config file '" << path << "': " << e.what() << std::endl;
        DEBUG_WARN("CONFIG", "Failed to parse config file: " + std::string(e.what()));
        return {};
    }
}

ConfigFileData ConfigFile::LoadFromString(const std::string& jsonStr) {
    try {
        return ParseConfigJson(nlohmann::json::parse(jsonStr));
    } catch (const nlohmann::json::exception& e) {
        DEBUG_WARN("CONFIG", "Failed to parse config string: " + std::string(e.what()));
        return {};
    }
}

bool ConfigFile::CreateDefault(const std::string& path) {
    fs::path dir = fs::path(path).parent_path();
    if (!dir.empty() && !fs::exists(dir)) {
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) return false;
    }

    nlohmann::ordered_json profile = {
        {"name",                 ""},
        {"host",                 ""},
        {"port",                 6837},
        {"key",                  ""},
        {"shortcut",             ""},
        {"auto_connect",         true},
        {"speech",               true},
        {"mute_on_local_control", false},
        {"forward_nvda_sounds",  true}
    };

    nlohmann::ordered_json j = {
        {"schema_version", CURRENT_SCHEMA_VERSION},
        {"debug_level",    "warning"},
        {"background",     false},
        {"audio",          true},
        {"shortcuts", nlohmann::ordered_json({
            {"cycle",          Config::DEFAULT_CYCLE_SHORTCUT},
            {"exit",           ""},
            {"reinstall_hook", ""},
            {"reconnect",      ""},
            {"clipboard",      ""},
            {"forward_keys",   Config::DEFAULT_FORWARD_KEYS_SHORTCUT}
        })},
        {"profiles", nlohmann::ordered_json::array({profile})}
    };

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << j.dump(4) << std::endl;
    return file.good();
}

bool ConfigFile::Save(const std::string& path, const ConfigFileData& data) {
    nlohmann::ordered_json j;

    j["schema_version"] = CURRENT_SCHEMA_VERSION;
    j["debug_level"] = data.debugLevel.value_or("warning");
    j["background"] = data.background.value_or(false);
    j["audio"] = data.audio.value_or(true);
    {
        nlohmann::ordered_json sc;
        sc["cycle"] = data.cycleShortcut.value_or(Config::DEFAULT_CYCLE_SHORTCUT);
        if (data.exitShortcut)           sc["exit"]           = *data.exitShortcut;
        if (data.reinstallHookShortcut)  sc["reinstall_hook"] = *data.reinstallHookShortcut;
        if (data.reconnectShortcut)      sc["reconnect"]      = *data.reconnectShortcut;
        if (data.clipboardShortcut)      sc["clipboard"]      = *data.clipboardShortcut;
        sc["forward_keys"] = data.forwardKeysShortcut.value_or(Config::DEFAULT_FORWARD_KEYS_SHORTCUT);
        j["shortcuts"] = std::move(sc);
    }

    auto profilesArr = nlohmann::ordered_json::array();
    for (const auto& p : data.profiles) {
        profilesArr.push_back(SerializeProfile(p));
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

void ConfigFile::StripInvalidProfiles(std::vector<ProfileConfig>& profiles) {
    profiles.erase(
        std::remove_if(profiles.begin(), profiles.end(),
            [](const ProfileConfig& p) { return p.host.empty() || p.key.empty(); }),
        profiles.end());
}

std::string ConfigFile::ProfileToJsonString(const ProfileConfig& p) {
    return SerializeProfile(p).dump();
}
