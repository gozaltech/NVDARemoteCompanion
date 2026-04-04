package org.gozaltech.nvdaremotecompanion.android;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

class UpdateChecker {

    interface Callback {
        void onResult(UpdateInfo update);
        void onError(Exception e);
    }

    private static final String API_URL = "https://api.github.com/repos/gozaltech/NVDARemoteCompanion/releases/latest";

    static void checkAsync(String currentVersion, Callback callback) {
        new Thread(() -> {
            try {
                UpdateInfo result = check(currentVersion);
                callback.onResult(result);
            } catch (Exception e) {
                callback.onError(e);
            }
        }, "update-checker").start();
    }

    private static UpdateInfo check(String currentVersion) throws Exception {
        HttpURLConnection conn = (HttpURLConnection) new URL(API_URL).openConnection();
        conn.setConnectTimeout(10_000);
        conn.setReadTimeout(10_000);
        conn.setRequestProperty("Accept", "application/vnd.github+json");

        int code = conn.getResponseCode();
        if (code != 200) throw new Exception("HTTP " + code);

        StringBuilder sb = new StringBuilder();
        try (BufferedReader br = new BufferedReader(new InputStreamReader(conn.getInputStream()))) {
            String line;
            while ((line = br.readLine()) != null) sb.append(line);
        }

        JSONObject json = new JSONObject(sb.toString());
        String tagName = json.getString("tag_name");
        String latestVersion = tagName.startsWith("v") ? tagName.substring(1) : tagName;
        String releaseNotes = json.optString("body", "");

        if (!isNewer(latestVersion, currentVersion)) return null;

        String downloadUrl = findApkUrl(json.optJSONArray("assets"));
        if (downloadUrl == null) return null;

        return new UpdateInfo(latestVersion, downloadUrl, releaseNotes);
    }

    private static String findApkUrl(JSONArray assets) throws Exception {
        if (assets == null) return null;
        for (int i = 0; i < assets.length(); i++) {
            JSONObject asset = assets.getJSONObject(i);
            String name = asset.optString("name", "");
            if (name.endsWith(".apk")) {
                return asset.getString("browser_download_url");
            }
        }
        return null;
    }

    static boolean isNewer(String candidate, String current) {
        int[] c = parseParts(candidate);
        int[] v = parseParts(current);
        int len = Math.max(c.length, v.length);
        for (int i = 0; i < len; i++) {
            int cv = i < c.length ? c[i] : 0;
            int vv = i < v.length ? v[i] : 0;
            if (cv != vv) return cv > vv;
        }
        return false;
    }

    private static int[] parseParts(String version) {
        String[] parts = version.split("\\.");
        int[] nums = new int[parts.length];
        for (int i = 0; i < parts.length; i++) {
            try { nums[i] = Integer.parseInt(parts[i].replaceAll("[^0-9]", "")); }
            catch (NumberFormatException ignored) {}
        }
        return nums;
    }
}
