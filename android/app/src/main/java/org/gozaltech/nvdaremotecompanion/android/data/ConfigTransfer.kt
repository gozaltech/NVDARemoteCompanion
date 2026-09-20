package org.gozaltech.nvdaremotecompanion.android.data

import android.content.Context
import android.net.Uri
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import org.gozaltech.nvdaremotecompanion.android.NativeBridge

class ConfigTransfer(private val context: Context) {

    private val json = Json { ignoreUnknownKeys = true }

    suspend fun export(uri: Uri): Result<Unit> =
        withContext(Dispatchers.IO) {
            runCatching {
                val payload = NativeBridge.nativeGetConfigJson()
                val stream =
                    context.contentResolver.openOutputStream(uri)
                        ?: error("Unable to open $uri for writing")
                stream.use { it.write(payload.toByteArray()) }
            }
        }

    suspend fun import(uri: Uri): Result<Int> =
        withContext(Dispatchers.IO) {
            runCatching {
                val stream =
                    context.contentResolver.openInputStream(uri)
                        ?: error("Unable to open $uri for reading")
                val payload = stream.bufferedReader().use { it.readText() }
                json.parseToJsonElement(payload)
                NativeBridge.nativeMergeConfig(payload)
            }
        }
}
