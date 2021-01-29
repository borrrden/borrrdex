#include "keyboard.h"
#include "keymaps.h"
#include "../cstr.h"
#include "../graphics/BasicRenderer.h"

bool leftShift, rightShift, cursor;

void HandleCursor(uint8_t scancode) {
    cursor = false;
    switch(scancode) {
        case CursorUp:
            GlobalRenderer->Up();
            return;
        case CursorDown:
            GlobalRenderer->Down();
            return;
        case CursorLeft:
            GlobalRenderer->Left();
            return;
        case CursorRight:
            GlobalRenderer->Right();
            return;
    }
}

void HandleKeyboard(uint8_t scancode) {
    if(cursor) {
        HandleCursor(scancode);
        return;
    }

    switch(scancode) {
        case LeftShift:
            leftShift = true;
            return;
        case LeftShift + 0x80:
            leftShift = false;
            return;
        case RightShift:
            rightShift = true;
            return;
        case RightShift + 0x80:
            rightShift = false;
            return;
        case Enter:
            GlobalRenderer->Next();
            return;
        case BackSpace:
            GlobalRenderer->ClearChar();
            return;
        case CursorStart:
            cursor = true;
            return;
    }

    char ascii = KeyboardMapFunction(scancode, leftShift || rightShift);
    if(ascii != 0) {
        GlobalRenderer->PutChar(ascii);
    }
}