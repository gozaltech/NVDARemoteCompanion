package org.gozaltech.nvdaremotecompanion.android;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.media.AudioAttributes;
import android.media.AudioManager;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

public class TtsManager implements TextToSpeech.OnInitListener {

    private static final String TAG = "NVDARemote/TTS";

    public static class TtsEngineInfo {
        public final String packageName;
        public final String label;

        public TtsEngineInfo(String packageName, String label) {
            this.packageName = packageName;
            this.label = label;
        }
    }

    public static List<TtsEngineInfo> getAvailableEngines(Context context) {
        try {
            Intent intent = new Intent("android.intent.action.TTS_SERVICE");
            List<ResolveInfo> resolveInfos = context.getPackageManager()
                    .queryIntentServices(intent, PackageManager.MATCH_ALL);
            List<TtsEngineInfo> result = new ArrayList<>();
            for (ResolveInfo ri : resolveInfos) {
                result.add(new TtsEngineInfo(
                        ri.serviceInfo.packageName,
                        ri.loadLabel(context.getPackageManager()).toString()
                ));
            }
            return result;
        } catch (Exception e) {
            Log.w(TAG, "Failed to enumerate TTS engines: " + e.getMessage());
            return Collections.emptyList();
        }
    }

    private final Context context;
    private TextToSpeech tts;
    private boolean ready = false;
    private boolean useAccessibilityStream = false;
    private boolean screenReaderMode = false;
    private float pitch  = 1.0f;
    private float rate   = 1.0f;
    private float volume = 1.0f;

    private final List<String[]> pendingQueue = new ArrayList<>();

    public TtsManager(Context context, String enginePackage) {
        this.context = context.getApplicationContext();
        if (enginePackage != null) {
            tts = new TextToSpeech(this.context, this, enginePackage);
        } else {
            tts = new TextToSpeech(this.context, this);
        }
    }

    public TtsManager(Context context) {
        this(context, null);
    }

    @Override
    public void onInit(int status) {
        if (status == TextToSpeech.SUCCESS) {
            tts.setLanguage(Locale.getDefault());
            applyAudioAttributes();
            tts.setPitch(pitch);
            tts.setSpeechRate(rate);
            ready = true;
            Log.i(TAG, "TTS engine ready");
            synchronized (pendingQueue) {
                for (String[] item : pendingQueue) {
                    speakNow(item[0], Boolean.parseBoolean(item[1]));
                }
                pendingQueue.clear();
            }
        } else {
            Log.e(TAG, "TTS initialization failed with status " + status);
        }
    }

    public void setUseAccessibilityStream(boolean use) {
        useAccessibilityStream = use;
        if (ready) applyAudioAttributes();
    }

    public void setScreenReaderMode(boolean use) {
        screenReaderMode = use;
    }

    public void setPitch(float value) {
        pitch = value;
        if (ready && tts != null) tts.setPitch(value);
    }

    public void setRate(float value) {
        rate = value;
        if (ready && tts != null) tts.setSpeechRate(value);
    }

    public void setVolume(float value) {
        volume = value;
    }

    private void applyAudioAttributes() {
        AudioAttributes.Builder builder = new AudioAttributes.Builder()
                .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH);
        if (useAccessibilityStream) {
            builder.setUsage(AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY);
        } else {
            builder.setLegacyStreamType(AudioManager.STREAM_MUSIC);
        }
        if (tts != null) {
            tts.setAudioAttributes(builder.build());
        }
    }

    public void speak(String text, boolean interrupt) {
        if (screenReaderMode) {
            announceViaAccessibility(text);
            return;
        }
        if (!ready) {
            synchronized (pendingQueue) {
                if (interrupt) pendingQueue.clear();
                pendingQueue.add(new String[]{text, String.valueOf(interrupt)});
            }
            return;
        }
        speakNow(text, interrupt);
    }

    private void announceViaAccessibility(String text) {
        AccessibilityManager am = (AccessibilityManager)
                context.getSystemService(Context.ACCESSIBILITY_SERVICE);
        if (am == null || !am.isEnabled()) return;
        AccessibilityEvent event = AccessibilityEvent.obtain(AccessibilityEvent.TYPE_ANNOUNCEMENT);
        event.getText().add(text);
        am.sendAccessibilityEvent(event);
    }

    private void speakNow(String text, boolean interrupt) {
        if (tts == null) return;
        int queueMode = interrupt ? TextToSpeech.QUEUE_FLUSH : TextToSpeech.QUEUE_ADD;
        Bundle params = null;
        if (volume != 1.0f) {
            params = new Bundle();
            params.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, volume);
        }
        tts.speak(text, queueMode, params, null);
    }

    public void stop() {
        if (tts != null) tts.stop();
    }

    public void shutdown() {
        if (tts != null) {
            tts.stop();
            tts.shutdown();
            tts = null;
        }
        ready = false;
    }
}
