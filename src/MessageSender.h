#pragma once
#include "KeyEvent.h"
#include <string>
#include <memory>

class NetworkClient;

class MessageSender {
private:
    static std::weak_ptr<NetworkClient> s_networkClient;

public:
    static void SetNetworkClient(std::shared_ptr<NetworkClient> client) {
        s_networkClient = client;
    }
    
    static void SendKeyEvent(const KeyEvent& keyEvent);
};