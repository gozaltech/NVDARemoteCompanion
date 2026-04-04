#include "AndroidClipboard.h"
#include "Clipboard.h"
#include <android/log.h>
#include <string>

#define TAG "NVDARemote/Clipboard"

extern JavaVM* g_jvm;

static jclass    g_bridgeClass         = nullptr;
static jmethodID g_onClipboardReceived = nullptr;

namespace AndroidClipboard {
    void Initialize(JNIEnv* env, jclass bridgeClass) {
        g_bridgeClass = bridgeClass;
        g_onClipboardReceived = env->GetStaticMethodID(
            bridgeClass, "onClipboardTextReceived", "(Ljava/lang/String;)V");
        if (!g_onClipboardReceived)
            __android_log_print(ANDROID_LOG_ERROR, TAG,
                                "onClipboardTextReceived method not found");
    }
}

std::string Clipboard::GetText() {
    return {};
}

void Clipboard::SetText(const std::string& text) {
    if (!g_bridgeClass || !g_onClipboardReceived || !g_jvm) return;

    bool didAttach = false;
    JNIEnv* env = nullptr;
    jint res = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (res == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) return;
        didAttach = true;
    }

    jstring jtext = env->NewStringUTF(text.c_str());
    env->CallStaticVoidMethod(g_bridgeClass, g_onClipboardReceived, jtext);
    env->DeleteLocalRef(jtext);

    if (didAttach) g_jvm->DetachCurrentThread();
}
