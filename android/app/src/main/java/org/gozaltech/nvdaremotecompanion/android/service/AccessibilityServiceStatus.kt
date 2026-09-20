package org.gozaltech.nvdaremotecompanion.android.service

import android.accessibilityservice.AccessibilityServiceInfo
import android.content.ComponentName
import android.content.Context
import android.view.accessibility.AccessibilityManager
import org.gozaltech.nvdaremotecompanion.android.NvdaRemoteAccessibilityService

class AccessibilityServiceStatus(private val context: Context) {

    private val component = ComponentName(context, NvdaRemoteAccessibilityService::class.java)

    fun isEnabled(): Boolean = runCatching {
        context
            .getSystemService(AccessibilityManager::class.java)
            .getEnabledAccessibilityServiceList(AccessibilityServiceInfo.FEEDBACK_ALL_MASK)
            .any { ComponentName.unflattenFromString(it.id) == component }
    }
        .getOrDefault(false)
}
