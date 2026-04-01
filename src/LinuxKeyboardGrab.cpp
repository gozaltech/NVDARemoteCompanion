#ifndef _WIN32

#include "LinuxKeyboardGrab.h"
#include "KeyboardState.h"
#include "AppState.h"
#include "KeyEvent.h"
#include "MessageSender.h"
#include "Debug.h"

#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <linux/uinput.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <dirent.h>
#include <sys/ioctl.h>

#include <cstring>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <atomic>
#include <functional>

static const std::unordered_map<uint32_t, uint32_t>& EvdevToVk() {
    static const std::unordered_map<uint32_t, uint32_t> table = {
        {KEY_ESC,        VK_ESCAPE},
        {KEY_1,          0x31}, {KEY_2, 0x32}, {KEY_3, 0x33}, {KEY_4, 0x34},
        {KEY_5,          0x35}, {KEY_6, 0x36}, {KEY_7, 0x37}, {KEY_8, 0x38},
        {KEY_9,          0x39}, {KEY_0, 0x30},
        {KEY_MINUS,      VK_OEM_MINUS},
        {KEY_EQUAL,      VK_OEM_PLUS},
        {KEY_BACKSPACE,  VK_BACK},
        {KEY_TAB,        VK_TAB},
        {KEY_Q, 'Q'}, {KEY_W, 'W'}, {KEY_E, 'E'}, {KEY_R, 'R'}, {KEY_T, 'T'},
        {KEY_Y, 'Y'}, {KEY_U, 'U'}, {KEY_I, 'I'}, {KEY_O, 'O'}, {KEY_P, 'P'},
        {KEY_LEFTBRACE,  VK_OEM_4},
        {KEY_RIGHTBRACE, VK_OEM_6},
        {KEY_ENTER,      VK_RETURN},
        {KEY_LEFTCTRL,   VK_LCONTROL},
        {KEY_A, 'A'}, {KEY_S, 'S'}, {KEY_D, 'D'}, {KEY_F, 'F'}, {KEY_G, 'G'},
        {KEY_H, 'H'}, {KEY_J, 'J'}, {KEY_K, 'K'}, {KEY_L, 'L'},
        {KEY_SEMICOLON,  VK_OEM_1},
        {KEY_APOSTROPHE, VK_OEM_7},
        {KEY_GRAVE,      VK_OEM_3},
        {KEY_LEFTSHIFT,  VK_LSHIFT},
        {KEY_BACKSLASH,  VK_OEM_5},
        {KEY_Z, 'Z'}, {KEY_X, 'X'}, {KEY_C, 'C'}, {KEY_V, 'V'}, {KEY_B, 'B'},
        {KEY_N, 'N'}, {KEY_M, 'M'},
        {KEY_COMMA,      VK_OEM_COMMA},
        {KEY_DOT,        VK_OEM_PERIOD},
        {KEY_SLASH,      VK_OEM_2},
        {KEY_RIGHTSHIFT, VK_RSHIFT},
        {KEY_KPASTERISK, VK_MULTIPLY},
        {KEY_LEFTALT,    VK_LMENU},
        {KEY_SPACE,      VK_SPACE},
        {KEY_CAPSLOCK,   VK_CAPITAL},
        {KEY_F1,  VK_F1},  {KEY_F2,  VK_F2},  {KEY_F3,  VK_F3},  {KEY_F4,  VK_F4},
        {KEY_F5,  VK_F5},  {KEY_F6,  VK_F6},  {KEY_F7,  VK_F7},  {KEY_F8,  VK_F8},
        {KEY_F9,  VK_F9},  {KEY_F10, VK_F10},
        {KEY_NUMLOCK,    VK_NUMLOCK},
        {KEY_SCROLLLOCK, VK_SCROLL},
        {KEY_KP7,  VK_NUMPAD7}, {KEY_KP8, VK_NUMPAD8}, {KEY_KP9, VK_NUMPAD9},
        {KEY_KPMINUS,    VK_SUBTRACT},
        {KEY_KP4,  VK_NUMPAD4}, {KEY_KP5, VK_NUMPAD5}, {KEY_KP6, VK_NUMPAD6},
        {KEY_KPPLUS,     VK_ADD},
        {KEY_KP1,  VK_NUMPAD1}, {KEY_KP2, VK_NUMPAD2}, {KEY_KP3, VK_NUMPAD3},
        {KEY_KP0,  VK_NUMPAD0},
        {KEY_KPDOT,      VK_DECIMAL},
        {KEY_F11, VK_F11}, {KEY_F12, VK_F12},
        {KEY_KPENTER,    VK_RETURN},
        {KEY_RIGHTCTRL,  VK_RCONTROL},
        {KEY_KPSLASH,    VK_DIVIDE},
        {KEY_SYSRQ,      VK_SNAPSHOT},
        {KEY_RIGHTALT,   VK_RMENU},
        {KEY_HOME,       VK_HOME},
        {KEY_UP,         VK_UP},
        {KEY_PAGEUP,     VK_PRIOR},
        {KEY_LEFT,       VK_LEFT},
        {KEY_RIGHT,      VK_RIGHT},
        {KEY_END,        VK_END},
        {KEY_DOWN,       VK_DOWN},
        {KEY_PAGEDOWN,   VK_NEXT},
        {KEY_INSERT,     VK_INSERT},
        {KEY_DELETE,     VK_DELETE},
        {KEY_LEFTMETA,   VK_LWIN},
        {KEY_RIGHTMETA,  VK_RWIN},
        {KEY_PAUSE,      VK_PAUSE},
        {KEY_F13, VK_F13}, {KEY_F14, VK_F14}, {KEY_F15, VK_F15}, {KEY_F16, VK_F16},
        {KEY_F17, VK_F17}, {KEY_F18, VK_F18}, {KEY_F19, VK_F19}, {KEY_F20, VK_F20},
        {KEY_F21, VK_F21}, {KEY_F22, VK_F22}, {KEY_F23, VK_F23}, {KEY_F24, VK_F24},
    };
    return table;
}

static bool IsExtendedEvdev(uint32_t evdevCode) {
    return evdevCode == KEY_KPENTER || evdevCode == KEY_RIGHTCTRL ||
           evdevCode == KEY_RIGHTALT || evdevCode == KEY_KPSLASH ||
           evdevCode == KEY_SYSRQ || evdevCode == KEY_HOME ||
           evdevCode == KEY_UP || evdevCode == KEY_PAGEUP ||
           evdevCode == KEY_LEFT || evdevCode == KEY_RIGHT ||
           evdevCode == KEY_END || evdevCode == KEY_DOWN ||
           evdevCode == KEY_PAGEDOWN || evdevCode == KEY_INSERT ||
           evdevCode == KEY_DELETE;
}

struct GrabbedDevice {
    int fd = -1;
    std::string path;
};

static std::vector<GrabbedDevice> g_devices;
static std::mutex g_devicesMutex;

static int g_uinputFd = -1;
static int g_inotifyFd = -1;
static int g_inotifyWd = -1;
static int g_wakeupPipe[2] = {-1, -1};

static std::function<void()> g_reconnectCallback;

static std::atomic<bool> g_numlockOn{true};

struct RepeatKey {
    uint32_t evdevCode{0};
    uint32_t vkCode{0};
    uint16_t scanCode{0};
    bool extended{false};
    bool isRemote{false};
    bool active{false};
};
static RepeatKey g_repeatKey{};

static void InjectEvdevEvent(uint32_t evdevCode, int value);

static bool IsModifierEvdev(uint32_t code) {
    switch (code) {
    case KEY_LEFTCTRL: case KEY_RIGHTCTRL:
    case KEY_LEFTALT:  case KEY_RIGHTALT:
    case KEY_LEFTSHIFT: case KEY_RIGHTSHIFT:
    case KEY_LEFTMETA: case KEY_RIGHTMETA:
        return true;
    default: return false;
    }
}

static bool SetupUinput() {
    g_uinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (g_uinputFd < 0) {
        DEBUG_ERROR("LKG", "Failed to open /dev/uinput");
        return false;
    }

    ioctl(g_uinputFd, UI_SET_EVBIT, EV_KEY);
    ioctl(g_uinputFd, UI_SET_EVBIT, EV_SYN);
    ioctl(g_uinputFd, UI_SET_EVBIT, EV_REP);

    for (int i = 0; i < KEY_MAX; i++)
        ioctl(g_uinputFd, UI_SET_KEYBIT, i);

    struct uinput_setup usetup = {};
    usetup.id.bustype = BUS_VIRTUAL;
    usetup.id.vendor  = 0x1234;
    usetup.id.product = 0x5678;
    strncpy(usetup.name, "NVDARemoteCompanion Virtual KB", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(g_uinputFd, UI_DEV_SETUP, &usetup) < 0) {
        DEBUG_ERROR("LKG", "UI_DEV_SETUP failed");
        close(g_uinputFd);
        g_uinputFd = -1;
        return false;
    }
    if (ioctl(g_uinputFd, UI_DEV_CREATE) < 0) {
        DEBUG_ERROR("LKG", "UI_DEV_CREATE failed");
        close(g_uinputFd);
        g_uinputFd = -1;
        return false;
    }

    {
        input_event rep = {};
        rep.type = EV_REP;
        rep.code = REP_DELAY;
        rep.value = 250;
        write(g_uinputFd, &rep, sizeof(rep));
        rep.code = REP_PERIOD;
        rep.value = 30;
        write(g_uinputFd, &rep, sizeof(rep));
    }

    DEBUG_INFO("LKG", "uinput virtual device created");
    return true;
}

static void TeardownUinput() {
    if (g_uinputFd >= 0) {
        ioctl(g_uinputFd, UI_DEV_DESTROY);
        close(g_uinputFd);
        g_uinputFd = -1;
        DEBUG_INFO("LKG", "uinput virtual device destroyed");
    }
}

static void InjectEvdevEvent(uint32_t evdevCode, int value) {
    if (g_uinputFd < 0) return;

    input_event ev = {};
    ev.type  = EV_KEY;
    ev.code  = static_cast<uint16_t>(evdevCode);
    ev.value = value;
    write(g_uinputFd, &ev, sizeof(ev));

    input_event syn = {};
    syn.type  = EV_SYN;
    syn.code  = SYN_REPORT;
    syn.value = 0;
    write(g_uinputFd, &syn, sizeof(syn));
}

static bool IsPhysicalKeyboard(int fd) {
    uint8_t evBits[(EV_MAX + 7) / 8] = {};
    if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0) return false;
    if (!(evBits[EV_KEY / 8] & (1 << (EV_KEY % 8)))) return false;

    uint8_t keyBits[(KEY_MAX + 7) / 8] = {};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) < 0) return false;
    if (!(keyBits[KEY_A / 8] & (1 << (KEY_A % 8)))) return false;

    char phys[256] = {};
    if (ioctl(fd, EVIOCGPHYS(sizeof(phys)), phys) < 0 || phys[0] == '\0') return false;

    char name[256] = {};
    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
        if (strstr(name, "NVDARemoteCompanion") != nullptr) return false;
    }

    return true;
}

static void GrabDevice(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) return;

    if (!IsPhysicalKeyboard(fd)) {
        close(fd);
        return;
    }

    if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        DEBUG_WARN_F("LKG", "Failed to grab {}", path);
        close(fd);
        return;
    }

    uint8_t leds[(LED_MAX + 7) / 8] = {};
    if (ioctl(fd, EVIOCGLED(sizeof(leds)), leds) >= 0) {
        bool numlock = (leds[LED_NUML / 8] >> (LED_NUML % 8)) & 1;
        g_numlockOn.store(numlock, std::memory_order_relaxed);
    }

    std::lock_guard<std::mutex> lock(g_devicesMutex);
    g_devices.push_back({fd, path});
    DEBUG_INFO_F("LKG", "Grabbed keyboard: {}", path);
}

static void UngrabAll() {
    std::lock_guard<std::mutex> lock(g_devicesMutex);
    for (auto& dev : g_devices) {
        if (dev.fd >= 0) {
            ioctl(dev.fd, EVIOCGRAB, 0);
            close(dev.fd);
        }
    }
    g_devices.clear();
    DEBUG_INFO("LKG", "All keyboards ungrabbed");
}

static void ScanAndGrabDevices() {
    DIR* dir = opendir("/dev/input");
    if (!dir) return;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.rfind("event", 0) != 0) continue;
        GrabDevice("/dev/input/" + name);
    }
    closedir(dir);
}

static bool IsNumpadDigitOrDot(uint32_t evdevCode) {
    switch (evdevCode) {
    case KEY_KP0: case KEY_KP1: case KEY_KP2: case KEY_KP3:
    case KEY_KP4: case KEY_KP5: case KEY_KP6: case KEY_KP7:
    case KEY_KP8: case KEY_KP9: case KEY_KPDOT:
        return true;
    default:
        return false;
    }
}

static uint32_t NumpadOffVk(uint32_t evdevCode) {
    switch (evdevCode) {
    case KEY_KP0:   return VK_INSERT;
    case KEY_KP1:   return VK_END;
    case KEY_KP2:   return VK_DOWN;
    case KEY_KP3:   return VK_NEXT;
    case KEY_KP4:   return VK_LEFT;
    case KEY_KP5:   return 0x0C;
    case KEY_KP6:   return VK_RIGHT;
    case KEY_KP7:   return VK_HOME;
    case KEY_KP8:   return VK_UP;
    case KEY_KP9:   return VK_PRIOR;
    case KEY_KPDOT: return VK_DELETE;
    default:        return 0;
    }
}

static void HandleKeyEvent(uint32_t evdevCode, int value) {
    if (value == 2) return;

    if (evdevCode == KEY_NUMLOCK && value == 1) {
        g_numlockOn.store(!g_numlockOn.load(std::memory_order_relaxed),
                          std::memory_order_relaxed);
    }

    const auto& table = EvdevToVk();
    auto it = table.find(evdevCode);
    if (it == table.end()) {
        InjectEvdevEvent(evdevCode, value);
        return;
    }

    uint32_t vkCode  = it->second;
    bool isPressed   = (value == 1);
    uint16_t scanCode = static_cast<uint16_t>(evdevCode);
    bool extended    = IsExtendedEvdev(evdevCode);

    if (IsNumpadDigitOrDot(evdevCode) && !g_numlockOn.load(std::memory_order_relaxed)) {
        uint32_t navVk = NumpadOffVk(evdevCode);
        if (navVk != 0) {
            vkCode = navVk;
            extended = false;
        }
    }

    if (isPressed) {
        KeyboardState::UpdateModifierState(vkCode, true);

        if (KeyboardState::CheckExitShortcut(vkCode)) {
            g_shutdown = true;
            g_repeatKey.active = false;
            if (g_wakeupPipe[1] >= 0) { char b = 'q'; write(g_wakeupPipe[1], &b, 1); }
            KeyboardState::ResetModifiers();
            return;
        }
        if (KeyboardState::CheckLocalShortcut(vkCode)) {
            g_repeatKey.active = false;
            AppState::GoLocal();
            KeyboardState::ResetModifiers();
            return;
        }
        if (KeyboardState::CheckReconnectShortcut(vkCode)) {
            g_repeatKey.active = false;
            KeyboardState::ResetModifiers();
            if (g_reconnectCallback) g_reconnectCallback();
            return;
        }
        if (KeyboardState::CheckCycleShortcut(vkCode)) {
            g_repeatKey.active = false;
            AppState::CycleProfile();
            KeyboardState::ResetModifiers();
            return;
        }
        int profileIndex = KeyboardState::CheckToggleShortcut(vkCode);
        if (profileIndex >= 0) {
            g_repeatKey.active = false;
            AppState::ToggleSendingKeys(profileIndex);
            KeyboardState::ResetModifiers();
            return;
        }

        if (AppState::IsSendingKeys() || AppState::IsReleasingKeys()) {
            if (AppState::IsSendingKeys()) {
                KeyboardState::TrackKeyPress(vkCode, scanCode, extended);
                KeyEvent keyEvent(vkCode, true, scanCode, extended);
                MessageSender::SendKeyEvent(keyEvent);
                g_repeatKey = {evdevCode, vkCode, scanCode, extended, true, true};
            }
            return;
        }

        g_repeatKey = {evdevCode, vkCode, scanCode, extended, false, true};
        InjectEvdevEvent(evdevCode, 1);
    } else {
        KeyboardState::UpdateModifierState(vkCode, false);

        if (AppState::IsSendingKeys() || AppState::IsReleasingKeys()) {
            bool isTracked = false;
            for (const auto& pk : KeyboardState::GetAllPressedKeys()) {
                if (pk.vkCode == vkCode) { isTracked = true; break; }
            }
            if (isTracked) {
                if (g_repeatKey.evdevCode == evdevCode) g_repeatKey.active = false;
                KeyboardState::TrackKeyRelease(vkCode);
                KeyEvent keyEvent(vkCode, false, scanCode, extended);
                MessageSender::SendKeyEvent(keyEvent);
                return;
            }
        }

        if (g_repeatKey.evdevCode == evdevCode) g_repeatKey.active = false;
        InjectEvdevEvent(evdevCode, 0);
    }
}

bool LinuxKeyboardGrab::Install() {
    if (pipe2(g_wakeupPipe, O_NONBLOCK) < 0) {
        DEBUG_ERROR("LKG", "Failed to create wakeup pipe");
        return false;
    }

    if (!SetupUinput()) return false;

    usleep(100'000);

    ScanAndGrabDevices();

    g_inotifyFd = inotify_init1(IN_NONBLOCK);
    if (g_inotifyFd >= 0) {
        g_inotifyWd = inotify_add_watch(g_inotifyFd, "/dev/input", IN_CREATE | IN_DELETE);
    }

    DEBUG_INFO("LKG", "Linux keyboard grab installed");
    return true;
}

void LinuxKeyboardGrab::Uninstall() {
    g_repeatKey.active = false;

    UngrabAll();

    if (g_inotifyFd >= 0) {
        if (g_inotifyWd >= 0) { inotify_rm_watch(g_inotifyFd, g_inotifyWd); g_inotifyWd = -1; }
        close(g_inotifyFd);
        g_inotifyFd = -1;
    }

    TeardownUinput();

    for (int fd : g_wakeupPipe) {
        if (fd >= 0) close(fd);
    }
    g_wakeupPipe[0] = g_wakeupPipe[1] = -1;

    DEBUG_INFO("LKG", "Linux keyboard grab uninstalled");
}

void LinuxKeyboardGrab::Reinstall() {
    DEBUG_INFO("LKG", "Reinstalling Linux keyboard grab");
    UngrabAll();
    usleep(100'000);
    ScanAndGrabDevices();
    DEBUG_INFO("LKG", "Linux keyboard grab reinstalled");
}

void LinuxKeyboardGrab::NotifyConnectionLost() {
    if (g_wakeupPipe[1] >= 0) {
        char b = 'c';
        write(g_wakeupPipe[1], &b, 1);
    }
}

void LinuxKeyboardGrab::SetReconnectCallback(std::function<void()> callback) {
    g_reconnectCallback = std::move(callback);
}

void LinuxKeyboardGrab::RunMessageLoop() {
    DEBUG_INFO("LKG", "Starting Linux keyboard grab message loop");

    using Clock = std::chrono::steady_clock;
    uint32_t lastRepeatCode = 0;
    Clock::time_point repeatNext;
    bool repeatDelayDone = false;

    while (!g_shutdown) {
        std::vector<pollfd> fds;

        fds.push_back({g_wakeupPipe[0], POLLIN, 0});

        if (g_inotifyFd >= 0) {
            fds.push_back({g_inotifyFd, POLLIN, 0});
        }

        {
            std::lock_guard<std::mutex> lock(g_devicesMutex);
            for (const auto& dev : g_devices) {
                fds.push_back({dev.fd, POLLIN, 0});
            }
        }

        int pollTimeoutMs = g_repeatKey.active ? 30 : 500;
        int ret = poll(fds.data(), static_cast<nfds_t>(fds.size()), pollTimeoutMs);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (fds[0].revents & POLLIN) {
            char buf[16];
            read(g_wakeupPipe[0], buf, sizeof(buf));
            DEBUG_INFO("LKG", "Wakeup pipe triggered, exiting message loop");
            break;
        }

        size_t devBase = (g_inotifyFd >= 0) ? 2 : 1;
        std::vector<int> deadFds;

        for (size_t fi = devBase; fi < fds.size(); fi++) {
            if (fds[fi].revents & (POLLHUP | POLLERR)) {
                deadFds.push_back(fds[fi].fd);
                continue;
            }
            if (!(fds[fi].revents & POLLIN)) continue;

            input_event events[32];
            ssize_t n = read(fds[fi].fd, events, sizeof(events));
            if (n <= 0) {
                deadFds.push_back(fds[fi].fd);
                continue;
            }

            for (ssize_t ei = 0; ei < static_cast<ssize_t>(n / sizeof(input_event)); ei++) {
                const auto& ev = events[ei];
                if (ev.type == EV_KEY) {
                    HandleKeyEvent(ev.code, ev.value);
                }
            }
        }

        if (!deadFds.empty()) {
            {
                std::lock_guard<std::mutex> lock(g_devicesMutex);
                for (int deadFd : deadFds) {
                    for (auto it = g_devices.begin(); it != g_devices.end(); ++it) {
                        if (it->fd == deadFd) {
                            close(it->fd);
                            g_devices.erase(it);
                            DEBUG_INFO("LKG", "Removed dead keyboard fd");
                            break;
                        }
                    }
                }
            }
            g_repeatKey.active = false;
            KeyboardState::ResetModifiers();
        }

        {
            auto now = Clock::now();
            uint32_t code = g_repeatKey.active ? g_repeatKey.evdevCode : 0;
            if (code != lastRepeatCode) {
                lastRepeatCode = code;
                repeatDelayDone = false;
                if (code != 0)
                    repeatNext = now + std::chrono::milliseconds(500);
            }
            if (code != 0 && now >= repeatNext) {
                repeatDelayDone = true;
                repeatNext = now + std::chrono::milliseconds(30);
                if (g_repeatKey.isRemote) {
                    KeyEvent keyEvent(g_repeatKey.vkCode, true, g_repeatKey.scanCode, g_repeatKey.extended);
                    MessageSender::SendKeyEvent(keyEvent);
                } else {
                    InjectEvdevEvent(code, 0);
                    InjectEvdevEvent(code, 1);
                }
            }
        }

        size_t inotifyIdx = (g_inotifyFd >= 0) ? 1 : SIZE_MAX;
        if (g_inotifyFd >= 0 && inotifyIdx < fds.size() && (fds[inotifyIdx].revents & POLLIN)) {
            char buf[4096] __attribute__((aligned(__alignof__(inotify_event))));
            ssize_t len = read(g_inotifyFd, buf, sizeof(buf));
            for (ssize_t i = 0; i < len; ) {
                auto* ev = reinterpret_cast<inotify_event*>(buf + i);
                if (ev->len > 0) {
                    std::string name(ev->name);
                    if (name.rfind("event", 0) == 0) {
                        if (ev->mask & IN_CREATE) {
                            usleep(200'000);
                            GrabDevice("/dev/input/" + name);
                        } else if (ev->mask & IN_DELETE) {
                            bool removed = false;
                            {
                                std::lock_guard<std::mutex> lock(g_devicesMutex);
                                auto path = "/dev/input/" + name;
                                for (auto it = g_devices.begin(); it != g_devices.end(); ++it) {
                                    if (it->path == path) {
                                        close(it->fd);
                                        g_devices.erase(it);
                                        DEBUG_INFO_F("LKG", "Removed disconnected keyboard: {}", path);
                                        removed = true;
                                        break;
                                    }
                                }
                            }
                            if (removed) KeyboardState::ResetModifiers();
                        }
                    }
                }
                i += sizeof(inotify_event) + ev->len;
            }
        }
    }

    DEBUG_INFO("LKG", "Linux keyboard grab message loop ended");
}

#endif
