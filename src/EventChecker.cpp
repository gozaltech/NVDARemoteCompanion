#include "EventChecker.h"

#ifdef _WIN32
    #define KEY_DOWN_EVENT_1 WM_KEYDOWN
    #define KEY_DOWN_EVENT_2 WM_SYSKEYDOWN
    #define KEY_UP_EVENT_1   WM_KEYUP
    #define KEY_UP_EVENT_2   WM_SYSKEYUP
#else
    #define KEY_DOWN_EVENT_1 0x100
    #define KEY_DOWN_EVENT_2 0x104
    #define KEY_UP_EVENT_1   0x101
    #define KEY_UP_EVENT_2   0x105
#endif

bool EventChecker::IsKeyDownEvent(EventParam wParam) {
    return wParam == KEY_DOWN_EVENT_1 || wParam == KEY_DOWN_EVENT_2;
}

bool EventChecker::IsKeyUpEvent(EventParam wParam) {
    return wParam == KEY_UP_EVENT_1 || wParam == KEY_UP_EVENT_2;
}