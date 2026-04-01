#include "ConnectionManager.h"
#include "Debug.h"
#include "Speech.h"
#include "Audio.h"
#include "Config.h"
#ifdef _WIN32
#include "AppState.h"
#endif
#include <iostream>
#include <thread>
#include <chrono>
#include <functional>
#include <unordered_map>

extern std::atomic<bool> g_shutdown;



ConnectionManager::ConnectionManager() : m_protocolHandshakeComplete(false) {
    m_client = std::make_shared<NetworkClient>();
}

ConnectionManager::~ConnectionManager() {
    DEBUG_INFO("CONN", "ConnectionManager destructor called");
    if (m_client) {
        if (m_client->IsConnected()) {
            Audio::PlayWave("disconnected");
        }
        DEBUG_VERBOSE("CONN", "Disconnecting network client");
        m_client->Disconnect();
        DEBUG_VERBOSE("CONN", "Network client disconnected");
    }
    DEBUG_INFO("CONN", "ConnectionManager destructor completed");
}

bool ConnectionManager::ShouldPlaySpeech() const {
    if (!m_speechEnabled) return false;
#ifdef _WIN32
    if (m_muteOnLocalControl && m_profileIndex >= 0 && AppState::GetActiveProfile() != m_profileIndex) {
        return false;
    }
#endif
    return true;
}

void ConnectionManager::HandleIncomingMessage(std::string_view message) {
    json j = json::parse(message, nullptr, false);
    if (j.is_null()) {
        DEBUG_ERROR("CONN", "Failed to parse incoming message as JSON");
        return;
    }

    using Handler = std::function<void(ConnectionManager&, const json&)>;
    static const std::unordered_map<std::string, Handler> dispatch = {
        {Config::MSG_TYPE_CHANNEL_JOINED, [](ConnectionManager& self, const json& msg) {
            DEBUG_INFO("CONN", "Successfully joined channel");
            DEBUG_VERBOSE_F("CONN", "Channel details: {}", msg.dump());
            if (self.m_client) {
                self.m_client->SendBrailleInfo();
                self.m_protocolHandshakeComplete = true;
                Audio::PlayWave("connected");
                Speech::Speak("Connected", false);
                DEBUG_INFO("CONN", "Protocol handshake complete");
            }
        }},
        {Config::MSG_TYPE_CANCEL, [](ConnectionManager& self, const json&) {
            DEBUG_VERBOSE("CONN", "Received speech cancel request");
            if (self.ShouldPlaySpeech()) Speech::Stop();
        }},
        {Config::MSG_TYPE_TONE, [](ConnectionManager& self, const json& msg) {
            if (self.m_forwardAudio)
                Audio::PlayTone(msg.value("hz", 440), msg.value("length", 100));
        }},
        {Config::MSG_TYPE_WAVE, [](ConnectionManager& self, const json& msg) {
            if (!self.m_forwardAudio) return;
            std::string fileName = msg.value("fileName", "");
            if (!fileName.empty()) Audio::PlayWave(fileName);
        }},
        {Config::MSG_TYPE_SPEAK, [](ConnectionManager& self, const json& msg) {
            if (!msg.contains("sequence") || !msg["sequence"].is_array()) {
                DEBUG_VERBOSE("CONN", "Speech message missing or invalid sequence field");
                return;
            }
            std::string speechText;
            for (const auto& item : msg["sequence"]) {
                if (item.is_string()) {
                    auto text = item.get<std::string>();
                    if (!text.empty()) speechText += text + " ";
                }
            }
            if (speechText.empty()) {
                DEBUG_VERBOSE("CONN", "Received empty speech sequence");
                return;
            }
            speechText.pop_back();
            DEBUG_VERBOSE_F("CONN", "Received speech: {}", speechText);
            if (self.ShouldPlaySpeech()) Speech::Speak(speechText, false);
        }},
    };

    auto messageType = j.value("type", "");
    auto it = dispatch.find(messageType);
    if (it != dispatch.end()) {
        it->second(*this, j);
    } else if (!messageType.empty()) {
        DEBUG_VERBOSE_F("CONN", "Unhandled message type: {}", messageType);
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

bool ConnectionManager::EstablishConnection(std::string_view host, int port, std::string_view key, std::string_view shortcut) {
    auto sanitizedHost = Config::TrimWhitespace(std::string(host));
    auto sanitizedKey = Config::TrimWhitespace(std::string(key));
    auto sanitizedShortcut = Config::TrimWhitespace(std::string(shortcut));
    
    auto validation = Config::Validator::ValidateConnectionParams(sanitizedHost, port, sanitizedKey);
    if (!validation) {
        DEBUG_ERROR_F("CONN", "Connection parameter validation failed: {}", validation.errorMessage);
        return false;
    }
    
    DEBUG_INFO_F("CONN", "Using validated connection parameters: {}:{}", sanitizedHost, port);
    m_params.host = std::move(sanitizedHost);
    m_params.port = port;
    m_params.key = std::move(sanitizedKey);
    m_params.shortcut = std::move(sanitizedShortcut);
    
    return EstablishConnectionInternal();
}

bool ConnectionManager::Reconnect() {
    if (m_params.host.empty()) {
        DEBUG_ERROR("CONN", "Cannot reconnect - no connection parameters available");
        return false;
    }
    auto savedCallback = m_disconnectCallback;
    SetDisconnectCallback(nullptr);
    Disconnect();
    SetDisconnectCallback(savedCallback);
    return EstablishConnectionInternal();
}

void ConnectionManager::Disconnect() {
    DEBUG_INFO("CONN", "Disconnecting...");
    if (m_client) {
        m_client->Disconnect();
    }
    m_protocolHandshakeComplete = false;
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

void ConnectionManager::SetDisconnectCallback(std::function<void()> callback) {
    m_disconnectCallback = callback;
    if (m_client) {
        m_client->SetDisconnectCallback(callback);
    }
}