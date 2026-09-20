#pragma once

#include <jni.h>
#include <string>

extern JavaVM* g_jvm;

class ScopedJniEnv {
public:
    ScopedJniEnv() {
        if (!g_jvm) return;
        const jint status = g_jvm->GetEnv(reinterpret_cast<void**>(&m_env), JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            if (g_jvm->AttachCurrentThread(&m_env, nullptr) == JNI_OK) {
                m_attached = true;
            } else {
                m_env = nullptr;
            }
        } else if (status != JNI_OK) {
            m_env = nullptr;
        }
    }

    ~ScopedJniEnv() {
        if (m_attached && g_jvm) g_jvm->DetachCurrentThread();
    }

    ScopedJniEnv(const ScopedJniEnv&) = delete;
    ScopedJniEnv& operator=(const ScopedJniEnv&) = delete;

    explicit operator bool() const { return m_env != nullptr; }
    JNIEnv* operator->() const { return m_env; }
    JNIEnv* get() const { return m_env; }

private:
    JNIEnv* m_env = nullptr;
    bool m_attached = false;
};

inline std::string JniToString(JNIEnv* env, jstring value) {
    if (!env || !value) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) return {};
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}
