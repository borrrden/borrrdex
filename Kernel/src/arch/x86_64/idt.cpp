#include <idt.h>
#include <gdt.h>
#include <io.h>
#include <debug.h>
#include <logging.h>
#include <stacktrace.h>
#include <panic.h>
#include <apic.h>
#include <kstring.h>

idt_desc_t s_idt[256];
idt_t idt_ptr;

struct isr_data_pair {
    isr_t handler;
    void* data;
};

isr_data_pair interrupt_handlers[256];

// From PIC0
constexpr uint8_t PIC_IRQ_TIMER         = 0x00;
constexpr uint8_t PIC_IRQ_KEYBOARD      = 0x01;
constexpr uint8_t PIC_IRQ_SERIAL2       = 0x03;
constexpr uint8_t PIC_IRQ_SERIAL1       = 0x04;
constexpr uint8_t PIC_IRQ_PARALLEL2     = 0x05;
constexpr uint8_t PIC_IRQ_FLOPPYDISK    = 0x06;
constexpr uint8_t PIC_IRQ_PARALLEL1     = 0x07;

// From PIC1
constexpr uint8_t PIC_IRQ_CMOSTIMER     = 0x08;
constexpr uint8_t PIC_IRQ_CGARETRACE    = 0x09;
constexpr uint8_t PIC_IRQ_PS2MOUSE      = 0x0C;
constexpr uint8_t PIC_IRQ_PRIMARYHD     = 0x0D;
constexpr uint8_t PIC_IRQ_SECONDARYHD   = 0x0E;

constexpr uint8_t PIC_OCW2_MASK_L1      = 0x01;
constexpr uint8_t PIC_OCW2_MASK_L2      = 0x02;
constexpr uint8_t PIC_OCW2_MASK_L3      = 0x04;
constexpr uint8_t PIC_OCW2_MASK_EOI     = 0x20;
constexpr uint8_t PIC_OCW2_MASK_SL      = 0x40;
constexpr uint8_t PIC_OCW2_MASK_ROTATE  = 0x80;

constexpr uint8_t PIC_OCW3_MASK_RIS     = 0x01;
constexpr uint8_t PIC_OCW3_MASK_RIR     = 0x02;
constexpr uint8_t PIC_OCW3_MASK_MODE    = 0x04;
constexpr uint8_t PIC_OCW3_MASK_SMM     = 0x20;
constexpr uint8_t PIC_OCW3_MASK_ESMM    = 0x40;
constexpr uint8_t PIC_OCW3_MASK_D7      = 0x80;

constexpr uint8_t PIC0_COMMAND_REGISTER     = 0x20;
constexpr uint8_t PIC0_INT_MASK_REGISTER    = 0x21;

constexpr uint8_t PIC1_COMMAND_REGISTER     = 0xA0;
constexpr uint8_t PIC1_INT_MASK_REGISTER    = 0xA1;

// Control Word 1
constexpr uint8_t PIC_ICW1_MASK_IC4     = 0x01;
constexpr uint8_t PIC_ICW1_MASK_SNGL    = 0x02;
constexpr uint8_t PIC_ICW1_MASK_ADI     = 0x04;
constexpr uint8_t PIC_ICW1_MASK_LTIM    = 0x08;
constexpr uint8_t PIC_ICW1_MASK_INIT    = 0x10;

// Control Word 4
constexpr uint8_t PIC_ICW4_MASK_UPM     = 0x01;
constexpr uint8_t PIC_ICW4_MASK_AEOI    = 0x02;
constexpr uint8_t PIC_ICW4_MASK_MS      = 0x04;
constexpr uint8_t PIC_ICW4_MASK_BUF     = 0x08;
constexpr uint8_t PIC_ICW4_MASK_SFNM    = 0x10;

extern "C" {
    void isr0();
    void isr1();
    void isr2();
    void isr3();
    void isr4();
    void isr5();
    void isr6();
    void isr7();
    void isr8();
    void isr9();
    void isr10();
    void isr11();
    void isr12();
    void isr13();
    void isr14();
    void isr15();
    void isr16();
    void isr17();
    void isr18();
    void isr19();
    void isr20();
    void isr21();
    void isr22();
    void isr23();
    void isr24();
    void isr25();
    void isr26();
    void isr27();
    void isr28();
    void isr29();
    void isr30();
    void isr31();

    void irq0();
    void irq1();
    void irq2();
    void irq3();
    void irq4();
    void irq5();
    void irq6();
    void irq7();
    void irq8();
    void irq9();
    void irq10();
    void irq11();
    void irq12();
    void irq13();
    void irq14();
    void irq15();

    void idt_flush();
}

extern uint64_t int_vectors[];

static void set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags, uint8_t ist = 0) {
    s_idt[num].base_high = (base >> 32);
    s_idt[num].base_mid = (base >> 16) & 0xFFFF;
    s_idt[num].base_low = base & 0xFFFF;

    s_idt[num].selector = sel;
    s_idt[num].zero = 0;
    s_idt[num].ist = ist & 0x7;

    s_idt[num].flags = flags;
}

static void pic_init() {
    uint8_t icw = PIC_ICW1_MASK_INIT | PIC_ICW1_MASK_IC4;

    // Start the process of initializing, now the PICs will expect
    // the sequence of words that follows
    port_write_8(PIC0_COMMAND_REGISTER, icw);
    port_write_8(PIC1_COMMAND_REGISTER, icw);

    // Remap them so that the IRQ do not overlap with hardwired
    // hardware exception interrupts.  After this IRQs will be
    // offset by 32 (0 - 15 -> 32 - 47)
    port_write_8(PIC0_INT_MASK_REGISTER, 0x20);
    port_write_8(PIC1_INT_MASK_REGISTER, 0x28);

    // Next control word, sets PIC1 as a slave of PIC0
    // After this all of PIC1 IRQ will flow through
    // PIC0 IRQ 2
    port_write_8(PIC0_INT_MASK_REGISTER, 0x04);
    port_write_8(PIC1_INT_MASK_REGISTER, 0x02);

    // Final control word (4), enable i86 mode
    port_write_8(PIC0_INT_MASK_REGISTER, PIC_ICW4_MASK_UPM);
    port_write_8(PIC1_INT_MASK_REGISTER, PIC_ICW4_MASK_UPM);

    // Unmask everything
    port_write_8(PIC0_INT_MASK_REGISTER, 0x0);
    port_write_8(PIC1_INT_MASK_REGISTER, 0x0);
}

int last_err_code = 0;

void idt::initialize() {
    idt_ptr.limit = sizeof(idt_desc_t) * 256 - 1;
    idt_ptr.base = (uint64_t)&s_idt;

    for(int i = 0; i < 48; i++) {
        set_gate(i, 0, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    }

    for(unsigned i = 48; i < 256; i++) {
        set_gate(i, int_vectors[i - 48], GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    }

    set_gate(0, (uint64_t)isr0, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(1, (uint64_t)isr1, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(2, (uint64_t)isr2, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(3, (uint64_t)isr3, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(4, (uint64_t)isr4, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(5, (uint64_t)isr5, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(6, (uint64_t)isr6, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(7, (uint64_t)isr7, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(8, (uint64_t)isr8, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(9, (uint64_t)isr9, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(10, (uint64_t)isr10, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(11, (uint64_t)isr11, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(12, (uint64_t)isr12, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(13, (uint64_t)isr13, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(14, (uint64_t)isr14, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(15, (uint64_t)isr15, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(16, (uint64_t)isr16, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(17, (uint64_t)isr17, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(18, (uint64_t)isr18, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(19, (uint64_t)isr19, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(20, (uint64_t)isr20, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(21, (uint64_t)isr21, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(22, (uint64_t)isr22, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(23, (uint64_t)isr23, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(24, (uint64_t)isr24, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(25, (uint64_t)isr25, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(26, (uint64_t)isr26, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(27, (uint64_t)isr27, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(28, (uint64_t)isr28, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(29, (uint64_t)isr29, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(30, (uint64_t)isr30, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(31, (uint64_t)isr31, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);

    idt_flush();
    pic_init();

    set_gate(32, (uint64_t)irq0, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(33, (uint64_t)irq1, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(34, (uint64_t)irq2, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(35, (uint64_t)irq3, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(36, (uint64_t)irq4, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(37, (uint64_t)irq5, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(38, (uint64_t)irq6, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(39, (uint64_t)irq7, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(40, (uint64_t)irq8, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(41, (uint64_t)irq9, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(42, (uint64_t)irq10, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(43, (uint64_t)irq11, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(44, (uint64_t)irq12, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(45, (uint64_t)irq13, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(46, (uint64_t)irq14, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
    set_gate(47, (uint64_t)irq15, GDT_SELECTOR_KERNEL_CODE, IDT_DESC_PRESENT | IDT_DESC_INT32);
}

void idt::register_interrupt_handler(uint8_t interrupt, isr_t handler, void* data) {
    interrupt_handlers[interrupt] = { .handler = handler, .data = data };
}

void idt::disable_pic() {
    // Same as init, but remap to 0xF0 - 0xF8 for both, then mask everything

    uint8_t icw = PIC_ICW1_MASK_INIT | PIC_ICW1_MASK_IC4;
    port_write_8(PIC0_COMMAND_REGISTER, icw);
    port_write_8(PIC1_COMMAND_REGISTER, icw);
    port_write_8(PIC0_INT_MASK_REGISTER, 0xF0);
    port_write_8(PIC1_INT_MASK_REGISTER, 0xF0);
    port_write_8(PIC0_INT_MASK_REGISTER, 0x04);
    port_write_8(PIC1_INT_MASK_REGISTER, 0x02);
    port_write_8(PIC0_INT_MASK_REGISTER, PIC_ICW4_MASK_UPM);
    port_write_8(PIC1_INT_MASK_REGISTER, PIC_ICW4_MASK_UPM);
    port_write_8(PIC0_INT_MASK_REGISTER, 0xFF);
    port_write_8(PIC1_INT_MASK_REGISTER, 0xFF);

    // Needed on some systems, so do it just in case
    port_write_8(io::CHIPSET_ADDRESS_REGISTER, io::IMCR_REGISTER_ADDRESS);
    port_write_8(io::CHIPSET_DATA_REGISTER, io::IMCR_VIA_APIC);
}

int idt::get_err_code() {
    return last_err_code;
}

extern "C" {
    void isr_handler(int int_num, register_context* regs, int err_code) {
        last_err_code = err_code;
        if(__builtin_expect(interrupt_handlers[int_num].handler != 0, true)) {
            isr_data_pair pair = interrupt_handlers[int_num];
            pair.handler(pair.data, regs);
        } else if(!(regs->ss & 0x3)) {
            // This happened while in the kernel
            log::error("Fatal Kernel Exception: %d", int_num);
            log::info("RIP: 0x%x", regs->rip);
            log::info("Error Code: %d", err_code);
            log::info("Register Dump: a: 0x%x, b: 0x%x, c: 0x%x, d: 0x%x", regs->rax, regs->rbx, regs->rcx, regs->rdx);
            log::info("               S: 0x%x, D: 0x%x, sp: 0x%x, bp: 0x%x", regs->rsi, regs->rdi, regs->rsp, regs->rbp);
            log::info("Stack Trace:");
            print_stack_trace(regs->rbp);

            char temp[19] = { '0', 'x' };
            char temp2[19] = { '0', 'x' };
            const char* reasons[]{"Generic Exception", "RIP: ", itoa(regs->rip, temp + 2, 16), "Exception: ", itoa(int_num, temp2 + 2, 16)};
            kernel_panic(reasons, 5);
            __builtin_unreachable();
        } else {
            // TODO: User process crash
        }
    }

    void irq_handler(int int_num, register_context* regs) {
        lapic_eoi();

        if(__builtin_expect(interrupt_handlers[int_num].handler != 0, true)) {
            isr_data_pair pair = interrupt_handlers[int_num];
            pair.handler(pair.data, regs);
        } else {
            log::warning("Unhandled IRQ: %d", int_num);
        }
    }

    void ipi_handler(int int_num, register_context* regs) {
        lapic_eoi();

        if(__builtin_expect(interrupt_handlers[int_num].handler != 0, true)) {
            isr_data_pair pair = interrupt_handlers[int_num];
            pair.handler(pair.data, regs);
        } else {
            log::warning("Unhandled IPI: %d", int_num);
        }
    }
}