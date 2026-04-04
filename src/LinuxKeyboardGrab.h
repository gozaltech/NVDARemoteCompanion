#pragma once
#ifndef _WIN32

#include "KeyboardHandler.h"

class LinuxKeyboardGrab : public KeyboardHandler {
public:
    bool Install() override;
    void Uninstall() override;
    void Reinstall() override;
    void RunMessageLoop() override;
    void NotifyConnectionLost() override;

protected:
    void OnExit() override;
    void OnClipboardShortcut() override;
};

#endif
