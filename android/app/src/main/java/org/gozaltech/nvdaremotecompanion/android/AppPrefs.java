package org.gozaltech.nvdaremotecompanion.android;

import android.content.Context;
import android.content.SharedPreferences;

public final class AppPrefs {

    public static final String NAME                = "nvdaremote_settings";
    public static final String TTS_ENGINE          = "tts_engine";
    public static final String AUTOSTART           = "autostart_service";
    public static final String AUTO_CONNECT        = "auto_connect_on_start";
    public static final String ACCESSIBILITY_STREAM = "tts_accessibility_stream";

    private AppPrefs() {}

    public static SharedPreferences get(Context ctx) {
        return ctx.getSharedPreferences(NAME, Context.MODE_PRIVATE);
    }

    public static String  getTtsEngine(Context ctx)           { return get(ctx).getString(TTS_ENGINE, null); }
    public static boolean getAutoConnect(Context ctx)         { return get(ctx).getBoolean(AUTO_CONNECT, false); }
    public static boolean getAutostart(Context ctx)           { return get(ctx).getBoolean(AUTOSTART, false); }
    public static boolean getAccessibilityStream(Context ctx) { return get(ctx).getBoolean(ACCESSIBILITY_STREAM, false); }

    public static void setTtsEngine(Context ctx, String pkg)       { get(ctx).edit().putString(TTS_ENGINE, pkg).apply(); }
    public static void setAutoConnect(Context ctx, boolean v)      { get(ctx).edit().putBoolean(AUTO_CONNECT, v).apply(); }
    public static void setAutostart(Context ctx, boolean v)        { get(ctx).edit().putBoolean(AUTOSTART, v).apply(); }
    public static void setAccessibilityStream(Context ctx, boolean v) { get(ctx).edit().putBoolean(ACCESSIBILITY_STREAM, v).apply(); }
}
