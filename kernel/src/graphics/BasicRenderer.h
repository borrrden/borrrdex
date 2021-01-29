#pragma once

#include "../math.h"
#include "../interrupts/interrupts.h"
#include <stdint.h>

class Framebuffer;
class PSF1_FONT;

class BasicRenderer {
public:
    BasicRenderer(Framebuffer* targetFrameBuffer, PSF1_FONT* font);

    void PutChar(char chr, unsigned xOff, unsigned yOff);
    void PutChar(char chr);
    void Print(const char* str);
    void PrintN(const char* str, uint64_t len);
    void Up();
    void Left();
    void Right();
    void Down();
    void Next();

    void SetColor(unsigned color) { _color = color; }
    void SetBackground(unsigned color) { _background = color; };
    void Clear();
    void ClearChar();
    
    Point CursorPosition {};
private:
    Framebuffer* _targetFrameBuffer;
    PSF1_FONT* _psf1Font;
    unsigned _color {0xffffffff};
    unsigned _background {0x0};

    void ClearCurrent();
    void ClearCursor();
    void ShowCursor();
    bool HasChar();

    static void TimerCallback();
    static timer_chain_t TimerEntry;
};

extern BasicRenderer* GlobalRenderer;