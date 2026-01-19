#pragma once
#include <string>
#include <string_view>
#include <cstdint>

namespace Config {
    
    constexpr int DEFAULT_PORT = 6837;
    constexpr int MIN_PORT = 1;
    constexpr int MAX_PORT = 65535;
    constexpr int HANDSHAKE_TIMEOUT_MS = 3000;
    constexpr int HANDSHAKE_RETRY_INTERVAL_MS = 30;
    constexpr int HANDSHAKE_MAX_ATTEMPTS = 100;
    
    constexpr int RECEIVER_BUFFER_SIZE = 4096;
    constexpr int SENDER_SLEEP_MS = 1;
    
    constexpr int PROTOCOL_VERSION = 2;
    constexpr const char* DEFAULT_CONNECTION_TYPE = "master";
    constexpr const char* BRAILLE_DISPLAY_NAME = "noBraille";
    constexpr int BRAILLE_CELL_COUNT = 0;
    
    constexpr int KEY_RELEASE_GRACE_PERIOD_MS = 500;
    constexpr int INPUT_TIMEOUT_MS = 100;
    
    constexpr size_t MAX_HOST_LENGTH = 253;
    constexpr size_t MAX_KEY_LENGTH = 256;
    constexpr size_t MAX_MESSAGE_SIZE = 8192;
    
    constexpr const char* APP_NAME = "NVDA Remote Client";
    constexpr const char* APP_DESCRIPTION = "Cross-platform client for NVDA Remote connections";
    
    constexpr const char* MSG_TYPE_PROTOCOL_VERSION = "protocol_version";
    constexpr const char* MSG_TYPE_JOIN = "join";
    constexpr const char* MSG_TYPE_SET_BRAILLE_INFO = "set_braille_info";
    constexpr const char* MSG_TYPE_CHANNEL_JOINED = "channel_joined";
    constexpr const char* MSG_TYPE_CANCEL = "cancel";
    constexpr const char* MSG_TYPE_SPEAK = "speak";
    constexpr const char* MSG_TYPE_KEY = "key";
    constexpr const char* MSG_TYPE_TONE = "tone";
    constexpr const char* MSG_TYPE_WAVE = "wave";
    
    constexpr const char* ERROR_PREFIX = "Error: ";
    constexpr const char* ERROR_HOST_EMPTY = "Host cannot be empty. Please enter a valid hostname or IP address.";
    constexpr const char* ERROR_KEY_EMPTY = "Connection key cannot be empty.";
    constexpr const char* ERROR_KEY_TOO_LONG = "Connection key too long";
    constexpr const char* ERROR_PORT_INVALID = "Invalid port number. Please enter a numeric value.";
    constexpr const char* ERROR_PORT_RANGE = "Port must be between";
    
    constexpr size_t SSL_ERROR_BUFFER_SIZE = 256;
    
    constexpr const char* DEBUG_CATEGORY_MAIN = "MAIN";
    constexpr const char* DEBUG_CATEGORY_NETWORK = "NETWORK";
    constexpr const char* DEBUG_CATEGORY_SSL = "SSL";
    constexpr const char* DEBUG_CATEGORY_CONN = "CONN";
    constexpr const char* DEBUG_CATEGORY_SPEECH = "SPEECH";
    constexpr const char* DEBUG_CATEGORY_KEYS = "KEYS";
    
    constexpr bool IsValidPort(int port) {
        return port >= MIN_PORT && port <= MAX_PORT;
    }
    
    inline bool IsValidStringLength(std::string_view str, size_t maxLength) {
        return !str.empty() && str.length() <= maxLength;
    }
    
    inline bool IsValidHost(std::string_view host) {
        return IsValidStringLength(host, MAX_HOST_LENGTH) && 
               host.find_first_of(" \t\r\n") == std::string_view::npos;
    }
    
    inline bool IsValidKey(std::string_view key) {
        return IsValidStringLength(key, MAX_KEY_LENGTH);
    }
    
    inline std::string TrimWhitespace(const std::string& str) {
        if (str.empty()) return str;
        
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }
    
    struct ValidationResult {
        bool isValid;
        std::string errorMessage;
        
        ValidationResult(bool valid = true, std::string error = "") 
            : isValid(valid), errorMessage(std::move(error)) {}
        
        operator bool() const { return isValid; }
    };
    
    class Validator {
    public:
        static ValidationResult ValidateHost(std::string_view host) {
            if (host.empty()) {
                return {false, ERROR_HOST_EMPTY};
            }
            if (!IsValidHost(host)) {
                return {false, "Invalid host. Must be under " + std::to_string(MAX_HOST_LENGTH) + 
                        " characters and contain no spaces or control characters."};
            }
            return {true};
        }
        
        static ValidationResult ValidatePort(int port) {
            if (!IsValidPort(port)) {
                return {false, std::string(ERROR_PORT_RANGE) + " " + 
                        std::to_string(MIN_PORT) + " and " + std::to_string(MAX_PORT)};
            }
            return {true};
        }
        
        static ValidationResult ValidateKey(std::string_view key) {
            if (key.empty()) {
                return {false, ERROR_KEY_EMPTY};
            }
            if (!IsValidKey(key)) {
                return {false, std::string(ERROR_KEY_TOO_LONG) + " (max " + 
                        std::to_string(MAX_KEY_LENGTH) + " characters)"};
            }
            return {true};
        }
        
        static ValidationResult ValidateConnectionParams(std::string_view host, int port, std::string_view key) {
            if (auto hostResult = ValidateHost(host); !hostResult) {
                return hostResult;
            }
            if (auto portResult = ValidatePort(port); !portResult) {
                return portResult;
            }
            if (auto keyResult = ValidateKey(key); !keyResult) {
                return keyResult;
            }
            return {true};
        }
    };
}
