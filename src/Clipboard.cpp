#include "Clipboard.h"

#ifdef _WIN32
#include <windows.h>

std::string Clipboard::GetText() {
    if (!OpenClipboard(nullptr)) return {};
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) { CloseClipboard(); return {}; }
    wchar_t* pwstr = static_cast<wchar_t*>(GlobalLock(hData));
    if (!pwstr) { CloseClipboard(); return {}; }
    int size = WideCharToMultiByte(CP_UTF8, 0, pwstr, -1, nullptr, 0, nullptr, nullptr);
    std::string result;
    if (size > 1) {
        result.resize(size - 1);
        WideCharToMultiByte(CP_UTF8, 0, pwstr, -1, result.data(), size, nullptr, nullptr);
    }
    GlobalUnlock(hData);
    CloseClipboard();
    return result;
}

void Clipboard::SetText(const std::string& text) {
    int wsize = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wsize <= 0) return;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, static_cast<SIZE_T>(wsize) * sizeof(wchar_t));
    if (!hMem) return;
    wchar_t* pwstr = static_cast<wchar_t*>(GlobalLock(hMem));
    if (!pwstr) { GlobalFree(hMem); return; }
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pwstr, wsize);
    GlobalUnlock(hMem);
    if (!OpenClipboard(nullptr)) { GlobalFree(hMem); return; }
    EmptyClipboard();
    if (!SetClipboardData(CF_UNICODETEXT, hMem)) GlobalFree(hMem);
    CloseClipboard();
}

#else

#include <cstdio>
#include <array>

static std::string RunPipeOut(const char* cmd) {
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return result;
    std::array<char, 512> buf;
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        result += buf.data();
    pclose(pipe);
    return result;
}

static bool RunPipeIn(const char* cmd, const std::string& data) {
    FILE* pipe = popen(cmd, "w");
    if (!pipe) return false;
    fwrite(data.c_str(), 1, data.size(), pipe);
    return pclose(pipe) == 0;
}

std::string Clipboard::GetText() {
    std::string result = RunPipeOut("xclip -selection clipboard -o 2>/dev/null");
    if (result.empty()) result = RunPipeOut("xsel --clipboard --output 2>/dev/null");
    return result;
}

void Clipboard::SetText(const std::string& text) {
    if (!RunPipeIn("xclip -selection clipboard 2>/dev/null", text))
        RunPipeIn("xsel --clipboard --input 2>/dev/null", text);
}

#endif
