package org.gozaltech.nvdaremotecompanion.android.input

import android.os.Handler
import android.os.Looper
import android.view.KeyEvent

private const val REPEAT_INITIAL_DELAY_MS = 400L
private const val REPEAT_INTERVAL_MS = 50L

class KeyEventRouter(private val forwarder: KeyForwarder) {

    private val handler = Handler(Looper.getMainLooper())
    private var repeatTask: Runnable? = null

    fun handle(event: KeyEvent): Boolean {
        val pressed =
            when (event.action) {
                KeyEvent.ACTION_DOWN -> true
                KeyEvent.ACTION_UP -> false
                else -> return false
            }
        val key = KeyMapper.map(event) ?: return false
        val isAutoRepeat = event.repeatCount > 0

        if (consumedAsShortcut(key, pressed, isAutoRepeat)) return true

        val profileIndex = forwarder.activeProfileIndex
        if (profileIndex < 0 || !forwarder.isForwarding) return false

        when {
            pressed && !isAutoRepeat -> {
                cancelRepeat()
                forwarder.send(key, pressed = true, profileIndex = profileIndex)
                startRepeat(key, profileIndex)
            }

            !pressed -> {
                cancelRepeat()
                forwarder.send(key, pressed = false, profileIndex = profileIndex)
            }
        }
        return true
    }

    fun cancelRepeat() {
        repeatTask?.let(handler::removeCallbacks)
        repeatTask = null
    }

    private fun consumedAsShortcut(key: WinKey, pressed: Boolean, isAutoRepeat: Boolean): Boolean {
        if (pressed && !isAutoRepeat) {
            if (forwarder.consumeAsModifierOrShortcut(key.vk, true)) {
                cancelRepeat()
                return true
            }
        } else if (!pressed) {
            forwarder.consumeAsModifierOrShortcut(key.vk, false)
        }
        return false
    }

    private fun startRepeat(key: WinKey, profileIndex: Int) {
        val task =
            object : Runnable {
                override fun run() {
                    if (forwarder.activeProfileIndex != profileIndex || !forwarder.isForwarding) {
                        return
                    }
                    forwarder.send(key, pressed = true, profileIndex = profileIndex)
                    handler.postDelayed(this, REPEAT_INTERVAL_MS)
                }
            }
        repeatTask = task
        handler.postDelayed(task, REPEAT_INITIAL_DELAY_MS)
    }
}
