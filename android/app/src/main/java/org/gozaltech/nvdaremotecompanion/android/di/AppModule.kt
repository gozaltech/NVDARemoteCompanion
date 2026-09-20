package org.gozaltech.nvdaremotecompanion.android.di

import org.gozaltech.nvdaremotecompanion.android.audio.SoundBridge
import org.gozaltech.nvdaremotecompanion.android.data.AppVersion
import org.gozaltech.nvdaremotecompanion.android.data.ClipboardRelay
import org.gozaltech.nvdaremotecompanion.android.data.ConfigTransfer
import org.gozaltech.nvdaremotecompanion.android.data.SettingsRepository
import org.gozaltech.nvdaremotecompanion.android.input.KeyEventRouter
import org.gozaltech.nvdaremotecompanion.android.input.KeyForwarder
import org.gozaltech.nvdaremotecompanion.android.remote.NvdaRemoteRepository
import org.gozaltech.nvdaremotecompanion.android.service.AccessibilityServiceStatus
import org.gozaltech.nvdaremotecompanion.android.speech.SpeechBridge
import org.gozaltech.nvdaremotecompanion.android.speech.TtsEngineCatalog
import org.gozaltech.nvdaremotecompanion.android.ui.MainViewModel
import org.gozaltech.nvdaremotecompanion.android.ui.profileedit.ProfileEditViewModel
import org.gozaltech.nvdaremotecompanion.android.ui.settings.SettingsViewModel
import org.gozaltech.nvdaremotecompanion.android.ui.update.UpdateViewModel
import org.gozaltech.nvdaremotecompanion.android.update.UpdateInstaller
import org.koin.android.ext.koin.androidContext
import org.koin.core.module.dsl.viewModelOf
import org.koin.dsl.module

val appModule = module {
    single { SettingsRepository(androidContext()) }
    single { ClipboardRelay(androidContext()) }
    single { ConfigTransfer(androidContext()) }
    single { AppVersion(androidContext()) }
    single { SpeechBridge(androidContext()) }
    single { SoundBridge(androidContext()) }
    single { TtsEngineCatalog(androidContext()) }
    single { KeyForwarder() }
    single { KeyEventRouter(get()) }
    single { AccessibilityServiceStatus(androidContext()) }
    single { UpdateInstaller(androidContext()) }
    single { NvdaRemoteRepository(androidContext(), get(), get(), get()) }

    viewModelOf(::MainViewModel)
    viewModelOf(::SettingsViewModel)
    viewModelOf(::ProfileEditViewModel)
    viewModelOf(::UpdateViewModel)
}
