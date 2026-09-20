package org.gozaltech.nvdaremotecompanion.android.ui.settings

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import org.gozaltech.nvdaremotecompanion.android.data.AppSettings
import org.gozaltech.nvdaremotecompanion.android.data.SettingsRepository
import org.gozaltech.nvdaremotecompanion.android.speech.TtsEngine
import org.gozaltech.nvdaremotecompanion.android.speech.TtsEngineCatalog

class SettingsViewModel(
    private val settingsRepository: SettingsRepository,
    private val ttsEngineCatalog: TtsEngineCatalog,
) : ViewModel() {

    val settings: StateFlow<AppSettings> =
        settingsRepository.settings.stateIn(
            scope = viewModelScope,
            started = SharingStarted.WhileSubscribed(5_000),
            initialValue = AppSettings(),
        )

    val ttsEngines: List<TtsEngine> by lazy { ttsEngineCatalog.available() }

    fun update(transform: (AppSettings) -> AppSettings) {
        viewModelScope.launch { settingsRepository.update(transform) }
    }
}
