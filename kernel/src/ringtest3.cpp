#include "ring3test.h"
#include "stdio.h"
#include "graphics/BasicRenderer.h"

extern "C" int main(int argc, const char** argv) {
    // Uncomment to cause a page fault
    //GlobalRenderer->Printf("Fail!");

    printf("Hello, World");
    while(true) {}

    return 0;
}