package org.gozaltech.nvdaremotecompanion.android.speech

import android.content.Context
import androidx.annotation.Keep
import org.gozaltech.nvdaremotecompanion.android.data.AppSettings
import org.gozaltech.nvdaremotecompanion.android.data.SpeechMode

@Keep
class SpeechBridge(context: Context) {

    private val appContext = context.applicationContext
    private val screenReader = ScreenReaderSpeechOutput(appContext)

    private var tts: TtsSpeechOutput? = null
    private var ttsEngine: String? = null

    @Volatile private var delegate: SpeechOutput = screenReader

    @Keep
    fun speak(text: String, interrupt: Boolean) {
        delegate.speak(text, interrupt)
    }

    @Keep
    fun stop() {
        delegate.stop()
    }

    @Synchronized
    fun configure(settings: AppSettings) {
        if (settings.speechMode == SpeechMode.ScreenReader) {
            delegate = screenReader
            releaseTts()
            return
        }
        val engine = existingTts(settings.ttsEngine) ?: createTts(settings.ttsEngine)
        engine.configure(settings)
        delegate = engine
    }

    @Synchronized
    fun shutdown() {
        delegate = screenReader
        releaseTts()
    }

    private fun existingTts(enginePackage: String?): TtsSpeechOutput? = tts?.takeIf {
        ttsEngine == enginePackage
    }

    private fun createTts(enginePackage: String?): TtsSpeechOutput {
        releaseTts()
        return TtsSpeechOutput(appContext, enginePackage).also {
            tts = it
            ttsEngine = enginePackage
        }
    }

    private fun releaseTts() {
        tts?.shutdown()
        tts = null
        ttsEngine = null
    }
}
