package org.gozaltech.nvdaremotecompanion.android.ui.update

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import java.io.File
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import org.gozaltech.nvdaremotecompanion.android.data.AppVersion
import org.gozaltech.nvdaremotecompanion.android.update.DownloadState
import org.gozaltech.nvdaremotecompanion.android.update.UpdateChecker
import org.gozaltech.nvdaremotecompanion.android.update.UpdateInfo
import org.gozaltech.nvdaremotecompanion.android.update.UpdateInstaller

sealed interface UpdateUiState {
    data object Hidden : UpdateUiState

    data object Checking : UpdateUiState

    data object UpToDate : UpdateUiState

    data object Failed : UpdateUiState

    data object SignatureMismatch : UpdateUiState

    data class Available(val info: UpdateInfo) : UpdateUiState

    data class Downloading(val percent: Int) : UpdateUiState

    data class ReadyToInstall(val info: UpdateInfo, val apk: File) : UpdateUiState
}

class UpdateViewModel(
    private val appVersion: AppVersion,
    private val installer: UpdateInstaller,
) : ViewModel() {

    val state: StateFlow<UpdateUiState>
        field = MutableStateFlow<UpdateUiState>(UpdateUiState.Hidden)

    private var downloadJob: Job? = null
    private var silentCheckDone = false

    fun checkOnLaunch() {
        if (silentCheckDone) return
        silentCheckDone = true
        viewModelScope.launch {
            UpdateChecker.check(appVersion.name).getOrNull()?.let {
                state.value = UpdateUiState.Available(it)
            }
        }
    }

    fun check() {
        state.value = UpdateUiState.Checking
        viewModelScope.launch {
            UpdateChecker.check(appVersion.name)
                .onSuccess { info ->
                    state.value =
                        info?.let { UpdateUiState.Available(it) } ?: UpdateUiState.UpToDate
                }
                .onFailure { state.value = UpdateUiState.Failed }
        }
    }

    fun download(info: UpdateInfo) {
        downloadJob?.cancel()
        state.value = UpdateUiState.Downloading(0)
        downloadJob = viewModelScope.launch {
            runCatching {
                installer.download(info.downloadUrl).collect { progress ->
                    when (progress) {
                        is DownloadState.Progress ->
                            state.value = UpdateUiState.Downloading(progress.percent)

                        is DownloadState.Complete -> onDownloaded(info, progress.apk)
                    }
                }
            }
                .onFailure { state.value = UpdateUiState.Failed }
        }
    }

    fun cancelDownload() {
        downloadJob?.cancel()
        downloadJob = null
        state.value = UpdateUiState.Hidden
    }

    fun install(apk: File) {
        installer.install(apk)
        state.value = UpdateUiState.Hidden
    }

    fun dismiss() {
        state.value = UpdateUiState.Hidden
    }

    private suspend fun onDownloaded(info: UpdateInfo, apk: File) {
        if (installer.hasMatchingSignature(apk)) {
            state.value = UpdateUiState.ReadyToInstall(info, apk)
        } else {
            apk.delete()
            state.value = UpdateUiState.SignatureMismatch
        }
    }
}
