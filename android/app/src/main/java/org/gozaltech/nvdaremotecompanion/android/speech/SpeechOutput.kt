package org.gozaltech.nvdaremotecompanion.android.speech

interface SpeechOutput {
    fun speak(text: String, interrupt: Boolean)

    fun stop()

    fun shutdown()
}
