package org.gozaltech.nvdaremotecompanion.android.ui.nav

import androidx.annotation.StringRes
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.List
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Settings
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.navigation3.runtime.NavKey
import kotlinx.serialization.Serializable
import org.gozaltech.nvdaremotecompanion.android.R

@Serializable data object ProfilesRoute : NavKey

@Serializable data object SettingsRoute : NavKey

@Serializable data object AboutRoute : NavKey

@Serializable data class ProfileEditRoute(val index: Int) : NavKey

enum class TopLevelDestination(
    val route: NavKey,
    @param:StringRes val label: Int,
    val icon: ImageVector,
) {
    Profiles(ProfilesRoute, R.string.tab_profiles, Icons.AutoMirrored.Filled.List),
    Settings(SettingsRoute, R.string.tab_settings, Icons.Filled.Settings),
    About(AboutRoute, R.string.tab_about, Icons.Filled.Info),
}
