package org.gozaltech.nvdaremotecompanion.android.data

import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable

const val DEFAULT_PORT = 6837

@Serializable
data class Profile(
    val name: String = "",
    val host: String = "",
    val port: Int = DEFAULT_PORT,
    val key: String = "",
    val speech: Boolean = true,
    @SerialName("forward_nvda_sounds") val sounds: Boolean = true,
    @SerialName("mute_on_local_control") val mute: Boolean = false,
    @SerialName("auto_connect") val autoConnect: Boolean = true,
)
