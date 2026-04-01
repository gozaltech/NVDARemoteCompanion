package org.gozaltech.nvdaremotecompanion.android;

public final class ProfileUiState {

    public final int index;
    public final String displayName;
    public final boolean connected;
    public final boolean active;

    public ProfileUiState(int index, String displayName, boolean connected, boolean active) {
        this.index = index;
        this.displayName = displayName;
        this.connected = connected;
        this.active = active;
    }
}
