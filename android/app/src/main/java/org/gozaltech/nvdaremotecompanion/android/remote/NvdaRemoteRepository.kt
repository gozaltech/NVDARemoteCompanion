package org.gozaltech.nvdaremotecompanion.android.remote

import android.content.Context
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import org.gozaltech.nvdaremotecompanion.android.NativeBridge
import org.gozaltech.nvdaremotecompanion.android.NativeEvents
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.audio.SoundBridge
import org.gozaltech.nvdaremotecompanion.android.data.ClipboardRelay
import org.gozaltech.nvdaremotecompanion.android.data.Profile
import org.gozaltech.nvdaremotecompanion.android.speech.SpeechBridge

class NvdaRemoteRepository(
    private val context: Context,
    private val speech: SpeechBridge,
    private val sound: SoundBridge,
    private val clipboard: ClipboardRelay,
) : NativeEvents {

    private val json = Json { ignoreUnknownKeys = true }
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private val mainScope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)

    private val desired = MutableStateFlow<Set<Int>>(emptySet())

    val state: StateFlow<RemoteState>
        field = MutableStateFlow(RemoteState())

    private var started = false

    val activeIndex: Int
        get() = NativeBridge.nativeGetActiveProfile()

    val isSendingKeys: Boolean
        get() = NativeBridge.nativeIsSendingKeys()

    @Synchronized
    fun start() {
        if (started) return
        NativeBridge.events = this
        NativeBridge.nativeInit(speech, sound, context.filesDir.absolutePath)
        started = true
        refresh()
    }

    @Synchronized
    fun shutdown() {
        if (!started) return
        state.value.profiles
            .filter { it.connected }
            .forEach { NativeBridge.nativeDisconnect(it.index) }
        NativeBridge.nativeShutdown()
        NativeBridge.events = null
        started = false
    }

    suspend fun connect(index: Int): Boolean =
        withContext(Dispatchers.IO) {
            desired.update { it + index }
            NativeBridge.nativeConnect(index).also { ok ->
                if (!ok) desired.update { it - index }
                refresh()
            }
        }

    suspend fun disconnect(index: Int) =
        withContext(Dispatchers.IO) {
            desired.update { it - index }
            NativeBridge.nativeDisconnect(index)
            refresh()
        }

    fun setActiveProfile(index: Int) {
        scope.launch {
            NativeBridge.nativeSetActiveProfile(index)
            refresh()
        }
    }

    fun toggleForwarding() {
        scope.launch {
            NativeBridge.nativeToggleForwarding()
            refresh()
        }
    }

    fun autoConnectAll() {
        scope.launch {
            for (index in 0 until NativeBridge.nativeGetProfileCount()) {
                if (NativeBridge.nativeGetAutoConnect(index)) connect(index)
            }
        }
    }

    fun reconnectDesired() {
        scope.launch {
            desired.value.forEach { index ->
                if (!NativeBridge.nativeIsConnected(index)) connect(index)
            }
        }
    }

    fun suspendConnections() {
        scope.launch {
            desired.value.forEach { index ->
                if (NativeBridge.nativeIsConnected(index)) NativeBridge.nativeDisconnect(index)
            }
            refresh()
        }
    }

    suspend fun profile(index: Int): Profile =
        withContext(Dispatchers.IO) {
            json.decodeFromString(NativeBridge.nativeGetProfileJson(index))
        }

    suspend fun saveProfile(index: Int, profile: Profile) =
        withContext(Dispatchers.IO) {
            NativeBridge.nativeSaveProfile(
                index,
                profile.name,
                profile.host,
                profile.port,
                profile.key,
                profile.speech,
                profile.sounds,
                profile.mute,
                profile.autoConnect,
            )
            refresh()
        }

    suspend fun deleteProfile(index: Int) =
        withContext(Dispatchers.IO) {
            if (NativeBridge.nativeIsConnected(index)) NativeBridge.nativeDisconnect(index)
            NativeBridge.nativeDeleteProfile(index)
            desired.update { set ->
                set.filterNot { it == index }.map { if (it > index) it - 1 else it }.toSet()
            }
            refresh()
        }

    fun sendClipboard(index: Int): Boolean {
        val text = clipboard.read() ?: return false
        NativeBridge.nativeSendClipboardText(text, index)
        return true
    }

    fun refreshState() = refresh()

    private fun refresh() {
        val fallbackName = context.getString(R.string.app_name)
        val profiles =
            (0 until NativeBridge.nativeGetProfileCount()).map { index ->
                RemoteProfile(
                    index = index,
                    name = NativeBridge.nativeGetProfileName(index).ifBlank { fallbackName },
                    connected = NativeBridge.nativeIsConnected(index),
                )
            }
        state.value =
            RemoteState(
                profiles = profiles,
                activeIndex = NativeBridge.nativeGetActiveProfile(),
                sendingKeys = NativeBridge.nativeIsSendingKeys(),
            )
    }

    override fun onConnectionStateChanged(profileIndex: Int, connected: Boolean) = refresh()

    override fun onForwardingStateChanged(forwarding: Boolean) = refresh()

    override fun onClipboardShortcutTriggered() {
        mainScope.launch {
            val active = activeIndex
            if (active >= 0) sendClipboard(active)
        }
    }

    override fun onClipboardTextReceived(text: String) {
        mainScope.launch { clipboard.write(text) }
    }
}
