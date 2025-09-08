#include "Speech.h"
#include "Debug.h"
#include <SRAL.h>

bool Speech::s_initialized = false;
bool Speech::s_enabled = true;

bool Speech::Initialize() {
    if (s_initialized) {
        return true;
    }
    
    if (!SRAL_Initialize(0)) {
        DEBUG_ERROR("SPEECH", "Failed to initialize SRAL");
        return false;
    }
    
    s_initialized = true;
    DEBUG_INFO("SPEECH", "SRAL initialized successfully");
    
    int currentEngine = SRAL_GetCurrentEngine();
    DEBUG_INFO_F("SPEECH", "Using speech engine: {}", currentEngine);
    
    return true;
}

void Speech::Cleanup() {
    if (s_initialized) {
        SRAL_Uninitialize();
        s_initialized = false;
        DEBUG_INFO("SPEECH", "SRAL uninitialized");
    }
}

void Speech::Speak(std::string_view text, bool interrupt) {
    if (!s_initialized || !s_enabled || text.empty()) {
        return;
    }
    
    DEBUG_VERBOSE_F("SPEECH", "Speaking: {}", text);
    
    if (text.data()[text.size()] == '\0') {
        SRAL_Speak(text.data(), interrupt);
    } else {
        std::string textStr(text);
        SRAL_Speak(textStr.c_str(), interrupt);
    }
}

void Speech::Stop() {
    if (s_initialized) {
        SRAL_StopSpeech();
        DEBUG_VERBOSE("SPEECH", "Speech stopped");
    }
}