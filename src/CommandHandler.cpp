#include "CommandHandler.h"
#include "Config.h"
#include "Debug.h"
#include "Input.h"
#include "Clipboard.h"
#include "MessageSender.h"
#include "KeyboardState.h"
#include "AppState.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#ifdef _WIN32
#include "KeyboardHook.h"
#include <windows.h>
extern DWORD g_mainThreadId;
#endif

namespace {
    using ValidatorFunc = std::function<Config::ValidationResult(const std::string&)>;
    using ProcessorFunc = std::function<std::string(const std::string&)>;

    bool GetValidatedInput(const std::string& prompt, std::string& result,
                           ValidatorFunc validator = nullptr,
                           ProcessorFunc processor = nullptr) {
        while (true) {
            std::cout << prompt << std::flush;
            std::string input;
            if (!Input::ReadLine(input)) return false;
            if (processor) input = processor(input);
            if (validator) {
                auto v = validator(input);
                if (!v) { std::cout << Config::ERROR_PREFIX << v.errorMessage << "\n\n"; continue; }
            }
            result = std::move(input);
            return true;
        }
    }

    bool GetValidatedPort(int& result, int defaultValue = Config::DEFAULT_PORT) {
        std::string prompt = "Enter server port [" + std::to_string(defaultValue) + "]: ";
        std::string input;
        auto validator = [](const std::string& s) -> Config::ValidationResult {
            if (s.empty()) return {true};
            try { return Config::Validator::ValidatePort(std::stoi(s)); }
            catch (...) { return {false, Config::ERROR_PORT_INVALID}; }
        };
        while (true) {
            if (!GetValidatedInput(prompt, input, validator, Config::TrimWhitespace)) return false;
            if (input.empty()) {
                result = defaultValue;
                std::cout << "Using default port: " << defaultValue << "\n\n";
                return true;
            }
            result = std::stoi(input);
            std::cout << "Port: " << result << "\n\n";
            return true;
        }
    }

    std::optional<ConnectionParams> PromptForConnectionParams() {
        ConnectionParams p;
        std::cout << "\n" << Config::APP_NAME << " - Interactive Setup\n"
                  << std::string(50, '=') << "\n\nServer Configuration:\n";
        if (!GetValidatedInput("Enter server host (IP address or domain name): ",
                               p.host, Config::Validator::ValidateHost, Config::TrimWhitespace))
            return std::nullopt;
        std::cout << "Host: " << p.host << "\n\n";
        if (!GetValidatedPort(p.port)) return std::nullopt;
        if (!GetValidatedInput("Enter connection key/channel: ",
                               p.key, Config::Validator::ValidateKey, Config::TrimWhitespace))
            return std::nullopt;
        std::cout << "Connection key: " << p.key << "\n\n";
#ifdef _WIN32
        if (!GetValidatedInput("Enter toggle shortcut (default: ctrl+win+f11): ",
                               p.shortcut, nullptr, Config::TrimWhitespace))
            return std::nullopt;
        if (p.shortcut.empty()) {
            p.shortcut = "ctrl+win+f11";
            std::cout << "Using default shortcut: " << p.shortcut << "\n\n";
        } else {
            std::cout << "Shortcut: " << p.shortcut << "\n\n";
        }
#endif
        std::cout << "Connection Summary:\n"
                  << "  Host:  " << p.host << "\n"
                  << "  Port:  " << p.port << "\n"
                  << "  Key:   " << p.key << "\n"
#ifdef _WIN32
                  << "  Shortcut: " << p.shortcut << "\n"
#endif
                  << "\nConnecting to NVDA Remote server...\n";
        return p;
    }
}

CommandHandler::CommandHandler(std::string configPath, ConfigFileData configData)
    : m_configPath(std::move(configPath)), m_configData(std::move(configData)) {
    for (auto& profile : m_configData.profiles) {
        ProfileSession session;
        session.config = profile;
        m_sessions.push_back(std::move(session));
    }
}

void CommandHandler::SetDisconnectCallback(std::function<void()> callback) {
    m_disconnectCallback = std::move(callback);
}

void CommandHandler::SetReconnectCallback(std::function<void()> callback) {
    m_reconnectCallback = std::move(callback);
}

int CommandHandler::ConnectAutoProfiles() {
    int connected = 0;
    for (int i = 0; i < Config::isize(m_sessions); i++) {
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

int CommandHandler::CountConnectedSessions() const {
    int count = 0;
    for (const auto& s : m_sessions) {
        if (s.connection && s.connection->IsConnected()) count++;
    }
    return count;
}

bool CommandHandler::HasAnyConnected() const {
    return CountConnectedSessions() > 0;
}

bool CommandHandler::HasDisconnectedSessions() const {
    for (const auto& s : m_sessions) {
        if (s.connection && !s.connection->IsConnected()) return true;
    }
    return false;
}

bool CommandHandler::ConnectFromParams(const ProfileConfig& p) {
    auto cm = std::make_unique<ConnectionManager>();
    cm->SetSpeechEnabled(p.speech);
    cm->SetMuteOnLocalControl(p.muteOnLocalControl);
    cm->SetForwardAudioEnabled(p.forwardAudio);
    if (m_disconnectCallback) cm->SetDisconnectCallback(m_disconnectCallback);
    if (m_reconnectCallback) cm->SetReconnectCallback(m_reconnectCallback);

    std::cout << "Connecting to " << p.host << ":" << p.port << "..." << std::endl;
    if (!cm->EstablishConnection(p.host, p.port, p.key, p.shortcut)) {
        return false;
    }

    if (!p.shortcut.empty()) {
        KeyboardState::SetToggleShortcutAt(0, p.shortcut);
    }
    MessageSender::SetNetworkClient(0, cm->GetClient());

    ProfileSession session;
    session.config = p;
    session.connection = std::move(cm);
    session.shortcutIndex = 0;
    session.unsaved = true;
    m_sessions.push_back(std::move(session));
    std::cout << "Connected" << std::endl;
    return true;
}

bool CommandHandler::ConnectInteractive() {
    auto paramsOpt = PromptForConnectionParams();
    if (!paramsOpt) return false;

    auto cm = std::make_unique<ConnectionManager>();
    cm->SetSpeechEnabled(true);
    if (m_disconnectCallback) cm->SetDisconnectCallback(m_disconnectCallback);
    if (m_reconnectCallback) cm->SetReconnectCallback(m_reconnectCallback);

    if (!cm->EstablishConnection(paramsOpt->host, paramsOpt->port, paramsOpt->key, paramsOpt->shortcut)) {
        return false;
    }

    if (!paramsOpt->shortcut.empty()) {
        KeyboardState::SetToggleShortcutAt(0, paramsOpt->shortcut);
    }
    MessageSender::SetNetworkClient(0, cm->GetClient());

    ProfileSession session;
    session.config.name     = "interactive";
    session.config.host     = paramsOpt->host;
    session.config.port     = paramsOpt->port;
    session.config.key      = paramsOpt->key;
    session.config.shortcut = paramsOpt->shortcut;
    session.connection = std::move(cm);
    session.shortcutIndex = 0;
    session.unsaved = true;
    m_sessions.push_back(std::move(session));
    return true;
}

void CommandHandler::ConnectSession(int index) {
    if (!IsValidSessionIndex(index)) return;
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
    session.connection->SetForwardAudioEnabled(p.forwardAudio);
    if (m_disconnectCallback) {
        session.connection->SetDisconnectCallback(m_disconnectCallback);
    }
    if (m_reconnectCallback) {
        session.connection->SetReconnectCallback(m_reconnectCallback);
    }

    if (session.connection->EstablishConnection(p.host, p.port, p.key, p.shortcut)) {
        std::cout << "Connected to " << p.name << std::endl;
    } else {
        std::cerr << "Failed to connect to " << p.name << std::endl;
        session.connection.reset();
    }
}

void CommandHandler::DisconnectSession(int index) {
    if (!IsValidSessionIndex(index)) return;
    auto& session = m_sessions[index];
    if (session.connection) {
        std::cout << "Disconnecting " << session.config.name << "..." << std::endl;
        session.connection->SetDisconnectCallback(nullptr);
        session.connection.reset();
        std::cout << "Disconnected " << session.config.name << std::endl;
    }
}

void CommandHandler::RebuildShortcuts() {
    KeyboardState::ClearShortcuts();
    int shortcutIdx = 0;
    std::vector<int> connectedIndices;
    std::vector<std::string> connectedNames;

    for (int i = 0; i < Config::isize(m_sessions); i++) {
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
}

void CommandHandler::UpdateNetworkClients() {
    for (auto& session : m_sessions) {
        if (session.shortcutIndex >= 0 && session.connection) {
            MessageSender::SetNetworkClient(session.shortcutIndex, session.connection->GetClient());
        }
    }
}

void CommandHandler::ToggleProfile(int index) {
    if (!IsValidSessionIndex(index)) return;
    auto& session = m_sessions[index];
    if (session.connection && session.connection->IsConnected()) {
        DisconnectSession(index);
    } else {
        ConnectSession(index);
    }
    RebuildShortcuts();
}

void CommandHandler::ReconnectAll() {
    for (int i = 0; i < Config::isize(m_sessions); i++) {
        DisconnectSession(i);
        ConnectSession(i);
    }
    RebuildShortcuts();
}

void CommandHandler::RunCommandLoop() {
    std::string line;
    std::cout << "> " << std::flush;
    while (Input::ReadLine(line)) {
        if (!line.empty()) HandleCommand(line);
        if (!g_shutdown) std::cout << "> " << std::flush;
    }
}

void CommandHandler::HandleCommand(const std::string& line) {
    std::string trimmed = Config::TrimWhitespace(line);
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

    using CmdHandler = std::function<void(CommandHandler&, const std::string&)>;
    struct Entry { std::vector<const char*> aliases; CmdHandler handler; };
    static const std::vector<Entry> table = {
        {{"status"},                    [](CommandHandler& h, const std::string&)  { h.CmdStatus(); }},
        {{"list", "ls"},                [](CommandHandler& h, const std::string&)  { h.CmdList(); }},
        {{"connect", "c"},              [](CommandHandler& h, const std::string& a){ h.CmdConnect(a); }},
        {{"disconnect", "dc"},          [](CommandHandler& h, const std::string& a){ h.CmdDisconnect(a); }},
        {{"add"},                       [](CommandHandler& h, const std::string& a){ h.CmdAdd(a); }},
        {{"edit"},                      [](CommandHandler& h, const std::string& a){ h.CmdEdit(a); }},
        {{"delete", "rm"},              [](CommandHandler& h, const std::string& a){ h.CmdDelete(a); }},
        {{"save"},                      [](CommandHandler& h, const std::string& a){ h.CmdSave(a); }},
        {{"clip"},                      [](CommandHandler& h, const std::string&)  { h.CmdClip(); }},
        {{"help", "?"},                 [](CommandHandler& h, const std::string&)  { h.CmdHelp(); }},
#ifdef _WIN32
        {{"reinstall-hook", "hook"},    [](CommandHandler& h, const std::string&)  { h.CmdReinstallHook(); }},
#endif
        {{"quit", "exit"},              [](CommandHandler&, const std::string&) {
            g_shutdown = true;
#ifdef _WIN32
            if (g_mainThreadId != 0) PostThreadMessage(g_mainThreadId, WM_QUIT, 0, 0);
#endif
        }},
    };
    static const auto dispatch = []() {
        std::unordered_map<std::string, const CmdHandler*> map;
        for (const auto& entry : table) {
            for (const char* alias : entry.aliases) {
                map.emplace(alias, &entry.handler);
            }
        }
        return map;
    }();

    auto it = dispatch.find(cmd);
    if (it != dispatch.end()) {
        (*it->second)(*this, args);
    } else {
        std::cout << "Unknown command: " << cmd << ". Type 'help' for available commands." << std::endl;
    }
}

void CommandHandler::CmdStatus() {
    int connected = CountConnectedSessions(), total = Config::isize(m_sessions);
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
    for (int i = 0; i < Config::isize(m_sessions); i++) {
        const auto& p = m_sessions[i].config;
        std::cout << "  [" << i << "] " << p.name
                  << " | " << p.host << ":" << p.port
                  << " | key=" << p.key
                  << " | shortcut=" << (p.shortcut.empty() ? "ctrl+win+f11" : p.shortcut)
                  << " | auto_connect=" << (p.autoConnect ? "yes" : "no")
                  << " | speech=" << (p.speech ? "yes" : "no")
                  << " | mute_on_local_control=" << (p.muteOnLocalControl ? "yes" : "no")
                  << " | forward_nvda_sounds=" << (p.forwardAudio ? "yes" : "no")
                  << std::endl;
    }
}

void CommandHandler::CmdConnect(const std::string& args) {
    if (args.empty()) {
        for (int i = 0; i < Config::isize(m_sessions); i++) {
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

static bool PromptLineImpl(const std::string& prompt, std::string& out, const std::string& defaultValue) {
    if (!defaultValue.empty())
        std::cout << prompt << " [" << defaultValue << "]: " << std::flush;
    else
        std::cout << prompt << ": " << std::flush;

    std::string line;
    if (!Input::ReadLineWithEscape(line)) return false;

    if (line.empty() && !defaultValue.empty())
        line = defaultValue;
    out = line;
    return true;
}

bool CommandHandler::PromptLine(const std::string& prompt, std::string& out, const std::string& defaultValue) {
    return PromptLineImpl(prompt, out, defaultValue);
}

static std::string MakeUniqueName(const std::string& base, const std::vector<ProfileConfig>& existing) {
    std::string name = base;
    int suffix = 2;
    while (std::any_of(existing.begin(), existing.end(),
            [&](const ProfileConfig& e) { return e.name == name; }))
        name = base + " " + std::to_string(suffix++);
    return name;
}

static bool PromptProfileInteractive(
        std::function<bool(const std::string&, std::string&, const std::string&)> prompt,
        ProfileConfig& p,
        const std::vector<ProfileConfig>& existing) {
    std::string input;

    if (!prompt("Name (optional)", input, "")) return false;
    std::string nameInput = input;

    if (!prompt("Host", input, "")) return false;
    if (input.empty()) { std::cout << "Host is required." << std::endl; return false; }
    p.host = input;

    if (nameInput.empty()) nameInput = p.host;
    p.name = MakeUniqueName(nameInput, existing);

    if (!prompt("Port", input, std::to_string(Config::DEFAULT_PORT))) return false;
    try { p.port = input.empty() ? Config::DEFAULT_PORT : std::stoi(input); }
    catch (...) { std::cout << "Invalid port, using default." << std::endl; p.port = Config::DEFAULT_PORT; }

    if (!prompt("Key", input, "")) return false;
    if (input.empty()) { std::cout << "Key is required." << std::endl; return false; }
    p.key = input;

#ifdef _WIN32
    if (!prompt("Shortcut (optional, e.g. ctrl+win+f12)", input, "")) return false;
    p.shortcut = input;
#endif

    if (!prompt("Auto connect (y/n)", input, "y")) return false;
    p.autoConnect = Config::StringToBool(input, true);

    if (!prompt("Speech (y/n)", input, "y")) return false;
    p.speech = Config::StringToBool(input, true);

    if (!prompt("Mute on local control (y/n)", input, "n")) return false;
    p.muteOnLocalControl = Config::StringToBool(input);

    if (!prompt("Forward NVDA sounds from remote (y/n)", input, "y")) return false;
    p.forwardAudio = Config::StringToBool(input, true);

    return true;
}

bool CommandHandler::AddProfileInteractive(const std::string& configPath, ConfigFileData& cfg,
                                           ProfileConfig partial) {
    cfg.profiles.erase(
        std::remove_if(cfg.profiles.begin(), cfg.profiles.end(),
            [](const ProfileConfig& p) { return p.host.empty() || p.key.empty(); }),
        cfg.profiles.end());

    ProfileConfig p = partial;

    if (!p.host.empty() && !p.key.empty()) {
        if (p.name.empty()) p.name = p.host;
        p.name = MakeUniqueName(p.name, cfg.profiles);
    } else {
        std::cout << "Adding new profile (press Escape to cancel)" << std::endl;
        auto promptWithDefaults = [&p](const std::string& label, std::string& out, const std::string& def) {
            const std::string& effective =
                (label == "Host"   && !p.host.empty())  ? p.host :
                (label == "Key"    && !p.key.empty())   ? p.key  :
                def;
            return PromptLineImpl(label, out, effective);
        };
        if (!PromptProfileInteractive(promptWithDefaults, p, cfg.profiles)) {
            std::cout << "Cancelled." << std::endl;
            return false;
        }
    }

    cfg.profiles.push_back(p);
    if (!ConfigFile::Save(configPath, cfg)) {
        std::cerr << "Error: Failed to save config to " << configPath << std::endl;
        return false;
    }

    std::cout << "Profile '" << p.name << "' saved to " << configPath << std::endl;
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
        p.host = host;
        p.key = key;
        p.name = MakeUniqueName(name.empty() ? host : name, m_configData.profiles);

        std::string token;
        if (ss >> token) {
            try { p.port = std::stoi(token); } catch (...) { p.port = Config::DEFAULT_PORT; }
        }
        if (ss >> token) p.shortcut = token;
        if (ss >> token) p.autoConnect = Config::StringToBool(token);

        AppendProfile(p);
        return;
    }

    std::cout << "Adding new profile (press Escape to cancel)" << std::endl;

    ProfileConfig p;
    if (!PromptProfileInteractive(PromptLineImpl, p, m_configData.profiles)) {
        std::cout << "Cancelled." << std::endl;
        return;
    }

    AppendProfile(p);
}

void CommandHandler::CmdEdit(const std::string& args) {
    std::istringstream ss(args);
    std::string target, field, value;
    ss >> target >> field >> value;

    if (target.empty() || field.empty() || value.empty()) {
        std::cout << "Usage: edit <name or index> <field> <value>" << std::endl;
        std::cout << "Fields: name, host, port, key, shortcut, auto_connect, speech, mute_on_local_control, forward_nvda_sounds" << std::endl;
        return;
    }

    int idx = FindProfileIndex(target);
    if (idx < 0) {
        std::cout << "Profile not found: " << target << std::endl;
        return;
    }

    auto& session = m_sessions[idx];
    auto& p = session.config;
    std::transform(field.begin(), field.end(), field.begin(), ::tolower);

    using FieldApplier = std::function<bool(ProfileSession&, const std::string&)>;
    static const std::unordered_map<std::string, FieldApplier> fieldAppliers = {
        {"name",    [](ProfileSession& s, const std::string& v) { s.config.name = v; return true; }},
        {"host",    [](ProfileSession& s, const std::string& v) { s.config.host = v; return true; }},
        {"port",    [](ProfileSession& s, const std::string& v) {
            try { s.config.port = std::stoi(v); return true; }
            catch (...) { std::cout << "Invalid port" << std::endl; return false; }
        }},
        {"key",      [](ProfileSession& s, const std::string& v) { s.config.key = v; return true; }},
        {"shortcut", [](ProfileSession& s, const std::string& v) { s.config.shortcut = v; return true; }},
        {"auto_connect", [](ProfileSession& s, const std::string& v) {
            s.config.autoConnect = Config::StringToBool(v); return true;
        }},
        {"speech", [](ProfileSession& s, const std::string& v) {
            s.config.speech = Config::StringToBool(v);
            if (s.connection) s.connection->SetSpeechEnabled(s.config.speech);
            return true;
        }},
        {"mute_on_local_control", [](ProfileSession& s, const std::string& v) {
            s.config.muteOnLocalControl = Config::StringToBool(v);
            if (s.connection) s.connection->SetMuteOnLocalControl(s.config.muteOnLocalControl);
            return true;
        }},
        {"forward_nvda_sounds", [](ProfileSession& s, const std::string& v) {
            s.config.forwardAudio = Config::StringToBool(v);
            if (s.connection) s.connection->SetForwardAudioEnabled(s.config.forwardAudio);
            return true;
        }},
    };

    auto it = fieldAppliers.find(field);
    if (it == fieldAppliers.end()) {
        std::cout << "Unknown field: " << field << std::endl;
        return;
    }
    if (!it->second(session, value)) return;

    if (idx < Config::isize(m_configData.profiles)) {
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
    if (idx < Config::isize(m_configData.profiles)) {
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
    std::cout << "  add <name> <host> <key> [port]"
#ifdef _WIN32
                 " [shortcut]"
#endif
                 " [auto_connect]" << std::endl;
    std::cout << "                      Add a new profile (one-liner)" << std::endl;
    std::cout << "  edit <name|idx> <field> <value>" << std::endl;
    std::cout << "                      Edit a profile field" << std::endl;
    std::cout << "  delete (rm) <name|idx>  Delete a profile" << std::endl;
    std::cout << "  save [name|idx]         Save current interactive connection as a profile" << std::endl;
    std::cout << "  clip                    Send local clipboard text to the active remote" << std::endl;
#ifdef _WIN32
    std::cout << "  reinstall-hook (hook)  Reinstall keyboard hook (fixes NVDA modifier after NVDA restart)" << std::endl;
#endif
    std::cout << "  help (?)            Show this help" << std::endl;
    std::cout << "  quit (exit)         Exit the application" << std::endl;
}

void CommandHandler::CmdSave(const std::string& args) {
    int idx = -1;
    if (!args.empty()) {
        idx = FindProfileIndex(args);
        if (idx < 0) {
            std::cout << "Profile not found: " << args << std::endl;
            return;
        }
    } else {
        for (int i = 0; i < Config::isize(m_sessions); i++) {
            if (m_sessions[i].unsaved) {
                idx = i;
                break;
            }
        }
        if (idx < 0) {
            std::cout << "No unsaved connection found. Use: save <name or index>" << std::endl;
            return;
        }
    }

    auto& session = m_sessions[idx];
    if (session.config.host.empty() || session.config.key.empty()) {
        std::cout << "Session [" << idx << "] has no host or key — cannot save." << std::endl;
        return;
    }
    if (!session.unsaved) {
        std::cout << "Session [" << idx << "] '" << session.config.name << "' is already saved as a profile." << std::endl;
        return;
    }

    std::string nameInput;
    std::string defaultName = session.config.host;
    if (!PromptLine("Profile name", nameInput, defaultName)) return;
    if (nameInput.empty()) nameInput = defaultName;

    ProfileConfig p = session.config;
    p.name = MakeUniqueName(nameInput, m_configData.profiles);

    session.config.name = p.name;
    session.unsaved = false;

    m_configData.profiles.push_back(p);
    SaveConfig();
    std::cout << "Session saved as profile [" << (Config::isize(m_configData.profiles) - 1) << "] '" << p.name << "'" << std::endl;
}

void CommandHandler::CmdClip() {
    if (!HasAnyConnected()) {
        std::cout << "Not connected to any profile." << std::endl;
        return;
    }
    std::string text = Clipboard::GetText();
    if (text.empty()) {
        std::cout << "Clipboard is empty." << std::endl;
        return;
    }
    MessageSender::SendClipboardText(text);
    std::cout << "Clipboard sent to remote." << std::endl;
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
        if (idx >= 0 && idx < Config::isize(m_sessions)) {
            return idx;
        }
    } catch (...) {}

    std::string lower = nameOrIndex;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    for (int i = 0; i < Config::isize(m_sessions); i++) {
        std::string pname = m_sessions[i].config.name;
        std::transform(pname.begin(), pname.end(), pname.begin(), ::tolower);
        if (pname == lower) return i;
    }
    return -1;
}

void CommandHandler::AppendProfile(const ProfileConfig& p) {
    ProfileSession session;
    session.config = p;
    m_sessions.push_back(std::move(session));
    m_configData.profiles.push_back(p);
    SaveConfig();
    std::cout << "Profile added: [" << (Config::isize(m_sessions) - 1) << "] " << p.name << std::endl;
}

void CommandHandler::SaveConfig() {
    if (m_configPath.empty()) {
        m_configPath = ConfigFile::DefaultPath();
        std::cout << "Creating config file: " << m_configPath << std::endl;
    }
    if (ConfigFile::Save(m_configPath, m_configData)) {
        DEBUG_INFO("CONFIG", "Config saved to: " + m_configPath);
    } else {
        std::cerr << "Warning: Failed to save config to " << m_configPath << std::endl;
    }
}
