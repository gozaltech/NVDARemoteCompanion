#include "AndroidClipboard.h"
#include "Clipboard.h"
#include "JniEnv.h"
#include <android/log.h>
#include <string>

#define TAG "NVDARemote/Clipboard"

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
    if (!g_bridgeClass || !g_onClipboardReceived) return;
    ScopedJniEnv env;
    if (!env) return;

    jstring jtext = env->NewStringUTF(text.c_str());
    env->CallStaticVoidMethod(g_bridgeClass, g_onClipboardReceived, jtext);
    env->DeleteLocalRef(jtext);
}
