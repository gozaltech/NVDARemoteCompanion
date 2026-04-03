package org.gozaltech.nvdaremotecompanion.android;

class UpdateInfo {
    final String version;
    final String downloadUrl;
    final String releaseNotes;

    UpdateInfo(String version, String downloadUrl, String releaseNotes) {
        this.version = version;
        this.downloadUrl = downloadUrl;
        this.releaseNotes = releaseNotes;
    }
}
