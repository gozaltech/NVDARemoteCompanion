package org.gozaltech.nvdaremotecompanion.android;

import android.content.Context;
import android.content.SharedPreferences;

public final class AppPrefs {

    public static final String NAME                 = "nvdaremote_settings";
    public static final String TTS_ENGINE           = "tts_engine";
    public static final String AUTOSTART            = "autostart_service";
    public static final String AUTO_CONNECT         = "auto_connect_on_start";
    public static final String ACCESSIBILITY_STREAM = "tts_accessibility_stream";
    public static final String SCREEN_READER_MODE   = "screen_reader_mode";
    public static final String TTS_PITCH            = "tts_pitch";
    public static final String TTS_RATE             = "tts_rate";
    public static final String TTS_VOLUME           = "tts_volume";

    private AppPrefs() {}

    public static SharedPreferences get(Context ctx) {
        return ctx.getSharedPreferences(NAME, Context.MODE_PRIVATE);
    }

    public static String  getTtsEngine(Context ctx)            { return get(ctx).getString(TTS_ENGINE, null); }
    public static boolean getAutoConnect(Context ctx)          { return get(ctx).getBoolean(AUTO_CONNECT, false); }
    public static boolean getAutostart(Context ctx)            { return get(ctx).getBoolean(AUTOSTART, false); }
    public static boolean getAccessibilityStream(Context ctx)  { return get(ctx).getBoolean(ACCESSIBILITY_STREAM, false); }
    public static boolean getScreenReaderMode(Context ctx)     { return get(ctx).getBoolean(SCREEN_READER_MODE, false); }
    public static float   getTtsPitch(Context ctx)             { return get(ctx).getFloat(TTS_PITCH, 1.0f); }
    public static float   getTtsRate(Context ctx)              { return get(ctx).getFloat(TTS_RATE, 1.0f); }
    public static float   getTtsVolume(Context ctx)            { return get(ctx).getFloat(TTS_VOLUME, 1.0f); }

    public static void setTtsEngine(Context ctx, String pkg)           { get(ctx).edit().putString(TTS_ENGINE, pkg).apply(); }
    public static void setAutoConnect(Context ctx, boolean v)          { get(ctx).edit().putBoolean(AUTO_CONNECT, v).apply(); }
    public static void setAutostart(Context ctx, boolean v)            { get(ctx).edit().putBoolean(AUTOSTART, v).apply(); }
    public static void setAccessibilityStream(Context ctx, boolean v)  { get(ctx).edit().putBoolean(ACCESSIBILITY_STREAM, v).apply(); }
    public static void setScreenReaderMode(Context ctx, boolean v)     { get(ctx).edit().putBoolean(SCREEN_READER_MODE, v).apply(); }
    public static void setTtsPitch(Context ctx, float v)               { get(ctx).edit().putFloat(TTS_PITCH, v).apply(); }
    public static void setTtsRate(Context ctx, float v)                { get(ctx).edit().putFloat(TTS_RATE, v).apply(); }
    public static void setTtsVolume(Context ctx, float v)              { get(ctx).edit().putFloat(TTS_VOLUME, v).apply(); }
}
