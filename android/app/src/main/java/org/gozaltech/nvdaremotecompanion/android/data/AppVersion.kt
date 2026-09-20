package org.gozaltech.nvdaremotecompanion.android.data

import android.content.Context

class AppVersion(private val context: Context) {

    @Suppress("DEPRECATION")
    val name: String by lazy {
        runCatching {
            context.packageManager.getPackageInfo(context.packageName, 0).versionName
        }
            .getOrNull() ?: "?"
    }
}
