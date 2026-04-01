#pragma once
#ifndef _WIN32

#include <atomic>

extern std::atomic<bool> g_shutdown;

class LinuxKeyboardGrab {
public:
    static bool Install();
    static void Uninstall();
    static void Reinstall();
    static void RunMessageLoop();
    static void NotifyConnectionLost();
};

#endif
