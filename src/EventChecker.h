#pragma once

#ifdef _WIN32
    #include <windows.h>
    using EventParam = WPARAM;
#else
    #include <cstdint>
    using EventParam = uint32_t;
#endif

class EventChecker {
public:
    static bool IsKeyDownEvent(EventParam wParam);
    static bool IsKeyUpEvent(EventParam wParam);
};