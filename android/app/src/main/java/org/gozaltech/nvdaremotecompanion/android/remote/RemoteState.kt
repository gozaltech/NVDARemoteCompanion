package org.gozaltech.nvdaremotecompanion.android.remote

data class RemoteProfile(
    val index: Int,
    val name: String,
    val connected: Boolean,
)

data class RemoteState(
    val profiles: List<RemoteProfile> = emptyList(),
    val activeIndex: Int = -1,
    val sendingKeys: Boolean = false,
) {
    val activeProfile: RemoteProfile?
        get() = profiles.firstOrNull { it.index == activeIndex }

    val connectedCount: Int
        get() = profiles.count { it.connected }
}
