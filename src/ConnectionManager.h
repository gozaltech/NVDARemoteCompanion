#pragma once
#include "NetworkClient.h"
#include <string>
#include <string_view>
#include <memory>
#include <optional>

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
    bool m_speechEnabled = false;
    bool m_muteOnLocalControl = false;
    int m_profileIndex = -1;

    void HandleIncomingMessage(std::string_view message);
    bool PerformHandshake();
    bool EstablishConnectionInternal();
    bool ShouldPlaySpeech() const;

public:
    ConnectionManager();
    ~ConnectionManager();

    bool EstablishConnection(std::string_view host, int port, std::string_view key, std::string_view shortcut = "");
    bool Reconnect();
    void Disconnect();
    void SetDisconnectCallback(std::function<void()> callback);
    std::shared_ptr<NetworkClient> GetClient() { return m_client; }
    std::string GetShortcut() const { return m_params.shortcut; }
    bool IsConnected() const;
    void SetSpeechEnabled(bool enabled) { m_speechEnabled = enabled; }
    void SetMuteOnLocalControl(bool enabled) { m_muteOnLocalControl = enabled; }
    void SetProfileIndex(int index) { m_profileIndex = index; }
};