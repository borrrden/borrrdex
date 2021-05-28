#include <borrrdex/core/keyboard.h>
#include <borrrdex/core/input.h>

#include "fterm.h"

static int keymap_us[128] = {
    0, KEY_ESCAPE, '1', '2',  '3', '4', '5', '6',  
    '7', '8', '9', '0', '-', '=', KEY_BACKSPACE, KEY_TAB, 
    'q', 'w', 'e', 'r', 't', 'y',  'u',  'i', 
    'o', 'p', '[', ']', KEY_ENTER, KEY_CONTROL, 'a', 's', 
    'd', 'f', 'g', 'h', 'j', 'k',  'l',  ';',
    '\'', '`',  KEY_SHIFT, '\\', 'z', 'x', 'c', 'v', 
    'b', 'n', 'm', ',', '.', '/',  0, '*',
    KEY_ALT, ' ', KEY_CAPS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0, 0, KEY_HOME,
    KEY_ARROW_UP, 0, '-', KEY_ARROW_LEFT, 0, KEY_ARROW_RIGHT, '+', KEY_END,
    KEY_ARROW_DOWN, 0, 0, 0, 0, 0, 0, 0,
    0, 0, KEY_GUI, KEY_GUI
};

static int keymap_shift_us[128] = {
    0, KEY_ESCAPE, '!', '@',  '#', '$', '%', '^',  
    '&', '*', '(', ')', '_', '+', KEY_BACKSPACE, KEY_TAB, 
    'Q', 'W', 'E', 'R', 'T', 'Y',  'U',  'I', 
    'O', 'P', '{', '}', KEY_ENTER, KEY_CONTROL, 'A', 'S', 
    'D', 'F', 'G', 'H', 'J', 'K',  'L',  ':',
    '"', '~',  KEY_SHIFT, '|', 'Z', 'X', 'C', 'V', 
    'B', 'N', 'M', '<', '>', '?',  0, '*',
    KEY_ALT, ' ', KEY_CAPS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0, 0, KEY_HOME,
    KEY_ARROW_UP, 0, '-', KEY_ARROW_LEFT, 0, KEY_ARROW_RIGHT, '+', KEY_END,
    KEY_ARROW_DOWN, 0, 0, 0, 0, 0, 0, 0,
    0, 0, KEY_GUI, KEY_GUI
};

static int keymap_jp[128] = {
    0, KEY_ESCAPE, '1', '2',  '3', '4', '5', '6',  
    '7', '8', '9', '0', '-', '^', KEY_BACKSPACE, KEY_TAB, 
    'q', 'w', 'e', 'r', 't', 'y',  'u',  'i', 
    'o', 'p', '@', '[', KEY_ENTER, KEY_CONTROL, 'a', 's', 
    'd', 'f', 'g', 'h', 'j', 'k',  'l',  ';',
    ':', ']',  KEY_SHIFT, ']', 'z', 'x', 'c', 'v', 
    'b', 'n', 'm', ',', '.', '/',  0, '\\',
    KEY_ALT, ' ', KEY_CAPS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0, 0, KEY_HOME,
    KEY_ARROW_UP, 0, '-', KEY_ARROW_LEFT, 0, KEY_ARROW_RIGHT, '+', KEY_END,
    KEY_ARROW_DOWN, 0, 0, 0, 0, 0, 0, 0,
    0, 0, KEY_GUI, KEY_GUI
};

static int keymap_shift_jp[128] = {
    0, KEY_ESCAPE, '!', '"',  '#', '$', '%', '&',  
    '\'', '(', ')', 0, '=', '~', KEY_BACKSPACE, KEY_TAB, 
    'Q', 'W', 'E', 'R', 'T', 'Y',  'U',  'I', 
    'O', 'P', '`', '{', KEY_ENTER, KEY_CONTROL, 'A', 'S', 
    'D', 'F', 'G', 'H', 'J', 'K',  'L',  '+',
    '*', '}',  KEY_SHIFT, '}', 'Z', 'X', 'C', 'V', 
    'B', 'N', 'M', '<', '>', '?',  0, '_',
    KEY_ALT, ' ', KEY_CAPS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0, 0, KEY_HOME,
    KEY_ARROW_UP, 0, '-', KEY_ARROW_LEFT, 0, KEY_ARROW_RIGHT, '+', KEY_END,
    KEY_ARROW_DOWN, 0, 0, 0, 0, 0, 0, 0,
    0, 0, KEY_GUI, KEY_GUI
};

void input_manager::poll() {
    uint8_t buf[16];
    ssize_t count = borrrdex::poll_keyboard(buf, 16);

    for(ssize_t i = 0; i < count; i++) {
        uint8_t code = buf[i] & 0x7F;
        bool is_pressed = !((buf[i] >> 7) & 1);
        int key = 0;

        if(_keyboard.shift) {
            key = keymap_shift_jp[code];
        } else {
            key = keymap_jp[code];
        }

        switch(key) {
            case KEY_SHIFT:
                _keyboard.shift = is_pressed;
                break;
            case KEY_CONTROL:
                _keyboard.control = is_pressed;
                break;
            case KEY_ALT:
                _keyboard.alt = is_pressed;
                break;
            case KEY_CAPS:
                if(is_pressed) {
                    _keyboard.caps = !_keyboard.caps;
                }

                break;
        }

        if(_cb && is_pressed) {
            _cb(key);
        }
    }
}