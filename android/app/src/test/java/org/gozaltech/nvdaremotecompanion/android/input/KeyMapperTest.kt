package org.gozaltech.nvdaremotecompanion.android.input

import android.view.KeyEvent
import kotlin.test.assertEquals
import kotlin.test.assertNull
import org.junit.Test

class KeyMapperTest {

    @Test
    fun `letters map to sequential virtual key codes`() {
        assertEquals(WinKey(0x41, 0x1E), KeyMapper.map(KeyEvent.KEYCODE_A, numLockOn = true))
        assertEquals(WinKey(0x5A, 0x2C), KeyMapper.map(KeyEvent.KEYCODE_Z, numLockOn = true))
    }

    @Test
    fun `digit zero does not follow the one-to-nine pattern`() {
        assertEquals(WinKey(0x30, 0x0B), KeyMapper.map(KeyEvent.KEYCODE_0, numLockOn = true))
        assertEquals(WinKey(0x31, 0x02), KeyMapper.map(KeyEvent.KEYCODE_1, numLockOn = true))
        assertEquals(WinKey(0x39, 0x0A), KeyMapper.map(KeyEvent.KEYCODE_9, numLockOn = true))
    }

    @Test
    fun `function keys F1 to F12 have explicit scan codes`() {
        assertEquals(WinKey(0x70, 0x3B), KeyMapper.map(KeyEvent.KEYCODE_F1, numLockOn = true))
        assertEquals(WinKey(0x7B, 0x58), KeyMapper.map(KeyEvent.KEYCODE_F12, numLockOn = true))
    }

    @Test
    fun `numpad keycodes are never treated as function keys`() {
        val numLock = KeyMapper.map(KeyEvent.KEYCODE_NUM_LOCK, numLockOn = true)
        assertEquals(WinKey(0x90, 0x45), numLock)

        val numpadDivide = KeyMapper.map(KeyEvent.KEYCODE_NUMPAD_DIVIDE, numLockOn = true)
        assertEquals(WinKey(0x6F, 0x35, extended = true), numpadDivide)
    }

    @Test
    fun `numpad falls back to navigation keys when num lock is off`() {
        val seven = KeyMapper.map(KeyEvent.KEYCODE_NUMPAD_7, numLockOn = false)
        assertEquals(WinKey(0x24, 0x47), seven)
        val dot = KeyMapper.map(KeyEvent.KEYCODE_NUMPAD_DOT, numLockOn = false)
        assertEquals(WinKey(0x2E, 0x53), dot)
    }

    @Test
    fun `numpad uses digits when num lock is on`() {
        val seven = KeyMapper.map(KeyEvent.KEYCODE_NUMPAD_7, numLockOn = true)
        assertEquals(WinKey(0x67, 0x47), seven)
        val dot = KeyMapper.map(KeyEvent.KEYCODE_NUMPAD_DOT, numLockOn = true)
        assertEquals(WinKey(0x6E, 0x53), dot)
    }

    @Test
    fun `non numpad keys ignore the num lock state`() {
        val withNumLock = KeyMapper.map(KeyEvent.KEYCODE_ENTER, numLockOn = true)
        val withoutNumLock = KeyMapper.map(KeyEvent.KEYCODE_ENTER, numLockOn = false)
        assertEquals(withNumLock, withoutNumLock)
    }

    @Test
    fun `extended flag is set for the navigation cluster`() {
        val home = KeyMapper.map(KeyEvent.KEYCODE_MOVE_HOME, numLockOn = true)
        assertEquals(WinKey(0x24, 0x47, extended = true), home)
    }

    @Test
    fun `unmapped keycodes return null`() {
        assertNull(KeyMapper.map(KeyEvent.KEYCODE_CAMERA, numLockOn = true))
    }
}
