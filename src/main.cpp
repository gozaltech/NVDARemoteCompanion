#include <iostream>
#include <locale>
#include <cstdlib>
#include <csignal>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <functional>
#include "CommandHandler.h"
#include "ConfigFile.h"
#include "Debug.h"
#include "Audio.h"
#include "Speech.h"
#include "Config.h"

#include "KeyboardState.h"
#include "KeyboardHandler.h"
#ifdef _WIN32
    #include "MessageSender.h"
    #include "KeyboardHook.h"
    #include "TrayIcon.h"
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
#else
    #include "LinuxKeyboardGrab.h"
    #include <unistd.h>
#endif

std::atomic<bool> g_shutdown(false);

#ifdef _WIN32
DWORD g_mainThreadId = 0;
BOOL WINAPI consoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT || signal == CTRL_CLOSE_EVENT) {
        DEBUG_INFO("MAIN", "Received shutdown signal, initiating graceful shutdown...");
        g_shutdown = true;
        if (g_mainThreadId != 0) {
            PostThreadMessage(g_mainThreadId, WM_QUIT, 0, 0);
            PostThreadMessage(g_mainThreadId, WM_CONNECTION_LOST, 0, 0);
        }
        return TRUE;
    }
    return FALSE;
}
#endif

enum class Command { Connect, CreateConfig, AddProfile, Help };

struct CommandLineArgs {
    Command command = Command::Connect;
    std::string helpTopic;

    std::string host;
    int port = Config::DEFAULT_PORT;
    std::string key;
    std::string shortcut;
    std::string cycleShortcut;
    bool speechEnabled = true;
    bool audioEnabled = true;
    bool backgroundMode = false;
    bool noBackground = false;
    bool hasConnectionParams = false;
    bool portSet = false;
    Debug::Level debugLevel = Debug::LEVEL_WARNING;
    bool debugEnabled = false;

    std::string configPath;

    ProfileConfig newProfile;

    std::vector<std::string> errors;

    void addError(const std::string& e) { errors.push_back(e); }
    bool hasErrors() const { return !errors.empty(); }
    std::string errorString() const {
        std::string out;
        for (const auto& e : errors) out += "Error: " + e + "\n";
        out += "Use 'help' for usage information\n";
        return out;
    }
};

using OptHandler = std::function<bool(CommandLineArgs&, int&, int, char**)>;
using OptMap     = std::unordered_map<std::string, OptHandler>;

static bool requireNext(int& i, int argc, char** argv,
                         const std::string& flag, std::string& out) {
    if (i + 1 >= argc) { std::cerr << flag << " requires an argument\n"; return false; }
    out = argv[++i];
    return true;
}

static OptHandler stringOpt(std::string CommandLineArgs::* m, const std::string& flag,
                             std::function<Config::ValidationResult(const std::string&)> validator = nullptr,
                             bool marksConnection = false) {
    return [m, flag, validator, marksConnection](CommandLineArgs& a, int& i, int argc, char** argv) {
        std::string v;
        if (!requireNext(i, argc, argv, flag, v)) { a.addError(flag + " requires an argument"); return false; }
        if (validator) { auto r = validator(v); if (!r) { a.addError(r.errorMessage); return false; } }
        a.*m = std::move(v);
        if (marksConnection) a.hasConnectionParams = true;
        return true;
    };
}

static OptHandler profileStringOpt(std::string ProfileConfig::* m, const std::string& flag) {
    return [m, flag](CommandLineArgs& a, int& i, int argc, char** argv) {
        std::string v;
        if (!requireNext(i, argc, argv, flag, v)) { a.addError(flag + " requires an argument"); return false; }
        a.newProfile.*m = std::move(v);
        return true;
    };
}

static OptHandler portOpt(bool marksConnection) {
    return [marksConnection](CommandLineArgs& a, int& i, int argc, char** argv) {
        std::string v;
        if (!requireNext(i, argc, argv, "--port", v)) { a.addError("--port requires a number"); return false; }
        try {
            int port = std::stoi(v);
            auto r = Config::Validator::ValidatePort(port);
            if (!r) { a.addError(r.errorMessage); return false; }
            a.port = port;
            a.portSet = true;
            if (marksConnection) a.hasConnectionParams = true;
            return true;
        } catch (...) { a.addError("Invalid port: " + v); return false; }
    };
}

static OptHandler profilePortOpt() {
    return [](CommandLineArgs& a, int& i, int argc, char** argv) {
        std::string v;
        if (!requireNext(i, argc, argv, "--port", v)) { a.addError("--port requires a number"); return false; }
        try {
            int port = std::stoi(v);
            auto r = Config::Validator::ValidatePort(port);
            if (!r) { a.addError(r.errorMessage); return false; }
            a.newProfile.port = port;
            return true;
        } catch (...) { a.addError("Invalid port: " + v); return false; }
    };
}

static OptHandler boolOpt(bool CommandLineArgs::* m, bool val) {
    return [m, val](CommandLineArgs& a, int&, int, char**) { a.*m = val; return true; };
}

static OptHandler debugOpt(Debug::Level level) {
    return [level](CommandLineArgs& a, int&, int, char**) {
        a.debugEnabled = true; a.debugLevel = level; return true;
    };
}

static OptHandler profileBoolOpt(bool ProfileConfig::* m, bool val) {
    return [m, val](CommandLineArgs& a, int&, int, char**) { a.newProfile.*m = val; return true; };
}

static void parseOpts(CommandLineArgs& args, int& i, int argc, char** argv, const OptMap& opts) {
    for (; i < argc; ++i) {
        std::string arg(argv[i]);
        auto it = opts.find(arg);
        if (it != opts.end()) it->second(args, i, argc, argv);
        else args.addError("Unknown option: " + arg);
    }
}

static void parseCreateConfig(CommandLineArgs& args, int startIdx, int argc, char** argv) {
    args.command = Command::CreateConfig;
    OptMap opts = {
        {"-c",       stringOpt(&CommandLineArgs::configPath, "-c")},
        {"--config", stringOpt(&CommandLineArgs::configPath, "--config")},
    };
    int i = startIdx;
    parseOpts(args, i, argc, argv, opts);
}

static void parseAddProfile(CommandLineArgs& args, int startIdx, int argc, char** argv) {
    args.command = Command::AddProfile;
    OptMap opts = {
        {"-c",         stringOpt(&CommandLineArgs::configPath, "-c")},
        {"--config",   stringOpt(&CommandLineArgs::configPath, "--config")},
        {"-n",         profileStringOpt(&ProfileConfig::name,     "-n")},
        {"--name",     profileStringOpt(&ProfileConfig::name,     "--name")},
        {"-h",         profileStringOpt(&ProfileConfig::host,     "-h")},
        {"--host",     profileStringOpt(&ProfileConfig::host,     "--host")},
        {"-p",         profilePortOpt()},
        {"--port",     profilePortOpt()},
        {"-k",         profileStringOpt(&ProfileConfig::key,      "-k")},
        {"--key",      profileStringOpt(&ProfileConfig::key,      "--key")},
        {"-s",            profileStringOpt(&ProfileConfig::shortcut, "-s")},
        {"--shortcut",    profileStringOpt(&ProfileConfig::shortcut, "--shortcut")},
        {"--auto-connect",    profileBoolOpt(&ProfileConfig::autoConnect,       true)},
        {"--no-auto-connect", profileBoolOpt(&ProfileConfig::autoConnect,       false)},
        {"--speech",          profileBoolOpt(&ProfileConfig::speech,            true)},
        {"--no-speech",       profileBoolOpt(&ProfileConfig::speech,            false)},
        {"--mute",            profileBoolOpt(&ProfileConfig::muteOnLocalControl, true)},
        {"--no-mute",         profileBoolOpt(&ProfileConfig::muteOnLocalControl, false)},
        {"--sounds",          profileBoolOpt(&ProfileConfig::forwardAudio,      true)},
        {"--no-sounds",       profileBoolOpt(&ProfileConfig::forwardAudio,      false)},
    };
    int i = startIdx;
    parseOpts(args, i, argc, argv, opts);
}

static void parseConnect(CommandLineArgs& args, int startIdx, int argc, char** argv) {
    args.command = Command::Connect;
    OptMap opts = {
        {"-h",              stringOpt(&CommandLineArgs::host, "-h", Config::Validator::ValidateHost, true)},
        {"--host",          stringOpt(&CommandLineArgs::host, "--host", Config::Validator::ValidateHost, true)},
        {"-p",              portOpt(true)},
        {"--port",          portOpt(true)},
        {"-k",              stringOpt(&CommandLineArgs::key, "-k", Config::Validator::ValidateKey, true)},
        {"--key",           stringOpt(&CommandLineArgs::key, "--key", Config::Validator::ValidateKey, true)},
        {"-s",              stringOpt(&CommandLineArgs::shortcut, "-s", nullptr, true)},
        {"--shortcut",      stringOpt(&CommandLineArgs::shortcut, "--shortcut", nullptr, true)},
        {"--cycle-shortcut",stringOpt(&CommandLineArgs::cycleShortcut, "--cycle-shortcut")},
        {"-c",              stringOpt(&CommandLineArgs::configPath, "-c")},
        {"--config",        stringOpt(&CommandLineArgs::configPath, "--config")},
        {"-d",              debugOpt(Debug::LEVEL_INFO)},
        {"--debug",         debugOpt(Debug::LEVEL_INFO)},
        {"-v",              debugOpt(Debug::LEVEL_VERBOSE)},
        {"--verbose",       debugOpt(Debug::LEVEL_VERBOSE)},
        {"-t",              debugOpt(Debug::LEVEL_TRACE)},
        {"--trace",         debugOpt(Debug::LEVEL_TRACE)},
        {"--no-speech",     boolOpt(&CommandLineArgs::speechEnabled, false)},
        {"--no-audio",      boolOpt(&CommandLineArgs::audioEnabled,  false)},
        {"-b",              boolOpt(&CommandLineArgs::backgroundMode, true)},
        {"--background",    boolOpt(&CommandLineArgs::backgroundMode, true)},
        {"--no-background", boolOpt(&CommandLineArgs::noBackground,  true)},
    };
    int i = startIdx;
    parseOpts(args, i, argc, argv, opts);

    if (args.hasConnectionParams) {
        if (args.host.empty())
            args.addError("--host is required when using connection options");
        if (args.key.empty())
            args.addError("--key is required when using connection options");
    }
}

CommandLineArgs parseArguments(int argc, char* argv[]) {
    CommandLineArgs args;
    if (argc < 2) { parseConnect(args, 1, argc, argv); return args; }

    std::string first(argv[1]);
    if      (first == "connect")       parseConnect(args, 2, argc, argv);
    else if (first == "create-config") parseCreateConfig(args, 2, argc, argv);
    else if (first == "add-profile")   parseAddProfile(args, 2, argc, argv);
    else if (first == "help") {
        args.command = Command::Help;
        if (argc > 2) args.helpTopic = argv[2];
    }
    else                               parseConnect(args, 1, argc, argv);

    return args;
}

static void printHelpRun(const char* prog) {
    std::cout << "Usage: " << prog << " connect [OPTIONS]\n\n";
    std::cout << "Connection Options:\n";
    std::cout << "  -h, --host HOST           Server hostname or IP address\n";
    std::cout << "  -p, --port PORT           Server port (default: " << Config::DEFAULT_PORT << ")\n";
    std::cout << "  -k, --key KEY             Connection key/channel\n";
    std::cout << "  -s, --shortcut KEY        Per-profile toggle shortcut\n";
    std::cout << "      --cycle-shortcut KEY  Cycle shortcut (default: " << Config::DEFAULT_CYCLE_SHORTCUT << ")\n\n";
    std::cout << "Config:\n";
    std::cout << "  -c, --config PATH         Path to config file (default: auto-detect)\n\n";
    std::cout << "Output:\n";
    std::cout << "      --no-speech           Disable speech synthesis\n";
    std::cout << "      --no-audio            Disable audio feedback\n";
#ifdef _WIN32
    std::cout << "  -b, --background          Run without console window (system tray only)\n";
#endif
    std::cout << "\nDebug:\n";
    std::cout << "  -d, --debug               INFO level logging\n";
    std::cout << "  -v, --verbose             VERBOSE level logging\n";
    std::cout << "  -t, --trace               TRACE level logging\n\n";
    std::cout << "Config file locations (in order):\n";
    std::cout << "  1. Current directory\n";
#ifdef _WIN32
    std::cout << "  2. %APPDATA%\\NVDARemoteCompanion\\\n";
#else
    std::cout << "  2. ~/.config/NVDARemoteCompanion/\n";
#endif
    std::cout << "\nInteractive commands while running:\n";
    std::cout << "  status, connect, disconnect, add, edit, delete, quit\n";
}

static void printHelpCreateConfig(const char* prog) {
    std::cout << "Usage: " << prog << " create-config [-c PATH]\n\n";
    std::cout << "Creates a default config file with all options documented and exits.\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c, --config PATH  Where to write the file (default: nvdaremote.json)\n";
}

static void printHelpAddProfile(const char* prog) {
    std::cout << "Usage: " << prog << " add-profile [OPTIONS]\n\n";
    std::cout << "Adds a new profile to the config file and exits.\n";
    std::cout << "Any option not supplied is prompted interactively.\n\n";
    std::cout << "Options:\n";
    std::cout << "  -c, --config PATH         Config file to modify (default: auto-detect)\n";
    std::cout << "  -n, --name NAME           Profile display name (default: host)\n";
    std::cout << "  -h, --host HOST           Server hostname or IP address\n";
    std::cout << "  -p, --port PORT           Server port (default: " << Config::DEFAULT_PORT << ")\n";
    std::cout << "  -k, --key KEY             Connection key\n";
    std::cout << "  -s, --shortcut KEY        Per-profile toggle shortcut\n";
    std::cout << "      --auto-connect        Auto-connect on startup (default)\n";
    std::cout << "      --no-auto-connect     Disable auto-connect\n";
    std::cout << "      --speech              Forward speech (default)\n";
    std::cout << "      --no-speech           Disable speech forwarding\n";
    std::cout << "      --mute                Mute when not active\n";
    std::cout << "      --no-mute             Don't mute when not active (default)\n";
    std::cout << "      --sounds              Forward NVDA sounds (default)\n";
    std::cout << "      --no-sounds           Disable sound forwarding\n";
}

void printHelp(const char* prog, const std::string& topic = "") {
    std::cout << Config::APP_NAME << " - " << Config::APP_DESCRIPTION << "\n\n";
    if (topic == "connect")        { printHelpRun(prog);          return; }
    if (topic == "create-config")  { printHelpCreateConfig(prog); return; }
    if (topic == "add-profile")    { printHelpAddProfile(prog);   return; }

    std::cout << "Usage: " << prog << " [COMMAND] [OPTIONS]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  connect         Connect to profiles (default when no command given)\n";
    std::cout << "  create-config   Create a default config file and exit\n";
    std::cout << "  add-profile     Add a profile to the config and exit\n";
    std::cout << "  help [COMMAND]  Show help for a command\n\n";
    std::cout << "Run '" << prog << " help <command>' for command-specific options.\n\n";
    printHelpRun(prog);
}

static std::optional<ProfileConfig> ProfileFromLegacyConfig(const ConfigFileData& cfg) {
    if (!cfg.profiles.empty()) return std::nullopt;
    if (!cfg.host || cfg.host->empty() || !cfg.key || cfg.key->empty()) return std::nullopt;
    ProfileConfig p;
    p.name = "default";
    p.host = *cfg.host;
    p.port = cfg.port.value_or(Config::DEFAULT_PORT);
    p.key = *cfg.key;
    if (cfg.shortcut) p.shortcut = *cfg.shortcut;
    p.autoConnect = true;
    return p;
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        DEBUG_INFO("MAIN", "Received SIGINT, initiating graceful shutdown...");
        g_shutdown = true;
#ifdef _WIN32
        PostQuitMessage(0);
#endif
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);

#ifdef _WIN32
    g_mainThreadId = GetCurrentThreadId();
    SetConsoleCtrlHandler(consoleHandler, TRUE);

    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    std::locale::global(std::locale(""));
#endif

    auto args = parseArguments(argc, argv);

    if (args.hasErrors()) {
        std::cerr << args.errorString();
        return 1;
    }

    if (args.command == Command::Help) {
        printHelp(argv[0], args.helpTopic);
        return 0;
    }

    if (args.command == Command::CreateConfig) {
        std::string path = args.configPath.empty() ? "nvdaremote.json" : args.configPath;
        if (ConfigFile::CreateDefault(path)) {
            std::cout << "Config file created: " << path << std::endl;
            return 0;
        }
        std::cerr << "Error: Failed to create config file: " << path << std::endl;
        return 1;
    }

    if (args.command == Command::AddProfile) {
        std::string configPath = ConfigFile::FindConfigFile(args.configPath);
        if (configPath.empty()) {
            configPath = args.configPath.empty() ? ConfigFile::DefaultPath() : args.configPath;
            ConfigFile::CreateDefault(configPath);
        }
        ConfigFile::Migrate(configPath);
        ConfigFileData cfg = ConfigFile::Load(configPath);
        return CommandHandler::AddProfileInteractive(configPath, cfg, args.newProfile) ? 0 : 1;
    }

    ConfigFileData cfg;
    std::string configPath = ConfigFile::FindConfigFile(args.configPath);
    if (!configPath.empty()) {
        ConfigFile::Migrate(configPath);
        cfg = ConfigFile::Load(configPath);
        if (cfg.background.has_value() && *cfg.background && !args.backgroundMode && !args.noBackground) {
            args.backgroundMode = true;
        }
        if (cfg.debugLevel && !args.debugEnabled) {
            static const std::unordered_map<std::string, Debug::Level> debugLevelMap = {
                {"info",    Debug::LEVEL_INFO},
                {"verbose", Debug::LEVEL_VERBOSE},
                {"trace",   Debug::LEVEL_TRACE},
            };
            auto it = debugLevelMap.find(*cfg.debugLevel);
            if (it != debugLevelMap.end()) {
                args.debugEnabled = true;
                args.debugLevel = it->second;
            }
        }
    } else if (!args.configPath.empty()) {
        std::cerr << "Warning: Config file not found: " << args.configPath << std::endl;
    }

    if (auto legacy = ProfileFromLegacyConfig(cfg)) {
        cfg.profiles.push_back(std::move(*legacy));
    }


#ifdef _WIN32
    if (args.backgroundMode) {
        bool hasValidProfile = false;
        for (const auto& p : cfg.profiles) {
            if (!p.host.empty() && !p.key.empty()) {
                hasValidProfile = true;
                break;
            }
        }
        if (!hasValidProfile) {
            std::cerr << "Error: Background mode requires at least one profile with host and key" << std::endl;
            return 1;
        }
        std::string cmdLine;
        char exePath[MAX_PATH];
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        cmdLine = "\"" + std::string(exePath) + "\"";
        cmdLine += " --no-background";
        if (!args.configPath.empty()) {
            cmdLine += " --config \"" + args.configPath + "\"";
        }
        if (args.hasConnectionParams) {
            cmdLine += " --host \"" + args.host + "\"";
            cmdLine += " --port " + std::to_string(args.port);
            cmdLine += " --key \"" + args.key + "\"";
            if (!args.shortcut.empty()) {
                cmdLine += " --shortcut \"" + args.shortcut + "\"";
            }
        }
        if (!args.cycleShortcut.empty()) {
            cmdLine += " --cycle-shortcut \"" + args.cycleShortcut + "\"";
        }
        if (!args.speechEnabled) {
            cmdLine += " --no-speech";
        }
        if (args.debugEnabled) {
            if (args.debugLevel == Debug::LEVEL_TRACE) cmdLine += " --trace";
            else if (args.debugLevel == Debug::LEVEL_VERBOSE) cmdLine += " --verbose";
            else cmdLine += " --debug";
        }

        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi = {};
        if (CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, FALSE,
                           DETACHED_PROCESS,
                           nullptr, nullptr, &si, &pi)) {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            std::cout << "Running in background (PID " << pi.dwProcessId << ")" << std::endl;
            return 0;
        } else {
            std::cerr << "Error: Failed to start background process" << std::endl;
            return 1;
        }
    }
#else
    if (args.backgroundMode) {
        std::cerr << "Warning: Background mode is only supported on Windows, ignoring" << std::endl;
        args.backgroundMode = false;
    }
#endif

    Debug::SetEnabled(args.debugEnabled);
    Debug::SetLevel(args.debugLevel);

    if (args.debugEnabled) {
        DEBUG_INFO("MAIN", "Debug system initialized");
        DEBUG_INFO_F("MAIN", "Debug level set to: {}", static_cast<int>(args.debugLevel));
        DEBUG_INFO_F("MAIN", "Number of profiles: {}", cfg.profiles.size());
    }

    if (cfg.audio.has_value() && !*cfg.audio) args.audioEnabled = false;
    Audio::SetEnabled(args.audioEnabled);

    Speech::SetEnabled(args.speechEnabled);
    if (args.speechEnabled) {
        if (!Speech::Initialize()) {
            DEBUG_WARN("MAIN", "Failed to initialize speech system - continuing without speech");
            Speech::SetEnabled(false);
        } else {
            DEBUG_INFO("MAIN", "Speech system initialized successfully");
        }
    } else {
        DEBUG_INFO("MAIN", "Speech system disabled by command line option");
    }

    CommandHandler cmdHandler(configPath, cfg);

    {
        std::string cycleSc = args.cycleShortcut;
        if (cycleSc.empty() && cfg.cycleShortcut) cycleSc = *cfg.cycleShortcut;
        if (cycleSc.empty()) cycleSc = Config::DEFAULT_CYCLE_SHORTCUT;
        KeyboardState::SetCycleShortcut(cycleSc);

        if (cfg.exitShortcut && !cfg.exitShortcut->empty())
            KeyboardState::SetExitShortcut(*cfg.exitShortcut);
        if (cfg.localShortcut && !cfg.localShortcut->empty())
            KeyboardState::SetLocalShortcut(*cfg.localShortcut);
        if (cfg.reconnectShortcut && !cfg.reconnectShortcut->empty())
            KeyboardState::SetReconnectShortcut(*cfg.reconnectShortcut);
        if (cfg.clipboardShortcut && !cfg.clipboardShortcut->empty())
            KeyboardState::SetClipboardShortcut(*cfg.clipboardShortcut);
#ifdef _WIN32
        if (cfg.reinstallHookShortcut && !cfg.reinstallHookShortcut->empty())
            KeyboardState::SetReinstallHookShortcut(*cfg.reinstallHookShortcut);
#endif
    }

#ifdef _WIN32
    auto keyboard = std::make_unique<KeyboardHook>();
#else
    auto keyboard = std::make_unique<LinuxKeyboardGrab>();
#endif

    cmdHandler.SetDisconnectCallback([&keyboard]() {
        DEBUG_INFO("MAIN", "Connection lost callback triggered");
        keyboard->NotifyConnectionLost();
    });

    cmdHandler.SetReconnectCallback([&cmdHandler]() {
        DEBUG_INFO("MAIN", "Auto-reconnect succeeded");
#ifdef _WIN32
        std::string tooltip = std::string(Config::APP_NAME) + " - " +
                              std::to_string(cmdHandler.CountConnectedSessions()) + "/" +
                              std::to_string(cmdHandler.GetSessionCount()) + " connected";
        TrayIcon::SetTooltip(tooltip);
#endif
    });

    keyboard->SetReconnectCallback([&cmdHandler]() {
        DEBUG_INFO("MAIN", "Reconnect all shortcut triggered");
        cmdHandler.ReconnectAll();
    });

    if (args.hasConnectionParams) {
        ProfileConfig p;
        p.host = args.host;
        p.port = args.port;
        p.key = args.key;
        p.shortcut = args.shortcut;
        p.speech = args.speechEnabled;
        if (!cmdHandler.ConnectFromParams(p)) {
            return 1;
        }
    } else if (cfg.profiles.empty()) {
        if (!cmdHandler.ConnectInteractive()) {
            return 1;
        }
    } else {
        int connected = cmdHandler.ConnectAutoProfiles();
        std::cout << connected << " of " << cfg.profiles.size() << " profile(s) connected" << std::endl;
    }

    std::cout << "Type 'help' for interactive commands." << std::endl;

#ifdef _WIN32
    bool isTrayMode = (GetConsoleWindow() == nullptr);

    auto updateTrayTooltip = [&cmdHandler]() {
        std::string tooltip = std::string(Config::APP_NAME) + " - " +
                              std::to_string(cmdHandler.CountConnectedSessions()) + "/" +
                              std::to_string(cmdHandler.GetSessionCount()) + " connected";
        TrayIcon::SetTooltip(tooltip);
    };

    if (isTrayMode) {
        if (!TrayIcon::Create()) {
            DEBUG_ERROR("MAIN", "Failed to create tray icon");
            return 1;
        }
        TrayIcon::SetProfileProvider([&cmdHandler]() {
            std::vector<TrayProfile> profiles;
            for (const auto& s : cmdHandler.GetSessions())
                profiles.push_back({s.config.name, s.connection && s.connection->IsConnected()});
            return profiles;
        });
        TrayIcon::SetProfileToggleCallback([&cmdHandler, &updateTrayTooltip](int index) {
            cmdHandler.ToggleProfile(index);
            updateTrayTooltip();
        });
        TrayIcon::SetReconnectAllCallback([&cmdHandler, &updateTrayTooltip]() {
            cmdHandler.ReconnectAll();
            updateTrayTooltip();
        });
        updateTrayTooltip();
    }
#endif

    std::thread cmdThread;
#ifdef _WIN32
    if (!isTrayMode)
#endif
    cmdThread = std::thread([&cmdHandler]() { cmdHandler.RunCommandLoop(); });

    while (!g_shutdown) {
        cmdHandler.UpdateNetworkClients();

        if (!keyboard->Install()) {
            DEBUG_ERROR("MAIN", "Failed to install keyboard handler");
#ifdef _WIN32
            if (isTrayMode) TrayIcon::Destroy();
#endif
            g_shutdown = true;
            break;
        }

#ifdef _WIN32
        if (isTrayMode)
            TrayIcon::RunMessageLoop();
        else
#endif
            keyboard->RunMessageLoop();

        keyboard->Uninstall();

        if (g_shutdown) break;

#ifdef _WIN32
        if (isTrayMode)
            TrayIcon::SetTooltip(std::string(Config::APP_NAME) + " - Reconnecting...");
#endif

        DEBUG_INFO("MAIN", "Checking connections...");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        cmdHandler.ReconnectAll();

        if (cmdHandler.CountConnectedSessions() == 0 && !g_shutdown) {
            DEBUG_INFO("MAIN", "No connections active. Retrying in 5 seconds...");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

#ifdef _WIN32
        if (isTrayMode) updateTrayTooltip();
#endif
    }

    if (cmdThread.joinable()) cmdThread.join();

#ifdef _WIN32
    if (isTrayMode) TrayIcon::Destroy();
#endif

    DEBUG_VERBOSE("MAIN", "Cleaning up speech system");
    Speech::Cleanup();
    DEBUG_VERBOSE("MAIN", "Speech system cleanup completed");

    DEBUG_INFO("MAIN", "Application shutdown completed successfully");
    return 0;
}
