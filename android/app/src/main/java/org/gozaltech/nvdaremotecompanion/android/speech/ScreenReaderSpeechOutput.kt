package org.gozaltech.nvdaremotecompanion.android.speech

import android.content.Context
import android.view.accessibility.AccessibilityEvent
import android.view.accessibility.AccessibilityManager

class ScreenReaderSpeechOutput(context: Context) : SpeechOutput {

    private val appContext = context.applicationContext

    private val manager: AccessibilityManager?
        get() = appContext.getSystemService(AccessibilityManager::class.java)

    @Suppress("DEPRECATION")
    override fun speak(text: String, interrupt: Boolean) {
        val manager = manager?.takeIf { it.isEnabled } ?: return
        val event = AccessibilityEvent.obtain(AccessibilityEvent.TYPE_ANNOUNCEMENT)
        event.text.add(text)
        manager.sendAccessibilityEvent(event)
    }

    override fun stop() = Unit

    override fun shutdown() = Unit
}
