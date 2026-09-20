package org.gozaltech.nvdaremotecompanion.android.speech

import android.content.Context
import android.media.AudioAttributes
import android.media.AudioManager
import android.os.Bundle
import android.speech.tts.TextToSpeech
import android.util.Log
import java.util.Locale
import org.gozaltech.nvdaremotecompanion.android.data.AppSettings

private const val TAG = "NVDARemote/TTS"

private data class Utterance(val text: String, val interrupt: Boolean)

class TtsSpeechOutput(
    context: Context,
    enginePackage: String?,
) : SpeechOutput, TextToSpeech.OnInitListener {

    private val pending = ArrayDeque<Utterance>()
    private val lock = Any()

    private var ready = false
    private var speakParams: Bundle? = null
    private var settings = AppSettings()
    private var tts: TextToSpeech? =
        TextToSpeech(
            context.applicationContext,
            this,
            enginePackage,
        )

    override fun onInit(status: Int) {
        if (status != TextToSpeech.SUCCESS) {
            Log.e(TAG, "TTS initialization failed with status $status")
            return
        }
        val queued =
            synchronized(lock) {
                ready = true
                applySettings()
                pending.toList().also { pending.clear() }
            }
        queued.forEach { speakNow(it.text, it.interrupt) }
    }

    fun configure(settings: AppSettings) {
        synchronized(lock) {
            this.settings = settings
            if (ready) applySettings()
        }
    }

    override fun speak(text: String, interrupt: Boolean) {
        val deferred =
            synchronized(lock) {
                if (ready) {
                    false
                } else {
                    if (interrupt) pending.clear()
                    pending.addLast(Utterance(text, interrupt))
                    true
                }
            }
        if (!deferred) speakNow(text, interrupt)
    }

    override fun stop() {
        tts?.stop()
    }

    override fun shutdown() {
        synchronized(lock) {
            ready = false
            pending.clear()
        }
        tts?.run {
            stop()
            shutdown()
        }
        tts = null
    }

    private fun applySettings() {
        val engine = tts ?: return
        engine.setLanguage(Locale.getDefault())
        engine.setAudioAttributes(audioAttributes())
        engine.setPitch(settings.pitch)
        engine.setSpeechRate(settings.rate)
        speakParams =
            settings.volume
                .takeIf { it != 1f }
                ?.let { Bundle().apply { putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, it) } }
    }

    private fun audioAttributes(): AudioAttributes =
        AudioAttributes.Builder()
            .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
            .also { builder ->
                if (settings.accessibilityStream) {
                    builder.setUsage(AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY)
                } else {
                    builder.setLegacyStreamType(AudioManager.STREAM_MUSIC)
                }
            }
            .build()

    private fun speakNow(text: String, interrupt: Boolean) {
        val engine = tts ?: return
        val mode = if (interrupt) TextToSpeech.QUEUE_FLUSH else TextToSpeech.QUEUE_ADD
        engine.speak(text, mode, speakParams, null)
    }
}
