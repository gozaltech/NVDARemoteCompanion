#include "MessageSender.h"
#include "NetworkClient.h"
#include "Debug.h"

std::weak_ptr<NetworkClient> MessageSender::s_networkClient;

void MessageSender::SendKeyEvent(const KeyEvent& keyEvent) {
    if (auto client = s_networkClient.lock()) {
        DEBUG_VERBOSE_F("KEYS", "Sending key: VK={}, pressed={}, scan={}, extended={}", 
                       keyEvent.vk_code, keyEvent.pressed, keyEvent.scan_code, keyEvent.extended);
        client->SendKeyEvent(keyEvent.ToJson());
    }
}