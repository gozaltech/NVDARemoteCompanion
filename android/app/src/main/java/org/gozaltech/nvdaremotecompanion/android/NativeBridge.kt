package org.gozaltech.nvdaremotecompanion.android

import androidx.annotation.Keep

interface NativeEvents {
    fun onConnectionStateChanged(profileIndex: Int, connected: Boolean)

    fun onForwardingStateChanged(forwarding: Boolean)

    fun onClipboardShortcutTriggered()

    fun onClipboardTextReceived(text: String)
}

@Keep
object NativeBridge {

    init {
        System.loadLibrary("nvdaremote")
    }

    @Volatile @JvmStatic var events: NativeEvents? = null

    external fun nativeInit(speech: Any, audio: Any, configDirPath: String)

    external fun nativeShutdown()

    external fun nativeConnect(profileIndex: Int): Boolean

    external fun nativeDisconnect(profileIndex: Int)

    external fun nativeIsConnected(profileIndex: Int): Boolean

    external fun nativeGetProfileCount(): Int

    external fun nativeGetProfileName(profileIndex: Int): String

    external fun nativeGetProfileJson(profileIndex: Int): String

    external fun nativeGetAutoConnect(profileIndex: Int): Boolean

    external fun nativeSaveProfile(
        profileIndex: Int,
        name: String,
        host: String,
        port: Int,
        key: String,
        speech: Boolean,
        sounds: Boolean,
        mute: Boolean,
        autoConnect: Boolean,
    )

    external fun nativeDeleteProfile(profileIndex: Int)

    external fun nativeGetConfigJson(): String

    external fun nativeMergeConfig(configJson: String): Int

    external fun nativeSetActiveProfile(profileIndex: Int)

    external fun nativeGetActiveProfile(): Int

    external fun nativeIsSendingKeys(): Boolean

    external fun nativeToggleForwarding()

    external fun nativeSendKeyEvent(
        vkCode: Int,
        scanCode: Int,
        pressed: Boolean,
        extended: Boolean,
        profileIndex: Int,
    )

    external fun nativeProcessModifiersAndShortcuts(vkCode: Int, pressed: Boolean): Boolean

    external fun nativeSendClipboardText(text: String, profileIndex: Int)

    @Keep
    @JvmStatic
    fun onConnectionStateChanged(profileIndex: Int, connected: Boolean) {
        events?.onConnectionStateChanged(profileIndex, connected)
    }

    @Keep
    @JvmStatic
    fun onForwardingStateChanged(forwarding: Boolean) {
        events?.onForwardingStateChanged(forwarding)
    }

    @Keep
    @JvmStatic
    fun onClipboardShortcutTriggered() {
        events?.onClipboardShortcutTriggered()
    }

    @Keep
    @JvmStatic
    fun onClipboardTextReceived(text: String) {
        events?.onClipboardTextReceived(text)
    }
}
