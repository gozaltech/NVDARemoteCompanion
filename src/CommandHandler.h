#pragma once
#include "ConnectionManager.h"
#include "ConfigFile.h"
#include "Config.h"
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
    bool unsaved = false;
};

class CommandHandler {
public:
    CommandHandler(std::string configPath, ConfigFileData configData);

    int ConnectAutoProfiles();
    bool ConnectInteractive();
    bool ConnectFromParams(const ProfileConfig& p);
    void RunCommandLoop();

    static bool AddProfileInteractive(const std::string& configPath, ConfigFileData& cfg,
                                      ProfileConfig partial = {});

    const std::vector<ProfileSession>& GetSessions() const { return m_sessions; }
    int GetSessionCount() const { return Config::isize(m_sessions); }

    void ReconnectAll();
    void ToggleProfile(int index);
    void UpdateNetworkClients();
    void SetDisconnectCallback(std::function<void()> callback);
    void SetReconnectCallback(std::function<void()> callback);

    int CountConnectedSessions() const;
    bool HasAnyConnected() const;
    bool HasDisconnectedSessions() const;

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
    void CmdSave(const std::string& args);
    void CmdReinstallHook();

    int FindProfileIndex(const std::string& nameOrIndex);
    bool PromptLine(const std::string& prompt, std::string& out, const std::string& defaultValue = "");
    bool IsValidSessionIndex(int index) const { return index >= 0 && index < Config::isize(m_sessions); }

    void AppendProfile(const ProfileConfig& p);
    void SaveConfig();
    void ConnectSession(int index);
    void DisconnectSession(int index);
    void RebuildShortcuts();

    std::string m_configPath;
    ConfigFileData m_configData;
    std::vector<ProfileSession> m_sessions;
    std::function<void()> m_disconnectCallback;
    std::function<void()> m_reconnectCallback;
};
