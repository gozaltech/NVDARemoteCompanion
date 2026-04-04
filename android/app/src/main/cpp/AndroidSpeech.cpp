#include "AndroidSpeech.h"
#include "DirectTts/RuTtsEngine.h"
#include "Speech.h"
#include <android/log.h>
#include <memory>
#include <string>

#define TAG "NVDARemote/Speech"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

bool Speech::s_initialized = false;
bool Speech::s_enabled = true;

static JavaVM*   s_jvm    = nullptr;
static jobject   s_ttsRef = nullptr;
static jmethodID s_speak  = nullptr;
static jmethodID s_stop   = nullptr;

static JNIEnv* GetEnv() {
    if (!s_jvm) return nullptr;
    JNIEnv* env = nullptr;
    if (s_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK)
        return env;
    s_jvm->AttachCurrentThreadAsDaemon(&env, nullptr);
    return env;
}

static int s_outputMode = 0;
static std::unique_ptr<DirectTtsEngine> s_directEngine;

static void EnsureDirectEngine(const std::string& name) {
    if (s_directEngine) return;
    s_directEngine = std::make_unique<RuTtsEngine>();
}

void AndroidSpeech::Initialize(JNIEnv* env, jobject ttsManagerRef) {
    env->GetJavaVM(&s_jvm);
    s_ttsRef = env->NewGlobalRef(ttsManagerRef);

    jclass cls = env->GetObjectClass(ttsManagerRef);
    s_speak = env->GetMethodID(cls, "speak", "(Ljava/lang/String;Z)V");
    s_stop  = env->GetMethodID(cls, "stop",  "()V");
    env->DeleteLocalRef(cls);

    if (!s_speak || !s_stop)
        LOGE("Failed to find TtsManager methods");

    Speech::Initialize();
}

void AndroidSpeech::Cleanup(JNIEnv* env) {
    s_directEngine.reset();
    if (s_ttsRef) {
        env->DeleteGlobalRef(s_ttsRef);
        s_ttsRef = nullptr;
    }
    s_speak = nullptr;
    s_stop  = nullptr;
    Speech::Cleanup();
}

void AndroidSpeech::SetOutputMode(int mode) {
    s_outputMode = mode;
    if (mode == 1) EnsureDirectEngine("ru_tts");
}

void AndroidSpeech::SetDirectEngine(const std::string& engineName) {
    s_directEngine.reset();
    if (s_outputMode == 1) EnsureDirectEngine(engineName);
}

bool Speech::Initialize() {
    s_initialized = true;
    return true;
}

void Speech::Cleanup() {
    s_initialized = false;
}

void Speech::Speak(std::string_view text, bool interrupt) {
    if (!s_initialized || !s_enabled || text.empty()) return;

    if (s_outputMode == 1 && s_directEngine) {
        s_directEngine->speak(text, interrupt);
        return;
    }

    if (!s_ttsRef || !s_speak) return;
    JNIEnv* env = GetEnv();
    if (!env) return;

    jstring jtext = env->NewStringUTF(std::string(text).c_str());
    env->CallVoidMethod(s_ttsRef, s_speak, jtext, static_cast<jboolean>(interrupt));
    env->DeleteLocalRef(jtext);
}

void Speech::SpeakSsml(std::string_view ssml, bool interrupt) {
    Speech::Speak(ssml, interrupt);
}

void Speech::Stop() {
    if (!s_initialized) return;

    if (s_outputMode == 1 && s_directEngine) {
        s_directEngine->stop();
        return;
    }

    if (!s_ttsRef || !s_stop) return;
    JNIEnv* env = GetEnv();
    if (!env) return;

    env->CallVoidMethod(s_ttsRef, s_stop);
}
