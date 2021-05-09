#include <kassert.h>
#include <logging.h>
#include <stacktrace.h>
#include <kstring.h>
#include <panic.h>

extern "C" {
    [[noreturn]] void kernel_assertion_failed(const char* msg, const char* file, int line) {
        asm("cli");

        log::error("Kernel Assertion Failed (%s) - %s:%d", msg, file, line);

        uint64_t rbp = 0;
        asm("mov %%rbp, %0" : "=r"(rbp));
        print_stack_trace(rbp);

        char buf[16];
        itoa(line, buf, 10);

        const char* panic[] = { "Kernel Assertion Failed", msg, "File: ", file, "Line: ", buf };
        kernel_panic(panic, 6);
        __builtin_unreachable();
    }
}