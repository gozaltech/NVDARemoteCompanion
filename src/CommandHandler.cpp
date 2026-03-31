#include "CommandHandler.h"
#include "Debug.h"
#include "Config.h"
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include "MessageSender.h"
#include "KeyboardState.h"
#include "KeyboardHook.h"
#include "AppState.h"
#include <windows.h>
#include <conio.h>
extern DWORD g_mainThreadId;
#else
#include <unistd.h>
#include <sys/select.h>
#endif

CommandHandler::CommandHandler(std::string configPath, ConfigFileData configData)
    : m_configPath(std::move(configPath)), m_configData(std::move(configData)) {
    for (auto& profile : m_configData.profiles) {
        ProfileSession session;
        session.config = profile;
        m_sessions.push_back(std::move(session));
    }
}

void CommandHandler::SetDisconnectCallbackFactory(std::function<void(ConnectionManager&)> factory) {
    m_disconnectCallbackFactory = std::move(factory);
}

int CommandHandler::ConnectAutoProfiles() {
    int connected = 0;
    for (int i = 0; i < static_cast<int>(m_sessions.size()); i++) {
        if (m_sessions[i].config.autoConnect &&
            !m_sessions[i].config.host.empty() &&
            !m_sessions[i].config.key.empty()) {
            ConnectSession(i);
            if (m_sessions[i].connection && m_sessions[i].connection->IsConnected()) {
                connected++;
            }
        }
    }
    RebuildShortcuts();
    return connected;
}

void CommandHandler::ConnectSession(int index) {
    if (index < 0 || index >= static_cast<int>(m_sessions.size())) return;
    auto& session = m_sessions[index];

    if (session.connection && session.connection->IsConnected()) {
        return;
    }

    const auto& p = session.config;
    if (p.host.empty() || p.key.empty()) {
        std::cout << "Profile '" << p.name << "' has no host or key configured" << std::endl;
        return;
    }

    std::cout << "Connecting to " << p.name << " (" << p.host << ":" << p.port << ")..." << std::endl;

    session.connection = std::make_unique<ConnectionManager>();
    session.connection->SetSpeechEnabled(p.speech);
    session.connection->SetMuteOnLocalControl(p.muteOnLocalControl);
    if (m_disconnectCallbackFactory) {
        m_disconnectCallbackFactory(*session.connection);
    }

    if (session.connection->EstablishConnection(p.host, p.port, p.key, p.shortcut)) {
        std::cout << "Connected to " << p.name << std::endl;
    } else {
        std::cerr << "Failed to connect to " << p.name << std::endl;
        session.connection.reset();
    }
}

void CommandHandler::DisconnectSession(int index) {
    if (index < 0 || index >= static_cast<int>(m_sessions.size())) return;
    auto& session = m_sessions[index];
    if (session.connection) {
        std::cout << "Disconnecting " << session.config.name << "..." << std::endl;
        session.connection->SetDisconnectCallback(nullptr);
        session.connection.reset();
        std::cout << "Disconnected " << session.config.name << std::endl;
    }
}

void CommandHandler::RebuildShortcuts() {
#ifdef _WIN32
    KeyboardState::ClearShortcuts();
    int shortcutIdx = 0;
    std::vector<int> connectedIndices;
    std::vector<std::string> connectedNames;

    for (int i = 0; i < static_cast<int>(m_sessions.size()); i++) {
        auto& session = m_sessions[i];
        if (session.connection && session.connection->IsConnected()) {
            if (!session.config.shortcut.empty()) {
                KeyboardState::SetToggleShortcutAt(shortcutIdx, session.config.shortcut);
            }
            MessageSender::SetNetworkClient(shortcutIdx, session.connection->GetClient());
            session.connection->SetProfileIndex(shortcutIdx);
            session.shortcutIndex = shortcutIdx;
            connectedIndices.push_back(shortcutIdx);
            connectedNames.push_back(session.config.name);
            shortcutIdx++;
        } else {
            session.shortcutIndex = -1;
        }
    }

    AppState::SetConnectedProfiles(connectedIndices, connectedNames);
#endif
}

void CommandHandler::UpdateNetworkClients() {
#ifdef _WIN32
    for (auto& session : m_sessions) {
        if (session.shortcutIndex >= 0 && session.connection) {
            MessageSender::SetNetworkClient(session.shortcutIndex, session.connection->GetClient());
        }
    }
#endif
}

void CommandHandler::ReconnectAll() {
    bool changed = false;
    for (int i = 0; i < static_cast<int>(m_sessions.size()); i++) {
        auto& session = m_sessions[i];
        if (session.connection && !session.connection->IsConnected()) {
            DEBUG_INFO_F("MAIN", "Reconnecting profile '{}'...", session.config.name);
            if (session.connection->Reconnect()) {
                DEBUG_INFO_F("MAIN", "Profile '{}' reconnected", session.config.name);
                changed = true;
            }
        }
    }
    if (changed) {
        RebuildShortcuts();
    }
}

void CommandHandler::RunCommandLoop() {
    std::string line;
    std::cout << "> " << std::flush;
    while (!g_shutdown) {
#ifdef _WIN32
        if (_kbhit()) {
            int ch = _getch();
            if (ch == '\r') {
                std::cout << std::endl;
                if (!line.empty()) {
                    HandleCommand(line);
                    line.clear();
                }
                if (!g_shutdown) std::cout << "> " << std::flush;
            } else if (ch == '\b' && !line.empty()) {
                line.pop_back();
                std::cout << "\b \b" << std::flush;
            } else if (ch == 3) {
                g_shutdown = true;
                break;
            } else if (ch >= 32 && ch <= 126) {
                line += static_cast<char>(ch);
                std::cout << static_cast<char>(ch) << std::flush;
            }
        } else {
            Sleep(10);
        }
#else
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        int ready = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &timeout);
        if (ready > 0) {
            if (std::getline(std::cin, line)) {
                if (!line.empty()) {
                    HandleCommand(line);
                    line.clear();
                }
                if (!g_shutdown) std::cout << "> " << std::flush;
            } else {
                break;
            }
        }
#endif
    }
}

void CommandHandler::HandleCommand(const std::string& line) {
    std::string trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
    if (trimmed.empty()) return;

    std::string cmd, args;
    auto spacePos = trimmed.find(' ');
    if (spacePos != std::string::npos) {
        cmd = trimmed.substr(0, spacePos);
        args = trimmed.substr(spacePos + 1);
        args.erase(0, args.find_first_not_of(" \t"));
    } else {
        cmd = trimmed;
    }
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "status") CmdStatus();
    else if (cmd == "list" || cmd == "ls") CmdList();
    else if (cmd == "connect" || cmd == "c") CmdConnect(args);
    else if (cmd == "disconnect" || cmd == "dc") CmdDisconnect(args);
    else if (cmd == "add") CmdAdd(args);
    else if (cmd == "edit") CmdEdit(args);
    else if (cmd == "delete" || cmd == "rm") CmdDelete(args);
    else if (cmd == "help" || cmd == "?") CmdHelp();
    else if (cmd == "reinstall-hook" || cmd == "hook") CmdReinstallHook();
    else if (cmd == "quit" || cmd == "exit") {
        g_shutdown = true;
#ifdef _WIN32
        if (g_mainThreadId != 0) {
            PostThreadMessage(g_mainThreadId, WM_QUIT, 0, 0);
        }
#endif
    }
    else {
        std::cout << "Unknown command: " << cmd << ". Type 'help' for available commands." << std::endl;
    }
}

void CommandHandler::CmdStatus() {
    int connected = 0, total = static_cast<int>(m_sessions.size());
    for (const auto& s : m_sessions) {
        if (s.connection && s.connection->IsConnected()) connected++;
    }
    std::cout << "Profiles: " << total << " total, " << connected << " connected" << std::endl;
    if (!m_configPath.empty()) {
        std::cout << "Config: " << m_configPath << std::endl;
    }
    for (int i = 0; i < total; i++) {
        const auto& s = m_sessions[i];
        bool isConnected = s.connection && s.connection->IsConnected();
        std::cout << "  [" << i << "] " << s.config.name
                  << " - " << s.config.host << ":" << s.config.port
                  << " - " << (isConnected ? "CONNECTED" : "DISCONNECTED")
                  << (s.config.autoConnect ? "" : " (manual)")
                  << std::endl;
    }
}

void CommandHandler::CmdList() {
    if (m_sessions.empty()) {
        std::cout << "No profiles configured" << std::endl;
        return;
    }
    for (int i = 0; i < static_cast<int>(m_sessions.size()); i++) {
        const auto& p = m_sessions[i].config;
        std::cout << "  [" << i << "] " << p.name
                  << " | " << p.host << ":" << p.port
                  << " | key=" << p.key
                  << " | shortcut=" << (p.shortcut.empty() ? "ctrl+win+f11" : p.shortcut)
                  << " | auto_connect=" << (p.autoConnect ? "yes" : "no")
                  << " | speech=" << (p.speech ? "yes" : "no")
                  << " | mute_on_local_control=" << (p.muteOnLocalControl ? "yes" : "no")
                  << std::endl;
    }
}

void CommandHandler::CmdConnect(const std::string& args) {
    if (args.empty()) {
        for (int i = 0; i < static_cast<int>(m_sessions.size()); i++) {
            if (!(m_sessions[i].connection && m_sessions[i].connection->IsConnected())) {
                ConnectSession(i);
            }
        }
        RebuildShortcuts();
        return;
    }
    int idx = FindProfileIndex(args);
    if (idx < 0) {
        std::cout << "Profile not found: " << args << std::endl;
        return;
    }
    ConnectSession(idx);
    RebuildShortcuts();
}

void CommandHandler::CmdDisconnect(const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: disconnect <name or index>" << std::endl;
        return;
    }
    int idx = FindProfileIndex(args);
    if (idx < 0) {
        std::cout << "Profile not found: " << args << std::endl;
        return;
    }
    DisconnectSession(idx);
    RebuildShortcuts();
}

bool CommandHandler::PromptLine(const std::string& prompt, std::string& out, const std::string& defaultValue) {
    if (!defaultValue.empty())
        std::cout << prompt << " [" << defaultValue << "]: " << std::flush;
    else
        std::cout << prompt << ": " << std::flush;

    std::string line;
#ifdef _WIN32
    while (!g_shutdown) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == '\r') {
                std::cout << std::endl;
                break;
            } else if (ch == '\b' && !line.empty()) {
                line.pop_back();
                std::cout << "\b \b" << std::flush;
            } else if (ch == 3) {
                std::cout << std::endl;
                g_shutdown = true;
                return false;
            } else if (ch == 27) {
                std::cout << std::endl;
                return false;
            } else if (ch >= 32 && ch <= 126) {
                line += static_cast<char>(ch);
                std::cout << static_cast<char>(ch) << std::flush;
            }
        } else {
            Sleep(10);
        }
    }
    if (g_shutdown) return false;
#else
    if (!std::getline(std::cin, line)) {
        return false;
    }
#endif
    if (line.empty() && !defaultValue.empty())
        line = defaultValue;
    out = line;
    return true;
}

void CommandHandler::CmdAdd(const std::string& args) {
    if (!args.empty()) {
        std::istringstream ss(args);
        std::string name, host, key;
        ss >> name >> host >> key;

        if (name.empty() || host.empty() || key.empty()) {
            std::cout << "Usage: add <name> <host> <key> [port] [shortcut] [auto_connect]" << std::endl;
            std::cout << "       add  (no arguments for interactive wizard)" << std::endl;
            return;
        }

        ProfileConfig p;
        p.name = name;
        p.host = host;
        p.key = key;

        std::string token;
        if (ss >> token) {
            try { p.port = std::stoi(token); } catch (...) { p.port = Config::DEFAULT_PORT; }
        }
        if (ss >> token) p.shortcut = token;
        if (ss >> token) {
            std::transform(token.begin(), token.end(), token.begin(), ::tolower);
            p.autoConnect = (token == "true" || token == "yes" || token == "1");
        }

        ProfileSession session;
        session.config = p;
        m_sessions.push_back(std::move(session));
        m_configData.profiles.push_back(p);
        SaveConfig();
        std::cout << "Profile added: [" << (m_sessions.size() - 1) << "] " << p.name << std::endl;
        return;
    }

    std::cout << "Adding new profile (press Escape to cancel)" << std::endl;

    ProfileConfig p;
    std::string input;

    if (!PromptLine("Name", input)) { std::cout << "Cancelled." << std::endl; return; }
    if (input.empty()) { std::cout << "Name is required." << std::endl; return; }
    p.name = input;

    if (!PromptLine("Host", input)) { std::cout << "Cancelled." << std::endl; return; }
    if (input.empty()) { std::cout << "Host is required." << std::endl; return; }
    p.host = input;

    if (!PromptLine("Port", input, std::to_string(Config::DEFAULT_PORT))) { std::cout << "Cancelled." << std::endl; return; }
    try { p.port = input.empty() ? Config::DEFAULT_PORT : std::stoi(input); }
    catch (...) { std::cout << "Invalid port, using default." << std::endl; p.port = Config::DEFAULT_PORT; }

    if (!PromptLine("Key", input)) { std::cout << "Cancelled." << std::endl; return; }
    if (input.empty()) { std::cout << "Key is required." << std::endl; return; }
    p.key = input;

    if (!PromptLine("Shortcut (optional, e.g. ctrl+win+f12)", input, "")) { std::cout << "Cancelled." << std::endl; return; }
    p.shortcut = input;

    if (!PromptLine("Auto connect (y/n)", input, "y")) { std::cout << "Cancelled." << std::endl; return; }
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
    p.autoConnect = (input == "y" || input == "yes" || input == "1" || input.empty());

    if (!PromptLine("Speech (y/n)", input, "y")) { std::cout << "Cancelled." << std::endl; return; }
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
    p.speech = (input == "y" || input == "yes" || input == "1" || input.empty());

    if (!PromptLine("Mute on local control (y/n)", input, "n")) { std::cout << "Cancelled." << std::endl; return; }
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
    p.muteOnLocalControl = (input == "y" || input == "yes" || input == "1");

    ProfileSession session;
    session.config = p;
    m_sessions.push_back(std::move(session));
    m_configData.profiles.push_back(p);
    SaveConfig();

    int idx = static_cast<int>(m_sessions.size()) - 1;
    std::cout << "Profile added: [" << idx << "] " << p.name << std::endl;
}

void CommandHandler::CmdEdit(const std::string& args) {
    std::istringstream ss(args);
    std::string target, field, value;
    ss >> target >> field >> value;

    if (target.empty() || field.empty() || value.empty()) {
        std::cout << "Usage: edit <name or index> <field> <value>" << std::endl;
        std::cout << "Fields: name, host, port, key, shortcut, auto_connect, speech, mute_on_local_control" << std::endl;
        return;
    }

    int idx = FindProfileIndex(target);
    if (idx < 0) {
        std::cout << "Profile not found: " << target << std::endl;
        return;
    }

    auto& p = m_sessions[idx].config;
    std::transform(field.begin(), field.end(), field.begin(), ::tolower);

    if (field == "name") p.name = value;
    else if (field == "host") p.host = value;
    else if (field == "port") {
        try { p.port = std::stoi(value); } catch (...) {
            std::cout << "Invalid port" << std::endl; return;
        }
    }
    else if (field == "key") p.key = value;
    else if (field == "shortcut") p.shortcut = value;
    else if (field == "auto_connect") {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        p.autoConnect = (value == "true" || value == "yes" || value == "1");
    }
    else if (field == "speech") {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        p.speech = (value == "true" || value == "yes" || value == "1");
        if (m_sessions[idx].connection) {
            m_sessions[idx].connection->SetSpeechEnabled(p.speech);
        }
    }
    else if (field == "mute_on_local_control") {
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        p.muteOnLocalControl = (value == "true" || value == "yes" || value == "1");
        if (m_sessions[idx].connection) {
            m_sessions[idx].connection->SetMuteOnLocalControl(p.muteOnLocalControl);
        }
    }
    else {
        std::cout << "Unknown field: " << field << std::endl;
        return;
    }

    if (idx < static_cast<int>(m_configData.profiles.size())) {
        m_configData.profiles[idx] = p;
    }
    SaveConfig();
    std::cout << "Profile [" << idx << "] " << p.name << " updated: " << field << " = " << value << std::endl;
}

void CommandHandler::CmdDelete(const std::string& args) {
    if (args.empty()) {
        std::cout << "Usage: delete <name or index>" << std::endl;
        return;
    }
    int idx = FindProfileIndex(args);
    if (idx < 0) {
        std::cout << "Profile not found: " << args << std::endl;
        return;
    }

    std::string name = m_sessions[idx].config.name;
    DisconnectSession(idx);
    m_sessions.erase(m_sessions.begin() + idx);
    if (idx < static_cast<int>(m_configData.profiles.size())) {
        m_configData.profiles.erase(m_configData.profiles.begin() + idx);
    }
    RebuildShortcuts();
    SaveConfig();
    std::cout << "Profile '" << name << "' deleted" << std::endl;
}

void CommandHandler::CmdHelp() {
    std::cout << "Available commands:" << std::endl;
    std::cout << "  status              Show connection status for all profiles" << std::endl;
    std::cout << "  list (ls)           List all profiles with details" << std::endl;
    std::cout << "  connect [name|idx]  Connect a profile (or all disconnected)" << std::endl;
    std::cout << "  disconnect (dc) <name|idx>  Disconnect a profile" << std::endl;
    std::cout << "  add                 Add a new profile (interactive wizard)" << std::endl;
    std::cout << "  add <name> <host> <key> [port] [shortcut] [auto_connect]" << std::endl;
    std::cout << "                      Add a new profile (one-liner)" << std::endl;
    std::cout << "  edit <name|idx> <field> <value>" << std::endl;
    std::cout << "                      Edit a profile field" << std::endl;
    std::cout << "  delete (rm) <name|idx>  Delete a profile" << std::endl;
    std::cout << "  reinstall-hook (hook)  Reinstall keyboard hook (fixes NVDA modifier after NVDA restart)" << std::endl;
    std::cout << "  help (?)            Show this help" << std::endl;
    std::cout << "  quit (exit)         Exit the application" << std::endl;
}

void CommandHandler::CmdReinstallHook() {
#ifdef _WIN32
    std::cout << "Reinstalling keyboard hook..." << std::endl;
    if (g_mainThreadId != 0) {
        PostThreadMessage(g_mainThreadId, WM_REINSTALL_HOOK, 0, 0);
        std::cout << "Keyboard hook reinstalled." << std::endl;
    } else {
        std::cout << "Error: Cannot reinstall hook (main thread not available)." << std::endl;
    }
#else
    std::cout << "Keyboard hook is not available on this platform." << std::endl;
#endif
}

int CommandHandler::FindProfileIndex(const std::string& nameOrIndex) {
    try {
        int idx = std::stoi(nameOrIndex);
        if (idx >= 0 && idx < static_cast<int>(m_sessions.size())) {
            return idx;
        }
    } catch (...) {}

    std::string lower = nameOrIndex;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (int i = 0; i < static_cast<int>(m_sessions.size()); i++) {
        std::string pname = m_sessions[i].config.name;
        std::transform(pname.begin(), pname.end(), pname.begin(), ::tolower);
        if (pname == lower) return i;
    }
    return -1;
}

void CommandHandler::SaveConfig() {
    if (m_configPath.empty()) {
        std::cout << "No config file path - changes not saved to disk" << std::endl;
        return;
    }
    if (ConfigFile::Save(m_configPath, m_configData)) {
        DEBUG_INFO("CONFIG", "Config saved to: " + m_configPath);
    } else {
        std::cerr << "Warning: Failed to save config to " << m_configPath << std::endl;
    }
}
