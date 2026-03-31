#pragma once
#include "ConnectionManager.h"
#include "ConfigFile.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <atomic>

extern std::atomic<bool> g_shutdown;

struct ProfileSession {
    ProfileConfig config;
    std::unique_ptr<ConnectionManager> connection;
    int shortcutIndex = -1;
};

class CommandHandler {
public:
    CommandHandler(std::string configPath, ConfigFileData configData);

    int ConnectAutoProfiles();

    void RunCommandLoop();

    std::vector<ProfileSession>& GetSessions() { return m_sessions; }
    const std::vector<ProfileSession>& GetSessions() const { return m_sessions; }

    void ReconnectAll();

    void UpdateNetworkClients();

    void SetDisconnectCallbackFactory(std::function<void(ConnectionManager&)> factory);

private:
    void HandleCommand(const std::string& line);

    void CmdStatus();
    void CmdList();
    void CmdConnect(const std::string& args);
    void CmdDisconnect(const std::string& args);
    void CmdAdd(const std::string& args);
    void CmdEdit(const std::string& args);
    void CmdDelete(const std::string& args);
    void CmdHelp();
    void CmdReinstallHook();

    int FindProfileIndex(const std::string& nameOrIndex);
    bool PromptLine(const std::string& prompt, std::string& out, const std::string& defaultValue = "");
    void SaveConfig();
    void ConnectSession(int index);
    void DisconnectSession(int index);
    void RebuildShortcuts();

    std::string m_configPath;
    ConfigFileData m_configData;
    std::vector<ProfileSession> m_sessions;
    std::function<void(ConnectionManager&)> m_disconnectCallbackFactory;
};
