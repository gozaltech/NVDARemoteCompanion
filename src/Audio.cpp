#include "Audio.h"
#include "Debug.h"
#include <vector>
#include <thread>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#else
// Linux implementation stub or use pulse/alsa if needed
#endif

namespace fs = std::filesystem;

void Audio::PlayTone(int hz, int length) {
#ifdef _WIN32
    std::thread([hz, length]() {
        Beep(static_cast<DWORD>(hz), static_cast<DWORD>(length));
    }).detach();
#else
    DEBUG_WARN("AUDIO", "PlayTone not implemented on Linux");
#endif
}

void Audio::PlayWave(const std::string& fileName) {
    if (fileName.empty()) return;

    std::vector<std::string> searchPaths = {
        "sounds",
        "../../sounds",
        "../NVDARemote/addon/sounds",
        "../../NVDARemote/addon/sounds",
    };

#ifdef _WIN32
    const char* programFiles = getenv("ProgramFiles");
    const char* programFilesX86 = getenv("ProgramFiles(x86)");
    const char* appData = getenv("AppData");

    if (programFiles) {
        searchPaths.push_back(std::string(programFiles) + "\\NVDA\\waves");
    }
    if (programFilesX86) {
        searchPaths.push_back(std::string(programFilesX86) + "\\NVDA\\waves");
    }
#endif

    std::string fullPath;
    for (const auto& path : searchPaths) {
        try {
            auto p = fs::path(path) / fileName;
            if (!p.has_extension()) {
                p.replace_extension(".wav");
            }
            
            if (fs::exists(p)) {
                fullPath = fs::absolute(p).string();
                break;
            }
        } catch (...) {
            continue;
        }
    }

    if (fullPath.empty()) {
        DEBUG_WARN_F("AUDIO", "Sound file not found: {}", fileName);
        return;
    }

    DEBUG_VERBOSE_F("AUDIO", "Playing sound: {}", fullPath);

#ifdef _WIN32
    PlaySoundA(fullPath.c_str(), NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
#else
    std::thread([fullPath]() {
        std::string cmd = "aplay \"" + fullPath + "\" >/dev/null 2>&1";
        system(cmd.c_str());
    }).detach();
#endif
}
