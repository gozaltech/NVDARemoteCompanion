package org.gozaltech.nvdaremotecompanion.android

import android.accessibilityservice.AccessibilityService
import android.content.Intent
import android.view.KeyEvent
import android.view.accessibility.AccessibilityEvent
import org.gozaltech.nvdaremotecompanion.android.input.KeyEventRouter
import org.gozaltech.nvdaremotecompanion.android.service.ConnectionService
import org.koin.android.ext.android.inject

class NvdaRemoteAccessibilityService : AccessibilityService() {

    private val router: KeyEventRouter by inject()

    override fun onServiceConnected() {
        super.onServiceConnected()
        startService(ConnectionService.startedFromBackground(this))
    }

    override fun onAccessibilityEvent(event: AccessibilityEvent) = Unit

    override fun onInterrupt() = router.cancelRepeat()

    override fun onUnbind(intent: Intent?): Boolean {
        router.cancelRepeat()
        return super.onUnbind(intent)
    }

    override fun onKeyEvent(event: KeyEvent): Boolean = router.handle(event)
}
