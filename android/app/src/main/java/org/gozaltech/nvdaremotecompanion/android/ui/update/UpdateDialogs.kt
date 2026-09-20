package org.gozaltech.nvdaremotecompanion.android.ui.update

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.LiveRegionMode
import androidx.compose.ui.semantics.liveRegion
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.data.AppVersion
import org.koin.compose.koinInject

@Composable
fun UpdateDialogs(
    viewModel: UpdateViewModel,
    appVersion: AppVersion = koinInject(),
) {
    val state by viewModel.state.collectAsStateWithLifecycle()

    when (val current = state) {
        is UpdateUiState.Hidden -> Unit

        is UpdateUiState.Checking ->
            InfoDialog(
                title = stringResource(R.string.update_checking),
                message = "…",
                onDismiss = viewModel::dismiss,
            )

        is UpdateUiState.UpToDate ->
            InfoDialog(
                title = stringResource(R.string.update_check),
                message = stringResource(R.string.update_up_to_date),
                onDismiss = viewModel::dismiss,
            )

        is UpdateUiState.Failed ->
            InfoDialog(
                title = stringResource(R.string.update_check),
                message = stringResource(R.string.update_error),
                onDismiss = viewModel::dismiss,
            )

        is UpdateUiState.SignatureMismatch ->
            InfoDialog(
                title = stringResource(R.string.update_check),
                message = stringResource(R.string.update_signature_mismatch),
                onDismiss = viewModel::dismiss,
            )

        is UpdateUiState.Available ->
            AvailableDialog(
                versionLine =
                    stringResource(
                        R.string.update_version_line,
                        appVersion.name,
                        current.info.version,
                    ),
                notes =
                    current.info.releaseNotes.ifBlank {
                        stringResource(R.string.update_no_changelog)
                    },
                onDownload = { viewModel.download(current.info) },
                onDismiss = viewModel::dismiss,
            )

        is UpdateUiState.Downloading ->
            DownloadDialog(
                percent = current.percent,
                onCancel = viewModel::cancelDownload,
            )

        is UpdateUiState.ReadyToInstall ->
            AlertDialog(
                onDismissRequest = viewModel::dismiss,
                title = { Text(stringResource(R.string.update_available_title)) },
                text = { Text(stringResource(R.string.update_ready, current.info.version)) },
                confirmButton = {
                    TextButton(onClick = { viewModel.install(current.apk) }) {
                        Text(stringResource(R.string.update_install))
                    }
                },
                dismissButton = {
                    TextButton(onClick = viewModel::dismiss) {
                        Text(stringResource(R.string.update_later))
                    }
                },
            )
    }
}

@Composable
private fun InfoDialog(title: String, message: String, onDismiss: () -> Unit) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(title) },
        text = { Text(message) },
        confirmButton = {
            TextButton(onClick = onDismiss) { Text(stringResource(android.R.string.ok)) }
        },
    )
}

@Composable
private fun AvailableDialog(
    versionLine: String,
    notes: String,
    onDownload: () -> Unit,
    onDismiss: () -> Unit,
) {
    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(stringResource(R.string.update_available_title)) },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Text(versionLine)
                Text(stringResource(R.string.update_changelog))
                Column(
                    modifier = Modifier.heightIn(max = 200.dp).verticalScroll(rememberScrollState())
                ) {
                    Text(notes)
                }
            }
        },
        confirmButton = {
            TextButton(onClick = onDownload) { Text(stringResource(R.string.update_download)) }
        },
        dismissButton = {
            TextButton(onClick = onDismiss) { Text(stringResource(R.string.update_later)) }
        },
    )
}

@Composable
private fun DownloadDialog(percent: Int, onCancel: () -> Unit) {
    var confirmingCancel by remember { mutableStateOf(false) }

    AlertDialog(
        onDismissRequest = {},
        title = { Text(stringResource(R.string.update_downloading_title)) },
        text = {
            Column(
                modifier = Modifier.fillMaxWidth(),
                verticalArrangement = Arrangement.spacedBy(8.dp),
            ) {
                LinearProgressIndicator(
                    progress = { percent / 100f },
                    modifier = Modifier.fillMaxWidth(),
                )
                Text(
                    text = stringResource(R.string.update_downloading, percent),
                    modifier = Modifier.semantics { liveRegion = LiveRegionMode.Polite },
                )
            }
        },
        confirmButton = {},
        dismissButton = {
            TextButton(onClick = { confirmingCancel = true }) {
                Text(stringResource(android.R.string.cancel))
            }
        },
    )

    if (confirmingCancel) {
        AlertDialog(
            onDismissRequest = { confirmingCancel = false },
            text = { Text(stringResource(R.string.update_cancel_confirm)) },
            confirmButton = {
                TextButton(
                    onClick = {
                        confirmingCancel = false
                        onCancel()
                    }
                ) {
                    Text(stringResource(android.R.string.ok))
                }
            },
            dismissButton = {
                TextButton(onClick = { confirmingCancel = false }) {
                    Text(stringResource(android.R.string.cancel))
                }
            },
        )
    }
}
