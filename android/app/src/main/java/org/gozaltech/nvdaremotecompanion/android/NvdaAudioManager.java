package org.gozaltech.nvdaremotecompanion.android;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.util.Log;

public class NvdaAudioManager {

    private static final String TAG = "NVDARemote/Audio";
    private static final int TONE_SAMPLE_RATE = 44100;

    private static class WavInfo {
        final int channels;
        final int sampleRate;
        final int bitsPerSample;
        final int dataOffset;
        final int dataSize;

        WavInfo(int channels, int sampleRate, int bitsPerSample, int dataOffset, int dataSize) {
            this.channels = channels;
            this.sampleRate = sampleRate;
            this.bitsPerSample = bitsPerSample;
            this.dataOffset = dataOffset;
            this.dataSize = dataSize;
        }
    }

    private final Context context;
    private final AudioAttributes audioAttributes;

    public NvdaAudioManager(Context context) {
        this.context = context;
        audioAttributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_MEDIA)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
    }

    public void playTone(final int hz, final int lengthMs) {
        Thread t = new Thread(() -> {
            int numSamples = TONE_SAMPLE_RATE * lengthMs / 1000;
            short[] buffer = new short[numSamples];
            for (int i = 0; i < numSamples; i++) {
                buffer[i] = (short) (Short.MAX_VALUE * Math.sin(2.0 * Math.PI * hz * i / TONE_SAMPLE_RATE));
            }

            AudioTrack track = new AudioTrack.Builder()
                    .setAudioAttributes(audioAttributes)
                    .setAudioFormat(new AudioFormat.Builder()
                            .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                            .setSampleRate(TONE_SAMPLE_RATE)
                            .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                            .build())
                    .setBufferSizeInBytes(buffer.length * 2)
                    .setTransferMode(AudioTrack.MODE_STATIC)
                    .build();

            track.write(buffer, 0, buffer.length);
            track.play();
            try {
                Thread.sleep(lengthMs + 50L);
            } catch (InterruptedException ignored) {
            }
            track.stop();
            track.release();
        }, "NVDATone");
        t.start();
    }

    public void playWave(final String fileName) {
        Thread t = new Thread(() -> {
            try {
                String assetPath = "sounds/" + fileName + ".wav";
                byte[] bytes;
                try (java.io.InputStream is = context.getAssets().open(assetPath)) {
                    bytes = readAllBytes(is);
                }

                WavInfo wav = parseWavHeader(bytes);
                if (wav == null) {
                    Log.w(TAG, "Could not parse WAV header for '" + fileName + "'");
                    return;
                }

                int channelMask = (wav.channels == 1)
                        ? AudioFormat.CHANNEL_OUT_MONO
                        : AudioFormat.CHANNEL_OUT_STEREO;

                int encoding = (wav.bitsPerSample == 8)
                        ? AudioFormat.ENCODING_PCM_8BIT
                        : AudioFormat.ENCODING_PCM_16BIT;

                int minBuf = AudioTrack.getMinBufferSize(wav.sampleRate, channelMask, encoding);
                int bufSize = Math.max(wav.dataSize, minBuf);

                AudioTrack track = new AudioTrack.Builder()
                        .setAudioAttributes(audioAttributes)
                        .setAudioFormat(new AudioFormat.Builder()
                                .setEncoding(encoding)
                                .setSampleRate(wav.sampleRate)
                                .setChannelMask(channelMask)
                                .build())
                        .setBufferSizeInBytes(bufSize)
                        .setTransferMode(AudioTrack.MODE_STATIC)
                        .build();

                track.write(bytes, wav.dataOffset, wav.dataSize);
                track.play();

                int bytesPerSample = wav.bitsPerSample / 8;
                long durationMs = (long) wav.dataSize * 1000L
                        / (wav.sampleRate * wav.channels * bytesPerSample);
                try {
                    Thread.sleep(durationMs + 100L);
                } catch (InterruptedException ignored) {
                }
                track.stop();
                track.release();

            } catch (Exception e) {
                Log.w(TAG, "Could not play wave '" + fileName + "': " + e.getMessage());
            }
        }, "NVDAWave");
        t.start();
    }

    private static byte[] readAllBytes(java.io.InputStream is) throws java.io.IOException {
        java.io.ByteArrayOutputStream buffer = new java.io.ByteArrayOutputStream();
        byte[] chunk = new byte[4096];
        int n;
        while ((n = is.read(chunk)) != -1) {
            buffer.write(chunk, 0, n);
        }
        return buffer.toByteArray();
    }

    private static WavInfo parseWavHeader(byte[] bytes) {
        if (bytes.length < 44) return null;

        String riff = new String(bytes, 0, 4);
        String wave = new String(bytes, 8, 4);
        if (!riff.equals("RIFF") || !wave.equals("WAVE")) return null;

        int pos = 12;
        int fmtOffset = -1;
        int dataOffset = -1;
        int dataSize = 0;

        while (pos + 8 <= bytes.length) {
            String chunkId = new String(bytes, pos, 4);
            int chunkSize = readInt32LE(bytes, pos + 4);
            if (chunkId.equals("fmt ")) {
                fmtOffset = pos + 8;
            } else if (chunkId.equals("data")) {
                dataOffset = pos + 8;
                dataSize = chunkSize;
            }
            pos += 8 + chunkSize;
            if (fmtOffset != -1 && dataOffset != -1) break;
        }

        if (fmtOffset == -1 || dataOffset == -1) return null;
        if (fmtOffset + 16 > bytes.length) return null;

        int audioFormat = readInt16LE(bytes, fmtOffset);
        int channels = readInt16LE(bytes, fmtOffset + 2);
        int sampleRate = readInt32LE(bytes, fmtOffset + 4);
        int bitsPerSample = readInt16LE(bytes, fmtOffset + 14);

        if (audioFormat != 1) {
            Log.w(TAG, "Unsupported WAV format " + audioFormat + " (only PCM=1 supported)");
            return null;
        }

        int actualDataSize = Math.min(dataSize, bytes.length - dataOffset);
        return new WavInfo(channels, sampleRate, bitsPerSample, dataOffset, actualDataSize);
    }

    private static int readInt16LE(byte[] bytes, int offset) {
        return (bytes[offset] & 0xFF) | ((bytes[offset + 1] & 0xFF) << 8);
    }

    private static int readInt32LE(byte[] bytes, int offset) {
        return (bytes[offset] & 0xFF)
                | ((bytes[offset + 1] & 0xFF) << 8)
                | ((bytes[offset + 2] & 0xFF) << 16)
                | ((bytes[offset + 3] & 0xFF) << 24);
    }
}
