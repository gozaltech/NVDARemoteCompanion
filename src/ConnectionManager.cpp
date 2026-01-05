#include "ConnectionManager.h"
#include "Debug.h"
#include "Speech.h"
#include "Config.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <functional>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <sys/select.h>
#endif

extern std::atomic<bool> g_shutdown;

bool GetLineWithShutdownCheck(const std::string& prompt, std::string& result);

namespace {
    class InputHandler {
    public:
        using ValidatorFunc = std::function<Config::ValidationResult(const std::string&)>;
        using ProcessorFunc = std::function<std::string(const std::string&)>;
        
        static bool GetValidatedInput(const std::string& prompt, 
                                    std::string& result,
                                    ValidatorFunc validator = nullptr,
                                    ProcessorFunc processor = nullptr) {
            while (true) {
                std::string input;
                if (!GetLineWithShutdownCheck(prompt, input)) {
                    return false;
                }
                
                if (processor) {
                    input = processor(input);
                }
                
                if (validator) {
                    auto validation = validator(input);
                    if (!validation) {
                        std::cout << Config::ERROR_PREFIX << validation.errorMessage << "\n\n";
                        continue;
                    }
                }
                
                result = std::move(input);
                return true;
            }
        }
        
        static bool GetValidatedPort(int& result, int defaultValue = Config::DEFAULT_PORT) {
            std::string prompt = "Enter server port [" + std::to_string(defaultValue) + "]: ";
            std::string input;
            
            auto validator = [](const std::string& str) -> Config::ValidationResult {
                if (str.empty()) return {true};
                
                try {
                    int port = std::stoi(str);
                    return Config::Validator::ValidatePort(port);
                } catch (const std::exception&) {
                    return {false, Config::ERROR_PORT_INVALID};
                }
            };
            
            while (true) {
                if (!GetValidatedInput(prompt, input, validator, Config::TrimWhitespace)) {
                    return false;
                }
                
                if (input.empty()) {
                    result = defaultValue;
                    std::cout << "Using default port: " << defaultValue << "\n\n";
                    DEBUG_VERBOSE("CONN", "Using default port");
                    return true;
                }
                
                result = std::stoi(input);
                std::cout << "Port: " << result << "\n\n";
                return true;
            }
        }
    };
}

bool GetLineWithShutdownCheck(const std::string& prompt, std::string& result) {
    std::cout << prompt;
    std::cout.flush();
    
    result.clear();
    
    while (!g_shutdown) {
#ifdef _WIN32
        if (_kbhit()) {
            int ch = _getch();
            if (ch == '\r') {
                std::cout << std::endl;
                return true;
            } else if (ch == '\b' && !result.empty()) {
                result.pop_back();
                std::cout << "\b \b";
            } else if (ch >= 32 && ch <= 126) {
                result += static_cast<char>(ch);
                std::cout << static_cast<char>(ch);
            }
            std::cout.flush();
        } else {
            Sleep(10);
        }
#else
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        
        int ready = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout);
        if (ready > 0) {
            if (std::getline(std::cin, result)) {
                return true;
            }
        }
#endif
    }
    
    DEBUG_INFO("CONN", "Input cancelled due to shutdown signal");
    return false;
}

ConnectionManager::ConnectionManager() : m_protocolHandshakeComplete(false) {
    m_client = std::make_shared<NetworkClient>();
}

ConnectionManager::~ConnectionManager() {
    DEBUG_INFO("CONN", "ConnectionManager destructor called");
    if (m_client) {
        DEBUG_VERBOSE("CONN", "Disconnecting network client");
        m_client->Disconnect();
        DEBUG_VERBOSE("CONN", "Network client disconnected");
    }
    DEBUG_INFO("CONN", "ConnectionManager destructor completed");
}

std::optional<ConnectionParams> ConnectionManager::PromptForConnectionParams() {
    ConnectionParams params;
    
    std::cout << "\n" << Config::APP_NAME << " - Interactive Setup\n";
    std::cout << std::string(50, '=') << "\n\n";
    
    std::cout << "Server Configuration:\n";
    auto hostValidator = [](const std::string& host) { return Config::Validator::ValidateHost(host); };
    if (!InputHandler::GetValidatedInput("Enter server host (IP address or domain name): ", 
                                        params.host, hostValidator, Config::TrimWhitespace)) {
        return std::nullopt;
    }
    std::cout << "Host: " << params.host << "\n\n";
    
    if (!InputHandler::GetValidatedPort(params.port)) {
        return std::nullopt;
    }
    
    auto keyValidator = [](const std::string& key) { return Config::Validator::ValidateKey(key); };
    if (!InputHandler::GetValidatedInput("Enter connection key/channel: ", 
                                        params.key, keyValidator, Config::TrimWhitespace)) {
        return std::nullopt;
    }
    std::cout << "Connection key: " << params.key << "\n\n";
    
    std::cout << "Connection Summary:\n";
    std::cout << "  Host: " << params.host << "\n";
    std::cout << "  Port: " << params.port << "\n";
    std::cout << "  Key:  " << params.key << "\n\n";
    std::cout << "Connecting to NVDA Remote server...\n";
    
    return params;
}

void ConnectionManager::HandleIncomingMessage(std::string_view message) {
    json j = json::parse(message, nullptr, false);
    if (j.is_null()) {
        DEBUG_ERROR("CONN", "Failed to parse incoming message as JSON");
        return;
    }
    
    auto messageType = j.value("type", "");
    
    if (messageType == Config::MSG_TYPE_CHANNEL_JOINED) {
        DEBUG_INFO("CONN", "Successfully joined channel");
        DEBUG_VERBOSE_F("CONN", "Channel details: {}", j.dump());
        
        if (m_client) {
            DEBUG_VERBOSE("CONN", "Sending braille info");
            m_client->SendBrailleInfo();
            m_protocolHandshakeComplete = true;
            DEBUG_INFO("CONN", "Protocol handshake complete");
        }
    }
    else if (messageType == Config::MSG_TYPE_CANCEL) {
        DEBUG_VERBOSE("CONN", "Received speech cancel request");
        Speech::Stop();
    }
    else if (messageType == Config::MSG_TYPE_SPEAK) {
        if (j.contains("sequence") && j["sequence"].is_array()) {
            std::string speechText;
            
            for (const auto& item : j["sequence"]) {
                if (item.is_string()) {
                    auto text = item.get<std::string>();
                    if (!text.empty()) {
                        speechText += text + " ";
                    }
                }
            }
            
            if (!speechText.empty()) {
                // Remove the last space added by the loop
                speechText.pop_back(); 
                DEBUG_VERBOSE_F("CONN", "Received speech: {}", speechText);
                
                // USER REQUEST: "Go the don't interrupt root".
                // We always queue speech (interrupt=false).
                // This mimics Spri.NEXT behavior for everything, ensuring "Desktop list" 
                // isn't cut off by "Recycle Bin".
                // "Speedy spelling" is a known trade-off, but "skipping text" is worse.
                // Interruption via Control key still works via MSG_TYPE_CANCEL.
                Speech::Speak(speechText, false);
            } else {
                DEBUG_VERBOSE("CONN", "Received empty speech sequence");
            }
        } else {
            DEBUG_VERBOSE("CONN", "Speech message missing or invalid sequence field");
        }
    }
}

bool ConnectionManager::PerformHandshake() {
    if (!m_client->IsConnected()) {
        return false;
    }

    
    if (!m_client->SendProtocolVersion()) {
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(Config::HANDSHAKE_RETRY_INTERVAL_MS));
    
    if (!m_client->IsConnected()) {
        return false;
    }
    
    if (!m_client->SendJoinChannel(m_params.key)) {
        return false;
    }
    
    return true;
}

bool ConnectionManager::EstablishConnection() {
    DEBUG_INFO("CONN", "Getting connection parameters from user");
    auto paramsOpt = PromptForConnectionParams();
    
    if (!paramsOpt.has_value()) {
        DEBUG_INFO("CONN", "Connection cancelled by user");
        return false;
    }
    
    m_params = paramsOpt.value();
    return EstablishConnectionInternal();
}

bool ConnectionManager::EstablishConnection(std::string_view host, int port, std::string_view key) {
    auto sanitizedHost = Config::TrimWhitespace(std::string(host));
    auto sanitizedKey = Config::TrimWhitespace(std::string(key));
    
    auto validation = Config::Validator::ValidateConnectionParams(sanitizedHost, port, sanitizedKey);
    if (!validation) {
        DEBUG_ERROR_F("CONN", "Connection parameter validation failed: {}", validation.errorMessage);
        return false;
    }
    
    DEBUG_INFO_F("CONN", "Using validated connection parameters: {}:{}", sanitizedHost, port);
    m_params.host = std::move(sanitizedHost);
    m_params.port = port;
    m_params.key = std::move(sanitizedKey);
    
    return EstablishConnectionInternal();
}

bool ConnectionManager::EstablishConnectionInternal() {
    DEBUG_INFO_F("CONN", "Attempting to connect to {}:{}", m_params.host, m_params.port);
    
    if (!m_client->Connect(m_params.host, m_params.port)) {
        DEBUG_ERROR("CONN", "Failed to connect to server");
        return false;
    }
    
    DEBUG_VERBOSE("CONN", "Setting up message handler");
    m_client->SetMessageHandler([this](const std::string& msg) {
        HandleIncomingMessage(msg);
    });
    
    DEBUG_VERBOSE("CONN", "Starting receiver threads");
    m_client->StartReceiving();
    
    DEBUG_VERBOSE("CONN", "Performing protocol handshake");
    if (!PerformHandshake()) {
        DEBUG_ERROR("CONN", "Protocol handshake failed - cleaning up");
        m_client->Disconnect();
        return false;
    }
    
    DEBUG_VERBOSE("CONN", "Waiting for handshake to complete");
    for (int i = 0; i < Config::HANDSHAKE_MAX_ATTEMPTS; ++i) {
        if (m_protocolHandshakeComplete) {
            DEBUG_INFO("CONN", "Connection established successfully");
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(Config::HANDSHAKE_RETRY_INTERVAL_MS));
    }
    
    DEBUG_ERROR("CONN", "Handshake timeout - cleaning up");
    m_client->Disconnect();
    return false;
}

bool ConnectionManager::IsConnected() const {
    return m_client && m_client->IsConnected() && m_protocolHandshakeComplete;
}