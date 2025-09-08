#pragma once
#include <atomic>
#include <functional>

namespace ConnectionState {
    enum class Status {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting
    };
    
    class StateManager {
    private:
        std::atomic<Status> m_status{Status::Disconnected};
        std::function<void(Status, Status)> m_stateChangeCallback;
        
    public:
        StateManager() = default;
        
        void SetStateChangeCallback(std::function<void(Status, Status)> callback) {
            m_stateChangeCallback = std::move(callback);
        }
        
        Status GetStatus() const { 
            return m_status.load(); 
        }
        
        bool IsConnected() const { 
            return m_status.load() == Status::Connected; 
        }
        
        bool IsConnecting() const { 
            return m_status.load() == Status::Connecting; 
        }
        
        bool IsDisconnected() const { 
            return m_status.load() == Status::Disconnected; 
        }
        
        bool TransitionTo(Status newStatus) {
            Status oldStatus = m_status.exchange(newStatus);
            if (oldStatus != newStatus && m_stateChangeCallback) {
                m_stateChangeCallback(oldStatus, newStatus);
            }
            return true;
        }
        
        bool AttemptTransition(Status expectedStatus, Status newStatus) {
            if (m_status.compare_exchange_strong(expectedStatus, newStatus)) {
                if (m_stateChangeCallback) {
                    m_stateChangeCallback(expectedStatus, newStatus);
                }
                return true;
            }
            return false;
        }
    };
    
    class ConnectionGuard {
    private:
        StateManager& m_stateManager;
        bool m_isValid;
        
    public:
        explicit ConnectionGuard(StateManager& manager) 
            : m_stateManager(manager), m_isValid(manager.IsConnected()) {}
        
        bool IsValid() const { return m_isValid && m_stateManager.IsConnected(); }
        operator bool() const { return IsValid(); }
    };
}