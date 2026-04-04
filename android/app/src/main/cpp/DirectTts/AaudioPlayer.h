#pragma once
#include <aaudio/AAudio.h>
#include <cstdint>

class AaudioPlayer {
public:
    AaudioPlayer();
    ~AaudioPlayer();

    bool open();
    bool write(const int8_t* data, int32_t bytes);
    void pause();
    bool resume();
    void stop();
    void close();
    bool isOpen() const { return m_stream != nullptr; }

private:
    AAudioStream* m_stream = nullptr;
};
