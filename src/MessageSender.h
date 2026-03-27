#pragma once
#include "KeyEvent.h"
#include <string>
#include <memory>
#include <vector>

class NetworkClient;

class MessageSender {
private:
    static std::vector<std::weak_ptr<NetworkClient>> s_clients;
    static int s_activeProfile;

public:
    static void SetNetworkClient(int index, std::shared_ptr<NetworkClient> client);
    static void SetActiveProfile(int index);
    static void SendKeyEvent(const KeyEvent& keyEvent);
};
