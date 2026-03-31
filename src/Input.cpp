#include "Input.h"
#include <atomic>
#include <iostream>

extern std::atomic<bool> g_shutdown;

#ifdef _WIN32
#include <windows.h>
#include <conio.h>

static bool ReadLineImpl(std::string& out, bool allowEscape) {
    out.clear();
    while (!g_shutdown) {
        if (!_kbhit()) {
            Sleep(10);
            continue;
        }
        int ch = _getch();
        if (ch == '\r') {
            std::cout << std::endl;
            return true;
        }
        if (ch == '\b') {
            if (!out.empty()) {
                out.pop_back();
                std::cout << "\b \b" << std::flush;
            }
        } else if (ch == 3) {
            std::cout << std::endl;
            g_shutdown = true;
            return false;
        } else if (allowEscape && ch == 27) {
            std::cout << std::endl;
            return false;
        } else if (ch >= 32 && ch <= 126) {
            out += static_cast<char>(ch);
            std::cout << static_cast<char>(ch) << std::flush;
        }
    }
    return false;
}

bool Input::ReadLine(std::string& out) {
    return ReadLineImpl(out, false);
}

bool Input::ReadLineWithEscape(std::string& out) {
    return ReadLineImpl(out, true);
}

#else
#include <unistd.h>
#include <sys/select.h>

bool Input::ReadLine(std::string& out) {
    out.clear();
    while (!g_shutdown) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval timeout { 0, 100000 };
        if (select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout) > 0) {
            if (std::getline(std::cin, out)) return true;
            return false;
        }
    }
    return false;
}

bool Input::ReadLineWithEscape(std::string& out) {
    return Input::ReadLine(out);
}

#endif
