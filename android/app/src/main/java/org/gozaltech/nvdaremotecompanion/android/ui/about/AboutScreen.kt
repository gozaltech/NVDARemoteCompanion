package org.gozaltech.nvdaremotecompanion.android.ui.about

import android.content.Intent
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.heading
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import androidx.core.net.toUri
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.data.AppVersion
import org.gozaltech.nvdaremotecompanion.android.ui.common.OutlinedActionButton
import org.koin.compose.koinInject

private const val GITHUB_URL = "https://github.com/gozaltech/NVDARemoteCompanion"
private const val ISSUES_URL = "https://github.com/gozaltech/NVDARemoteCompanion/issues"
private const val TELEGRAM_APP_URL = "tg://resolve?domain=gozaltech"
private const val TELEGRAM_WEB_URL = "https://t.me/gozaltech"
private const val DONATE_URL = "https://paypal.me/gozaltech"

@Composable
fun AboutScreen(
    onCheckUpdates: () -> Unit,
    appVersion: AppVersion = koinInject(),
) {
    val context = LocalContext.current
    val openUrl: (String) -> Unit = { url ->
        context.startActivity(Intent(Intent.ACTION_VIEW, url.toUri()))
    }

    Column(
        modifier = Modifier.fillMaxWidth().verticalScroll(rememberScrollState()).padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp),
    ) {
        Text(
            text = stringResource(R.string.app_name),
            style = MaterialTheme.typography.headlineSmall,
            modifier = Modifier.semantics { heading() },
        )
        Text(stringResource(R.string.about_version, appVersion.name))
        Text(stringResource(R.string.about_description))
        Text(stringResource(R.string.about_author))

        OutlinedActionButton(
            text = stringResource(R.string.about_github),
            onClick = { openUrl(GITHUB_URL) },
        )
        OutlinedActionButton(
            text = stringResource(R.string.about_contact),
            onClick = {
                runCatching { openUrl(TELEGRAM_APP_URL) }.onFailure { openUrl(TELEGRAM_WEB_URL) }
            },
        )
        OutlinedActionButton(
            text = stringResource(R.string.about_report_issue),
            onClick = { openUrl(ISSUES_URL) },
        )
        OutlinedActionButton(
            text = stringResource(R.string.about_donate),
            onClick = { openUrl(DONATE_URL) },
        )
        OutlinedActionButton(text = stringResource(R.string.update_check), onClick = onCheckUpdates)
    }
}
