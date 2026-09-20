package org.gozaltech.nvdaremotecompanion.android.input

import android.view.KeyEvent

data class WinKey(val vk: Int, val scan: Int = 0, val extended: Boolean = false)

object KeyMapper {

    private val letterScans =
        intArrayOf(
            0x1E,
            0x30,
            0x2E,
            0x20,
            0x12,
            0x21,
            0x22,
            0x23,
            0x17,
            0x24,
            0x25,
            0x26,
            0x32,
            0x31,
            0x18,
            0x19,
            0x10,
            0x13,
            0x1F,
            0x14,
            0x16,
            0x2F,
            0x11,
            0x2D,
            0x15,
            0x2C,
        )

    private val functionScans =
        intArrayOf(
            0x3B,
            0x3C,
            0x3D,
            0x3E,
            0x3F,
            0x40,
            0x41,
            0x42,
            0x43,
            0x44,
            0x57,
            0x58,
        )

    private val standard: Map<Int, WinKey> = buildMap {
        letterScans.forEachIndexed { index, scan ->
            put(KeyEvent.KEYCODE_A + index, WinKey(0x41 + index, scan))
        }
        put(KeyEvent.KEYCODE_0, WinKey(0x30, 0x0B))
        for (digit in 1..9) {
            put(KeyEvent.KEYCODE_0 + digit, WinKey(0x30 + digit, 0x01 + digit))
        }
        functionScans.forEachIndexed { index, scan ->
            put(KeyEvent.KEYCODE_F1 + index, WinKey(0x70 + index, scan))
        }
        putAll(
            mapOf(
                KeyEvent.KEYCODE_ESCAPE to WinKey(0x1B, 0x01),
                KeyEvent.KEYCODE_TAB to WinKey(0x09, 0x0F),
                KeyEvent.KEYCODE_CAPS_LOCK to WinKey(0x14, 0x3A),
                KeyEvent.KEYCODE_ENTER to WinKey(0x0D, 0x1C),
                KeyEvent.KEYCODE_DEL to WinKey(0x08, 0x0E),
                KeyEvent.KEYCODE_FORWARD_DEL to WinKey(0x2E, 0x53, extended = true),
                KeyEvent.KEYCODE_SPACE to WinKey(0x20, 0x39),
                KeyEvent.KEYCODE_INSERT to WinKey(0x2D, 0x52, extended = true),
                KeyEvent.KEYCODE_MOVE_HOME to WinKey(0x24, 0x47, extended = true),
                KeyEvent.KEYCODE_MOVE_END to WinKey(0x23, 0x4F, extended = true),
                KeyEvent.KEYCODE_PAGE_UP to WinKey(0x21, 0x49, extended = true),
                KeyEvent.KEYCODE_PAGE_DOWN to WinKey(0x22, 0x51, extended = true),
                KeyEvent.KEYCODE_DPAD_LEFT to WinKey(0x25, 0x4B, extended = true),
                KeyEvent.KEYCODE_DPAD_RIGHT to WinKey(0x27, 0x4D, extended = true),
                KeyEvent.KEYCODE_DPAD_UP to WinKey(0x26, 0x48, extended = true),
                KeyEvent.KEYCODE_DPAD_DOWN to WinKey(0x28, 0x50, extended = true),
                KeyEvent.KEYCODE_SHIFT_LEFT to WinKey(0xA0, 0x2A),
                KeyEvent.KEYCODE_SHIFT_RIGHT to WinKey(0xA1, 0x36),
                KeyEvent.KEYCODE_CTRL_LEFT to WinKey(0xA2, 0x1D),
                KeyEvent.KEYCODE_CTRL_RIGHT to WinKey(0xA3, 0x1D, extended = true),
                KeyEvent.KEYCODE_ALT_LEFT to WinKey(0xA4, 0x38),
                KeyEvent.KEYCODE_ALT_RIGHT to WinKey(0xA5, 0x38, extended = true),
                KeyEvent.KEYCODE_META_LEFT to WinKey(0x5B, 0x5B, extended = true),
                KeyEvent.KEYCODE_META_RIGHT to WinKey(0x5C, 0x5C, extended = true),
                KeyEvent.KEYCODE_NUM_LOCK to WinKey(0x90, 0x45),
                KeyEvent.KEYCODE_SCROLL_LOCK to WinKey(0x91, 0x46),
                KeyEvent.KEYCODE_BREAK to WinKey(0x13, 0x45),
                KeyEvent.KEYCODE_SYSRQ to WinKey(0x2C, 0x37, extended = true),
                KeyEvent.KEYCODE_NUMPAD_0 to WinKey(0x60, 0x52),
                KeyEvent.KEYCODE_NUMPAD_1 to WinKey(0x61, 0x4F),
                KeyEvent.KEYCODE_NUMPAD_2 to WinKey(0x62, 0x50),
                KeyEvent.KEYCODE_NUMPAD_3 to WinKey(0x63, 0x51),
                KeyEvent.KEYCODE_NUMPAD_4 to WinKey(0x64, 0x4B),
                KeyEvent.KEYCODE_NUMPAD_5 to WinKey(0x65, 0x4C),
                KeyEvent.KEYCODE_NUMPAD_6 to WinKey(0x66, 0x4D),
                KeyEvent.KEYCODE_NUMPAD_7 to WinKey(0x67, 0x47),
                KeyEvent.KEYCODE_NUMPAD_8 to WinKey(0x68, 0x48),
                KeyEvent.KEYCODE_NUMPAD_9 to WinKey(0x69, 0x49),
                KeyEvent.KEYCODE_NUMPAD_MULTIPLY to WinKey(0x6A, 0x37),
                KeyEvent.KEYCODE_NUMPAD_ADD to WinKey(0x6B, 0x4E),
                KeyEvent.KEYCODE_NUMPAD_SUBTRACT to WinKey(0x6D, 0x4A),
                KeyEvent.KEYCODE_NUMPAD_DOT to WinKey(0x6E, 0x53),
                KeyEvent.KEYCODE_NUMPAD_DIVIDE to WinKey(0x6F, 0x35, extended = true),
                KeyEvent.KEYCODE_NUMPAD_ENTER to WinKey(0x0D, 0x1C, extended = true),
                KeyEvent.KEYCODE_SEMICOLON to WinKey(0xBA, 0x27),
                KeyEvent.KEYCODE_EQUALS to WinKey(0xBB, 0x0D),
                KeyEvent.KEYCODE_COMMA to WinKey(0xBC, 0x33),
                KeyEvent.KEYCODE_MINUS to WinKey(0xBD, 0x0C),
                KeyEvent.KEYCODE_PERIOD to WinKey(0xBE, 0x34),
                KeyEvent.KEYCODE_SLASH to WinKey(0xBF, 0x35),
                KeyEvent.KEYCODE_GRAVE to WinKey(0xC0, 0x29),
                KeyEvent.KEYCODE_LEFT_BRACKET to WinKey(0xDB, 0x1A),
                KeyEvent.KEYCODE_BACKSLASH to WinKey(0xDC, 0x2B),
                KeyEvent.KEYCODE_RIGHT_BRACKET to WinKey(0xDD, 0x1B),
                KeyEvent.KEYCODE_APOSTROPHE to WinKey(0xDE, 0x28),
                KeyEvent.KEYCODE_MENU to WinKey(0x5D, 0x5D, extended = true),
            )
        )
    }

    private val numpadWithoutNumLock: Map<Int, WinKey> =
        mapOf(
            KeyEvent.KEYCODE_NUMPAD_0 to WinKey(0x2D, 0x52),
            KeyEvent.KEYCODE_NUMPAD_1 to WinKey(0x23, 0x4F),
            KeyEvent.KEYCODE_NUMPAD_2 to WinKey(0x28, 0x50),
            KeyEvent.KEYCODE_NUMPAD_3 to WinKey(0x22, 0x51),
            KeyEvent.KEYCODE_NUMPAD_4 to WinKey(0x25, 0x4B),
            KeyEvent.KEYCODE_NUMPAD_5 to WinKey(0x0C, 0x4C),
            KeyEvent.KEYCODE_NUMPAD_6 to WinKey(0x27, 0x4D),
            KeyEvent.KEYCODE_NUMPAD_7 to WinKey(0x24, 0x47),
            KeyEvent.KEYCODE_NUMPAD_8 to WinKey(0x26, 0x48),
            KeyEvent.KEYCODE_NUMPAD_9 to WinKey(0x21, 0x49),
            KeyEvent.KEYCODE_NUMPAD_DOT to WinKey(0x2E, 0x53),
        )

    fun map(keyCode: Int, numLockOn: Boolean): WinKey? =
        (if (numLockOn) null else numpadWithoutNumLock[keyCode]) ?: standard[keyCode]

    fun map(event: KeyEvent): WinKey? =
        map(event.keyCode, event.metaState and KeyEvent.META_NUM_LOCK_ON != 0)
}
