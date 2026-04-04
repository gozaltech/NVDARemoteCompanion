#pragma once
#include <functional>
#include <atomic>
#include <cstdint>

extern std::atomic<bool> g_shutdown;

class KeyboardHandler {
public:
    virtual ~KeyboardHandler() = default;

    virtual bool Install() = 0;
    virtual void Uninstall() = 0;
    virtual void RunMessageLoop() = 0;
    virtual void NotifyConnectionLost() = 0;

    virtual void Reinstall();

    void SetReconnectCallback(std::function<void()> callback);

    bool HandleShortcut(uint32_t vkCode);

protected:
    std::function<void()> m_reconnectCallback;

    virtual void OnExit() = 0;
    virtual void OnReinstallHook() {}
    virtual void OnClipboardShortcut() {}
};
