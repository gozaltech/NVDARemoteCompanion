#include "AndroidAudio.h"
#include "Audio.h"
#include <android/log.h>
#include <string>

#define TAG "NVDARemote/Audio"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static JavaVM*   s_jvm          = nullptr;
static jobject   s_audioRef     = nullptr;
static jmethodID s_playTone     = nullptr;
static jmethodID s_playWave     = nullptr;
static bool      s_enabled      = true;

static JNIEnv* GetEnv(bool& didAttach) {
    didAttach = false;
    if (!s_jvm) return nullptr;
    JNIEnv* env = nullptr;
    jint result = s_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        if (s_jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            didAttach = true;
        } else {
            return nullptr;
        }
    }
    return env;
}

void AndroidAudio::Initialize(JNIEnv* env, jobject audioManagerRef) {
    env->GetJavaVM(&s_jvm);
    s_audioRef = env->NewGlobalRef(audioManagerRef);

    jclass cls = env->GetObjectClass(audioManagerRef);
    s_playTone = env->GetMethodID(cls, "playTone", "(II)V");
    s_playWave = env->GetMethodID(cls, "playWave", "(Ljava/lang/String;)V");
    env->DeleteLocalRef(cls);

    if (!s_playTone || !s_playWave) {
        LOGE("Failed to find NvdaAudioManager methods — check Kotlin method signatures");
    }
}

void AndroidAudio::Cleanup(JNIEnv* env) {
    if (s_audioRef) {
        env->DeleteGlobalRef(s_audioRef);
        s_audioRef = nullptr;
    }
    s_playTone = nullptr;
    s_playWave = nullptr;
}

void Audio::SetEnabled(bool enabled) {
    s_enabled = enabled;
}

bool Audio::IsEnabled() {
    return s_enabled;
}

void Audio::PlayTone(int hz, int length) {
    if (!s_enabled || !s_audioRef || !s_playTone) return;

    bool didAttach = false;
    JNIEnv* env = GetEnv(didAttach);
    if (!env) return;

    env->CallVoidMethod(s_audioRef, s_playTone,
                        static_cast<jint>(hz), static_cast<jint>(length));

    if (didAttach) s_jvm->DetachCurrentThread();
}

void Audio::PlayWave(const std::string& fileName) {
    if (!s_enabled || !s_audioRef || !s_playWave) return;

    bool didAttach = false;
    JNIEnv* env = GetEnv(didAttach);
    if (!env) return;

    jstring jname = env->NewStringUTF(fileName.c_str());
    env->CallVoidMethod(s_audioRef, s_playWave, jname);
    env->DeleteLocalRef(jname);

    if (didAttach) s_jvm->DetachCurrentThread();
}
