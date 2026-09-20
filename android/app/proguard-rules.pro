-keep class org.gozaltech.nvdaremotecompanion.android.NativeBridge { *; }

-keep @androidx.annotation.Keep class * { *; }
-keepclassmembers class * {
    @androidx.annotation.Keep *;
}

-keep class org.gozaltech.nvdaremotecompanion.android.speech.SpeechBridge {
    public void speak(java.lang.String, boolean);
    public void stop();
}
-keep class org.gozaltech.nvdaremotecompanion.android.audio.SoundBridge {
    public void playTone(int, int);
    public void playWave(java.lang.String);
}

-keep class org.gozaltech.nvdaremotecompanion.android.service.ConnectionService { *; }
-keep class org.gozaltech.nvdaremotecompanion.android.NvdaRemoteAccessibilityService { *; }
-keep class org.gozaltech.nvdaremotecompanion.android.service.BootReceiver { *; }
-keep class org.gozaltech.nvdaremotecompanion.android.update.InstallResultReceiver { *; }

-keepclassmembers class org.gozaltech.nvdaremotecompanion.android.** {
    *** Companion;
}
-keepclasseswithmembers class org.gozaltech.nvdaremotecompanion.android.** {
    kotlinx.serialization.KSerializer serializer(...);
}
-keep,includedescriptorclasses class org.gozaltech.nvdaremotecompanion.android.**$$serializer { *; }
