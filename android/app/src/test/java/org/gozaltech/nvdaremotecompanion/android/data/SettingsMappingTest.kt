package org.gozaltech.nvdaremotecompanion.android.data

import androidx.datastore.preferences.core.mutablePreferencesOf
import kotlin.test.assertEquals
import kotlin.test.assertNull
import org.junit.Test

class SettingsMappingTest {

    @Test
    fun `empty preferences produce defaults`() {
        val settings = mutablePreferencesOf().toAppSettings()
        assertEquals(AppSettings(), settings)
        assertNull(settings.ttsEngine)
        assertEquals(SpeechMode.Tts, settings.speechMode)
        assertEquals(ThemeMode.System, settings.themeMode)
    }

    @Test
    fun `settings survive a write and read round trip`() {
        val original =
            AppSettings(
                ttsEngine = "com.example.tts",
                autoConnect = true,
                autostart = true,
                accessibilityStream = true,
                speechMode = SpeechMode.ScreenReader,
                themeMode = ThemeMode.Dark,
                pitch = 1.4f,
                rate = 0.8f,
                volume = 0.5f,
            )
        val prefs = mutablePreferencesOf().apply { writeSettings(original) }
        assertEquals(original, prefs.toAppSettings())
    }

    @Test
    fun `clearing the tts engine removes the stored key`() {
        val prefs = mutablePreferencesOf()
        prefs.writeSettings(AppSettings(ttsEngine = "com.example.tts"))
        assertEquals("com.example.tts", prefs.toAppSettings().ttsEngine)

        prefs.writeSettings(AppSettings(ttsEngine = null))
        assertNull(prefs.toAppSettings().ttsEngine)
        assertNull(prefs[SettingsKeys.TtsEngine])
    }

    @Test
    fun `an unrecognised stored theme falls back to the default`() {
        val prefs = mutablePreferencesOf(SettingsKeys.ThemeMode to "Solarized")
        assertEquals(ThemeMode.System, prefs.toAppSettings().themeMode)
    }

    @Test
    fun `legacy screen reader flag maps onto the speech mode enum`() {
        val prefs = mutablePreferencesOf(SettingsKeys.ScreenReaderMode to true)
        assertEquals(SpeechMode.ScreenReader, prefs.toAppSettings().speechMode)

        val disabled = mutablePreferencesOf(SettingsKeys.ScreenReaderMode to false)
        assertEquals(SpeechMode.Tts, disabled.toAppSettings().speechMode)
    }
}
