package org.gozaltech.nvdaremotecompanion.android.service

import android.annotation.SuppressLint
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.os.Build
import android.os.IBinder
import android.os.PowerManager
import android.util.Log
import androidx.core.app.NotificationCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.launch
import org.gozaltech.nvdaremotecompanion.android.MainActivity
import org.gozaltech.nvdaremotecompanion.android.R
import org.gozaltech.nvdaremotecompanion.android.data.SettingsRepository
import org.gozaltech.nvdaremotecompanion.android.remote.NvdaRemoteRepository
import org.gozaltech.nvdaremotecompanion.android.speech.SpeechBridge
import org.koin.android.ext.android.inject

private const val TAG = "NVDARemote/Service"
private const val NOTIFICATION_ID = 1
private const val CHANNEL_ID = "nvdaremote_connection"
private const val WAKE_LOCK_TAG = "NVDARemote:connection"

private const val MEDIA_CAPABLE_SERVICE_TYPES =
    ServiceInfo.FOREGROUND_SERVICE_TYPE_REMOTE_MESSAGING or
        ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK

class ConnectionService : Service() {

    private val repository: NvdaRemoteRepository by inject()
    private val settingsRepository: SettingsRepository by inject()
    private val speech: SpeechBridge by inject()

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)

    private lateinit var wakeLock: PowerManager.WakeLock

    private val connectivityManager: ConnectivityManager
        get() = getSystemService(ConnectivityManager::class.java)

    private val notificationManager: NotificationManager
        get() = getSystemService(NotificationManager::class.java)

    private val networkCallback =
        object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(network: Network) = repository.reconnectDesired()

            override fun onLost(network: Network) = repository.suspendConnections()
        }

    override fun onBind(intent: Intent): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        wakeLock =
            getSystemService(PowerManager::class.java)
                .newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, WAKE_LOCK_TAG)

        val settings = settingsRepository.currentBlocking()
        speech.configure(settings)
        repository.start()
        if (settings.autoConnect) repository.autoConnectAll()

        connectivityManager.registerNetworkCallback(
            NetworkRequest.Builder()
                .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .build(),
            networkCallback,
        )

        scope.launch { settingsRepository.settings.collect(speech::configure) }
        scope.launch {
            repository.state
                .map { it.connectedCount }
                .distinctUntilChanged()
                .collect(::onConnectedCountChanged)
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val mediaCapable = intent?.getBooleanExtra(EXTRA_FOREGROUND_START, false) == true
        runCatching { startForegroundNotification(mediaCapable) }
            .onFailure { Log.w(TAG, "Could not enter the foreground: ${it.message}") }
        return START_STICKY
    }

    override fun onDestroy() {
        scope.cancel()
        runCatching { connectivityManager.unregisterNetworkCallback(networkCallback) }
        repository.shutdown()
        speech.shutdown()
        if (wakeLock.isHeld) wakeLock.release()
        super.onDestroy()
    }

    @SuppressLint("WakelockTimeout")
    private fun onConnectedCountChanged(connectedCount: Int) {
        notificationManager.notify(NOTIFICATION_ID, buildNotification(connectedCount))
        when {
            connectedCount > 0 && !wakeLock.isHeld -> wakeLock.acquire()
            connectedCount == 0 && wakeLock.isHeld -> wakeLock.release()
        }
    }

    private fun createNotificationChannel() {
        val channel =
            NotificationChannel(
                    CHANNEL_ID,
                    getString(R.string.notification_channel_name),
                    NotificationManager.IMPORTANCE_LOW,
                )
                .apply { description = getString(R.string.notification_channel_desc) }
        notificationManager.createNotificationChannel(channel)
    }

    private fun buildNotification(connectedCount: Int): Notification {
        val text =
            if (connectedCount == 0) {
                getString(R.string.notification_no_connections)
            } else {
                resources.getQuantityString(
                    R.plurals.notification_connected,
                    connectedCount,
                    connectedCount,
                )
            }
        val contentIntent =
            PendingIntent.getActivity(
                this,
                0,
                Intent(this, MainActivity::class.java),
                PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
            )
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle(getString(R.string.app_name))
            .setContentText(text)
            .setSmallIcon(R.drawable.ic_notification)
            .setContentIntent(contentIntent)
            .setOngoing(true)
            .setSilent(true)
            .build()
    }

    private fun startForegroundNotification(mediaCapable: Boolean) {
        val notification = buildNotification(repository.state.value.connectedCount)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            val types =
                if (mediaCapable) {
                    MEDIA_CAPABLE_SERVICE_TYPES
                } else {
                    ServiceInfo.FOREGROUND_SERVICE_TYPE_REMOTE_MESSAGING
                }
            startForeground(NOTIFICATION_ID, notification, types)
        } else {
            startForeground(NOTIFICATION_ID, notification)
        }
    }

    companion object {
        const val EXTRA_FOREGROUND_START = "foreground_start"

        fun startedFromUi(context: Context): Intent =
            Intent(context, ConnectionService::class.java).putExtra(EXTRA_FOREGROUND_START, true)

        fun startedFromBackground(context: Context): Intent =
            Intent(context, ConnectionService::class.java)
    }
}
