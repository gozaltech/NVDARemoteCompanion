package org.gozaltech.nvdaremotecompanion.android.ui

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.LiveRegionMode
import androidx.compose.ui.semantics.liveRegion
import androidx.compose.ui.semantics.semantics
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.lifecycle.viewmodel.navigation3.rememberViewModelStoreNavEntryDecorator
import androidx.navigation3.runtime.entryProvider
import androidx.navigation3.runtime.rememberNavBackStack
import androidx.navigation3.runtime.rememberSaveableStateHolderNavEntryDecorator
import androidx.navigation3.ui.NavDisplay
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.remote.RemoteState
import org.gozaltech.nvdaremotecompanion.android.ui.about.AboutScreen
import org.gozaltech.nvdaremotecompanion.android.ui.nav.AboutRoute
import org.gozaltech.nvdaremotecompanion.android.ui.nav.ProfileEditRoute
import org.gozaltech.nvdaremotecompanion.android.ui.nav.ProfilesRoute
import org.gozaltech.nvdaremotecompanion.android.ui.nav.SettingsRoute
import org.gozaltech.nvdaremotecompanion.android.ui.nav.TopLevelDestination
import org.gozaltech.nvdaremotecompanion.android.ui.profileedit.ProfileEditScreen
import org.gozaltech.nvdaremotecompanion.android.ui.profiles.ProfilesScreen
import org.gozaltech.nvdaremotecompanion.android.ui.settings.SettingsScreen
import org.gozaltech.nvdaremotecompanion.android.ui.update.UpdateDialogs
import org.gozaltech.nvdaremotecompanion.android.ui.update.UpdateViewModel
import org.koin.androidx.compose.koinViewModel

private const val NEW_PROFILE_INDEX = -1

@Composable
fun MainScreen() {
    val mainViewModel: MainViewModel = koinViewModel()
    val updateViewModel: UpdateViewModel = koinViewModel()

    val remoteState by mainViewModel.state.collectAsStateWithLifecycle()
    val backStack = rememberNavBackStack(ProfilesRoute)
    val snackbarHostState = remember { SnackbarHostState() }

    UiMessageHost(mainViewModel.messages, snackbarHostState)

    LaunchedEffect(Unit) { updateViewModel.checkOnLaunch() }

    Scaffold(
        topBar = { StatusTopBar(remoteState) },
        bottomBar = {
            NavigationBar {
                TopLevelDestination.entries.forEach { destination ->
                    NavigationBarItem(
                        selected = backStack.firstOrNull() == destination.route,
                        onClick = {
                            backStack.clear()
                            backStack.add(destination.route)
                        },
                        icon = { Icon(destination.icon, contentDescription = null) },
                        label = { Text(stringResource(destination.label)) },
                    )
                }
            }
        },
        snackbarHost = { SnackbarHost(snackbarHostState) },
    ) { innerPadding ->
        NavDisplay(
            backStack = backStack,
            modifier = Modifier.padding(innerPadding),
            onBack = { popBackStack(backStack) },
            entryDecorators =
                listOf(
                    rememberSaveableStateHolderNavEntryDecorator(),
                    rememberViewModelStoreNavEntryDecorator(),
                ),
            entryProvider =
                entryProvider {
                    entry<ProfilesRoute> {
                        ProfilesScreen(
                            state = remoteState,
                            viewModel = mainViewModel,
                            onAddProfile = { backStack.add(ProfileEditRoute(NEW_PROFILE_INDEX)) },
                            onEditProfile = { backStack.add(ProfileEditRoute(it)) },
                        )
                    }
                    entry<SettingsRoute> { SettingsScreen(mainViewModel = mainViewModel) }
                    entry<AboutRoute> { AboutScreen(onCheckUpdates = updateViewModel::check) }
                    entry<ProfileEditRoute> { route ->
                        ProfileEditScreen(
                            profileIndex = route.index,
                            snackbarHostState = snackbarHostState,
                            onDone = { popBackStack(backStack) },
                        )
                    }
                },
        )
    }

    UpdateDialogs(updateViewModel)
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun StatusTopBar(state: RemoteState) {
    TopAppBar(
        title = {
            Column {
                Text(
                    text = stringResource(R.string.app_name),
                    style = MaterialTheme.typography.titleMedium,
                )
                Text(
                    text = statusText(state),
                    style = MaterialTheme.typography.bodySmall,
                    modifier = Modifier.semantics { liveRegion = LiveRegionMode.Polite },
                )
            }
        }
    )
}

@Composable
private fun statusText(state: RemoteState): String {
    val name = state.activeProfile?.name.orEmpty()
    return when {
        state.sendingKeys -> stringResource(R.string.status_sending_to, name)
        state.activeIndex >= 0 -> stringResource(R.string.status_active_profile, name)
        else -> stringResource(R.string.status_local)
    }
}

private fun popBackStack(backStack: MutableList<*>) {
    if (backStack.size > 1) backStack.removeAt(backStack.lastIndex)
}
