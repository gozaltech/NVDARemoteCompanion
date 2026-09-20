package org.gozaltech.nvdaremotecompanion.android.input

import org.gozaltech.nvdaremotecompanion.android.NativeBridge

class KeyForwarder {

    val activeProfileIndex: Int
        get() = NativeBridge.nativeGetActiveProfile()

    val isForwarding: Boolean
        get() = NativeBridge.nativeIsSendingKeys()

    fun consumeAsModifierOrShortcut(vkCode: Int, pressed: Boolean): Boolean =
        NativeBridge.nativeProcessModifiersAndShortcuts(vkCode, pressed)

    fun send(key: WinKey, pressed: Boolean, profileIndex: Int) {
        NativeBridge.nativeSendKeyEvent(key.vk, key.scan, pressed, key.extended, profileIndex)
    }
}
