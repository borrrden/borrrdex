#include "BasicRenderer.h"
#include "SimpleFont.h"
#include "../uefi/FrameBuffer.h"

#define CURSOR_COLOR 0xff0000cc
#define CURSOR_TICKS 10

BasicRenderer* GlobalRenderer;

bool GlobalInitialized = false;
static int TimerCount = 0;
void BasicRenderer::TimerCallback() {
    if(TimerCount < CURSOR_TICKS) {
        GlobalRenderer->ShowCursor();
    } else {
        GlobalRenderer->ClearCursor();
    }

    TimerCount = (TimerCount + 1) % (CURSOR_TICKS * 2);
}

timer_chain_t BasicRenderer::TimerEntry = {
    TimerCallback,
    NULL
};

bool BasicRenderer::HasChar() {
    unsigned* pixPtr = (unsigned *)_targetFrameBuffer->baseAddress;
    for(unsigned i = 0; i < 8; i++) {
        for(unsigned j = 0; j < 16; j++) {
            unsigned col = *(unsigned*)(pixPtr + CursorPosition.x + (CursorPosition.y * _targetFrameBuffer->pixelsPerScanline));
            if(col != _background) {
                return true;
            }
        }
    }

    return false;
}

BasicRenderer::BasicRenderer(Framebuffer* targetFrameBuffer, PSF1_FONT* font) 
    :_targetFrameBuffer(targetFrameBuffer)
    ,_psf1Font(font) {
        if(!GlobalInitialized) {
            GlobalInitialized = true;
            register_timer_cb(&TimerEntry);
        }
}

void BasicRenderer::Print(const char* str) {
    PrintN(str, UINT64_MAX);
}

void BasicRenderer::PrintN(const char* str, uint64_t len) {
    const char* chr = str;
    while(*chr && len-- > 0) {
        PutChar(*chr, CursorPosition.x, CursorPosition.y);
        CursorPosition.x += 8;
        if(CursorPosition.x + 8 > _targetFrameBuffer->width) {
            CursorPosition.x = 0;
            CursorPosition.y += 16;
        }

        chr++;
    }
}

void BasicRenderer::ClearCurrent() {
    unsigned xOff = CursorPosition.x;
    unsigned yOff = CursorPosition.y;
    unsigned* pixPtr = (unsigned *)_targetFrameBuffer->baseAddress;
    for(unsigned long y = yOff; y < yOff + 16; y++) {
        for(unsigned long x = xOff; x < xOff + 8; x++) {
            *(unsigned*)(pixPtr + x + (y * _targetFrameBuffer->pixelsPerScanline)) = _background;
        }
    }
}

void BasicRenderer::ShowCursor() {
    if(TimerCount >= CURSOR_TICKS) {
        return;
    }
    
    unsigned xOff = CursorPosition.x;
    unsigned yOff = CursorPosition.y;
    unsigned* pixPtr = (unsigned *)_targetFrameBuffer->baseAddress;
    for(unsigned long y = yOff; y < yOff + 16; y++) {
        for(unsigned long x = xOff; x < xOff + 8; x++) {
            unsigned* curColor = (unsigned*)(pixPtr + x + (y * _targetFrameBuffer->pixelsPerScanline));
            if(*curColor == _background) {
                *curColor = CURSOR_COLOR;
            }
        }
    }
}

void BasicRenderer::ClearCursor() {
    unsigned xOff = CursorPosition.x;
    unsigned yOff = CursorPosition.y;
    unsigned* pixPtr = (unsigned *)_targetFrameBuffer->baseAddress;
    for(unsigned long y = yOff; y < yOff + 16; y++) {
        for(unsigned long x = xOff; x < xOff + 8; x++) {
            unsigned* curColor = (unsigned*)(pixPtr + x + (y * _targetFrameBuffer->pixelsPerScanline));
            if(*curColor == CURSOR_COLOR) {
                *curColor = _background;
            }
        }
    }
}

void BasicRenderer::PutChar(char chr) {
    ClearCurrent();

    PutChar(chr, CursorPosition.x, CursorPosition.y);
    CursorPosition.x += 8;
    if(CursorPosition.x + 8 > _targetFrameBuffer->width) {
        Next();
    }
}

void BasicRenderer::PutChar(char chr, unsigned xOff, unsigned yOff) {
    unsigned* pixPtr = (unsigned *)_targetFrameBuffer->baseAddress;
    char* fontPtr = (char *)_psf1Font->glyphBuffer + (chr * _psf1Font->psf1_header->charsize);
    for(unsigned long y = yOff; y < yOff + 16; y++) {
        for(unsigned long x = xOff; x < xOff + 8; x++) {
            if ((*fontPtr & (0b10000000 >> (x - xOff))) > 0) {
                *(unsigned*)(pixPtr + x + (y * _targetFrameBuffer->pixelsPerScanline)) = _color;
            }
        }

        fontPtr++;
    }
}

void BasicRenderer::Next() {
    ClearCursor();

    CursorPosition.x = 0;
    CursorPosition.y += 16;
}

void BasicRenderer::Clear() {
    uint64_t fbBase = (uint64_t)_targetFrameBuffer->baseAddress;
    uint64_t bytesPerScanline = _targetFrameBuffer->pixelsPerScanline * 4;
    uint64_t fbHeight = _targetFrameBuffer->height;
    uint64_t fbSize = _targetFrameBuffer->bufferSize;

    for(int verticalScanLine = 0; verticalScanLine < fbHeight; verticalScanLine++) {
        uint64_t pixPtrBase = fbBase + (bytesPerScanline * verticalScanLine);
        for(uint32_t* pixPtr = (uint32_t *)pixPtrBase; pixPtr < (uint32_t *)(pixPtrBase + bytesPerScanline); pixPtr++) {
            *pixPtr = _background;
        }
    }
}

void BasicRenderer::ClearChar() {
    ClearCurrent();

    CursorPosition.x -= 8;
    if(CursorPosition.x < 0) {
        CursorPosition.x = _targetFrameBuffer->width - 8;
        CursorPosition.y -= 16;
        if(CursorPosition.y < 0) {
            CursorPosition.y = 0;
            CursorPosition.x = 0;
        }
    }

    ClearCurrent();
    ShowCursor();
}

void BasicRenderer::Up() {
    if(CursorPosition.y == 0) {
        return;
    }

    ClearCursor();
    CursorPosition.y -= 16;
    ShowCursor();
}

void BasicRenderer::Down() {
    if(CursorPosition.y == _targetFrameBuffer->height - 16) {
        return;
    }

    ClearCursor();
    CursorPosition.y += 16;
    ShowCursor();
}

void BasicRenderer::Left() {
    if(CursorPosition.x == 0) {
        return;
    }

    ClearCursor();
    CursorPosition.x -= 8;
    ShowCursor();
}

void BasicRenderer::Right() {
    if(CursorPosition.x == _targetFrameBuffer->width - 8) {
        return;
    }

    ClearCursor();
    CursorPosition.x += 8;
    ShowCursor();
}