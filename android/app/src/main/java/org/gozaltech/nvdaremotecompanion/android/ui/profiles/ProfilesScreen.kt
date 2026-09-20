package org.gozaltech.nvdaremotecompanion.android.ui.profiles

import android.content.Intent
import android.provider.Settings
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.rememberModalBottomSheetState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.CustomAccessibilityAction
import androidx.compose.ui.semantics.customActions
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.remote.RemoteProfile
import org.gozaltech.nvdaremotecompanion.android.remote.RemoteState
import org.gozaltech.nvdaremotecompanion.android.service.AccessibilityServiceStatus
import org.gozaltech.nvdaremotecompanion.android.ui.MainViewModel
import org.koin.compose.koinInject

private data class ProfileAction(val label: String, val invoke: () -> Unit)

private data class ProfileActions(val toggleConnection: ProfileAction, val all: List<ProfileAction>)

@Composable
fun ProfilesScreen(
    state: RemoteState,
    viewModel: MainViewModel,
    onAddProfile: () -> Unit,
    onEditProfile: (Int) -> Unit,
) {
    val context = LocalContext.current
    val accessibilityStatus: AccessibilityServiceStatus = koinInject()
    var pendingDeletion by rememberSaveable { mutableStateOf<Int?>(null) }
    var showCaptureWarning by rememberSaveable { mutableStateOf(false) }
    var actionSheetFor by rememberSaveable { mutableStateOf<Int?>(null) }

    LazyColumn(
        modifier = Modifier.fillMaxWidth(),
        contentPadding = PaddingValues(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        item {
            Button(onClick = onAddProfile, modifier = Modifier.fillMaxWidth()) {
                Text(stringResource(R.string.add_profile))
            }
        }
        item {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Text(stringResource(R.string.send_keys))
                Switch(
                    checked = state.sendingKeys,
                    onCheckedChange = { enabled ->
                        if (enabled && !accessibilityStatus.isEnabled()) {
                            showCaptureWarning = true
                        } else {
                            viewModel.toggleForwarding()
                        }
                    },
                )
            }
        }
        item {
            OutlinedButton(
                onClick = { viewModel.sendClipboard() },
                modifier = Modifier.fillMaxWidth(),
            ) {
                Text(stringResource(R.string.send_clipboard))
            }
        }
        if (state.profiles.isEmpty()) {
            item { Text(stringResource(R.string.no_profiles)) }
        }
        items(state.profiles, key = { it.index }) { profile ->
            ProfileCard(
                profile = profile,
                isActive = profile.index == state.activeIndex,
                sendingKeys = state.sendingKeys,
                actions =
                    profileActions(
                        profile = profile,
                        isActive = profile.index == state.activeIndex,
                        viewModel = viewModel,
                        onEdit = onEditProfile,
                        onDelete = { pendingDeletion = it },
                    ),
                onShowActions = { actionSheetFor = profile.index },
            )
        }
    }

    actionSheetFor?.let { index ->
        state.profiles
            .firstOrNull { it.index == index }
            ?.let { profile ->
                ProfileActionSheet(
                    profile = profile,
                    actions =
                        profileActions(
                            profile = profile,
                            isActive = profile.index == state.activeIndex,
                            viewModel = viewModel,
                            onEdit = onEditProfile,
                            onDelete = { pendingDeletion = it },
                        ),
                    onDismiss = { actionSheetFor = null },
                )
            } ?: run { actionSheetFor = null }
    }

    if (showCaptureWarning) {
        AlertDialog(
            onDismissRequest = { showCaptureWarning = false },
            title = { Text(stringResource(R.string.keyboard_capture_limited_title)) },
            text = { Text(stringResource(R.string.keyboard_capture_limited_message)) },
            confirmButton = {
                TextButton(
                    onClick = {
                        showCaptureWarning = false
                        context.startActivity(Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS))
                    }
                ) {
                    Text(stringResource(R.string.open_settings))
                }
            },
            dismissButton = {
                TextButton(
                    onClick = {
                        showCaptureWarning = false
                        viewModel.toggleForwarding()
                    }
                ) {
                    Text(stringResource(R.string.continue_anyway))
                }
            },
        )
    }

    pendingDeletion?.let { index ->
        val name = state.profiles.firstOrNull { it.index == index }?.name.orEmpty()
        DeleteConfirmationDialog(
            profileName = name,
            onConfirm = {
                viewModel.deleteProfile(index)
                pendingDeletion = null
            },
            onDismiss = { pendingDeletion = null },
        )
    }
}

@Composable
private fun profileActions(
    profile: RemoteProfile,
    isActive: Boolean,
    viewModel: MainViewModel,
    onEdit: (Int) -> Unit,
    onDelete: (Int) -> Unit,
): ProfileActions {
    val toggleConnection =
        if (profile.connected) {
            ProfileAction(stringResource(R.string.disconnect_profile, profile.name)) {
                viewModel.disconnect(profile.index)
            }
        } else {
            ProfileAction(stringResource(R.string.connect_profile, profile.name)) {
                viewModel.connect(profile.index)
            }
        }

    val all = buildList {
        add(toggleConnection)
        if (profile.connected) {
            if (!isActive) {
                add(
                    ProfileAction(stringResource(R.string.set_active_profile, profile.name)) {
                        viewModel.setActiveProfile(profile.index)
                    }
                )
            }
            add(
                ProfileAction(stringResource(R.string.send_clipboard)) {
                    viewModel.sendClipboard(profile.index)
                }
            )
        }
        add(
            ProfileAction(stringResource(R.string.edit_profile, profile.name)) {
                onEdit(profile.index)
            }
        )
        add(ProfileAction(stringResource(R.string.delete_profile)) { onDelete(profile.index) })
    }

    return ProfileActions(toggleConnection, all)
}

@Composable
private fun ProfileCard(
    profile: RemoteProfile,
    isActive: Boolean,
    sendingKeys: Boolean,
    actions: ProfileActions,
    onShowActions: () -> Unit,
) {
    val status =
        when {
            isActive && sendingKeys -> stringResource(R.string.profile_sending_keys)
            isActive -> stringResource(R.string.profile_active)
            profile.connected -> stringResource(R.string.profile_connected)
            else -> stringResource(R.string.profile_disconnected)
        }

    Card(
        modifier =
            Modifier.fillMaxWidth()
                .combinedClickable(
                    onClick = actions.toggleConnection.invoke,
                    onLongClick = onShowActions,
                )
                .semantics(mergeDescendants = true) {
                    customActions =
                        actions.all.map { action ->
                            CustomAccessibilityAction(action.label) {
                                action.invoke()
                                true
                            }
                        }
                },
        colors =
            if (isActive) {
                CardDefaults.cardColors(
                    containerColor = MaterialTheme.colorScheme.secondaryContainer
                )
            } else {
                CardDefaults.cardColors()
            },
    ) {
        Text(
            text = "${profile.name} — $status",
            style = MaterialTheme.typography.titleMedium,
            modifier = Modifier.padding(16.dp),
        )
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun ProfileActionSheet(
    profile: RemoteProfile,
    actions: ProfileActions,
    onDismiss: () -> Unit,
) {
    ModalBottomSheet(
        onDismissRequest = onDismiss,
        sheetState = rememberModalBottomSheetState(skipPartiallyExpanded = true),
    ) {
        Column(modifier = Modifier.fillMaxWidth().navigationBarsPadding()) {
            Text(
                text = profile.name,
                style = MaterialTheme.typography.titleMedium,
                modifier =
                    Modifier.padding(horizontal = 24.dp, vertical = 12.dp).semantics { heading() },
            )
            HorizontalDivider()
            actions.all.forEach { action ->
                Text(
                    text = action.label,
                    style = MaterialTheme.typography.bodyLarge,
                    modifier =
                        Modifier.fillMaxWidth()
                            .combinedClickable(
                                onClick = {
                                    onDismiss()
                                    action.invoke()
                                }
                            )
                            .padding(horizontal = 24.dp, vertical = 16.dp),
                )
            }
        }
    }
}

@Composable
private fun DeleteConfirmationDialog(
    profileName: String,
    onConfirm: () -> Unit,
    onDismiss: () -> Unit,
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(stringResource(R.string.delete_profile)) },
        text = { Text(stringResource(R.string.delete_confirm_message, profileName)) },
        confirmButton = {
            TextButton(onClick = onConfirm) { Text(stringResource(android.R.string.ok)) }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) { Text(stringResource(android.R.string.cancel)) }
        },
    )
}
