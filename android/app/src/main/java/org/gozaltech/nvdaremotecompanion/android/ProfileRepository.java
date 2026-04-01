package org.gozaltech.nvdaremotecompanion.android;

import org.json.JSONArray;
import org.json.JSONObject;

public final class ProfileRepository {

    private ProfileRepository() {}

    public static JSONArray getProfiles() throws Exception {
        JSONObject config = new JSONObject(NativeBridge.nativeGetConfigJson());
        JSONArray arr = config.optJSONArray("profiles");
        return arr != null ? arr : new JSONArray();
    }

    public static void saveProfile(int index, JSONObject profile) throws Exception {
        JSONObject config = new JSONObject(NativeBridge.nativeGetConfigJson());
        JSONArray profiles = config.optJSONArray("profiles");
        if (profiles == null) profiles = new JSONArray();

        if (index >= 0 && index < profiles.length()) {
            profiles.put(index, profile);
        } else {
            profiles.put(profile);
        }

        config.put("profiles", profiles);
        config.put("schema_version", 1);
        NativeBridge.nativeLoadConfig(config.toString(4));
    }

    public static void deleteProfile(int index) throws Exception {
        JSONObject config = new JSONObject(NativeBridge.nativeGetConfigJson());
        JSONArray profiles = config.optJSONArray("profiles");
        if (profiles == null) return;

        JSONArray updated = new JSONArray();
        for (int i = 0; i < profiles.length(); i++) {
            if (i != index) updated.put(profiles.get(i));
        }
        config.put("profiles", updated);
        NativeBridge.nativeLoadConfig(config.toString());
        NativeBridge.syncConnectionStates();
    }

    public static JSONObject buildProfileJson(String name, String host, int port, String key,
                                              boolean speech, boolean sounds,
                                              boolean mute, boolean autoConnect) throws Exception {
        JSONObject p = new JSONObject();
        p.put("name",                  name.isEmpty() ? host : name);
        p.put("host",                  host);
        p.put("port",                  port);
        p.put("key",                   key);
        p.put("shortcut",              "");
        p.put("speech",                speech);
        p.put("forward_nvda_sounds",   sounds);
        p.put("mute_on_local_control", mute);
        p.put("auto_connect",          autoConnect);
        return p;
    }
}
