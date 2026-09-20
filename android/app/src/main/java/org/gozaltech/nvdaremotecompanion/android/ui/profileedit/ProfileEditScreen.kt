package org.gozaltech.nvdaremotecompanion.android.ui.profileedit

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.ui.UiMessageHost
import org.gozaltech.nvdaremotecompanion.android.ui.common.SwitchRow
import org.koin.androidx.compose.koinViewModel

@Composable
fun ProfileEditScreen(
    profileIndex: Int,
    snackbarHostState: SnackbarHostState,
    onDone: () -> Unit,
    viewModel: ProfileEditViewModel = koinViewModel(),
) {
    val form by viewModel.form.collectAsStateWithLifecycle()

    LaunchedEffect(profileIndex) { viewModel.load(profileIndex) }
    LaunchedEffect(Unit) { viewModel.closeRequests.collect { onDone() } }
    UiMessageHost(viewModel.messages, snackbarHostState)

    Column(
        modifier = Modifier.fillMaxWidth().verticalScroll(rememberScrollState()).padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        val isEditing = profileIndex >= 0
        val titleRes = if (isEditing) R.string.edit_profile_title else R.string.add_profile_title

        Text(
            text = stringResource(titleRes),
            style = MaterialTheme.typography.titleLarge,
            modifier = Modifier.semantics { heading() },
        )

        LabeledField(
            label = stringResource(R.string.field_name),
            value = form.name,
            onValueChange = { value -> viewModel.update { it.copy(name = value) } },
        )
        LabeledField(
            label = stringResource(R.string.field_host),
            value = form.host,
            onValueChange = { value -> viewModel.update { it.copy(host = value) } },
        )
        LabeledField(
            label = stringResource(R.string.field_port),
            value = form.port,
            onValueChange = { value -> viewModel.update { it.copy(port = value) } },
            keyboardType = KeyboardType.Number,
        )
        LabeledField(
            label = stringResource(R.string.field_key),
            value = form.key,
            onValueChange = { value -> viewModel.update { it.copy(key = value) } },
        )

        SwitchRow(
            label = stringResource(R.string.field_speech),
            checked = form.speech,
            onCheckedChange = { value -> viewModel.update { it.copy(speech = value) } },
        )
        SwitchRow(
            label = stringResource(R.string.field_sounds),
            checked = form.sounds,
            onCheckedChange = { value -> viewModel.update { it.copy(sounds = value) } },
        )
        SwitchRow(
            label = stringResource(R.string.field_mute_on_local),
            checked = form.mute,
            onCheckedChange = { value -> viewModel.update { it.copy(mute = value) } },
        )
        SwitchRow(
            label = stringResource(R.string.field_auto_connect),
            checked = form.autoConnect,
            onCheckedChange = { value -> viewModel.update { it.copy(autoConnect = value) } },
        )

        Button(
            onClick = { viewModel.save(profileIndex) },
            modifier = Modifier.fillMaxWidth(),
        ) {
            Text(stringResource(R.string.save))
        }

        if (profileIndex >= 0) {
            TextButton(
                onClick = { viewModel.delete(profileIndex) },
                modifier = Modifier.fillMaxWidth(),
            ) {
                Text(stringResource(R.string.delete_profile))
            }
        }
    }
}

@Composable
private fun LabeledField(
    label: String,
    value: String,
    onValueChange: (String) -> Unit,
    keyboardType: KeyboardType = KeyboardType.Text,
) {
    OutlinedTextField(
        value = value,
        onValueChange = onValueChange,
        label = { Text(label) },
        singleLine = true,
        keyboardOptions = KeyboardOptions(keyboardType = keyboardType),
        modifier = Modifier.fillMaxWidth(),
    )
}
