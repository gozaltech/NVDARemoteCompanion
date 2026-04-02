package org.gozaltech.nvdaremotecompanion.android;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.util.Log;

public class BootReceiver extends BroadcastReceiver {

    private static final String TAG = "NVDARemote/Boot";

    @Override
    public void onReceive(Context context, Intent intent) {
        if (!Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) return;
        if (!AppPrefs.getAutostart(context)) return;

        Log.i(TAG, "Boot completed — starting ConnectionService");
        Intent svcIntent = new Intent(context, ConnectionService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            context.startForegroundService(svcIntent);
        } else {
            context.startService(svcIntent);
        }
    }
}
