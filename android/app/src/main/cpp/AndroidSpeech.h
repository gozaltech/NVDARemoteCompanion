#pragma once
#include <jni.h>
#include <string>

namespace AndroidSpeech {
    void Initialize(JNIEnv* env, jobject ttsManagerRef);
    void Cleanup(JNIEnv* env);

    void SetOutputMode(int mode);
    void SetDirectEngine(const std::string& engineName);
}
