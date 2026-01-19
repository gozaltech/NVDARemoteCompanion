#pragma once
#include <string>

class Audio {
public:
    static void PlayTone(int hz, int length);
    static void PlayWave(const std::string& fileName);
};
