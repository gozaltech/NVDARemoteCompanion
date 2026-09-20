package org.gozaltech.nvdaremotecompanion.android.data

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context

private const val CLIP_LABEL = "NVDARemote"

class ClipboardRelay(private val context: Context) {

    private val manager: ClipboardManager?
        get() = context.getSystemService(ClipboardManager::class.java)

    fun read(): String? =
        manager
            ?.primaryClip
            ?.takeIf { it.itemCount > 0 }
            ?.getItemAt(0)
            ?.text
            ?.toString()
            ?.takeIf { it.isNotEmpty() }

    fun write(text: String) {
        manager?.setPrimaryClip(ClipData.newPlainText(CLIP_LABEL, text))
    }
}
