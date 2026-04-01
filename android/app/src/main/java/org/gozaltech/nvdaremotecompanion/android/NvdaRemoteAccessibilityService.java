package org.gozaltech.nvdaremotecompanion.android;

import android.accessibilityservice.AccessibilityService;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityEvent;

public class NvdaRemoteAccessibilityService extends AccessibilityService {

    private static final String TAG = "NVDARemote/A11y";
    private static final long REPEAT_INITIAL_DELAY_MS = 400L;
    private static final long REPEAT_INTERVAL_MS = 50L;

    private ConnectionService connectionService;
    private boolean serviceBound = false;

    private final Handler handler = new Handler(Looper.getMainLooper());
    private Runnable repeatRunnable;

    private final ServiceConnection serviceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder binder) {
            if (binder instanceof ConnectionService.LocalBinder) {
                connectionService = ((ConnectionService.LocalBinder) binder).getService();
            }
            Log.i(TAG, "Bound to ConnectionService");
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            connectionService = null;
            serviceBound = false;
            Log.w(TAG, "ConnectionService disconnected unexpectedly");
        }
    };

    @Override
    protected void onServiceConnected() {
        super.onServiceConnected();
        Log.i(TAG, "Accessibility service connected");

        Intent intent = new Intent(this, ConnectionService.class);
        startService(intent);
        serviceBound = bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE);
    }

    @Override
    public boolean onUnbind(Intent intent) {
        stopRepeat();
        if (serviceBound) {
            unbindService(serviceConnection);
            serviceBound = false;
        }
        return super.onUnbind(intent);
    }

    @Override
    public void onInterrupt() {
        stopRepeat();
        Log.i(TAG, "Accessibility service interrupted");
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent event) {
    }

    @Override
    protected boolean onKeyEvent(KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_MULTIPLE) return false;

        KeyMapper.WinKey winKey = KeyMapper.mapEvent(event);
        if (winKey == null) return false;

        boolean pressed = event.getAction() == KeyEvent.ACTION_DOWN;
        boolean isRepeat = event.getRepeatCount() > 0;

        if (pressed && !isRepeat) {
            if (NativeBridge.nativeProcessModifiersAndShortcuts(winKey.vk, true)) {
                stopRepeat();
                return true;
            }
        } else if (!pressed) {
            NativeBridge.nativeProcessModifiersAndShortcuts(winKey.vk, false);
        }

        int activeProfile = NativeBridge.nativeGetActiveProfile();
        if (activeProfile < 0) return false;

        if (pressed && !isRepeat) {
            stopRepeat();
            NativeBridge.nativeSendKeyEvent(winKey.vk, winKey.scan, true, winKey.extended, activeProfile);
            startRepeat(winKey, activeProfile);
        } else if (!pressed) {
            stopRepeat();
            NativeBridge.nativeSendKeyEvent(winKey.vk, winKey.scan, false, winKey.extended, activeProfile);
        }
        return true;
    }

    private void startRepeat(final KeyMapper.WinKey winKey, final int profileIndex) {
        Runnable runnable = new Runnable() {
            @Override
            public void run() {
                int current = NativeBridge.nativeGetActiveProfile();
                if (current == profileIndex) {
                    NativeBridge.nativeSendKeyEvent(
                            winKey.vk, winKey.scan, true, winKey.extended, profileIndex
                    );
                    handler.postDelayed(this, REPEAT_INTERVAL_MS);
                }
            }
        };
        repeatRunnable = runnable;
        handler.postDelayed(runnable, REPEAT_INITIAL_DELAY_MS);
    }

    private void stopRepeat() {
        if (repeatRunnable != null) {
            handler.removeCallbacks(repeatRunnable);
            repeatRunnable = null;
        }
    }
}
