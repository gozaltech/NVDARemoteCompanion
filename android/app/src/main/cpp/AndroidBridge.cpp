#include <jni.h>
#include <android/log.h>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

#include "AndroidSpeech.h"
#include "AndroidAudio.h"
#include "AndroidClipboard.h"
#include "ConfigFile.h"
#include "ConnectionManager.h"
#include "MessageSender.h"
#include "AppState.h"
#include "KeyEvent.h"
#include "KeyboardState.h"
#include "Clipboard.h"
#include "Debug.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

#define TAG "NVDARemote/Bridge"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

JavaVM* g_jvm = nullptr;

static std::vector<std::unique_ptr<ConnectionManager>> g_managers;
static ConfigFileData g_config;
static std::string g_configPath;
static std::mutex g_managersMutex;

static jclass    g_bridgeClass        = nullptr;
static jmethodID g_onConnStateChanged = nullptr;

static JNIEnv* GetEnv(bool& didAttach) {
    didAttach = false;
    if (!g_jvm) return nullptr;
    JNIEnv* env = nullptr;
    jint res = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (res == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            didAttach = true;
        } else {
            return nullptr;
        }
    }
    return env;
}

static void NotifyConnectionState(int profileIndex, bool connected) {
    if (!g_bridgeClass || !g_onConnStateChanged) return;
    bool didAttach = false;
    JNIEnv* env = GetEnv(didAttach);
    if (!env) return;
    env->CallStaticVoidMethod(g_bridgeClass, g_onConnStateChanged,
                              static_cast<jint>(profileIndex),
                              static_cast<jboolean>(connected));
    if (didAttach) g_jvm->DetachCurrentThread();
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

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeInit(
        JNIEnv* env, jobject,
        jobject ttsManager,
        jobject audioManager,
        jstring configDirPath) {

    env->GetJavaVM(&g_jvm);

    AndroidSpeech::Initialize(env, ttsManager);
    AndroidAudio::Initialize(env, audioManager);

    const char* dirChars = env->GetStringUTFChars(configDirPath, nullptr);
    std::string configDir(dirChars);
    env->ReleaseStringUTFChars(configDirPath, dirChars);

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
        g_onConnStateChanged = env->GetStaticMethodID(
                g_bridgeClass, "onConnectionStateChanged", "(IZ)V");
        if (!g_onConnStateChanged)
            LOGE("onConnectionStateChanged method not found");
        AndroidClipboard::Initialize(env, g_bridgeClass);
    }

    {
        KeyboardState::ClearShortcuts();

        std::string cycleSc = g_config.cycleShortcut.value_or("ctrl+alt+f11");
        KeyboardState::SetCycleShortcut(cycleSc);

std::string reconnectSc = g_config.reconnectShortcut.value_or("");
        if (!reconnectSc.empty()) KeyboardState::SetReconnectShortcut(reconnectSc);

        std::string clipboardSc = g_config.clipboardShortcut.value_or("");
        if (!clipboardSc.empty()) KeyboardState::SetClipboardShortcut(clipboardSc);

        std::string fwSc = g_config.forwardKeysShortcut.value_or(Config::DEFAULT_FORWARD_KEYS_SHORTCUT);
        if (!fwSc.empty()) KeyboardState::SetForwardKeysShortcut(fwSc);

        for (int i = 0; i < static_cast<int>(g_config.profiles.size()); i++) {
            if (!g_config.profiles[i].shortcut.empty())
                KeyboardState::SetToggleShortcutAt(i, g_config.profiles[i].shortcut);
        }
    }

    LOGI("nativeInit complete — %d profile(s) loaded",
         static_cast<int>(g_config.profiles.size()));
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeShutdown(
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

JNIEXPORT jboolean JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeConnect(
        JNIEnv*, jobject,
        jint profileIndex) {

    std::unique_lock<std::mutex> lock(g_managersMutex);

    int idx = static_cast<int>(profileIndex);
    if (idx < 0 || idx >= static_cast<int>(g_config.profiles.size())) {
        LOGE("nativeConnect: invalid profile index %d", idx);
        return JNI_FALSE;
    }

    if (g_managers[idx] && g_managers[idx]->IsConnected()) {
        return JNI_TRUE;
    }

    if (g_managers[idx]) g_managers[idx]->SetDisconnectCallback(nullptr);
    g_managers[idx] = std::make_unique<ConnectionManager>();

    const auto& p = g_config.profiles[idx];
    g_managers[idx]->SetSpeechEnabled(p.speech);
    g_managers[idx]->SetMuteOnLocalControl(p.muteOnLocalControl);
    g_managers[idx]->SetForwardAudioEnabled(p.forwardAudio);
    g_managers[idx]->SetProfileIndex(idx);

    g_managers[idx]->SetDisconnectCallback([idx]() {
        LOGI("Profile %d disconnected", idx);
        SyncConnectedProfiles();
        NotifyConnectionState(idx, false);
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
    NotifyConnectionState(idx, static_cast<bool>(ok));

    return ok ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeDisconnect(
        JNIEnv*, jobject,
        jint profileIndex) {

    {
        std::lock_guard<std::mutex> lock(g_managersMutex);

        int idx = static_cast<int>(profileIndex);
        if (idx < 0 || idx >= static_cast<int>(g_managers.size())) return;
        if (!g_managers[idx]) return;

        g_managers[idx]->SetDisconnectCallback(nullptr);
        g_managers[idx]->Disconnect();
        SyncConnectedProfiles();
        LOGI("Profile %d disconnected by request", idx);
    }
    NotifyConnectionState(static_cast<int>(profileIndex), false);
}

JNIEXPORT jboolean JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeIsConnected(
        JNIEnv*, jobject,
        jint profileIndex) {

    std::lock_guard<std::mutex> lock(g_managersMutex);

    int idx = static_cast<int>(profileIndex);
    if (idx < 0 || idx >= static_cast<int>(g_managers.size())) return JNI_FALSE;
    if (!g_managers[idx]) return JNI_FALSE;
    return g_managers[idx]->IsConnected() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeGetProfileCount(
        JNIEnv*, jobject) {
    return static_cast<jint>(g_config.profiles.size());
}

JNIEXPORT jstring JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeGetProfileName(
        JNIEnv* env, jobject,
        jint profileIndex) {
    int idx = static_cast<int>(profileIndex);
    if (idx < 0 || idx >= static_cast<int>(g_config.profiles.size()))
        return env->NewStringUTF("");
    return env->NewStringUTF(g_config.profiles[idx].name.c_str());
}

JNIEXPORT jint JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeMergeConfig(
        JNIEnv* env, jobject,
        jstring configJson) {

    const char* chars = env->GetStringUTFChars(configJson, nullptr);
    std::string jsonStr(chars);
    env->ReleaseStringUTFChars(configJson, chars);

    ConfigFileData imported = ConfigFile::LoadFromString(jsonStr);

    std::lock_guard<std::mutex> lock(g_managersMutex);
    int added = 0;
    for (const auto& incoming : imported.profiles) {
        if (incoming.host.empty() || incoming.key.empty()) continue;
        bool exists = std::any_of(g_config.profiles.begin(), g_config.profiles.end(),
            [&](const ProfileConfig& existing) {
                return existing.host == incoming.host && existing.key == incoming.key;
            });
        if (!exists) {
            g_config.profiles.push_back(incoming);
            ++added;
        }
    }
    if (added > 0) {
        ConfigFile::Save(g_configPath, g_config);
        g_managers.resize(g_config.profiles.size());
        LOGI("Merged config — added %d profile(s)", added);
    }
    return static_cast<jint>(added);
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeLoadConfig(
        JNIEnv* env, jobject,
        jstring configJson) {

    const char* chars = env->GetStringUTFChars(configJson, nullptr);
    {
        std::ofstream out(g_configPath);
        if (out.is_open()) out << chars;
    }
    env->ReleaseStringUTFChars(configJson, chars);

    std::lock_guard<std::mutex> lock(g_managersMutex);
    g_config = ConfigFile::Load(g_configPath);
    g_managers.resize(g_config.profiles.size());
    LOGI("Config reloaded — %d profile(s)", static_cast<int>(g_config.profiles.size()));
}

JNIEXPORT jstring JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeGetConfigJson(
        JNIEnv* env, jobject) {
    std::ifstream in(g_configPath);
    if (!in.is_open()) return env->NewStringUTF("{}");
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    return env->NewStringUTF(content.c_str());
}

JNIEXPORT jboolean JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeGetAutoConnect(
        JNIEnv*, jobject,
        jint profileIndex) {

    std::lock_guard<std::mutex> lock(g_managersMutex);
    int idx = static_cast<int>(profileIndex);
    if (idx < 0 || idx >= static_cast<int>(g_config.profiles.size())) return JNI_FALSE;
    return g_config.profiles[idx].autoConnect ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeGetProfileJson(
        JNIEnv* env, jobject,
        jint profileIndex) {

    std::lock_guard<std::mutex> lock(g_managersMutex);
    int idx = static_cast<int>(profileIndex);
    if (idx < 0 || idx >= static_cast<int>(g_config.profiles.size()))
        return env->NewStringUTF("{}");

    const ProfileConfig& p = g_config.profiles[idx];
    nlohmann::ordered_json j;
    j[ProfileFields::NAME]                  = p.name;
    j[ProfileFields::HOST]                  = p.host;
    j[ProfileFields::PORT]                  = p.port;
    j[ProfileFields::KEY]                   = p.key;
    j[ProfileFields::SHORTCUT]              = p.shortcut;
    j[ProfileFields::AUTO_CONNECT]          = p.autoConnect;
    j[ProfileFields::SPEECH]                = p.speech;
    j[ProfileFields::MUTE_ON_LOCAL_CONTROL] = p.muteOnLocalControl;
    j[ProfileFields::FORWARD_AUDIO]         = p.forwardAudio;
    return env->NewStringUTF(j.dump().c_str());
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeSaveProfile(
        JNIEnv* env, jobject,
        jint profileIndex,
        jstring name, jstring host, jint port, jstring key,
        jboolean speech, jboolean sounds, jboolean mute, jboolean autoConnect) {

    auto jstr = [&](jstring s) -> std::string {
        const char* c = env->GetStringUTFChars(s, nullptr);
        std::string r(c);
        env->ReleaseStringUTFChars(s, c);
        return r;
    };

    ProfileConfig p;
    p.host              = jstr(host);
    p.key               = jstr(key);
    if (p.host.empty() || p.key.empty()) return;
    std::string nameStr = jstr(name);
    p.name              = nameStr.empty() ? p.host : nameStr;
    p.port              = static_cast<int>(port);
    p.speech            = static_cast<bool>(speech);
    p.forwardAudio      = static_cast<bool>(sounds);
    p.muteOnLocalControl = static_cast<bool>(mute);
    p.autoConnect       = static_cast<bool>(autoConnect);

    std::lock_guard<std::mutex> lock(g_managersMutex);

    g_config.profiles.erase(
        std::remove_if(g_config.profiles.begin(), g_config.profiles.end(),
            [](const ProfileConfig& x) { return x.host.empty() || x.key.empty(); }),
        g_config.profiles.end());

    int idx = static_cast<int>(profileIndex);
    if (idx >= 0 && idx < static_cast<int>(g_config.profiles.size())) {
        g_config.profiles[idx] = std::move(p);
    } else {
        g_config.profiles.push_back(std::move(p));
    }

    ConfigFile::Save(g_configPath, g_config);
    g_managers.resize(g_config.profiles.size());
    LOGI("Profile saved — %d profile(s)", static_cast<int>(g_config.profiles.size()));
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeDeleteProfile(
        JNIEnv*, jobject,
        jint profileIndex) {

    std::lock_guard<std::mutex> lock(g_managersMutex);
    int idx = static_cast<int>(profileIndex);
    if (idx < 0 || idx >= static_cast<int>(g_config.profiles.size())) return;

    g_config.profiles.erase(g_config.profiles.begin() + idx);
    ConfigFile::Save(g_configPath, g_config);
    g_managers.resize(g_config.profiles.size());
    LOGI("Profile deleted — %d profile(s)", static_cast<int>(g_config.profiles.size()));
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeSendKeyEvent(
        JNIEnv*, jobject,
        jint vkCode, jint scanCode,
        jboolean pressed, jboolean extended,
        jint profileIndex) {

    if (AppState::GetActiveProfile() != static_cast<int>(profileIndex)) return;

    uint32_t vk  = static_cast<uint32_t>(vkCode);
    uint16_t sc  = static_cast<uint16_t>(scanCode);
    bool     prs = static_cast<bool>(pressed);
    bool     ext = static_cast<bool>(extended);

    if (prs) {
        KeyboardState::TrackKeyPress(vk, sc, ext);
    } else {
        KeyboardState::TrackKeyRelease(vk);
    }

    KeyEvent event(vk, prs, sc, ext);
    MessageSender::SendKeyEvent(event);
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeSetActiveProfile(
        JNIEnv*, jobject,
        jint profileIndex) {

    AppState::SetActiveProfile(static_cast<int>(profileIndex));
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeToggleForwarding(
        JNIEnv*, jobject) {

    AppState::ToggleForwarding();
}

JNIEXPORT jboolean JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeProcessModifiersAndShortcuts(
        JNIEnv*, jobject,
        jint vkCode, jboolean pressed) {

    uint32_t vk = static_cast<uint32_t>(vkCode);
    bool prs = static_cast<bool>(pressed);

    KeyboardState::UpdateModifierState(vk, prs);

    if (!prs) return JNI_FALSE;

    if (KeyboardState::CheckForwardKeysShortcut(vk)) {
        KeyboardState::ResetModifiers();
        AppState::ToggleForwarding();
        return JNI_TRUE;
    }

    if (AppState::IsSendingKeys()) return JNI_FALSE;

    if (KeyboardState::CheckCycleShortcut(vk)) {
        KeyboardState::ResetModifiers();
        AppState::CycleProfile();
        return JNI_TRUE;
    }
    int toggleIdx = KeyboardState::CheckToggleShortcut(vk);
    if (toggleIdx >= 0) {
        KeyboardState::ResetModifiers();
        AppState::SetActiveProfile(toggleIdx);
        return JNI_TRUE;
    }
    if (KeyboardState::CheckClipboardShortcut(vk)) {
        KeyboardState::ResetModifiers();
        if (g_bridgeClass) {
            bool didAttach = false;
            JNIEnv* cbEnv = GetEnv(didAttach);
            if (cbEnv) {
                static jmethodID s_onClipShortcut = cbEnv->GetStaticMethodID(
                    g_bridgeClass, "onClipboardShortcutTriggered", "()V");
                if (s_onClipShortcut)
                    cbEnv->CallStaticVoidMethod(g_bridgeClass, s_onClipShortcut);
                if (didAttach) g_jvm->DetachCurrentThread();
            }
        }
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeUpdateTtsManager(
        JNIEnv* env, jobject,
        jobject ttsManager) {
    AndroidSpeech::Cleanup(env);
    AndroidSpeech::Initialize(env, ttsManager);
}

JNIEXPORT jint JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeGetActiveProfile(
        JNIEnv*, jobject) {
    return static_cast<jint>(AppState::GetActiveProfile());
}

JNIEXPORT jboolean JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeIsSendingKeys(
        JNIEnv*, jobject) {
    return AppState::IsSendingKeys() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_org_gozaltech_nvdaremotecompanion_android_NativeBridge_nativeSendClipboardText(
        JNIEnv* env, jobject,
        jstring text,
        jint profileIndex) {

    int idx = static_cast<int>(profileIndex);
    if (AppState::GetActiveProfile() != idx && idx != -1) return;

    const char* chars = env->GetStringUTFChars(text, nullptr);
    std::string clipText(chars);
    env->ReleaseStringUTFChars(text, chars);

    if (clipText.empty()) return;

    MessageSender::SendClipboardText(clipText);
    LOGI("Clipboard text sent from Android");
}

}
