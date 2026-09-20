package org.gozaltech.nvdaremotecompanion.android.data

enum class SpeechMode {
    Tts,
    ScreenReader,
}

enum class ThemeMode {
    System,
    Light,
    Dark,
}

data class AppSettings(
    val ttsEngine: String? = null,
    val autoConnect: Boolean = false,
    val autostart: Boolean = false,
    val accessibilityStream: Boolean = false,
    val speechMode: SpeechMode = SpeechMode.Tts,
    val themeMode: ThemeMode = ThemeMode.System,
    val pitch: Float = 1f,
    val rate: Float = 1f,
    val volume: Float = 1f,
) {
    companion object {
        val PitchRange = 0.5f..2f
        val RateRange = 0.5f..2f
        val VolumeRange = 0f..1f
    }
}
