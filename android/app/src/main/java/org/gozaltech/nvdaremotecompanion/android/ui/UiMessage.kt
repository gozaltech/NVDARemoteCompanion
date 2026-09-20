package org.gozaltech.nvdaremotecompanion.android.ui

import androidx.annotation.StringRes
import androidx.compose.material3.SnackbarHostState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.ui.res.stringResource
import java.util.concurrent.atomic.AtomicLong
import kotlinx.coroutines.flow.Flow

private val messageIds = AtomicLong()

data class UiMessage(
    @param:StringRes val textRes: Int,
    val arg: String? = null,
    val id: Long = messageIds.incrementAndGet(),
)

@Composable
fun UiMessageHost(messages: Flow<UiMessage>, snackbarHostState: SnackbarHostState) {
    val queue = remember { mutableStateListOf<UiMessage>() }

    LaunchedEffect(messages) { messages.collect(queue::add) }

    queue.firstOrNull()?.let { message ->
        val text =
            message.arg?.let { stringResource(message.textRes, it) }
                ?: stringResource(message.textRes)
        LaunchedEffect(message.id) {
            snackbarHostState.showSnackbar(text)
            queue.remove(message)
        }
    }
}
