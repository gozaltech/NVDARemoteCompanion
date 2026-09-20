package org.gozaltech.nvdaremotecompanion.android

import android.app.Application
import org.gozaltech.nvdaremotecompanion.android.di.appModule
import org.koin.android.ext.koin.androidContext
import org.koin.core.context.startKoin

class NvdaApp : Application() {

    override fun onCreate() {
        super.onCreate()
        startKoin {
            androidContext(this@NvdaApp)
            modules(appModule)
        }
    }
}
