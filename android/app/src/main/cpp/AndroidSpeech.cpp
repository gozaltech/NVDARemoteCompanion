#include "AndroidSpeech.h"
#include "Speech.h"
#include "JniEnv.h"
#include <android/log.h>
#include <string>

#define TAG "NVDARemote/Speech"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

bool Speech::s_initialized = false;
bool Speech::s_enabled = true;

static jobject  s_ttsRef    = nullptr;
static jmethodID s_speak    = nullptr;
static jmethodID s_stop     = nullptr;

void AndroidSpeech::Initialize(JNIEnv* env, jobject ttsManagerRef) {
    s_ttsRef = env->NewGlobalRef(ttsManagerRef);

    jclass cls = env->GetObjectClass(ttsManagerRef);
    s_speak = env->GetMethodID(cls, "speak", "(Ljava/lang/String;Z)V");
    s_stop  = env->GetMethodID(cls, "stop",  "()V");
    env->DeleteLocalRef(cls);

    if (!s_speak || !s_stop) {
        LOGE("Failed to find TtsManager methods — check Kotlin method signatures");
    }

    Speech::Initialize();
}

void AndroidSpeech::Cleanup(JNIEnv* env) {
    if (s_ttsRef) {
        env->DeleteGlobalRef(s_ttsRef);
        s_ttsRef = nullptr;
    }
    s_speak = nullptr;
    s_stop  = nullptr;
    Speech::Cleanup();
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
    if (!s_ttsRef || !s_speak) return;
    ScopedJniEnv env;
    if (!env) return;

    jstring jtext = env->NewStringUTF(std::string(text).c_str());
    env->CallVoidMethod(s_ttsRef, s_speak, jtext, static_cast<jboolean>(interrupt));
    env->DeleteLocalRef(jtext);
}

void Speech::SpeakSsml(std::string_view ssml, bool interrupt) {
    Speech::Speak(ssml, interrupt);
}

void Speech::Stop() {
    if (!s_initialized || !s_ttsRef || !s_stop) return;
    ScopedJniEnv env;
    if (!env) return;

    env->CallVoidMethod(s_ttsRef, s_stop);
}
