#pragma once
#include <jni.h>

namespace AndroidAudio {
    void Initialize(JNIEnv* env, jobject audioManagerRef);
    void Cleanup(JNIEnv* env);
}
