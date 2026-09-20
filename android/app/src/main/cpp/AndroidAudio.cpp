#include "AndroidAudio.h"
#include "Audio.h"
#include "JniEnv.h"
#include <android/log.h>
#include <string>

#define TAG "NVDARemote/Audio"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static jobject   s_audioRef     = nullptr;
static jmethodID s_playTone     = nullptr;
static jmethodID s_playWave     = nullptr;
static bool      s_enabled      = true;

void AndroidAudio::Initialize(JNIEnv* env, jobject audioManagerRef) {
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
    ScopedJniEnv env;
    if (!env) return;

    env->CallVoidMethod(s_audioRef, s_playTone,
                        static_cast<jint>(hz), static_cast<jint>(length));
}

void Audio::PlayWave(const std::string& fileName) {
    if (!s_enabled || !s_audioRef || !s_playWave) return;
    ScopedJniEnv env;
    if (!env) return;

    jstring jname = env->NewStringUTF(fileName.c_str());
    env->CallVoidMethod(s_audioRef, s_playWave, jname);
    env->DeleteLocalRef(jname);
}
