#include "MessageSender.h"
#include "NetworkClient.h"
#include "Config.h"
#include "Debug.h"
#include <nlohmann/json.hpp>

std::vector<std::weak_ptr<NetworkClient>> MessageSender::s_clients;
int MessageSender::s_activeProfile = -1;

void MessageSender::SetNetworkClient(int index, std::shared_ptr<NetworkClient> client) {
    while (static_cast<int>(s_clients.size()) <= index) {
        s_clients.push_back({});
    }
    s_clients[index] = client;
}

void MessageSender::SetActiveProfile(int index) {
    s_activeProfile = index;
}

void MessageSender::SendClipboardText(const std::string& text) {
    if (s_activeProfile < 0 || s_activeProfile >= static_cast<int>(s_clients.size())) return;
    if (auto client = s_clients[s_activeProfile].lock()) {
        nlohmann::ordered_json msg;
        msg["type"] = Config::MSG_TYPE_SET_CLIPBOARD_TEXT;
        msg["text"] = text;
        client->SendJsonMessage(msg);
        DEBUG_INFO("CLIP", "Clipboard text sent to remote");
    }
}

void MessageSender::SendKeyEvent(const KeyEvent& keyEvent) {
    if (s_activeProfile < 0 || s_activeProfile >= static_cast<int>(s_clients.size())) {
        return;
    }
    if (auto client = s_clients[s_activeProfile].lock()) {
        DEBUG_VERBOSE_F("KEYS", "Sending key to profile {}: VK={}, pressed={}, scan={}, extended={}",
                       s_activeProfile, keyEvent.vk_code, keyEvent.pressed, keyEvent.scan_code, keyEvent.extended);
        client->SendKeyEvent(keyEvent.ToJson());
    }
}
