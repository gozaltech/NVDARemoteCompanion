package org.gozaltech.nvdaremotecompanion.android.data

import android.app.LocaleManager
import android.content.Context
import android.content.res.Configuration
import android.os.Build
import android.os.LocaleList
import java.util.Locale

private const val LOCALE_PREFS = "app_locale"
private const val KEY_LANGUAGE_TAG = "language_tag"

object AppLocales {

    val isSystemManaged: Boolean
        get() = Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU

    fun current(context: Context): Locale? =
        if (isSystemManaged) {
            context
                .getSystemService(LocaleManager::class.java)
                .applicationLocales
                .takeIf { !it.isEmpty }
                ?.get(0)
        } else {
            storedTag(context)?.let(Locale::forLanguageTag)
        }

    fun set(context: Context, locale: Locale?) {
        if (isSystemManaged) {
            context.getSystemService(LocaleManager::class.java).applicationLocales =
                locale?.let { LocaleList(it) } ?: LocaleList.getEmptyLocaleList()
        } else {
            prefs(context).edit().apply {
                if (locale == null) {
                    remove(KEY_LANGUAGE_TAG)
                } else {
                    putString(KEY_LANGUAGE_TAG, locale.toLanguageTag())
                }
                apply()
            }
        }
    }

    fun wrap(base: Context): Context {
        if (isSystemManaged) return base
        val locale = current(base) ?: return base
        val configuration = Configuration(base.resources.configuration)
        configuration.setLocales(LocaleList(locale))
        return base.createConfigurationContext(configuration)
    }

    private fun storedTag(context: Context): String? =
        prefs(context).getString(KEY_LANGUAGE_TAG, null)

    private fun prefs(context: Context) =
        context.getSharedPreferences(LOCALE_PREFS, Context.MODE_PRIVATE)
}
