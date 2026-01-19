#include "NetworkClient.h"
#include "Debug.h"
#include "Config.h"
#include <iostream>
#include <thread>
#include <chrono>

template<typename F>
class ScopeGuard {
private:
    F cleanup_;
    bool active_;
public:
    explicit ScopeGuard(F&& f) : cleanup_(std::move(f)), active_(true) {}
    ~ScopeGuard() { if (active_) cleanup_(); }
    void dismiss() { active_ = false; }
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&& other) noexcept : cleanup_(std::move(other.cleanup_)), active_(other.active_) {
        other.active_ = false;
    }
};

template<typename F>
ScopeGuard<F> make_scope_guard(F&& f) {
    return ScopeGuard<F>(std::forward<F>(f));
}

NetworkClient::NetworkClient() {
}

NetworkClient::~NetworkClient() {
    Disconnect();
}

bool NetworkClient::Connect(const std::string& host, int port) {
    if (m_connectionState.IsConnected()) {
        return true;
    }

    if (m_sslClient.Connect(host, port)) {
        m_connectionState.TransitionTo(ConnectionState::Status::Connected);
        return true;
    }

    return false;
}

void NetworkClient::Disconnect() {
    bool expected = false;
    static std::atomic<bool> disconnectInProgress(false);
    if (!disconnectInProgress.compare_exchange_strong(expected, true)) {
        DEBUG_VERBOSE("NETWORK", "Disconnect already in progress, skipping");
        return;
    }
    
    auto disconnectGuard = make_scope_guard([&] {
        disconnectInProgress.store(false);
    });
    
    DEBUG_INFO("NETWORK", "Starting disconnect sequence");
    
    try {
        DEBUG_VERBOSE("NETWORK", "Stopping worker threads");
        m_threadPool.StopAll();
        
        DEBUG_VERBOSE("NETWORK", "Notifying sender thread");
        m_sendCondition.notify_all();
        
        DEBUG_VERBOSE("NETWORK", "Closing SSL connection");
        m_sslClient.Disconnect();
        
        DEBUG_VERBOSE("NETWORK", "Clearing send queue");
        {
            std::lock_guard<std::mutex> lock(m_sendMutex);
            size_t queueSize = m_sendQueue.size();
            while (!m_sendQueue.empty()) {
                m_sendQueue.pop();
            }
            if (queueSize > 0) {
                DEBUG_VERBOSE_F("NETWORK", "Cleared {} unsent messages from queue", queueSize);
            }
        }
        
        m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
        DEBUG_INFO("NETWORK", "Disconnect sequence completed successfully");

        if (m_disconnectCallback) {
            DEBUG_VERBOSE("NETWORK", "Triggering disconnect callback");
            m_disconnectCallback();
        }
        
    } catch (...) {
        DEBUG_ERROR("NETWORK", "Exception occurred during disconnect - continuing cleanup");
        m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
    }
}

bool NetworkClient::SendRawMessage(const std::string& message) {
    if (!m_connectionState.IsConnected()) {
        DEBUG_ERROR("NETWORK", "Cannot send - not connected");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_sendMutex);
        m_sendQueue.push(message + "\n");
    }
    m_sendCondition.notify_one();
    
    DEBUG_VERBOSE_F("NETWORK", "Queued message for sending: {}", message);
    return true;
}

bool NetworkClient::SendJsonMessage(const json& message) {
    return SendRawMessage(message.dump());
}

void NetworkClient::SetMessageHandler(std::function<void(const std::string&)> handler) {
    m_messageHandler = handler;
}

void NetworkClient::SetDisconnectCallback(std::function<void()> callback) {
    m_disconnectCallback = callback;
}

void NetworkClient::SenderThreadLoop() {
    DEBUG_INFO("NETWORK", "Sender thread started");
    
    while (m_connectionState.IsConnected()) {
        std::unique_lock<std::mutex> lock(m_sendMutex);
        
        m_sendCondition.wait(lock, [this] { 
            return !m_connectionState.IsConnected() || !m_sendQueue.empty(); 
        });
        
        while (!m_sendQueue.empty() && m_connectionState.IsConnected()) {
            auto message = m_sendQueue.front();
            m_sendQueue.pop();
            lock.unlock();
            
            auto result = m_sslClient.Send(message.c_str(), static_cast<int>(message.length()));
            
            if (result < 0) {
                DEBUG_ERROR("NETWORK", "SSL send failed");
                m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
                break;
            }
            
            DEBUG_VERBOSE_F("NETWORK", "Actually sent: {} (bytes: {})", 
                           message.substr(0, message.length()-1), result);
            
            lock.lock();
        }
    }
    
    DEBUG_INFO("NETWORK", "Sender thread terminated");
}

void NetworkClient::ReceiverThreadLoop() {
    char buffer[Config::RECEIVER_BUFFER_SIZE];
    std::string receivedData;
    DEBUG_INFO("NETWORK", "Receiver thread started");

    while (m_connectionState.IsConnected()) {
        int bytesReceived = m_sslClient.Receive(buffer, sizeof(buffer) - 1);
        
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            DEBUG_TRACE_F("NETWORK", "Raw SSL received ({} bytes): {}", bytesReceived, std::string(buffer, bytesReceived));
            receivedData += std::string(buffer, bytesReceived);
            
            size_t pos;
            while ((pos = receivedData.find('\n')) != std::string::npos) {
                auto message = receivedData.substr(0, pos);
                if (!message.empty() && message.back() == '\r') {
                    message.pop_back();
                }
                receivedData.erase(0, pos + 1);
                
                if (!message.empty()) {
                    DEBUG_VERBOSE_F("NETWORK", "Received message: {}", message);
                    if (m_messageHandler) {
                        m_messageHandler(message);
                    }
                }
            }
        } else if (bytesReceived == -2) {
            // WANT_READ: No data available yet
            std::this_thread::sleep_for(std::chrono::milliseconds(Config::SENDER_SLEEP_MS));
            continue;
        } else {
            // 0 (EOF) or -1 (Error)
            if (bytesReceived == 0) {
                 DEBUG_INFO("NETWORK", "SSL connection closed by peer");
            } else {
                 DEBUG_ERROR("NETWORK", "SSL receive failed");
            }
            m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
            if (m_disconnectCallback) {
                m_disconnectCallback();
            }
            break;
        }
    }
    
    DEBUG_INFO("NETWORK", "Receiver thread terminated");
}

void NetworkClient::StartReceiving() {
    if (!m_connectionState.IsConnected()) {
        return;
    }
    
    m_threadPool.AddWorker("Sender", [this](const std::atomic<bool>& shouldStop) {
        SenderThreadLoop();
    });
    m_threadPool.AddWorker("Receiver", [this](const std::atomic<bool>& shouldStop) {
        ReceiverThreadLoop();
    });
}

bool NetworkClient::SendProtocolVersion() {
    json message = {
        {"version", 2},
        {"type", Config::MSG_TYPE_PROTOCOL_VERSION}
    };
    return SendJsonMessage(message);
}

bool NetworkClient::SendJoinChannel(const std::string& channel, const std::string& connectionType) {
    json message = {
        {"channel", channel},
        {"connection_type", connectionType},
        {"type", Config::MSG_TYPE_JOIN}
    };
    return SendJsonMessage(message);
}

bool NetworkClient::SendBrailleInfo() {
    json message = {
        {"name", "noBraille"},
        {"numCells", 0},
        {"type", Config::MSG_TYPE_SET_BRAILLE_INFO}
    };
    return SendJsonMessage(message);
}

bool NetworkClient::SendKeyEvent(const json& keyEvent) {
    return SendJsonMessage(keyEvent);
}