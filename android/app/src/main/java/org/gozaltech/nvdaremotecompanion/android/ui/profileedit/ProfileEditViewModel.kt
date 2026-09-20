package org.gozaltech.nvdaremotecompanion.android.ui.profileedit

import androidx.annotation.StringRes
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.receiveAsFlow
import kotlinx.coroutines.launch
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.data.DEFAULT_PORT
import org.gozaltech.nvdaremotecompanion.android.data.Profile
import org.gozaltech.nvdaremotecompanion.android.remote.NvdaRemoteRepository
import org.gozaltech.nvdaremotecompanion.android.ui.UiMessage

data class ProfileForm(
    val name: String = "",
    val host: String = "",
    val port: String = DEFAULT_PORT.toString(),
    val key: String = "",
    val speech: Boolean = true,
    val sounds: Boolean = true,
    val mute: Boolean = false,
    val autoConnect: Boolean = true,
) {
    val isValid: Boolean
        get() = host.isNotBlank() && key.isNotBlank()

    fun toProfile() =
        Profile(
            name = name.trim(),
            host = host.trim(),
            port = port.toIntOrNull() ?: DEFAULT_PORT,
            key = key.trim(),
            speech = speech,
            sounds = sounds,
            mute = mute,
            autoConnect = autoConnect,
        )

    companion object {
        fun from(profile: Profile) =
            ProfileForm(
                name = profile.name,
                host = profile.host,
                port = profile.port.toString(),
                key = profile.key,
                speech = profile.speech,
                sounds = profile.sounds,
                mute = profile.mute,
                autoConnect = profile.autoConnect,
            )
    }
}

class ProfileEditViewModel(private val repository: NvdaRemoteRepository) : ViewModel() {

    val form: StateFlow<ProfileForm>
        field = MutableStateFlow(ProfileForm())

    private val messageChannel = Channel<UiMessage>(Channel.BUFFERED)
    val messages: Flow<UiMessage> = messageChannel.receiveAsFlow()

    private val closeChannel = Channel<Unit>(Channel.BUFFERED)
    val closeRequests: Flow<Unit> = closeChannel.receiveAsFlow()

    private var loaded = false

    fun load(index: Int) {
        if (loaded || index < 0) return
        loaded = true
        viewModelScope.launch {
            runCatching { repository.profile(index) }
                .onSuccess { form.value = ProfileForm.from(it) }
                .onFailure { notify(R.string.error_loading_profile) }
        }
    }

    fun update(transform: (ProfileForm) -> ProfileForm) {
        form.value = transform(form.value)
    }

    fun save(index: Int) {
        val form = form.value
        if (!form.isValid) {
            notify(R.string.error_host_key_required)
            return
        }
        viewModelScope.launch {
            runCatching { repository.saveProfile(index, form.toProfile()) }
                .onSuccess {
                    notify(R.string.profile_saved)
                    closeChannel.trySend(Unit)
                }
                .onFailure { notify(R.string.error_saving_profile) }
        }
    }

    fun delete(index: Int) {
        if (index < 0) return
        viewModelScope.launch {
            repository.deleteProfile(index)
            notify(R.string.profile_deleted)
            closeChannel.trySend(Unit)
        }
    }

    private fun notify(@StringRes textRes: Int) {
        messageChannel.trySend(UiMessage(textRes))
    }
}
