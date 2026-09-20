package org.gozaltech.nvdaremotecompanion.android.speech

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.speech.tts.TextToSpeech

data class TtsEngine(val packageName: String, val label: String)

class TtsEngineCatalog(private val context: Context) {

    fun available(): List<TtsEngine> = runCatching {
        val manager = context.packageManager
        manager
            .queryIntentServices(
                Intent(TextToSpeech.Engine.INTENT_ACTION_TTS_SERVICE),
                PackageManager.MATCH_ALL,
            )
            .map { TtsEngine(it.serviceInfo.packageName, it.loadLabel(manager).toString()) }
    }
        .getOrDefault(emptyList())
}
