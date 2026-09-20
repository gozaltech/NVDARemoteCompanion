package org.gozaltech.nvdaremotecompanion.android

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.Color.TRANSPARENT
import android.os.Build
import android.os.Bundle
import android.view.KeyEvent
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.LocalActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.ui.platform.LocalContext
import androidx.core.content.ContextCompat
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import org.gozaltech.nvdaremotecompanion.android.data.AppLocales
import org.gozaltech.nvdaremotecompanion.android.data.AppSettings
import org.gozaltech.nvdaremotecompanion.android.data.SettingsRepository
import org.gozaltech.nvdaremotecompanion.android.input.KeyEventRouter
import org.gozaltech.nvdaremotecompanion.android.service.AccessibilityServiceStatus
import org.gozaltech.nvdaremotecompanion.android.service.ConnectionService
import org.gozaltech.nvdaremotecompanion.android.ui.MainScreen
import org.gozaltech.nvdaremotecompanion.android.ui.theme.NvdaRemoteTheme
import org.gozaltech.nvdaremotecompanion.android.ui.theme.resolveDarkTheme
import org.koin.android.ext.android.inject
import org.koin.compose.koinInject

class MainActivity : ComponentActivity() {

    private val router: KeyEventRouter by inject()
    private val accessibilityStatus: AccessibilityServiceStatus by inject()

    private var accessibilityCapturesKeys = false

    override fun attachBaseContext(newBase: Context) {
        super.attachBaseContext(AppLocales.wrap(newBase))
    }

    override fun onResume() {
        super.onResume()
        accessibilityCapturesKeys = accessibilityStatus.isEnabled()
    }

    override fun onPause() {
        router.cancelRepeat()
        super.onPause()
    }

    override fun onKeyDown(keyCode: Int, event: KeyEvent): Boolean =
        forwardKey(event) || super.onKeyDown(keyCode, event)

    override fun onKeyUp(keyCode: Int, event: KeyEvent): Boolean =
        forwardKey(event) || super.onKeyUp(keyCode, event)

    private fun forwardKey(event: KeyEvent): Boolean =
        !accessibilityCapturesKeys && router.handle(event)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        startService(ConnectionService.startedFromUi(this))
        setContent {
            val settingsRepository: SettingsRepository = koinInject()
            val settings by
                settingsRepository.settings.collectAsStateWithLifecycle(
                    initialValue = AppSettings()
                )
            SyncSystemBars(settings.themeMode.resolveDarkTheme())
            NvdaRemoteTheme(themeMode = settings.themeMode) {
                RequestRuntimePermissions()
                MainScreen()
            }
        }
    }
}

@Composable
private fun RequestRuntimePermissions() {
    val context = LocalContext.current
    val launcher =
        rememberLauncherForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) {}

    LaunchedEffect(Unit) {
        val required = buildList {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                add(Manifest.permission.POST_NOTIFICATIONS)
            }
            if (Build.VERSION.SDK_INT >= VERSION_CODE_ANDROID_17) {
                add(ACCESS_LOCAL_NETWORK)
            }
        }
        val missing = required.filter {
            ContextCompat.checkSelfPermission(context, it) != PackageManager.PERMISSION_GRANTED
        }
        if (missing.isNotEmpty()) launcher.launch(missing.toTypedArray())
    }
}

private const val VERSION_CODE_ANDROID_17 = 37
private const val ACCESS_LOCAL_NETWORK = "android.permission.ACCESS_LOCAL_NETWORK"

@Composable
private fun SyncSystemBars(darkTheme: Boolean) {
    val activity = LocalActivity.current as? ComponentActivity ?: return
    DisposableEffect(darkTheme) {
        activity.enableEdgeToEdge(
            statusBarStyle = SystemBarStyle.auto(TRANSPARENT, TRANSPARENT) { darkTheme },
            navigationBarStyle = SystemBarStyle.auto(TRANSPARENT, TRANSPARENT) { darkTheme },
        )
        onDispose {}
    }
}
