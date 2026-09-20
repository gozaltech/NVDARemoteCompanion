package org.gozaltech.nvdaremotecompanion.android.audio

import android.content.Context
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import android.media.SoundPool
import android.util.Log
import androidx.annotation.Keep
import java.util.concurrent.ConcurrentHashMap
import kotlin.math.PI
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sin
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock

private const val TAG = "NVDARemote/Audio"
private const val TONE_SAMPLE_RATE = 44_100
private const val MAX_STREAMS = 4
private const val LOAD_SUCCESS = 0
private const val FULL_VOLUME = 1f
private const val DEFAULT_PRIORITY = 1
private const val NO_LOOP = 0
private const val NORMAL_RATE = 1f
private const val MILLIS_PER_SECOND = 1000
private const val TONE_RAMP_MS = 5
private const val TONE_BUFFER_MS = 500
private const val SOUND_DIR = "sounds"
private const val SOUND_EXTENSION = ".ogg"

@Keep
class SoundBridge(context: Context) {

    private val appContext = context.applicationContext
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    private val attributes =
        AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_MEDIA)
            .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
            .build()

    private val soundPool =
        SoundPool.Builder().setMaxStreams(MAX_STREAMS).setAudioAttributes(attributes).build()

    private val samples = ConcurrentHashMap<String, Int>()
    private val loaded: MutableSet<Int> = ConcurrentHashMap.newKeySet()
    private val pendingPlayback: MutableSet<Int> = ConcurrentHashMap.newKeySet()

    private val toneMutex = Mutex()
    private var toneTrack: AudioTrack? = null

    init {
        soundPool.setOnLoadCompleteListener { pool, sampleId, status ->
            if (status != LOAD_SUCCESS) return@setOnLoadCompleteListener
            loaded.add(sampleId)
            if (pendingPlayback.remove(sampleId)) pool.playOnce(sampleId)
        }
        scope.launch { preloadBundledSounds() }
    }

    @Keep
    fun playWave(fileName: String) {
        val name = assetNameFor(fileName)
        if (name.isEmpty()) return
        val sampleId = samples[name]
        when {
            sampleId == null -> loadAndPlay(name)
            loaded.contains(sampleId) -> soundPool.playOnce(sampleId)
            else -> pendingPlayback.add(sampleId)
        }
    }

    @Keep
    fun playTone(hz: Int, lengthMs: Int) {
        scope.launch { emitTone(hz, lengthMs) }
    }

    private fun preloadBundledSounds() {
        val files = runCatching { appContext.assets.list(SOUND_DIR) }.getOrNull().orEmpty()
        files
            .filter { it.endsWith(SOUND_EXTENSION) }
            .forEach { file -> load(file.removeSuffix(SOUND_EXTENSION)) }
    }

    private fun load(name: String): Int? =
        samples[name]
            ?: runCatching {
                appContext.assets.openFd("$SOUND_DIR/$name$SOUND_EXTENSION").use {
                    soundPool.load(it, 1)
                }
            }
                .onSuccess { samples[name] = it }
                .onFailure { Log.w(TAG, "Could not load sound '$name': ${it.message}") }
                .getOrNull()

    private fun loadAndPlay(name: String) {
        scope.launch { load(name)?.let(pendingPlayback::add) }
    }

    private suspend fun emitTone(hz: Int, lengthMs: Int) = toneMutex.withLock {
        val buffer = toneSamples(hz, lengthMs) ?: return@withLock
        runCatching {
            val track = obtainToneTrack()
            track.play()
            track.write(buffer, 0, buffer.size)
            track.stop()
        }
            .onFailure { Log.w(TAG, "Tone playback failed: ${it.message}") }
    }

    private fun obtainToneTrack(): AudioTrack =
        toneTrack
            ?: AudioTrack.Builder()
                .setAudioAttributes(attributes)
                .setAudioFormat(
                    AudioFormat.Builder()
                        .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                        .setSampleRate(TONE_SAMPLE_RATE)
                        .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                        .build()
                )
                .setBufferSizeInBytes(toneBufferSize())
                .setTransferMode(AudioTrack.MODE_STREAM)
                .build()
                .also { toneTrack = it }

    private fun toneBufferSize(): Int {
        val minimum =
            AudioTrack.getMinBufferSize(
                TONE_SAMPLE_RATE,
                AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
            )
        val preferred = TONE_SAMPLE_RATE * TONE_BUFFER_MS / MILLIS_PER_SECOND * Short.SIZE_BYTES
        return max(minimum, preferred)
    }

    private fun toneSamples(hz: Int, lengthMs: Int): ShortArray? {
        val count = TONE_SAMPLE_RATE * lengthMs / MILLIS_PER_SECOND
        if (count <= 0) return null
        val ramp = min(TONE_SAMPLE_RATE * TONE_RAMP_MS / MILLIS_PER_SECOND, count / 2)
        return ShortArray(count) { index ->
            val envelope =
                when {
                    ramp <= 0 -> 1.0
                    index < ramp -> index.toDouble() / ramp
                    index >= count - ramp -> (count - index).toDouble() / ramp
                    else -> 1.0
                }
            val sample = Short.MAX_VALUE * envelope * sin(2.0 * PI * hz * index / TONE_SAMPLE_RATE)
            sample.toInt().toShort()
        }
    }
}

private fun assetNameFor(fileName: String): String =
    fileName.substringAfterLast('\\').substringAfterLast('/').substringBeforeLast('.')

private fun SoundPool.playOnce(sampleId: Int) {
    play(sampleId, FULL_VOLUME, FULL_VOLUME, DEFAULT_PRIORITY, NO_LOOP, NORMAL_RATE)
}
