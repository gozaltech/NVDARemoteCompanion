#include <iostream>
#include <locale>
#include <cstdlib>
#include <csignal>
#include <atomic>
#include <map>
#include <vector>
#include <functional>
#include "ConnectionManager.h"
#include "CommandHandler.h"
#include "ConfigFile.h"
#include "Debug.h"
#include "Speech.h"
#include "Config.h"

#ifdef _WIN32
    #include "MessageSender.h"
    #include "KeyboardHook.h"
    #include "KeyboardState.h"
    #include "TrayIcon.h"
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
#else
    #include <unistd.h>
    #include <sys/select.h>
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

    void printErrors() const {
        for (const auto& error : errors) {
            std::cerr << "Error: " << error << std::endl;
        }
        std::cerr << "Use --help for usage information" << std::endl;
    }
};

namespace ArgHandlers {
    template<typename T>
    auto createStringHandler(T CommandLineArgs::* member, const std::string& errorMsg,
                           std::function<Config::ValidationResult(const std::string&)> validator = nullptr) {
        return [member, errorMsg, validator](CommandLineArgs& args, int& i, int argc, char** argv) -> bool {
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
            args.hasConnectionParams = true;
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
        "--host requires a hostname or IP address", Config::Validator::ValidateHost);
    auto portHandler = ArgHandlers::createPortHandler();
    auto keyHandler = ArgHandlers::createStringHandler(&CommandLineArgs::key,
        "--key requires a connection key", Config::Validator::ValidateKey);
    auto shortcutHandler = ArgHandlers::createStringHandler(&CommandLineArgs::shortcut,
        "--shortcut requires a key combination (e.g., ctrl+win+f11)");

    auto debugHandler = ArgHandlers::createDebugHandler(Debug::LEVEL_INFO);
    auto verboseHandler = ArgHandlers::createDebugHandler(Debug::LEVEL_VERBOSE);
    auto traceHandler = ArgHandlers::createDebugHandler(Debug::LEVEL_TRACE);

    auto noSpeechHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::speechEnabled, false);
    auto helpHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::showHelp, true);

    auto configHandler = [](CommandLineArgs& args, int& i, int argc, char** argv) -> bool {
        if (i + 1 >= argc) {
            args.addError("--config requires a file path");
            return false;
        }
        args.configPath = argv[++i];
        return true;
    };
    auto cycleShortcutHandler = [](CommandLineArgs& args, int& i, int argc, char** argv) -> bool {
        if (i + 1 >= argc) {
            args.addError("--cycle-shortcut requires a key combination");
            return false;
        }
        args.cycleShortcut = argv[++i];
        return true;
    };
    auto createConfigHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::createConfig, true);
    auto backgroundHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::backgroundMode, true);
    auto noBackgroundHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::noBackground, true);

    std::map<std::string, std::function<bool(CommandLineArgs&, int&, int, char**)>> argHandlers = {
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
#ifdef _WIN32
    std::cout << "  - Windows version includes keyboard forwarding\n";
    std::cout << "  - Each profile can have its own toggle shortcut\n";
#else
    std::cout << "  - Linux version runs in receive-only mode (no keyboard forwarding)\n";
#endif
    std::cout << std::endl;
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
        args.printErrors();
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
        cfg = ConfigFile::Load(configPath);
        if (cfg.background.has_value() && *cfg.background && !args.backgroundMode && !args.noBackground) {
            args.backgroundMode = true;
        }
        if (cfg.debugLevel && !args.debugEnabled) {
            std::string level = *cfg.debugLevel;
            if (level == "info") { args.debugEnabled = true; args.debugLevel = Debug::LEVEL_INFO; }
            else if (level == "verbose") { args.debugEnabled = true; args.debugLevel = Debug::LEVEL_VERBOSE; }
            else if (level == "trace") { args.debugEnabled = true; args.debugLevel = Debug::LEVEL_TRACE; }
        }
    } else if (!args.configPath.empty()) {
        std::cerr << "Warning: Config file not found: " << args.configPath << std::endl;
    }

    if (cfg.profiles.empty() && cfg.host && !cfg.host->empty() && cfg.key && !cfg.key->empty()) {
        ProfileConfig p;
        p.name = "default";
        p.host = *cfg.host;
        p.port = cfg.port.value_or(Config::DEFAULT_PORT);
        p.key = *cfg.key;
        if (cfg.shortcut) p.shortcut = *cfg.shortcut;
        p.autoConnect = true;
        cfg.profiles.push_back(std::move(p));
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

#ifdef _WIN32
    {
        std::string cycleSc = args.cycleShortcut;
        if (cycleSc.empty() && cfg.cycleShortcut) cycleSc = *cfg.cycleShortcut;
        if (cycleSc.empty()) cycleSc = "ctrl+alt+f11";
        KeyboardState::SetCycleShortcut(cycleSc);

        if (cfg.exitShortcut && !cfg.exitShortcut->empty())
            KeyboardState::SetExitShortcut(*cfg.exitShortcut);
        if (cfg.reinstallHookShortcut && !cfg.reinstallHookShortcut->empty())
            KeyboardState::SetReinstallHookShortcut(*cfg.reinstallHookShortcut);
    }

    DWORD mainThreadId = GetCurrentThreadId();
    cmdHandler.SetDisconnectCallbackFactory([mainThreadId](ConnectionManager& cm) {
        cm.SetDisconnectCallback([mainThreadId]() {
            DEBUG_INFO("MAIN", "Connection lost callback triggered");
            PostThreadMessage(mainThreadId, WM_CONNECTION_LOST, 0, 0);
        });
    });
#endif

    if (cfg.profiles.empty()) {
        auto cm = std::make_unique<ConnectionManager>();
        cm->SetSpeechEnabled(true);
#ifdef _WIN32
        cm->SetDisconnectCallback([mainThreadId]() {
            PostThreadMessage(mainThreadId, WM_CONNECTION_LOST, 0, 0);
        });
#endif
        if (!cm->EstablishConnection()) {
            return 1;
        }
#ifdef _WIN32
        std::string interactiveShortcut = cm->GetShortcut();
        if (!interactiveShortcut.empty()) {
            KeyboardState::SetToggleShortcutAt(0, interactiveShortcut);
        }
        MessageSender::SetNetworkClient(0, cm->GetClient());
#endif
        ProfileSession session;
        session.config.name = "interactive";
        session.config.host = "";
        session.connection = std::move(cm);
        session.shortcutIndex = 0;
        cmdHandler.GetSessions().push_back(std::move(session));
    } else {
        int connected = cmdHandler.ConnectAutoProfiles();
        std::cout << connected << " of " << cfg.profiles.size() << " profile(s) connected" << std::endl;
    }

    std::cout << "Type 'help' for interactive commands." << std::endl;

#ifdef _WIN32
    bool isTrayMode = (GetConsoleWindow() == nullptr);
    if (isTrayMode) {
        if (!TrayIcon::Create()) {
            DEBUG_ERROR("MAIN", "Failed to create tray icon");
            return 1;
        }
        auto& sessions = cmdHandler.GetSessions();
        int connCount = 0;
        for (const auto& s : sessions) {
            if (s.connection && s.connection->IsConnected()) connCount++;
        }
        std::string tooltip = std::string(Config::APP_NAME) + " - " +
                              std::to_string(connCount) + "/" +
                              std::to_string(sessions.size()) + " connected";
        TrayIcon::SetTooltip(tooltip);
    }

    std::thread cmdThread;
    if (!isTrayMode) {
        cmdThread = std::thread([&cmdHandler]() {
            cmdHandler.RunCommandLoop();
        });
    }

    while (!g_shutdown) {
        cmdHandler.UpdateNetworkClients();

        if (!KeyboardHook::Install()) {
            if (isTrayMode) TrayIcon::Destroy();
            g_shutdown = true;
            break;
        }

        if (isTrayMode) {
            TrayIcon::RunMessageLoop();
        } else {
            KeyboardHook::RunMessageLoop();
        }
        DEBUG_INFO("MAIN", "Message loop ended");

        DEBUG_VERBOSE("MAIN", "Uninstalling keyboard hook");
        KeyboardHook::Uninstall();
        DEBUG_VERBOSE("MAIN", "Keyboard hook uninstalled");

        if (g_shutdown) break;

        if (isTrayMode) {
            TrayIcon::SetTooltip(std::string(Config::APP_NAME) + " - Reconnecting...");
        }

        DEBUG_INFO("MAIN", "Checking connections...");
        std::this_thread::sleep_for(std::chrono::seconds(2));

        cmdHandler.ReconnectAll();

        bool anyConnected = false;
        for (const auto& s : cmdHandler.GetSessions()) {
            if (s.connection && s.connection->IsConnected()) { anyConnected = true; break; }
        }

        if (!anyConnected && !g_shutdown) {
            DEBUG_INFO("MAIN", "No connections active. Retrying in 5 seconds...");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        if (isTrayMode) {
            int connCount = 0;
            for (const auto& s : cmdHandler.GetSessions()) {
                if (s.connection && s.connection->IsConnected()) connCount++;
            }
            std::string tooltip = std::string(Config::APP_NAME) + " - " +
                                  std::to_string(connCount) + "/" +
                                  std::to_string(cmdHandler.GetSessions().size()) + " connected";
            TrayIcon::SetTooltip(tooltip);
        }
    }

    if (!isTrayMode && cmdThread.joinable()) {
        cmdThread.join();
    }

    if (isTrayMode) {
        TrayIcon::Destroy();
    }
#else
    DEBUG_INFO("MAIN", "Starting input loop (Linux mode - no keyboard hook)");

    std::thread cmdThread([&cmdHandler]() {
        cmdHandler.RunCommandLoop();
    });

    while (!g_shutdown) {
        bool allConnected = true;
        for (const auto& s : cmdHandler.GetSessions()) {
            if (s.connection && !s.connection->IsConnected()) { allConnected = false; break; }
        }
        if (allConnected) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (g_shutdown) break;

        DEBUG_INFO("MAIN", "Connection lost. Reconnecting in 2 seconds...");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        cmdHandler.ReconnectAll();
    }

    if (cmdThread.joinable()) {
        cmdThread.join();
    }

    DEBUG_INFO("MAIN", "Input loop ended, starting cleanup");
#endif

    DEBUG_VERBOSE("MAIN", "Cleaning up speech system");
    Speech::Cleanup();
    DEBUG_VERBOSE("MAIN", "Speech system cleanup completed");

    DEBUG_INFO("MAIN", "Application shutdown completed successfully");
    return 0;
}
