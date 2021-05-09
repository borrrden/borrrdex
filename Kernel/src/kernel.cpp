

extern "C" [[noreturn]] void kmain() {
    while(true) {
        asm("hlt");
    }
}