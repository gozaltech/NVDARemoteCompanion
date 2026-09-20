package org.gozaltech.nvdaremotecompanion.android.update

import java.net.HttpURLConnection
import java.net.URL
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.SerialName
import kotlinx.serialization.Serializable
import kotlinx.serialization.json.Json

private const val API_URL =
    "https://api.github.com/repos/gozaltech/NVDARemoteCompanion/releases/latest"
private const val TIMEOUT_MS = 10_000
private const val HTTP_OK = 200

data class UpdateInfo(
    val version: String,
    val downloadUrl: String,
    val releaseNotes: String,
)

@Serializable
private data class Release(
    @SerialName("tag_name") val tagName: String,
    val body: String = "",
    val assets: List<ReleaseAsset> = emptyList(),
)

@Serializable
private data class ReleaseAsset(
    val name: String,
    @SerialName("browser_download_url") val downloadUrl: String,
)

object UpdateChecker {

    private val json = Json { ignoreUnknownKeys = true }

    suspend fun check(currentVersion: String): Result<UpdateInfo?> =
        withContext(Dispatchers.IO) {
            runCatching {
                val release = json.decodeFromString<Release>(fetchLatestRelease())
                val latest = release.tagName.removePrefix("v")
                if (!isNewer(latest, currentVersion)) return@runCatching null
                val apk =
                    release.assets.firstOrNull { it.name.endsWith(".apk") }
                        ?: return@runCatching null
                UpdateInfo(latest, apk.downloadUrl, release.body)
            }
        }

    fun isNewer(candidate: String, current: String): Boolean {
        val candidateParts = parseVersion(candidate)
        val currentParts = parseVersion(current)
        repeat(maxOf(candidateParts.size, currentParts.size)) { index ->
            val left = candidateParts.getOrElse(index) { 0 }
            val right = currentParts.getOrElse(index) { 0 }
            if (left != right) return left > right
        }
        return false
    }

    private fun parseVersion(version: String): List<Int> =
        version.split('.').map { part -> part.filter(Char::isDigit).toIntOrNull() ?: 0 }

    private fun fetchLatestRelease(): String {
        val connection =
            (URL(API_URL).openConnection() as HttpURLConnection).apply {
                connectTimeout = TIMEOUT_MS
                readTimeout = TIMEOUT_MS
                setRequestProperty("Accept", "application/vnd.github+json")
            }
        try {
            check(connection.responseCode == HTTP_OK) { "HTTP ${connection.responseCode}" }
            return connection.inputStream.bufferedReader().use { it.readText() }
        } finally {
            connection.disconnect()
        }
    }
}
