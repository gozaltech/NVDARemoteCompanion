#pragma once
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "SSLClient.h"
#include "ThreadManager.h"
#include "ConnectionState.h"

using json = nlohmann::ordered_json;

class NetworkClient {
private:
    SSLClient m_sslClient;
    ConnectionState::StateManager m_connectionState;
    std::function<void(const std::string&)> m_messageHandler;
    std::function<void()> m_disconnectCallback;

    std::queue<std::string> m_sendQueue;
    std::mutex m_sendMutex;
    std::condition_variable m_sendCondition;
    ThreadManager::ThreadPool m_threadPool;

    bool SendRawMessage(const std::string& message);
    void SenderThreadLoop();
    void ReceiverThreadLoop();

public:
    NetworkClient();
    ~NetworkClient();
    bool Connect(const std::string& host, int port);
    void Disconnect();
    bool IsConnected() const { return m_connectionState.IsConnected() && m_sslClient.IsConnected(); }
    bool SendJsonMessage(const json& message);
    void SetMessageHandler(std::function<void(const std::string&)> handler);
    void SetDisconnectCallback(std::function<void()> callback);
    void StartReceiving();
    bool SendProtocolVersion();
    bool SendJoinChannel(const std::string& channel, const std::string& connectionType = "master");
    bool SendBrailleInfo();
    bool SendKeyEvent(const json& keyEvent);
};
