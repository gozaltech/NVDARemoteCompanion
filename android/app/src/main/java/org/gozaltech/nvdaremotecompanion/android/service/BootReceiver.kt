package org.gozaltech.nvdaremotecompanion.android.service

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch
import org.gozaltech.nvdaremotecompanion.android.data.SettingsRepository
import org.koin.core.component.KoinComponent
import org.koin.core.component.inject

class BootReceiver : BroadcastReceiver(), KoinComponent {

    private val settingsRepository: SettingsRepository by inject()

    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != Intent.ACTION_BOOT_COMPLETED) return
        val pendingResult = goAsync()
        CoroutineScope(SupervisorJob() + Dispatchers.IO).launch {
            try {
                if (settingsRepository.current().autostart) {
                    context.startForegroundService(ConnectionService.startedFromBackground(context))
                }
            } finally {
                pendingResult.finish()
            }
        }
    }
}
