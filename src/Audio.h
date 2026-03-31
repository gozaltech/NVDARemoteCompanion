#pragma once
#include <string>

class Audio {
public:
    static void SetEnabled(bool enabled);
    static bool IsEnabled();
    static void PlayTone(int hz, int length);
    static void PlayWave(const std::string& fileName);
};
