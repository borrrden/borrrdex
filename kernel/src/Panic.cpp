#include "Panic.h"
#include "graphics/BasicRenderer.h"

void Panic(const char* panicMessage) {
    GlobalRenderer->SetBackground(0x00ff0000);
    GlobalRenderer->Clear();
    GlobalRenderer->CursorPosition = {0, 0};
    GlobalRenderer->SetColor(0);
    GlobalRenderer->Printf("Kernel Panic");
    GlobalRenderer->Next();
    GlobalRenderer->Next();

    GlobalRenderer->Printf(panicMessage);
}