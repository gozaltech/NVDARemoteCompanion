#pragma once
#include <jni.h>

namespace AndroidSpeech {
    void Initialize(JNIEnv* env, jobject ttsManagerRef);
    void Cleanup(JNIEnv* env);
}
