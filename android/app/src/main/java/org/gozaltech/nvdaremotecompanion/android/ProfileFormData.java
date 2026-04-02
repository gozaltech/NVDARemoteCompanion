package org.gozaltech.nvdaremotecompanion.android;

public final class ProfileFormData {

    public final String name;
    public final String host;
    public final int    port;
    public final String key;
    public final boolean speech;
    public final boolean sounds;
    public final boolean mute;
    public final boolean autoConnect;

    public ProfileFormData(String name, String host, int port, String key,
                           boolean speech, boolean sounds, boolean mute, boolean autoConnect) {
        this.name        = name;
        this.host        = host;
        this.port        = port;
        this.key         = key;
        this.speech      = speech;
        this.sounds      = sounds;
        this.mute        = mute;
        this.autoConnect = autoConnect;
    }
}
