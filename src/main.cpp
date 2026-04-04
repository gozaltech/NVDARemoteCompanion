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

struct CommandLineArgs {
    std::string host;
    int port = Config::DEFAULT_PORT;
    std::string key;
    std::string shortcut;
    std::string cycleShortcut;
    std::string configPath;
    Debug::Level debugLevel = Debug::LEVEL_WARNING;
    bool debugEnabled = false;
    bool speechEnabled = true;
    bool audioEnabled = true;
    bool showHelp = false;
    bool createConfig = false;
    bool backgroundMode = false;
    bool noBackground = false;
    bool hasConnectionParams = false;
    bool portSet = false;

    std::vector<std::string> errors;

    void addError(const std::string& error) {
        errors.push_back(error);
    }

    bool hasErrors() const {
        return !errors.empty();
    }

    std::string errorString() const {
        std::string out;
        for (const auto& e : errors) out += "Error: " + e + "\n";
        out += "Use --help for usage information\n";
        return out;
    }
};

namespace ArgHandlers {
    template<typename T>
    auto createStringHandler(T CommandLineArgs::* member, const std::string& errorMsg,
                           std::function<Config::ValidationResult(const std::string&)> validator = nullptr,
                           bool marksConnection = false) {
        return [member, errorMsg, validator, marksConnection](CommandLineArgs& args, int& i, int argc, char** argv) -> bool {
            if (i + 1 >= argc) {
                args.addError(errorMsg);
                return false;
            }
            std::string value = argv[++i];
            if (validator) {
                auto result = validator(value);
                if (!result) {
                    args.addError(result.errorMessage);
                    return false;
                }
            }
            args.*member = std::move(value);
            if (marksConnection) args.hasConnectionParams = true;
            return true;
        };
    }

    auto createPortHandler() {
        return [](CommandLineArgs& args, int& i, int argc, char** argv) -> bool {
            if (i + 1 >= argc) {
                args.addError("--port requires a port number");
                return false;
            }
            try {
                int port = std::stoi(argv[++i]);
                auto result = Config::Validator::ValidatePort(port);
                if (!result) {
                    args.addError(result.errorMessage);
                    return false;
                }
                args.port = port;
                args.portSet = true;
                args.hasConnectionParams = true;
                return true;
            } catch (const std::exception&) {
                args.addError("Invalid port number: " + std::string(argv[i]));
                return false;
            }
        };
    }

    template<typename T>
    auto createFlagHandler(T CommandLineArgs::* member, T value) {
        return [member, value](CommandLineArgs& args, int&, int, char**) -> bool {
            args.*member = value;
            return true;
        };
    }

    auto createDebugHandler(Debug::Level level) {
        return [level](CommandLineArgs& args, int&, int, char**) -> bool {
            args.debugEnabled = true;
            args.debugLevel = level;
            return true;
        };
    }
}

CommandLineArgs parseArguments(int argc, char* argv[]) {
    CommandLineArgs args;

    auto hostHandler = ArgHandlers::createStringHandler(&CommandLineArgs::host,
        "--host requires a hostname or IP address", Config::Validator::ValidateHost, true);
    auto portHandler = ArgHandlers::createPortHandler();
    auto keyHandler = ArgHandlers::createStringHandler(&CommandLineArgs::key,
        "--key requires a connection key", Config::Validator::ValidateKey, true);
    auto shortcutHandler = ArgHandlers::createStringHandler(&CommandLineArgs::shortcut,
        "--shortcut requires a key combination (e.g., ctrl+win+f11)", nullptr, true);

    auto debugHandler = ArgHandlers::createDebugHandler(Debug::LEVEL_INFO);
    auto verboseHandler = ArgHandlers::createDebugHandler(Debug::LEVEL_VERBOSE);
    auto traceHandler = ArgHandlers::createDebugHandler(Debug::LEVEL_TRACE);

    auto noSpeechHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::speechEnabled, false);
    auto noAudioHandler  = ArgHandlers::createFlagHandler(&CommandLineArgs::audioEnabled,  false);
    auto helpHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::showHelp, true);

    auto configHandler = ArgHandlers::createStringHandler(&CommandLineArgs::configPath,
        "--config requires a file path");
    auto cycleShortcutHandler = ArgHandlers::createStringHandler(&CommandLineArgs::cycleShortcut,
        "--cycle-shortcut requires a key combination");
    auto createConfigHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::createConfig, true);
    auto backgroundHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::backgroundMode, true);
    auto noBackgroundHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::noBackground, true);

    std::unordered_map<std::string, std::function<bool(CommandLineArgs&, int&, int, char**)>> argHandlers = {
        {"-h", hostHandler},
        {"--host", hostHandler},
        {"-p", portHandler},
        {"--port", portHandler},
        {"-k", keyHandler},
        {"--key", keyHandler},
        {"-s", shortcutHandler},
        {"--shortcut", shortcutHandler},
        {"--cycle-shortcut", cycleShortcutHandler},
        {"-c", configHandler},
        {"--config", configHandler},
        {"--create-config", createConfigHandler},
        {"-b", backgroundHandler},
        {"--background", backgroundHandler},
        {"--no-background", noBackgroundHandler},
        {"-d", debugHandler},
        {"--debug", debugHandler},
        {"-v", verboseHandler},
        {"--verbose", verboseHandler},
        {"-t", traceHandler},
        {"--trace", traceHandler},
        {"--no-speech", noSpeechHandler},
        {"--no-audio",  noAudioHandler},
        {"--help", helpHandler}
    };

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);

        auto it = argHandlers.find(arg);
        if (it != argHandlers.end()) {
            if (!it->second(args, i, argc, argv)) {
            }
        } else {
            args.addError("Unknown argument: " + arg);
        }
    }

    if (args.hasConnectionParams) {
        if (args.host.empty()) {
            args.addError("Host is required when using command line connection options");
        } else {
            auto hostResult = Config::Validator::ValidateHost(args.host);
            if (!hostResult) {
                args.addError(hostResult.errorMessage);
            }
        }

        if (args.key.empty()) {
            args.addError("Connection key is required when using command line connection options");
        }
    }

    return args;
}

void printHelp(const char* programName) {
    std::cout << Config::APP_NAME << " - " << Config::APP_DESCRIPTION << "\n\n";
    std::cout << "Usage: " << programName << " [OPTIONS]\n\n";
    std::cout << "Connection Options:\n";
    std::cout << "  -h, --host HOST       Server hostname or IP address\n";
    std::cout << "  -p, --port PORT       Server port (default: " << Config::DEFAULT_PORT << ")\n";
    std::cout << "  -k, --key KEY         Connection key/channel\n";
    std::cout << "  -s, --shortcut KEY    Set per-profile toggle shortcut (default: none)\n";
    std::cout << "      --cycle-shortcut KEY  Set cycle shortcut (default: ctrl+alt+f11)\n\n";
    std::cout << "Debug Options:\n";
    std::cout << "  -d, --debug           Enable debug logging (INFO level)\n";
    std::cout << "  -v, --verbose         Enable verbose debug logging\n";
    std::cout << "  -t, --trace           Enable trace debug logging (most detailed)\n\n";
    std::cout << "Config File Options:\n";
    std::cout << "  -c, --config PATH     Path to config file (default: auto-detect)\n";
    std::cout << "      --create-config   Create a default config file and exit\n\n";
    std::cout << "Other Options:\n";
    std::cout << "      --no-speech       Disable speech synthesis\n";
    std::cout << "      --no-audio        Disable companion audio feedback (toggle tones, connect/disconnect sounds)\n";
#ifdef _WIN32
    std::cout << "  -b, --background      Run without console window (system tray only)\n";
#endif
    std::cout << "      --help            Show this help message\n\n";
    std::cout << "Config File:\n";
    std::cout << "  The app looks for nvdaremote.json in:\n";
    std::cout << "    1. Current directory\n";
#ifdef _WIN32
    std::cout << "    2. %APPDATA%\\NVDARemoteCompanion\\\n";
#else
    std::cout << "    2. ~/.config/NVDARemoteCompanion/\n";
#endif
    std::cout << "  Command-line arguments override config file values.\n";
    std::cout << "  Use \"profiles\" array in config for multiple connections.\n\n";
    std::cout << "Interactive Commands (while running):\n";
    std::cout << "  status, list, connect, disconnect, add, edit, delete, help, quit\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " -h example.com -k mykey\n";
    std::cout << "  " << programName << " --host 192.168.1.100 --port " << Config::DEFAULT_PORT << " --key shared_session\n";
    std::cout << "  " << programName << " --config myconfig.json\n";
    std::cout << "  " << programName << " --background\n\n";
    std::cout << "Notes:\n";
    std::cout << "  - Host must be a valid hostname or IP address (max " << Config::MAX_HOST_LENGTH << " chars)\n";
    std::cout << "  - Port must be in range " << Config::MIN_PORT << "-" << Config::MAX_PORT << "\n";
    std::cout << "  - Connection key must not exceed " << Config::MAX_KEY_LENGTH << " characters\n";
    std::cout << "  - Keyboard forwarding requires read access to /dev/input/event* and /dev/uinput\n";
#ifdef _WIN32
    std::cout << "  - Each profile can have its own toggle shortcut\n";
#endif
    std::cout << std::endl;
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

    if (args.showHelp) {
        printHelp(argv[0]);
        return 0;
    }

    if (args.hasErrors()) {
        std::cerr << args.errorString();
        return 1;
    }

    if (args.createConfig) {
        std::string path = args.configPath.empty() ? "nvdaremote.json" : args.configPath;
        if (ConfigFile::CreateDefault(path)) {
            std::cout << "Default config file created: " << path << std::endl;
            return 0;
        } else {
            std::cerr << "Error: Failed to create config file: " << path << std::endl;
            return 1;
        }
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

    if (args.hasConnectionParams) {
        ProfileConfig p;
        p.name = "cli";
        p.host = args.host;
        p.port = args.port;
        p.key = args.key;
        p.shortcut = args.shortcut;
        p.autoConnect = true;
        p.speech = args.speechEnabled;
        cfg.profiles.clear();
        cfg.profiles.push_back(std::move(p));
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

    if (cfg.profiles.empty()) {
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
