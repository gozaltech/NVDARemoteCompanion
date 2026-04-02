# Keep the JNI bridge intact — native code references these by name
-keep class org.gozaltech.nvdaremotecompanion.android.NativeBridge { *; }

# Keep all @Keep-annotated members (used on onConnectionStateChanged)
-keep @androidx.annotation.Keep class * { *; }
-keepclassmembers class * {
    @androidx.annotation.Keep *;
}

# Keep classes looked up by name from native code via GetMethodID / FindClass
-keep class org.gozaltech.nvdaremotecompanion.android.TtsManager {
    void speak(java.lang.String, boolean);
    void stop();
}
-keep class org.gozaltech.nvdaremotecompanion.android.NvdaAudioManager {
    void playTone(int, int);
    void playWave(java.lang.String);
}

# Keep service/activity/receiver classes referenced from AndroidManifest
-keep class org.gozaltech.nvdaremotecompanion.android.ConnectionService { *; }
-keep class org.gozaltech.nvdaremotecompanion.android.NvdaRemoteAccessibilityService { *; }
-keep class org.gozaltech.nvdaremotecompanion.android.BootReceiver { *; }
