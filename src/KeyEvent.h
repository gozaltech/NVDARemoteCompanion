#pragma once
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "Config.h"

using json = nlohmann::ordered_json;

struct KeyEvent {
    uint32_t vk_code;
    bool extended;
    bool pressed;
    uint16_t scan_code;
    std::string type;

    KeyEvent() : vk_code(0), extended(false), pressed(false), scan_code(0), type(Config::MSG_TYPE_KEY) {}

    KeyEvent(uint32_t vkCode, bool isPressed, uint16_t scanCode, bool isExtended = false) 
        : vk_code(vkCode), pressed(isPressed), scan_code(scanCode), extended(isExtended), type(Config::MSG_TYPE_KEY) {}

    json ToJson() const {
        return json{
            {"vk_code", vk_code},
            {"extended", extended},
            {"pressed", pressed},
            {"scan_code", scan_code},
            {"type", type}
        };
    }

    static KeyEvent FromJson(const json& j) {
        KeyEvent event;
        event.vk_code = j.at("vk_code").get<uint32_t>();
        event.extended = j.at("extended").get<bool>();
        event.pressed = j.at("pressed").get<bool>();
        event.scan_code = j.at("scan_code").get<uint16_t>();
        event.type = j.at("type").get<std::string>();
        return event;
    }

    static KeyEvent FromJsonString(const std::string& jsonString) {
        return FromJson(json::parse(jsonString));
    }
};