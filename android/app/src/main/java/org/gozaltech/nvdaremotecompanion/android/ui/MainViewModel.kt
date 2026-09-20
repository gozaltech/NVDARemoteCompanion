package org.gozaltech.nvdaremotecompanion.android.ui

import android.net.Uri
import androidx.annotation.StringRes
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.launch
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.data.ConfigTransfer
import org.gozaltech.nvdaremotecompanion.android.remote.NvdaRemoteRepository
import org.gozaltech.nvdaremotecompanion.android.remote.RemoteState

class MainViewModel(
    private val repository: NvdaRemoteRepository,
    private val configTransfer: ConfigTransfer,
) : ViewModel() {

    val state: StateFlow<RemoteState> = repository.state

    private val messageChannel = Channel<UiMessage>(Channel.BUFFERED)
    val messages: Flow<UiMessage> = messageChannel.receiveAsFlow()

    fun connect(index: Int) {
        viewModelScope.launch {
            if (!repository.connect(index)) notify(R.string.error_connect_failed_generic)
        }
    }

    fun disconnect(index: Int) {
        viewModelScope.launch { repository.disconnect(index) }
    }

    fun setActiveProfile(index: Int) = repository.setActiveProfile(index)

    fun toggleForwarding() = repository.toggleForwarding()

    fun deleteProfile(index: Int) {
        viewModelScope.launch {
            repository.deleteProfile(index)
            notify(R.string.profile_deleted)
        }
    }

    fun sendClipboard(profileIndex: Int? = null) {
        val target = profileIndex ?: repository.activeIndex
        when {
            target < 0 -> notify(R.string.clipboard_not_connected)
            repository.sendClipboard(target) -> notify(R.string.clipboard_sent)
            else -> notify(R.string.clipboard_empty)
        }
    }

    fun exportConfig(uri: Uri) {
        viewModelScope.launch {
            configTransfer
                .export(uri)
                .onSuccess { notify(R.string.export_success) }
                .onFailure { notify(R.string.export_failed) }
        }
    }

    fun importConfig(uri: Uri) {
        viewModelScope.launch {
            configTransfer
                .import(uri)
                .onSuccess { added ->
                    repository.refreshState()
                    notify(
                        if (added > 0) R.string.import_success else R.string.import_no_new_profiles
                    )
                }
                .onFailure { notify(R.string.import_failed) }
        }
    }

    private fun notify(@StringRes textRes: Int) {
        messageChannel.trySend(UiMessage(textRes))
    }
}
