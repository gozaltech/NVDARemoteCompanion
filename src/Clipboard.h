#pragma once
#include <string>

class Clipboard {
public:
    static std::string GetText();
    static void SetText(const std::string& text);
};
