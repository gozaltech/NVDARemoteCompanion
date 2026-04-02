package org.gozaltech.nvdaremotecompanion.android;

import android.view.KeyEvent;

import java.util.HashMap;
import java.util.Map;

public class KeyMapper {

    static class WinKey {
        final int vk;
        final int scan;
        final boolean extended;

        WinKey(int vk, int scan, boolean extended) {
            this.vk = vk;
            this.scan = scan;
            this.extended = extended;
        }

        WinKey(int vk, int scan) {
            this(vk, scan, false);
        }

        WinKey(int vk) {
            this(vk, 0, false);
        }
    }

    private static final Map<Integer, WinKey> map = new HashMap<>();
    private static final Map<Integer, WinKey> numpadOffMap = new HashMap<>();

    static {
        int[] letterScans = {
            0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24,
            0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14,
            0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
        };
        for (int i = 0; i <= 25; i++) {
            map.put(KeyEvent.KEYCODE_A + i, new WinKey(0x41 + i, letterScans[i]));
        }

        map.put(KeyEvent.KEYCODE_0, new WinKey(0x30, 0x0B));
        for (int i = 1; i <= 9; i++) {
            map.put(KeyEvent.KEYCODE_0 + i, new WinKey(0x30 + i, 0x01 + i));
        }

        int[] fScans = {
            0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40,
            0x41, 0x42, 0x43, 0x44, 0x57, 0x58
        };
        for (int i = 0; i <= 11; i++) {
            map.put(KeyEvent.KEYCODE_F1 + i, new WinKey(0x70 + i, fScans[i]));
        }
        for (int i = 12; i <= 23; i++) {
            map.put(KeyEvent.KEYCODE_F1 + i, new WinKey(0x70 + i));
        }

        map.put(KeyEvent.KEYCODE_ESCAPE,      new WinKey(0x1B, 0x01));
        map.put(KeyEvent.KEYCODE_TAB,         new WinKey(0x09, 0x0F));
        map.put(KeyEvent.KEYCODE_CAPS_LOCK,   new WinKey(0x14, 0x3A));
        map.put(KeyEvent.KEYCODE_ENTER,       new WinKey(0x0D, 0x1C));
        map.put(KeyEvent.KEYCODE_DEL,         new WinKey(0x08, 0x0E));
        map.put(KeyEvent.KEYCODE_FORWARD_DEL, new WinKey(0x2E, 0x53, true));
        map.put(KeyEvent.KEYCODE_SPACE,       new WinKey(0x20, 0x39));
        map.put(KeyEvent.KEYCODE_INSERT,      new WinKey(0x2D, 0x52, true));
        map.put(KeyEvent.KEYCODE_MOVE_HOME,   new WinKey(0x24, 0x47, true));
        map.put(KeyEvent.KEYCODE_MOVE_END,    new WinKey(0x23, 0x4F, true));
        map.put(KeyEvent.KEYCODE_PAGE_UP,     new WinKey(0x21, 0x49, true));
        map.put(KeyEvent.KEYCODE_PAGE_DOWN,   new WinKey(0x22, 0x51, true));
        map.put(KeyEvent.KEYCODE_DPAD_LEFT,   new WinKey(0x25, 0x4B, true));
        map.put(KeyEvent.KEYCODE_DPAD_RIGHT,  new WinKey(0x27, 0x4D, true));
        map.put(KeyEvent.KEYCODE_DPAD_UP,     new WinKey(0x26, 0x48, true));
        map.put(KeyEvent.KEYCODE_DPAD_DOWN,   new WinKey(0x28, 0x50, true));
        map.put(KeyEvent.KEYCODE_SHIFT_LEFT,  new WinKey(0xA0, 0x2A));
        map.put(KeyEvent.KEYCODE_SHIFT_RIGHT, new WinKey(0xA1, 0x36));
        map.put(KeyEvent.KEYCODE_CTRL_LEFT,   new WinKey(0xA2, 0x1D));
        map.put(KeyEvent.KEYCODE_CTRL_RIGHT,  new WinKey(0xA3, 0x1D, true));
        map.put(KeyEvent.KEYCODE_ALT_LEFT,    new WinKey(0xA4, 0x38));
        map.put(KeyEvent.KEYCODE_ALT_RIGHT,   new WinKey(0xA5, 0x38, true));
        map.put(KeyEvent.KEYCODE_META_LEFT,   new WinKey(0x5B, 0x5B, true));
        map.put(KeyEvent.KEYCODE_META_RIGHT,  new WinKey(0x5C, 0x5C, true));
        map.put(KeyEvent.KEYCODE_NUM_LOCK,    new WinKey(0x90, 0x45));
        map.put(KeyEvent.KEYCODE_SCROLL_LOCK, new WinKey(0x91, 0x46));
        map.put(KeyEvent.KEYCODE_BREAK,       new WinKey(0x13, 0x45));
        map.put(KeyEvent.KEYCODE_SYSRQ,       new WinKey(0x2C, 0x37, true));
        map.put(KeyEvent.KEYCODE_NUMPAD_0,    new WinKey(0x60, 0x52));
        map.put(KeyEvent.KEYCODE_NUMPAD_1,    new WinKey(0x61, 0x4F));
        map.put(KeyEvent.KEYCODE_NUMPAD_2,    new WinKey(0x62, 0x50));
        map.put(KeyEvent.KEYCODE_NUMPAD_3,    new WinKey(0x63, 0x51));
        map.put(KeyEvent.KEYCODE_NUMPAD_4,    new WinKey(0x64, 0x4B));
        map.put(KeyEvent.KEYCODE_NUMPAD_5,    new WinKey(0x65, 0x4C));
        map.put(KeyEvent.KEYCODE_NUMPAD_6,    new WinKey(0x66, 0x4D));
        map.put(KeyEvent.KEYCODE_NUMPAD_7,    new WinKey(0x67, 0x47));
        map.put(KeyEvent.KEYCODE_NUMPAD_8,    new WinKey(0x68, 0x48));
        map.put(KeyEvent.KEYCODE_NUMPAD_9,    new WinKey(0x69, 0x49));
        map.put(KeyEvent.KEYCODE_NUMPAD_MULTIPLY, new WinKey(0x6A, 0x37));
        map.put(KeyEvent.KEYCODE_NUMPAD_ADD,      new WinKey(0x6B, 0x4E));
        map.put(KeyEvent.KEYCODE_NUMPAD_SUBTRACT, new WinKey(0x6D, 0x4A));
        map.put(KeyEvent.KEYCODE_NUMPAD_DOT,      new WinKey(0x6E, 0x53));
        map.put(KeyEvent.KEYCODE_NUMPAD_DIVIDE,   new WinKey(0x6F, 0x35, true));
        map.put(KeyEvent.KEYCODE_NUMPAD_ENTER,    new WinKey(0x0D, 0x1C, true));
        map.put(KeyEvent.KEYCODE_SEMICOLON,       new WinKey(0xBA, 0x27));
        map.put(KeyEvent.KEYCODE_EQUALS,          new WinKey(0xBB, 0x0D));
        map.put(KeyEvent.KEYCODE_COMMA,           new WinKey(0xBC, 0x33));
        map.put(KeyEvent.KEYCODE_MINUS,           new WinKey(0xBD, 0x0C));
        map.put(KeyEvent.KEYCODE_PERIOD,          new WinKey(0xBE, 0x34));
        map.put(KeyEvent.KEYCODE_SLASH,           new WinKey(0xBF, 0x35));
        map.put(KeyEvent.KEYCODE_GRAVE,           new WinKey(0xC0, 0x29));
        map.put(KeyEvent.KEYCODE_LEFT_BRACKET,    new WinKey(0xDB, 0x1A));
        map.put(KeyEvent.KEYCODE_BACKSLASH,       new WinKey(0xDC, 0x2B));
        map.put(KeyEvent.KEYCODE_RIGHT_BRACKET,   new WinKey(0xDD, 0x1B));
        map.put(KeyEvent.KEYCODE_APOSTROPHE,      new WinKey(0xDE, 0x28));
        map.put(KeyEvent.KEYCODE_MENU,            new WinKey(0x5D, 0x5D, true));

        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_0,   new WinKey(0x2D, 0x52, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_1,   new WinKey(0x23, 0x4F, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_2,   new WinKey(0x28, 0x50, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_3,   new WinKey(0x22, 0x51, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_4,   new WinKey(0x25, 0x4B, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_5,   new WinKey(0x0C, 0x4C, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_6,   new WinKey(0x27, 0x4D, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_7,   new WinKey(0x24, 0x47, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_8,   new WinKey(0x26, 0x48, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_9,   new WinKey(0x21, 0x49, false));
        numpadOffMap.put(KeyEvent.KEYCODE_NUMPAD_DOT, new WinKey(0x2E, 0x53, false));
    }

    static WinKey mapEvent(KeyEvent event) {
        boolean numlockOff = (event.getMetaState() & KeyEvent.META_NUM_LOCK_ON) == 0;
        if (numlockOff) {
            WinKey numpadKey = numpadOffMap.get(event.getKeyCode());
            if (numpadKey != null) return numpadKey;
        }
        return map.get(event.getKeyCode());
    }

    public static boolean handleKeyEvent(KeyEvent event, int profileIndex) {
        WinKey winKey = mapEvent(event);
        if (winKey == null) return false;

        boolean pressed = event.getAction() == KeyEvent.ACTION_DOWN;

        NativeBridge.nativeSendKeyEvent(
                winKey.vk,
                winKey.scan,
                pressed,
                winKey.extended,
                profileIndex
        );

        return true;
    }
}
