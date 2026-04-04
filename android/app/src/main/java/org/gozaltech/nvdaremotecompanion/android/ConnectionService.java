package org.gozaltech.nvdaremotecompanion.android;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.os.PowerManager;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import androidx.core.app.NotificationCompat;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ConnectionService extends Service {

    private static final String TAG = "NVDARemote/Service";
    private static final int NOTIFICATION_ID = 1;
    private static final String CHANNEL_ID = "nvdaremote_connection";

    public class LocalBinder extends Binder {
        public ConnectionService getService() {
            return ConnectionService.this;
        }
    }

    private final IBinder binder = new LocalBinder();
    private final ExecutorService executor = Executors.newCachedThreadPool();

    private TtsManager ttsManager;
    private NvdaAudioManager audioManager;
    private boolean initialized = false;
    private PowerManager.WakeLock wakeLock;

    private final Set<Integer> desiredConnected =
            Collections.synchronizedSet(new HashSet<>());

    private final ConnectivityManager.NetworkCallback networkCallback =
            new ConnectivityManager.NetworkCallback() {
        @Override
        public void onAvailable(Network network) {
            Set<Integer> toReconnect = new HashSet<>(desiredConnected);
            if (toReconnect.isEmpty()) return;
            Log.i(TAG, "Network available — reconnecting " + toReconnect.size() + " desired profile(s)");
            for (int idx : toReconnect) {
                executor.submit(() -> {
                    if (!NativeBridge.nativeIsConnected(idx)) {
                        boolean ok = NativeBridge.nativeConnect(idx);
                        if (!ok) desiredConnected.remove(idx);
                    }
                });
            }
        }

        @Override
        public void onLost(Network network) {
            Set<Integer> toDisconnect = new HashSet<>(desiredConnected);
            if (toDisconnect.isEmpty()) return;
            Log.i(TAG, "Network lost — disconnecting " + toDisconnect.size() + " profile(s)");
            for (int idx : toDisconnect) {
                executor.submit(() -> {
                    if (NativeBridge.nativeIsConnected(idx))
                        NativeBridge.nativeDisconnect(idx);
                });
            }
        }
    };

    private final NativeBridge.ConnectionStateListener connectionStateListener =
            this::updateNotification;

    @Override
    public void onCreate() {
        super.onCreate();
        createNotificationChannel();

        String enginePkg = AppPrefs.getTtsEngine(this);
        ttsManager = new TtsManager(getApplicationContext(), enginePkg);
        ttsManager.setUseAccessibilityStream(AppPrefs.getAccessibilityStream(this));
        ttsManager.setPitch(AppPrefs.getTtsPitch(this));
        ttsManager.setRate(AppPrefs.getTtsRate(this));
        ttsManager.setVolume(AppPrefs.getTtsVolume(this));
        audioManager = new NvdaAudioManager(getApplicationContext());

        ttsManager.setScreenReaderMode(AppPrefs.getScreenReaderMode(this));
        PowerManager pm = (PowerManager) getSystemService(POWER_SERVICE);
        wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "NVDARemote:connection");

        NativeBridge.setAppContext(this);
        NativeBridge.nativeInit(ttsManager, audioManager, getFilesDir().getAbsolutePath());
        initialized = true;
        Log.i(TAG, "ConnectionService created, native initialized");

        startForeground();
        autoConnect();
        NativeBridge.addConnectionStateListener(connectionStateListener);

        NetworkRequest request = new NetworkRequest.Builder()
                .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .build();
        getSystemService(ConnectivityManager.class).registerNetworkCallback(request, networkCallback);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public void onDestroy() {
        NativeBridge.removeConnectionStateListener(connectionStateListener);
        getSystemService(ConnectivityManager.class).unregisterNetworkCallback(networkCallback);
        executor.shutdownNow();
        if (initialized) {
            int count = NativeBridge.nativeGetProfileCount();
            for (int i = 0; i < count; i++) {
                if (NativeBridge.nativeIsConnected(i)) NativeBridge.nativeDisconnect(i);
            }
            NativeBridge.nativeShutdown();
            initialized = false;
        }
        if (wakeLock != null && wakeLock.isHeld()) wakeLock.release();
        ttsManager.shutdown();
        Log.i(TAG, "ConnectionService destroyed");
        super.onDestroy();
    }

    public interface ConnectResultCallback {
        void onResult(boolean success);
    }

    public void connect(int profileIndex, ConnectResultCallback callback) {
        desiredConnected.add(profileIndex);
        executor.submit(() -> {
            boolean ok = NativeBridge.nativeConnect(profileIndex);
            Log.i(TAG, "connect(" + profileIndex + ") = " + ok);
            if (!ok) desiredConnected.remove(profileIndex);
            if (callback != null) callback.onResult(ok);
        });
    }

    public void disconnect(int profileIndex) {
        desiredConnected.remove(profileIndex);
        executor.submit(() -> NativeBridge.nativeDisconnect(profileIndex));
    }

    public void setActiveProfile(int profileIndex) { NativeBridge.nativeSetActiveProfile(profileIndex); }
    public void toggleForwarding()                 { NativeBridge.nativeToggleForwarding(); }
    public boolean isConnected(int profileIndex)   { return NativeBridge.nativeIsConnected(profileIndex); }
    public int getProfileCount()                   { return NativeBridge.nativeGetProfileCount(); }
    public String getProfileName(int profileIndex) { return NativeBridge.nativeGetProfileName(profileIndex); }
    public int getActiveProfile()                  { return NativeBridge.nativeGetActiveProfile(); }

    public void setTtsEngine(String enginePackage) {
        ttsManager.shutdown();
        ttsManager = new TtsManager(getApplicationContext(), enginePackage);
        ttsManager.setScreenReaderMode(AppPrefs.getScreenReaderMode(this));
        ttsManager.setUseAccessibilityStream(AppPrefs.getAccessibilityStream(this));
        ttsManager.setPitch(AppPrefs.getTtsPitch(this));
        ttsManager.setRate(AppPrefs.getTtsRate(this));
        ttsManager.setVolume(AppPrefs.getTtsVolume(this));
        NativeBridge.nativeUpdateTtsManager(ttsManager);
        AppPrefs.setTtsEngine(this, enginePackage);
    }

    public void setUseAccessibilityStream(boolean use) {
        ttsManager.setUseAccessibilityStream(use);
        AppPrefs.setAccessibilityStream(this, use);
    }

    public void setScreenReaderMode(boolean use) {
        ttsManager.setScreenReaderMode(use);
        AppPrefs.setScreenReaderMode(this, use);
    }

    public void setTtsPitch(float value) {
        ttsManager.setPitch(value);
        AppPrefs.setTtsPitch(this, value);
    }

    public void setTtsRate(float value) {
        ttsManager.setRate(value);
        AppPrefs.setTtsRate(this, value);
    }

    public void setTtsVolume(float value) {
        ttsManager.setVolume(value);
        AppPrefs.setTtsVolume(this, value);
    }

    public void deleteProfile(int profileIndex) {
        desiredConnected.remove(profileIndex);
        executor.submit(() -> {
            if (NativeBridge.nativeIsConnected(profileIndex)) NativeBridge.nativeDisconnect(profileIndex);
            try {
                ProfileRepository.deleteProfile(profileIndex);
            } catch (Exception e) {
                Log.e(TAG, "Failed to delete profile " + profileIndex + ": " + e.getMessage());
            }
        });
    }

    private void autoConnect() {
        if (!AppPrefs.getAutoConnect(this)) return;
        executor.submit(() -> {
            int count = NativeBridge.nativeGetProfileCount();
            for (int i = 0; i < count; i++) {
                if (NativeBridge.nativeGetAutoConnect(i)) {
                    Log.i(TAG, "Auto-connecting profile " + i);
                    desiredConnected.add(i);
                    NativeBridge.nativeConnect(i);
                }
            }
        });
    }

    private void createNotificationChannel() {
        NotificationChannel channel = new NotificationChannel(
                CHANNEL_ID,
                getString(R.string.notification_channel_name),
                NotificationManager.IMPORTANCE_LOW);
        channel.setDescription(getString(R.string.notification_channel_desc));
        getSystemService(NotificationManager.class).createNotificationChannel(channel);
    }

    private Notification buildNotification(int connectedCount) {
        String text = connectedCount == 0
                ? getString(R.string.notification_no_connections)
                : getResources().getQuantityString(
                        R.plurals.notification_connected, connectedCount, connectedCount);

        PendingIntent openIntent = PendingIntent.getActivity(this, 0,
                new Intent(this, MainActivity.class),
                PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);

        return new NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle(getString(R.string.app_name))
                .setContentText(text)
                .setSmallIcon(android.R.drawable.ic_dialog_info)
                .setContentIntent(openIntent)
                .setOngoing(true)
                .setSilent(true)
                .build();
    }

    private void startForeground() {
        Notification n = buildNotification(0);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(NOTIFICATION_ID, n, ServiceInfo.FOREGROUND_SERVICE_TYPE_REMOTE_MESSAGING);
        } else {
            startForeground(NOTIFICATION_ID, n);
        }
    }

    private void updateNotification(Map<Integer, Boolean> states) {
        int count = 0;
        for (boolean v : states.values()) if (v) count++;
        getSystemService(NotificationManager.class).notify(NOTIFICATION_ID, buildNotification(count));
        if (count > 0) {
            if (!wakeLock.isHeld()) wakeLock.acquire();
        } else {
            if (wakeLock.isHeld()) wakeLock.release();
        }
    }
}
