package org.gozaltech.nvdaremotecompanion.android;

import org.json.JSONObject;

public final class ProfileRepository {

    private ProfileRepository() {}

    public static JSONObject getProfile(int index) throws Exception {
        return new JSONObject(NativeBridge.nativeGetProfileJson(index));
    }

    public static void saveProfile(int index, String name, String host, int port, String key,
                                   boolean speech, boolean sounds,
                                   boolean mute, boolean autoConnect) {
        NativeBridge.nativeSaveProfile(index, name, host, port, key, speech, sounds, mute, autoConnect);
        NativeBridge.syncConnectionStates();
    }

    public static void deleteProfile(int index) {
        NativeBridge.nativeDeleteProfile(index);
        NativeBridge.syncConnectionStates();
    }
}
