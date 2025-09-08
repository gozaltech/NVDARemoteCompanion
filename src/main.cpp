#include <iostream>
#include <locale>
#include <cstdlib>
#include <csignal>
#include <atomic>
#include <map>
#include <vector>
#include <functional>
#include "ConnectionManager.h"
#include "Debug.h"
#include "Speech.h"
#include "Config.h"

#ifdef _WIN32
    #include "MessageSender.h"
    #include "KeyboardHook.h"
    #include <windows.h>
    #include <io.h>
    #include <fcntl.h>
#else
    #include <unistd.h>
    #include <sys/select.h>
#endif

std::atomic<bool> g_shutdown(false);

struct CommandLineArgs {
    std::string host;
    int port = Config::DEFAULT_PORT;
    std::string key;
    Debug::Level debugLevel = Debug::LEVEL_WARNING;
    bool debugEnabled = false;
    bool speechEnabled = true;
    bool showHelp = false;
    bool hasConnectionParams = false;
    
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
    
    auto debugHandler = ArgHandlers::createDebugHandler(Debug::LEVEL_INFO);
    auto verboseHandler = ArgHandlers::createDebugHandler(Debug::LEVEL_VERBOSE);
    auto traceHandler = ArgHandlers::createDebugHandler(Debug::LEVEL_TRACE);
    
    auto noSpeechHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::speechEnabled, false);
    auto helpHandler = ArgHandlers::createFlagHandler(&CommandLineArgs::showHelp, true);
    
    std::map<std::string, std::function<bool(CommandLineArgs&, int&, int, char**)>> argHandlers = {
        {"-h", hostHandler},
        {"--host", hostHandler},
        {"-p", portHandler},
        {"--port", portHandler},
        {"-k", keyHandler},
        {"--key", keyHandler},
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
    std::cout << "  -k, --key KEY         Connection key/channel\n\n";
    std::cout << "Debug Options:\n";
    std::cout << "  -d, --debug           Enable debug logging (INFO level)\n";
    std::cout << "  -v, --verbose         Enable verbose debug logging\n";
    std::cout << "  -t, --trace           Enable trace debug logging (most detailed)\n\n";
    std::cout << "Other Options:\n";
    std::cout << "      --no-speech       Disable speech synthesis\n";
    std::cout << "      --help            Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " -h example.com -k mykey\n";
    std::cout << "  " << programName << " --host 192.168.1.100 --port " << Config::DEFAULT_PORT << " --key shared_session\n";
    std::cout << "  " << programName << " --verbose --no-speech\n\n";
    std::cout << "Notes:\n";
    std::cout << "  - Host must be a valid hostname or IP address (max " << Config::MAX_HOST_LENGTH << " chars)\n";
    std::cout << "  - Port must be in range " << Config::MIN_PORT << "-" << Config::MAX_PORT << "\n";
    std::cout << "  - Connection key must not exceed " << Config::MAX_KEY_LENGTH << " characters\n";
#ifdef _WIN32
    std::cout << "  - Windows version includes keyboard forwarding\n";
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
    
    Debug::SetEnabled(args.debugEnabled);
    Debug::SetLevel(args.debugLevel);
    
    if (args.debugEnabled) {
        DEBUG_INFO("MAIN", "Debug system initialized");
        DEBUG_INFO_F("MAIN", "Debug level set to: {}", static_cast<int>(args.debugLevel));
        if (args.hasConnectionParams) {
            DEBUG_INFO_F("MAIN", "Command line connection: {}:{} key={}", args.host, args.port, args.key);
        }
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
    
    ConnectionManager connectionManager;
    bool connectionSuccess = false;
    
    if (args.hasConnectionParams) {
        connectionSuccess = connectionManager.EstablishConnection(args.host, args.port, args.key);
    } else {
        connectionSuccess = connectionManager.EstablishConnection();
    }
    
    if (!connectionSuccess) {
        return 1;
    }
    
#ifdef _WIN32
    MessageSender::SetNetworkClient(connectionManager.GetClient());
    if (!KeyboardHook::Install()) {
        return 1;
    }
    
    KeyboardHook::RunMessageLoop();
    DEBUG_INFO("MAIN", "Message loop ended, starting cleanup");
    
    DEBUG_VERBOSE("MAIN", "Uninstalling keyboard hook");
    KeyboardHook::Uninstall();
    DEBUG_VERBOSE("MAIN", "Keyboard hook uninstalled");
#else
    DEBUG_INFO("MAIN", "Starting input loop (Linux mode - no keyboard hook)");
    std::cout << "NVDA Remote Client running. Press Enter to quit..." << std::endl;
    
    std::thread inputThread([&]() {
        std::string dummy;
        std::getline(std::cin, dummy);
        g_shutdown = true;
    });
    
    while (!g_shutdown) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (inputThread.joinable()) {
        inputThread.join();
    }
    
    DEBUG_INFO("MAIN", "Input loop ended, starting cleanup");
#endif
    
    DEBUG_VERBOSE("MAIN", "Cleaning up speech system");
    Speech::Cleanup();
    DEBUG_VERBOSE("MAIN", "Speech system cleanup completed");
    
    DEBUG_INFO("MAIN", "Application shutdown completed successfully");
    return 0;
}