#pragma once
#include <string_view>

class DirectTtsEngine {
public:
    virtual ~DirectTtsEngine() = default;
    virtual void speak(std::string_view text, bool interrupt) = 0;
    virtual void stop() = 0;
};
