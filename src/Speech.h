#pragma once
#include <string>
#include <string_view>

class Speech {
private:
    static bool s_initialized;
    static bool s_enabled;

public:
    static bool Initialize();
    static void Cleanup();
    static bool IsInitialized() { return s_initialized; }
    
    static void SetEnabled(bool enabled) { s_enabled = enabled; }
    static bool IsEnabled() { return s_enabled; }
    
    static void Speak(std::string_view text, bool interrupt = false);
    static void SpeakSsml(std::string_view ssml, bool interrupt = false);
    static void Stop();
};