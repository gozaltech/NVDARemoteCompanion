package org.gozaltech.nvdaremotecompanion.android.update

import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageInstaller.SessionParams
import android.content.pm.PackageManager
import android.content.pm.Signature
import android.content.pm.SigningInfo
import android.os.Build
import android.os.Environment
import androidx.core.content.FileProvider
import java.io.File
import java.net.HttpURLConnection
import java.net.URL
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.currentCoroutineContext
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOn
import kotlinx.coroutines.withContext

private const val APK_NAME = "NVDARemoteCompanion-update.apk"
private const val CONNECT_TIMEOUT_MS = 15_000
private const val READ_TIMEOUT_MS = 30_000
private const val DOWNLOAD_BUFFER = 8 * 1024
private const val INSTALL_BUFFER = 64 * 1024
private const val PERCENT = 100

sealed interface DownloadState {
    data class Progress(val percent: Int) : DownloadState

    data class Complete(val apk: File) : DownloadState
}

class UpdateInstaller(private val context: Context) {

    fun download(downloadUrl: String): Flow<DownloadState> = flow {
        val target =
            File(
                context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS) ?: context.cacheDir,
                APK_NAME,
            )
        val connection =
            (URL(downloadUrl).openConnection() as HttpURLConnection).apply {
                connectTimeout = CONNECT_TIMEOUT_MS
                readTimeout = READ_TIMEOUT_MS
                connect()
            }
        try {
            val total = connection.contentLength
            var downloaded = 0L
            var lastPercent = -1
            connection.inputStream.use { input ->
                target.outputStream().use { output ->
                    val buffer = ByteArray(DOWNLOAD_BUFFER)
                    while (true) {
                        currentCoroutineContext().ensureActive()
                        val read = input.read(buffer)
                        if (read == -1) break
                        output.write(buffer, 0, read)
                        downloaded += read
                        if (total > 0) {
                            val percent = (downloaded * PERCENT / total).toInt()
                            if (percent != lastPercent) {
                                lastPercent = percent
                                emit(DownloadState.Progress(percent))
                            }
                        }
                    }
                }
            }
        } finally {
            connection.disconnect()
        }
        emit(DownloadState.Complete(target))
    }
        .flowOn(Dispatchers.IO)

    suspend fun hasMatchingSignature(apk: File): Boolean =
        withContext(Dispatchers.IO) {
            runCatching {
                    val installed = installedSignatures() ?: return@runCatching false
                    val candidate = archiveSignatures(apk.absolutePath) ?: return@runCatching false
                    installed.size == candidate.size &&
                        installed.all { signature ->
                            candidate.any {
                                it.toByteArray().contentEquals(signature.toByteArray())
                            }
                        }
                }
                .getOrDefault(false)
        }

    fun install(apk: File) {
        runCatching { installViaSession(apk) }.onFailure { installInteractive(apk) }
    }

    private fun installViaSession(apk: File) {
        val installer = context.packageManager.packageInstaller
        val params = SessionParams(SessionParams.MODE_FULL_INSTALL)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            params.setRequireUserAction(SessionParams.USER_ACTION_NOT_REQUIRED)
        }
        val sessionId = installer.createSession(params)
        installer.openSession(sessionId).use { session ->
            apk.inputStream().use { input ->
                session.openWrite("package", 0, apk.length()).use { output ->
                    input.copyTo(output, INSTALL_BUFFER)
                    session.fsync(output)
                }
            }
            val intent = Intent(context, InstallResultReceiver::class.java)
            val pendingIntent =
                PendingIntent.getBroadcast(
                    context,
                    sessionId,
                    intent,
                    PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE,
                )
            session.commit(pendingIntent.intentSender)
        }
    }

    @Suppress("DEPRECATION")
    private fun installInteractive(apk: File) {
        val uri = FileProvider.getUriForFile(context, "${context.packageName}.fileprovider", apk)
        val intent =
            Intent(Intent.ACTION_INSTALL_PACKAGE)
                .setData(uri)
                .setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_ACTIVITY_NEW_TASK)
        context.startActivity(intent)
    }

    private fun installedSignatures(): Array<Signature>? =
        context.packageManager
            .getPackageInfo(context.packageName, PackageManager.GET_SIGNING_CERTIFICATES)
            .signingInfo
            ?.certificates()

    private fun archiveSignatures(apkPath: String): Array<Signature>? =
        context.packageManager
            .getPackageArchiveInfo(apkPath, PackageManager.GET_SIGNING_CERTIFICATES)
            ?.signingInfo
            ?.certificates()
}

private fun SigningInfo.certificates(): Array<Signature>? =
    if (hasMultipleSigners()) apkContentsSigners else signingCertificateHistory
