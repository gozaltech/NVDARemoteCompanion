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
};

class ConnectionManager {
private:
    std::shared_ptr<NetworkClient> m_client;
    ConnectionParams m_params;
    bool m_protocolHandshakeComplete;

    std::optional<ConnectionParams> PromptForConnectionParams();
    void HandleIncomingMessage(std::string_view message);
    bool PerformHandshake();
    bool EstablishConnectionInternal();

public:
    ConnectionManager();
    ~ConnectionManager();

    bool EstablishConnection();
    bool EstablishConnection(std::string_view host, int port, std::string_view key);
    std::shared_ptr<NetworkClient> GetClient() { return m_client; }
    bool IsConnected() const;
};