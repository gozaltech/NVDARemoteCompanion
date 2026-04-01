package org.gozaltech.nvdaremotecompanion.android;

import androidx.annotation.Keep;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

public class NativeBridge {

    static {
        System.loadLibrary("nvdaremote");
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
}
