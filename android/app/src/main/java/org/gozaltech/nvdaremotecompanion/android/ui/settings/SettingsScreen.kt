package org.gozaltech.nvdaremotecompanion.android.ui.settings

import android.annotation.SuppressLint
import android.app.LocaleConfig
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.PowerManager
import android.provider.Settings
import android.widget.Toast
import androidx.activity.compose.LocalActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.core.net.toUri
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import java.util.Locale
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.data.AppLocales
import org.gozaltech.nvdaremotecompanion.android.data.AppSettings
import org.gozaltech.nvdaremotecompanion.android.data.SpeechMode
import org.gozaltech.nvdaremotecompanion.android.data.ThemeMode
import org.gozaltech.nvdaremotecompanion.android.ui.MainViewModel
import org.gozaltech.nvdaremotecompanion.android.ui.common.ChoiceRow
import org.gozaltech.nvdaremotecompanion.android.ui.common.LabeledSlider
import org.gozaltech.nvdaremotecompanion.android.ui.common.OutlinedActionButton
import org.gozaltech.nvdaremotecompanion.android.ui.common.SectionHeader
import org.gozaltech.nvdaremotecompanion.android.ui.common.SwitchRow
import org.koin.androidx.compose.koinViewModel

private const val EXPORT_FILE_NAME = "nvdaremote_config.json"
private val ImportMimeTypes = arrayOf("application/json", "application/octet-stream", "*/*")

@Composable
fun SettingsScreen(
    mainViewModel: MainViewModel,
    viewModel: SettingsViewModel = koinViewModel(),
) {
    val context = LocalContext.current
    val settings by viewModel.settings.collectAsStateWithLifecycle()

    var showAccessibilityDialog by remember { mutableStateOf(false) }
    var showEnginePicker by remember { mutableStateOf(false) }
    var showLanguagePicker by remember { mutableStateOf(false) }
    var showThemePicker by remember { mutableStateOf(false) }
    var showSpeechModePicker by remember { mutableStateOf(false) }

    val exportLauncher =
        rememberLauncherForActivityResult(
            ActivityResultContracts.CreateDocument("application/json")
        ) { uri ->
            uri?.let(mainViewModel::exportConfig)
        }

    val importLauncher =
        rememberLauncherForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
            uri?.let(mainViewModel::importConfig)
        }

    Column(
        modifier = Modifier.fillMaxWidth().verticalScroll(rememberScrollState()).padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        Button(onClick = { showAccessibilityDialog = true }, modifier = Modifier.fillMaxWidth()) {
            Text(stringResource(R.string.enable_accessibility_service))
        }
        OutlinedActionButton(
            text = stringResource(R.string.battery_optimization),
            onClick = { requestBatteryExemption(context) },
        )

        SwitchRow(
            label = stringResource(R.string.pref_auto_connect),
            checked = settings.autoConnect,
            onCheckedChange = { value -> viewModel.update { it.copy(autoConnect = value) } },
        )
        SwitchRow(
            label = stringResource(R.string.pref_autostart),
            checked = settings.autostart,
            onCheckedChange = { value -> viewModel.update { it.copy(autostart = value) } },
        )

        SectionHeader(stringResource(R.string.appearance_header))

        OutlinedActionButton(
            text = "${stringResource(R.string.pref_theme)}: ${themeLabel(settings.themeMode)}",
            onClick = { showThemePicker = true },
        )

        SectionHeader(stringResource(R.string.speech_output_header))

        OutlinedActionButton(
            text =
                "${stringResource(R.string.speech_output_header)}: " +
                    speechModeLabel(settings.speechMode),
            onClick = { showSpeechModePicker = true },
        )

        if (settings.speechMode == SpeechMode.Tts) {
            OutlinedActionButton(
                text = stringResource(R.string.tts_engine),
                onClick = { showEnginePicker = true },
            )
            SwitchRow(
                label = stringResource(R.string.pref_accessibility_stream),
                checked = settings.accessibilityStream,
                onCheckedChange = { value ->
                    viewModel.update { it.copy(accessibilityStream = value) }
                },
            )
            LabeledSlider(
                label = stringResource(R.string.tts_pitch),
                value = settings.pitch,
                range = AppSettings.PitchRange,
            ) { value ->
                viewModel.update { it.copy(pitch = value) }
            }
            LabeledSlider(
                label = stringResource(R.string.tts_rate),
                value = settings.rate,
                range = AppSettings.RateRange,
            ) { value ->
                viewModel.update { it.copy(rate = value) }
            }
            LabeledSlider(
                label = stringResource(R.string.tts_volume),
                value = settings.volume,
                range = AppSettings.VolumeRange,
            ) { value ->
                viewModel.update { it.copy(volume = value) }
            }
        }

        OutlinedActionButton(
            text = "${stringResource(R.string.pref_language)}: ${currentLanguageLabel()}",
            onClick = { showLanguagePicker = true },
        )
        OutlinedActionButton(
            text = stringResource(R.string.export_config),
            onClick = { exportLauncher.launch(EXPORT_FILE_NAME) },
        )
        OutlinedActionButton(
            text = stringResource(R.string.import_config),
            onClick = { importLauncher.launch(ImportMimeTypes) },
        )
    }

    if (showAccessibilityDialog) {
        AccessibilityDialog(
            onConfirm = {
                showAccessibilityDialog = false
                context.startActivity(Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS))
            },
            onDismiss = { showAccessibilityDialog = false },
        )
    }

    if (showEnginePicker) {
        val engines = viewModel.ttsEngines
        val labels = listOf(stringResource(R.string.tts_engine_default)) + engines.map { it.label }
        val packages = listOf<String?>(null) + engines.map { it.packageName }
        SingleChoiceDialog(
            title = stringResource(R.string.select_tts_engine),
            labels = labels,
            selectedIndex = packages.indexOf(settings.ttsEngine).coerceAtLeast(0),
            onSelected = { index ->
                viewModel.update { it.copy(ttsEngine = packages[index]) }
                showEnginePicker = false
            },
            onDismiss = { showEnginePicker = false },
        )
    }

    if (showSpeechModePicker) {
        val modes = SpeechMode.entries
        SingleChoiceDialog(
            title = stringResource(R.string.speech_output_header),
            labels = modes.map { speechModeLabel(it) },
            selectedIndex = modes.indexOf(settings.speechMode),
            onSelected = { index ->
                viewModel.update { it.copy(speechMode = modes[index]) }
                showSpeechModePicker = false
            },
            onDismiss = { showSpeechModePicker = false },
        )
    }

    if (showThemePicker) {
        val modes = ThemeMode.entries
        SingleChoiceDialog(
            title = stringResource(R.string.pref_theme),
            labels = modes.map { themeLabel(it) },
            selectedIndex = modes.indexOf(settings.themeMode),
            onSelected = { index ->
                viewModel.update { it.copy(themeMode = modes[index]) }
                showThemePicker = false
            },
            onDismiss = { showThemePicker = false },
        )
    }

    if (showLanguagePicker) {
        LanguagePickerDialog(onDismiss = { showLanguagePicker = false })
    }
}

@Composable
private fun AccessibilityDialog(onConfirm: () -> Unit, onDismiss: () -> Unit) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(stringResource(R.string.accessibility_dialog_title)) },
        text = { Text(stringResource(R.string.accessibility_dialog_message)) },
        confirmButton = {
            TextButton(onClick = onConfirm) { Text(stringResource(R.string.open_settings)) }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) { Text(stringResource(android.R.string.cancel)) }
        },
    )
}

@Composable
private fun SingleChoiceDialog(
    title: String,
    labels: List<String>,
    selectedIndex: Int,
    onSelected: (Int) -> Unit,
    onDismiss: () -> Unit,
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(title) },
        text = {
            Column(modifier = Modifier.verticalScroll(rememberScrollState())) {
                labels.forEachIndexed { index, label ->
                    ChoiceRow(
                        label = label,
                        selected = index == selectedIndex,
                        onClick = { onSelected(index) },
                    )
                }
            }
        },
        confirmButton = {},
        dismissButton = {
            TextButton(onClick = onDismiss) { Text(stringResource(android.R.string.cancel)) }
        },
    )
}

@Composable
private fun LanguagePickerDialog(onDismiss: () -> Unit) {
    val context = LocalContext.current
    val activity = LocalActivity.current
    val tags = remember { supportedLocaleTags(context) }
    val labels =
        listOf(stringResource(R.string.language_system)) +
            tags.map { tag -> Locale.forLanguageTag(tag).let { it.getDisplayName(it) } }

    val currentLanguage = AppLocales.current(context)?.language
    val selectedIndex =
        tags
            .indexOfFirst { Locale.forLanguageTag(it).language == currentLanguage }
            .let { if (it >= 0) it + 1 else 0 }

    SingleChoiceDialog(
        title = stringResource(R.string.pref_language),
        labels = labels,
        selectedIndex = selectedIndex,
        onSelected = { index ->
            AppLocales.set(context, tags.getOrNull(index - 1)?.let(Locale::forLanguageTag))
            onDismiss()
            if (!AppLocales.isSystemManaged) activity?.recreate()
        },
        onDismiss = onDismiss,
    )
}

@Composable
private fun speechModeLabel(mode: SpeechMode): String =
    when (mode) {
        SpeechMode.Tts -> stringResource(R.string.speech_mode_tts)
        SpeechMode.ScreenReader -> stringResource(R.string.speech_mode_screen_reader)
    }

@Composable
private fun themeLabel(mode: ThemeMode): String =
    when (mode) {
        ThemeMode.System -> stringResource(R.string.theme_system)
        ThemeMode.Light -> stringResource(R.string.theme_light)
        ThemeMode.Dark -> stringResource(R.string.theme_dark)
    }

@Composable
private fun currentLanguageLabel(): String {
    val locale =
        AppLocales.current(LocalContext.current) ?: return stringResource(R.string.language_system)
    return locale.getDisplayName(locale)
}

private fun supportedLocaleTags(context: Context): List<String> =
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        val supported = LocaleConfig(context).supportedLocales
        (0 until (supported?.size() ?: 0)).mapNotNull { supported?.get(it)?.toLanguageTag() }
    } else {
        context.assets.locales
            .map { Locale.forLanguageTag(it).language }
            .filter { it.isNotEmpty() }
            .distinct()
    }

@SuppressLint("BatteryLife")
private fun requestBatteryExemption(context: Context) {
    val powerManager = context.getSystemService(PowerManager::class.java)
    if (powerManager.isIgnoringBatteryOptimizations(context.packageName)) {
        Toast.makeText(context, R.string.battery_optimization_already_exempt, Toast.LENGTH_SHORT)
            .show()
        return
    }
    context.startActivity(
        Intent(
            Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS,
            "package:${context.packageName}".toUri(),
        )
    )
}
