package org.gozaltech.nvdaremotecompanion.android.data

import android.content.Context
import androidx.datastore.core.DataStore
import androidx.datastore.preferences.SharedPreferencesMigration
import androidx.datastore.preferences.core.MutablePreferences
import androidx.datastore.preferences.core.Preferences
import androidx.datastore.preferences.core.booleanPreferencesKey
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.floatPreferencesKey
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.runBlocking

private const val LEGACY_PREFS_NAME = "nvdaremote_settings"

private val Context.settingsStore: DataStore<Preferences> by
    preferencesDataStore(
        name = "settings",
        produceMigrations = { listOf(SharedPreferencesMigration(it, LEGACY_PREFS_NAME)) },
    )

internal object SettingsKeys {
    val TtsEngine = stringPreferencesKey("tts_engine")
    val AutoConnect = booleanPreferencesKey("auto_connect_on_start")
    val Autostart = booleanPreferencesKey("autostart_service")
    val AccessibilityStream = booleanPreferencesKey("tts_accessibility_stream")
    val ScreenReaderMode = booleanPreferencesKey("screen_reader_mode")
    val ThemeMode = stringPreferencesKey("theme_mode")
    val Pitch = floatPreferencesKey("tts_pitch")
    val Rate = floatPreferencesKey("tts_rate")
    val Volume = floatPreferencesKey("tts_volume")
}

internal fun Preferences.toAppSettings(): AppSettings {
    val defaults = AppSettings()
    return AppSettings(
        ttsEngine = this[SettingsKeys.TtsEngine],
        autoConnect = this[SettingsKeys.AutoConnect] ?: defaults.autoConnect,
        autostart = this[SettingsKeys.Autostart] ?: defaults.autostart,
        accessibilityStream =
            this[SettingsKeys.AccessibilityStream] ?: defaults.accessibilityStream,
        speechMode =
            if (this[SettingsKeys.ScreenReaderMode] == true) {
                SpeechMode.ScreenReader
            } else {
                SpeechMode.Tts
            },
        themeMode =
            this[SettingsKeys.ThemeMode]?.let { stored ->
                ThemeMode.entries.firstOrNull { it.name == stored }
            } ?: defaults.themeMode,
        pitch = this[SettingsKeys.Pitch] ?: defaults.pitch,
        rate = this[SettingsKeys.Rate] ?: defaults.rate,
        volume = this[SettingsKeys.Volume] ?: defaults.volume,
    )
}

internal fun MutablePreferences.writeSettings(settings: AppSettings) {
    val engine = settings.ttsEngine
    if (engine == null) remove(SettingsKeys.TtsEngine) else this[SettingsKeys.TtsEngine] = engine
    this[SettingsKeys.AutoConnect] = settings.autoConnect
    this[SettingsKeys.Autostart] = settings.autostart
    this[SettingsKeys.AccessibilityStream] = settings.accessibilityStream
    this[SettingsKeys.ScreenReaderMode] = settings.speechMode == SpeechMode.ScreenReader
    this[SettingsKeys.ThemeMode] = settings.themeMode.name
    this[SettingsKeys.Pitch] = settings.pitch
    this[SettingsKeys.Rate] = settings.rate
    this[SettingsKeys.Volume] = settings.volume
}

class SettingsRepository(private val context: Context) {

    val settings: Flow<AppSettings> = context.settingsStore.data.map { it.toAppSettings() }

    suspend fun current(): AppSettings = settings.first()

    fun currentBlocking(): AppSettings = runBlocking { current() }

    suspend fun update(transform: (AppSettings) -> AppSettings) {
        context.settingsStore.edit { prefs ->
            prefs.writeSettings(transform(prefs.toAppSettings()))
        }
    }
}
