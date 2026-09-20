#include <jni.h>
#include <android/log.h>
#include <algorithm>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include "AndroidAudio.h"
#include "AndroidClipboard.h"
#include "AndroidSpeech.h"
#include "JniEnv.h"
#include "AppState.h"
#include "Clipboard.h"
#include "ConfigFile.h"
#include "ConnectionManager.h"
#include "Debug.h"
#include "KeyEvent.h"
#include "KeyboardState.h"
#include "MessageSender.h"

#define TAG "NVDARemote/Bridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define NVDA_JNI(ret, name) \
    JNIEXPORT ret JNICALL Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_##name

JavaVM* g_jvm = nullptr;

static std::vector<std::unique_ptr<ConnectionManager>> g_managers;
static ConfigFileData g_config;
static std::string g_configPath;
static std::mutex g_managersMutex;

static jclass    g_bridgeClass         = nullptr;
static jmethodID g_onConnStateChanged  = nullptr;
static jmethodID g_onForwardingChanged = nullptr;
static jmethodID g_onClipShortcut     = nullptr;

static bool IsValidProfileIdx(int idx) {
    return idx >= 0 && idx < static_cast<int>(g_config.profiles.size());
}

void NotifyForwardingState(bool forwarding) {
    if (!g_bridgeClass || !g_onForwardingChanged) return;
    ScopedJniEnv env;
    if (!env) return;
    env->CallStaticVoidMethod(g_bridgeClass, g_onForwardingChanged,
                              static_cast<jboolean>(forwarding));
}

static void NotifyConnectionState(int profileIndex, bool connected) {
    if (!g_bridgeClass || !g_onConnStateChanged) return;
    ScopedJniEnv env;
    if (!env) return;
    env->CallStaticVoidMethod(g_bridgeClass, g_onConnStateChanged,
                              static_cast<jint>(profileIndex),
                              static_cast<jboolean>(connected));
}

static void NotifyClipboardShortcut() {
    if (!g_bridgeClass || !g_onClipShortcut) return;
    ScopedJniEnv env;
    if (!env) return;
    env->CallStaticVoidMethod(g_bridgeClass, g_onClipShortcut);
}

static void NotifyForwardingChanged() {
    NotifyForwardingChanged();
}

static ConnectionManager* ManagerAt(int idx) {
    if (idx < 0 || idx >= static_cast<int>(g_managers.size())) return nullptr;
    return g_managers[idx].get();
}

static void SyncConnectedProfiles() {
    std::vector<int> indices;
    std::vector<std::string> names;
    for (int i = 0; i < static_cast<int>(g_config.profiles.size()); i++) {
        if (i < static_cast<int>(g_managers.size()) && g_managers[i] && g_managers[i]->IsConnected()) {
            indices.push_back(i);
            names.push_back(g_config.profiles[i].name);
        }
    }
    AppState::SetConnectedProfiles(indices, names);
}

static void ApplyShortcutsFromConfig() {
    KeyboardState::ClearShortcuts();
    KeyboardState::ApplyGlobalShortcuts(g_config);

    for (int i = 0; i < static_cast<int>(g_config.profiles.size()); i++) {
        if (!g_config.profiles[i].shortcut.empty())
            KeyboardState::SetToggleShortcutAt(i, g_config.profiles[i].shortcut);
    }
}

static void SaveAndResizeManagers() {
    ConfigFile::Save(g_configPath, g_config);
    g_managers.resize(g_config.profiles.size());
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void*) {
    g_jvm = vm;
    Debug::SetEnabled(true);
    Debug::SetLevel(Debug::LEVEL_INFO);
    LOGI("Native library loaded");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM*, void*) {
    if (g_bridgeClass) {
        JNIEnv* env = nullptr;
        if (g_jvm) g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (env) env->DeleteGlobalRef(g_bridgeClass);
        g_bridgeClass = nullptr;
    }
    g_jvm = nullptr;
}

extern "C" {

NVDA_JNI(void, nativeInit)(
        JNIEnv* env, jobject,
        jobject ttsManager,
        jobject audioManager,
        jstring configDirPath) {

    env->GetJavaVM(&g_jvm);

    AndroidSpeech::Initialize(env, ttsManager);
    AndroidAudio::Initialize(env, audioManager);

    std::string configDir = JniToString(env, configDirPath);
    ConfigFile::SetAndroidDataDir(configDir);
    g_configPath = ConfigFile::DefaultPath();
    LOGI("Config path: %s", g_configPath.c_str());

    ConfigFile::Migrate(g_configPath);
    g_config = ConfigFile::Load(g_configPath);

    if (g_config.profiles.empty()) {
        LOGI("No config found — creating default");
        ConfigFile::CreateDefault(g_configPath);
        g_config = ConfigFile::Load(g_configPath);
    }

    {
        std::lock_guard<std::mutex> lock(g_managersMutex);
        g_managers.resize(g_config.profiles.size());
    }

    jclass localCls = env->FindClass("org/gozaltech/nvdaremotecompanion/android/NativeBridge");
    if (localCls) {
        g_bridgeClass = reinterpret_cast<jclass>(env->NewGlobalRef(localCls));
        env->DeleteLocalRef(localCls);

        g_onConnStateChanged = env->GetStaticMethodID(g_bridgeClass, "onConnectionStateChanged", "(IZ)V");
        if (!g_onConnStateChanged) LOGE("onConnectionStateChanged method not found");

        g_onForwardingChanged = env->GetStaticMethodID(g_bridgeClass, "onForwardingStateChanged", "(Z)V");
        if (!g_onForwardingChanged) LOGE("onForwardingStateChanged method not found");

        g_onClipShortcut = env->GetStaticMethodID(g_bridgeClass, "onClipboardShortcutTriggered", "()V");
        if (!g_onClipShortcut) LOGE("onClipboardShortcutTriggered method not found");

        AndroidClipboard::Initialize(env, g_bridgeClass);
    }

    ApplyShortcutsFromConfig();

    LOGI("nativeInit complete — %d profile(s) loaded", static_cast<int>(g_config.profiles.size()));
}

NVDA_JNI(void, nativeShutdown)(
        JNIEnv* env, jobject) {

    {
        std::lock_guard<std::mutex> lock(g_managersMutex);
        g_managers.clear();
    }

    AndroidSpeech::Cleanup(env);
    AndroidAudio::Cleanup(env);

    if (g_bridgeClass) {
        env->DeleteGlobalRef(g_bridgeClass);
        g_bridgeClass = nullptr;
    }

    LOGI("nativeShutdown complete");
}

NVDA_JNI(jboolean, nativeConnect)(
        JNIEnv*, jobject,
        jint profileIndex) {

    std::unique_lock<std::mutex> lock(g_managersMutex);

    int idx = static_cast<int>(profileIndex);
    if (!IsValidProfileIdx(idx)) {
        LOGE("nativeConnect: invalid profile index %d", idx);
        return JNI_FALSE;
    }

    if (g_managers[idx] && g_managers[idx]->IsConnected()) return JNI_TRUE;

    if (g_managers[idx]) g_managers[idx]->SetDisconnectCallback(nullptr);
    g_managers[idx] = std::make_unique<ConnectionManager>();

    const auto& p = g_config.profiles[idx];
    g_managers[idx]->ApplyProfileConfig(p);
    g_managers[idx]->SetProfileIndex(idx);

    g_managers[idx]->SetDisconnectCallback([idx]() {
        LOGI("Profile %d disconnected", idx);
        SyncConnectedProfiles();
        NotifyConnectionState(idx, false);
        NotifyForwardingChanged();
    });

    g_managers[idx]->SetReconnectCallback([idx]() {
        LOGI("Profile %d reconnected", idx);
        SyncConnectedProfiles();
        NotifyConnectionState(idx, true);
    });

    MessageSender::SetNetworkClient(idx, g_managers[idx]->GetClient());

    bool ok = g_managers[idx]->EstablishConnection(p.host, p.port, p.key);
    if (ok) {
        SyncConnectedProfiles();
        LOGI("Profile %d connected to %s", idx, p.host.c_str());
    } else {
        LOGE("Profile %d failed to connect to %s", idx, p.host.c_str());
    }
    lock.unlock();
    NotifyConnectionState(idx, ok);

    return ok ? JNI_TRUE : JNI_FALSE;
}

NVDA_JNI(void, nativeDisconnect)(
        JNIEnv*, jobject,
        jint profileIndex) {

    int idx = static_cast<int>(profileIndex);
    {
        std::lock_guard<std::mutex> lock(g_managersMutex);
        ConnectionManager* manager = ManagerAt(idx);
        if (!manager) return;
        manager->SetDisconnectCallback(nullptr);
        manager->Disconnect();
        SyncConnectedProfiles();
        LOGI("Profile %d disconnected by request", idx);
    }
    NotifyConnectionState(idx, false);
    NotifyForwardingChanged();
}

NVDA_JNI(jboolean, nativeIsConnected)(
        JNIEnv*, jobject,
        jint profileIndex) {

    std::lock_guard<std::mutex> lock(g_managersMutex);
    int idx = static_cast<int>(profileIndex);
    const ConnectionManager* manager = ManagerAt(idx);
    return (manager && manager->IsConnected()) ? JNI_TRUE : JNI_FALSE;
}

NVDA_JNI(jint, nativeGetProfileCount)(
        JNIEnv*, jobject) {
    return static_cast<jint>(g_config.profiles.size());
}

NVDA_JNI(jstring, nativeGetProfileName)(
        JNIEnv* env, jobject,
        jint profileIndex) {
    int idx = static_cast<int>(profileIndex);
    if (!IsValidProfileIdx(idx)) return env->NewStringUTF("");
    return env->NewStringUTF(g_config.profiles[idx].name.c_str());
}

NVDA_JNI(jboolean, nativeGetAutoConnect)(
        JNIEnv*, jobject,
        jint profileIndex) {

    std::lock_guard<std::mutex> lock(g_managersMutex);
    int idx = static_cast<int>(profileIndex);
    if (!IsValidProfileIdx(idx)) return JNI_FALSE;
    return g_config.profiles[idx].autoConnect ? JNI_TRUE : JNI_FALSE;
}

NVDA_JNI(jstring, nativeGetProfileJson)(
        JNIEnv* env, jobject,
        jint profileIndex) {

    std::lock_guard<std::mutex> lock(g_managersMutex);
    int idx = static_cast<int>(profileIndex);
    if (!IsValidProfileIdx(idx)) return env->NewStringUTF("{}");
    return env->NewStringUTF(ConfigFile::ProfileToJsonString(g_config.profiles[idx]).c_str());
}

NVDA_JNI(void, nativeSaveProfile)(
        JNIEnv* env, jobject,
        jint profileIndex,
        jstring name, jstring host, jint port, jstring key,
        jboolean speech, jboolean sounds, jboolean mute, jboolean autoConnect) {

    ProfileConfig p;
    p.host = JniToString(env, host);
    p.key  = JniToString(env, key);
    if (p.host.empty() || p.key.empty()) return;
    std::string nameStr = JniToString(env, name);
    p.name             = nameStr.empty() ? p.host : nameStr;
    p.port             = static_cast<int>(port);
    p.speech           = static_cast<bool>(speech);
    p.forwardAudio     = static_cast<bool>(sounds);
    p.muteOnLocalControl = static_cast<bool>(mute);
    p.autoConnect      = static_cast<bool>(autoConnect);

    std::lock_guard<std::mutex> lock(g_managersMutex);
    ConfigFile::StripInvalidProfiles(g_config.profiles);

    int idx = static_cast<int>(profileIndex);
    if (IsValidProfileIdx(idx))
        g_config.profiles[idx] = std::move(p);
    else
        g_config.profiles.push_back(std::move(p));

    SaveAndResizeManagers();
    LOGI("Profile saved — %d profile(s)", static_cast<int>(g_config.profiles.size()));
}

NVDA_JNI(void, nativeDeleteProfile)(
        JNIEnv*, jobject,
        jint profileIndex) {

    std::lock_guard<std::mutex> lock(g_managersMutex);
    int idx = static_cast<int>(profileIndex);
    if (!IsValidProfileIdx(idx)) return;

    g_config.profiles.erase(g_config.profiles.begin() + idx);
    SaveAndResizeManagers();
    LOGI("Profile deleted — %d profile(s)", static_cast<int>(g_config.profiles.size()));
}

NVDA_JNI(jint, nativeMergeConfig)(
        JNIEnv* env, jobject,
        jstring configJson) {

    ConfigFileData imported = ConfigFile::LoadFromString(JniToString(env, configJson));

    std::lock_guard<std::mutex> lock(g_managersMutex);
    int added = 0;
    for (const auto& incoming : imported.profiles) {
        if (incoming.host.empty() || incoming.key.empty()) continue;
        bool exists = std::any_of(g_config.profiles.begin(), g_config.profiles.end(),
            [&](const ProfileConfig& e) {
                return e.host == incoming.host && e.key == incoming.key;
            });
        if (!exists) {
            g_config.profiles.push_back(incoming);
            ++added;
        }
    }
    if (added > 0) {
        SaveAndResizeManagers();
        LOGI("Merged config — added %d profile(s)", added);
    }
    return static_cast<jint>(added);
}

NVDA_JNI(void, nativeLoadConfig)(
        JNIEnv* env, jobject,
        jstring configJson) {

    std::string json = JniToString(env, configJson);
    {
        std::ofstream out(g_configPath);
        if (out.is_open()) out << json;
    }
    std::lock_guard<std::mutex> lock(g_managersMutex);
    g_config = ConfigFile::Load(g_configPath);
    g_managers.resize(g_config.profiles.size());
    LOGI("Config reloaded — %d profile(s)", static_cast<int>(g_config.profiles.size()));
}

NVDA_JNI(jstring, nativeGetConfigJson)(
        JNIEnv* env, jobject) {
    std::ifstream in(g_configPath);
    if (!in.is_open()) return env->NewStringUTF("{}");
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    return env->NewStringUTF(content.c_str());
}

NVDA_JNI(void, nativeSendKeyEvent)(
        JNIEnv*, jobject,
        jint vkCode, jint scanCode,
        jboolean pressed, jboolean extended,
        jint profileIndex) {

    if (AppState::GetActiveProfile() != static_cast<int>(profileIndex)) return;

    uint32_t vk  = static_cast<uint32_t>(vkCode);
    uint16_t sc  = static_cast<uint16_t>(scanCode);
    bool     prs = static_cast<bool>(pressed);
    bool     ext = static_cast<bool>(extended);

    if (prs) KeyboardState::TrackKeyPress(vk, sc, ext);
    else     KeyboardState::TrackKeyRelease(vk);

    MessageSender::SendKeyEvent(KeyEvent(vk, prs, sc, ext));
}

NVDA_JNI(jboolean, nativeProcessModifiersAndShortcuts)(
        JNIEnv*, jobject,
        jint vkCode, jboolean pressed) {

    uint32_t vk  = static_cast<uint32_t>(vkCode);
    bool     prs = static_cast<bool>(pressed);

    KeyboardState::UpdateModifierState(vk, prs);
    if (!prs) return JNI_FALSE;

    const auto consume = [](auto&& action) -> jboolean {
        KeyboardState::ResetModifiers();
        action();
        NotifyForwardingChanged();
        return JNI_TRUE;
    };

    if (KeyboardState::CheckForwardKeysShortcut(vk))
        return consume([] { AppState::ToggleForwarding(); });

    if (AppState::IsSendingKeys()) return JNI_FALSE;

    if (KeyboardState::CheckCycleShortcut(vk))
        return consume([] { AppState::CycleProfile(); });

    const int toggleIdx = KeyboardState::CheckToggleShortcut(vk);
    if (toggleIdx >= 0)
        return consume([toggleIdx] { AppState::SetActiveProfile(toggleIdx); });

    if (KeyboardState::CheckClipboardShortcut(vk)) {
        KeyboardState::ResetModifiers();
        NotifyClipboardShortcut();
        return JNI_TRUE;
    }

    return JNI_FALSE;
}

NVDA_JNI(void, nativeSetActiveProfile)(
        JNIEnv*, jobject,
        jint profileIndex) {

    AppState::SetActiveProfile(static_cast<int>(profileIndex));
    NotifyForwardingChanged();
}

NVDA_JNI(void, nativeToggleForwarding)(
        JNIEnv*, jobject) {

    AppState::ToggleForwarding();
    NotifyForwardingChanged();
}

NVDA_JNI(jint, nativeGetActiveProfile)(
        JNIEnv*, jobject) {
    return static_cast<jint>(AppState::GetActiveProfile());
}

NVDA_JNI(jboolean, nativeIsSendingKeys)(
        JNIEnv*, jobject) {
    return AppState::IsSendingKeys() ? JNI_TRUE : JNI_FALSE;
}

NVDA_JNI(void, nativeUpdateTtsManager)(
        JNIEnv* env, jobject,
        jobject ttsManager) {
    AndroidSpeech::Cleanup(env);
    AndroidSpeech::Initialize(env, ttsManager);
}

NVDA_JNI(void, nativeSendClipboardText)(
        JNIEnv* env, jobject,
        jstring text,
        jint profileIndex) {

    int idx = static_cast<int>(profileIndex);
    if (AppState::GetActiveProfile() != idx && idx != -1) return;

    std::string clipText = JniToString(env, text);
    if (clipText.empty()) return;

    MessageSender::SendClipboardText(clipText);
    LOGI("Clipboard text sent from Android");
}
}
