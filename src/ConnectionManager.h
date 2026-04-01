#pragma once
#include "NetworkClient.h"
#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

struct ConnectionParams {
    std::string host;
    int port;
    std::string key;
    std::string shortcut;
};

class ConnectionManager {
private:
    std::shared_ptr<NetworkClient> m_client;
    ConnectionParams m_params;
    bool m_protocolHandshakeComplete;
    std::function<void()> m_disconnectCallback;
    std::function<void()> m_reconnectCallback; // fired after a successful auto-reconnect
    bool m_speechEnabled = false;
    bool m_muteOnLocalControl = false;
    bool m_forwardAudio = true;
    int m_profileIndex = -1;

    std::atomic<bool> m_wantsConnection{false};
    std::thread m_reconnectThread;
    std::mutex m_reconnectMutex;
    std::condition_variable m_reconnectCv;
    bool m_reconnectPending = false;

    void HandleIncomingMessage(std::string_view message);
    bool PerformHandshake();
    bool EstablishConnectionInternal();
    bool ShouldPlaySpeech() const;
    void TriggerReconnect();
    void ReconnectLoop();

public:
    ConnectionManager();
    ~ConnectionManager();

    bool EstablishConnection(std::string_view host, int port, std::string_view key, std::string_view shortcut = "");
    bool Reconnect();
    void Disconnect();
    void SetDisconnectCallback(std::function<void()> callback);
    void SetReconnectCallback(std::function<void()> callback) { m_reconnectCallback = callback; }
    std::shared_ptr<NetworkClient> GetClient() { return m_client; }
    std::string GetShortcut() const { return m_params.shortcut; }
    bool IsConnected() const;
    void SetSpeechEnabled(bool enabled) { m_speechEnabled = enabled; }
    void SetMuteOnLocalControl(bool enabled) { m_muteOnLocalControl = enabled; }
    void SetForwardAudioEnabled(bool enabled) { m_forwardAudio = enabled; }
    void SetProfileIndex(int index) { m_profileIndex = index; }
};