package org.gozaltech.nvdaremotecompanion.android;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import androidx.annotation.Keep;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

public class NativeBridge {

    static {
        System.loadLibrary("nvdaremote");
    }

    private static Context appContext;

    public static void setAppContext(Context context) {
        appContext = context.getApplicationContext();
    }

    public interface ConnectionStateListener {
        void onConnectionStatesChanged(Map<Integer, Boolean> states);
    }

    private static final CopyOnWriteArrayList<ConnectionStateListener> listeners =
            new CopyOnWriteArrayList<>();
    private static final Map<Integer, Boolean> connectionStates =
            Collections.synchronizedMap(new HashMap<>());

    public static Map<Integer, Boolean> getConnectionStates() {
        return Collections.unmodifiableMap(new HashMap<>(connectionStates));
    }

    public static void addConnectionStateListener(ConnectionStateListener listener) {
        listeners.addIfAbsent(listener);
    }

    public static void removeConnectionStateListener(ConnectionStateListener listener) {
        listeners.remove(listener);
    }

    private static void notifyListeners() {
        Map<Integer, Boolean> snapshot = Collections.unmodifiableMap(new HashMap<>(connectionStates));
        for (ConnectionStateListener l : listeners) {
            l.onConnectionStatesChanged(snapshot);
        }
    }

    @Keep
    public static void onConnectionStateChanged(int profileIndex, boolean connected) {
        connectionStates.put(profileIndex, connected);
        notifyListeners();
    }

    public static void syncConnectionStates() {
        int count = nativeGetProfileCount();
        Map<Integer, Boolean> map = new HashMap<>();
        for (int i = 0; i < count; i++) {
            map.put(i, nativeIsConnected(i));
        }
        connectionStates.clear();
        connectionStates.putAll(map);
        notifyListeners();
    }

    public static native void nativeInit(
            TtsManager ttsManager,
            NvdaAudioManager audioManager,
            String configDirPath
    );

    public static native void nativeShutdown();

    public static native boolean nativeConnect(int profileIndex);
    public static native void nativeDisconnect(int profileIndex);
    public static native boolean nativeIsConnected(int profileIndex);

    public static native int nativeGetProfileCount();
    public static native String nativeGetProfileName(int profileIndex);

    public static native void nativeLoadConfig(String configJson);
    public static native String nativeGetConfigJson();
    public static native int nativeMergeConfig(String configJson);

    public static native boolean nativeGetAutoConnect(int profileIndex);
    public static native String nativeGetProfileJson(int profileIndex);
    public static native void nativeSaveProfile(int profileIndex,
            String name, String host, int port, String key,
            boolean speech, boolean sounds, boolean mute, boolean autoConnect);
    public static native void nativeDeleteProfile(int profileIndex);

    public static native void nativeSendKeyEvent(
            int vkCode,
            int scanCode,
            boolean pressed,
            boolean extended,
            int profileIndex
    );

    public static native boolean nativeProcessModifiersAndShortcuts(int vkCode, boolean pressed);

    public static native void nativeUpdateTtsManager(TtsManager ttsManager);

    public static native void nativeSetActiveProfile(int profileIndex);
    public static native int nativeGetActiveProfile();
    public static native boolean nativeIsSendingKeys();
    public static native void nativeToggleForwarding();

    public static native void nativeSendClipboardText(String text, int profileIndex);

    @Keep
    public static void onForwardingStateChanged(boolean forwarding) {
        for (ConnectionStateListener l : listeners) {
            l.onConnectionStatesChanged(Collections.unmodifiableMap(new HashMap<>(connectionStates)));
        }
    }

    @Keep
    public static void onClipboardShortcutTriggered() {
        if (appContext == null) return;
        new Handler(Looper.getMainLooper()).post(() -> {
            int activeProfile = nativeGetActiveProfile();
            if (activeProfile < 0) return;
            ClipboardManager clipboard = (ClipboardManager)
                    appContext.getSystemService(Context.CLIPBOARD_SERVICE);
            if (clipboard == null || !clipboard.hasPrimaryClip()) return;
            ClipData.Item item = clipboard.getPrimaryClip().getItemAt(0);
            CharSequence text = item != null ? item.getText() : null;
            if (text == null || text.length() == 0) return;
            nativeSendClipboardText(text.toString(), activeProfile);
        });
    }

    @Keep
    public static void onClipboardTextReceived(final String text) {
        new Handler(Looper.getMainLooper()).post(() -> {
            if (appContext == null) return;
            ClipboardManager clipboard = (ClipboardManager)
                    appContext.getSystemService(Context.CLIPBOARD_SERVICE);
            if (clipboard != null) {
                clipboard.setPrimaryClip(ClipData.newPlainText("NVDARemote", text));
            }
        });
    }
}
