/**
 * AuroraOS Kernel - IDT Implementation
 *
 * Manages interrupt descriptor table and interrupt handling
 */

#include "idt.h"
#include "console.h"
#include "io.h"

// IDT entries and pointer
static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

// Exception names for debugging
static const char *exception_names[] = {
    "Divide By Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Security Exception",
    "Reserved"
};

/**
 * Set an IDT gate
 */
void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t type_attr) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_mid = (handler >> 16) & 0xFFFF;
    idt[num].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[num].selector = selector;
    idt[num].ist = 0;  // Not using IST for now
    idt[num].type_attr = type_attr;
    idt[num].reserved = 0;
}

/**
 * Remap PIC (Programmable Interrupt Controller)
 *
 * By default, IRQs 0-7 are mapped to exceptions 8-15, which conflicts
 * with CPU exceptions. We remap them to 32-47.
 */
static void pic_remap(void) {
    // Save masks
    uint8_t mask1 = inb(0x21);
    uint8_t mask2 = inb(0xA1);

    // Start initialization sequence (ICW1)
    outb(0x20, 0x11);  // Master PIC
    outb(0xA0, 0x11);  // Slave PIC

    // Set vector offsets (ICW2)
    outb(0x21, IRQ_BASE);      // Master PIC: IRQ 0-7 → 32-39
    outb(0xA1, IRQ_BASE + 8);  // Slave PIC: IRQ 8-15 → 40-47

    // Tell Master PIC there's a slave at IRQ2 (ICW3)
    outb(0x21, 0x04);
    // Tell Slave PIC its cascade identity (ICW3)
    outb(0xA1, 0x02);

    // Set 8086 mode (ICW4)
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // Restore masks
    outb(0x21, mask1);
    outb(0xA1, mask2);
}

/**
 * Initialize IDT
 */
void idt_init(void) {
    console_print("[IDT] Initializing Interrupt Descriptor Table...\n");

    // Clear IDT
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // Remap PIC before setting up IRQ handlers
    pic_remap();

    // Install exception handlers (0-31)
    idt_set_gate(0, (uint64_t)isr0, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(1, (uint64_t)isr1, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(2, (uint64_t)isr2, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(3, (uint64_t)isr3, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(4, (uint64_t)isr4, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(5, (uint64_t)isr5, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(6, (uint64_t)isr6, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(7, (uint64_t)isr7, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(8, (uint64_t)isr8, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(9, (uint64_t)isr9, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(10, (uint64_t)isr10, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(11, (uint64_t)isr11, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(12, (uint64_t)isr12, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(13, (uint64_t)isr13, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(14, (uint64_t)isr14, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(15, (uint64_t)isr15, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(16, (uint64_t)isr16, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(17, (uint64_t)isr17, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(18, (uint64_t)isr18, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(19, (uint64_t)isr19, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(20, (uint64_t)isr20, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(21, (uint64_t)isr21, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(22, (uint64_t)isr22, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(23, (uint64_t)isr23, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(24, (uint64_t)isr24, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(25, (uint64_t)isr25, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(26, (uint64_t)isr26, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(27, (uint64_t)isr27, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(28, (uint64_t)isr28, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(29, (uint64_t)isr29, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(30, (uint64_t)isr30, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(31, (uint64_t)isr31, 0x08, IDT_TYPE_INTERRUPT_GATE);

    // Install IRQ handlers (32-47)
    idt_set_gate(32, (uint64_t)irq0, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(33, (uint64_t)irq1, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(34, (uint64_t)irq2, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(35, (uint64_t)irq3, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(36, (uint64_t)irq4, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(37, (uint64_t)irq5, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(38, (uint64_t)irq6, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(39, (uint64_t)irq7, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(40, (uint64_t)irq8, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(41, (uint64_t)irq9, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(42, (uint64_t)irq10, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(43, (uint64_t)irq11, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(44, (uint64_t)irq12, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(45, (uint64_t)irq13, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(46, (uint64_t)irq14, 0x08, IDT_TYPE_INTERRUPT_GATE);
    idt_set_gate(47, (uint64_t)irq15, 0x08, IDT_TYPE_INTERRUPT_GATE);

    // Load IDT
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint64_t)&idt;
    idt_load();

    console_print("[IDT] Loaded with 256 entries\n");
    console_print("[IDT] Exceptions: 0-31, IRQs: 32-47\n");
}

/**
 * Load IDT (called from C, implemented in idt_asm.S)
 */
extern void idt_load_asm(idt_ptr_t *ptr);

void idt_load(void) {
    idt_load_asm(&idt_ptr);
}

/**
 * CPU Exception Handler
 */
void exception_handler(interrupt_frame_t *frame) {
    console_print("\n========================================\n");
    console_print("[EXCEPTION] CPU Exception Occurred!\n");
    console_print("========================================\n");

    // Print exception name
    if (frame->int_no < 32) {
        console_print("Exception: ");
        console_print(exception_names[frame->int_no]);
        console_print("\n");
    } else {
        console_print("Unknown exception: ");
        console_print_hex(frame->int_no);
        console_print("\n");
    }

    // Print error code if present
    console_print("Error Code: ");
    console_print_hex(frame->error_code);
    console_print("\n");

    // Print register dump
    console_print("\nRegisters:\n");
    console_print("  RIP="); console_print_hex(frame->rip);
    console_print(" RSP="); console_print_hex(frame->rsp);
    console_print("\n");
    console_print("  RAX="); console_print_hex(frame->rax);
    console_print(" RBX="); console_print_hex(frame->rbx);
    console_print("\n");
    console_print("  RCX="); console_print_hex(frame->rcx);
    console_print(" RDX="); console_print_hex(frame->rdx);
    console_print("\n");
    console_print("  RSI="); console_print_hex(frame->rsi);
    console_print(" RDI="); console_print_hex(frame->rdi);
    console_print("\n");
    console_print("  CS="); console_print_hex(frame->cs);
    console_print(" SS="); console_print_hex(frame->ss);
    console_print("\n");
    console_print("  RFLAGS="); console_print_hex(frame->rflags);
    console_print("\n");

    console_print("\n[HALT] System halted due to exception\n");
    console_print("========================================\n");

    // Halt the system
    while (1) {
        __asm__ __volatile__("cli; hlt");
    }
}

/**
 * IRQ Handler
 */
void irq_handler(interrupt_frame_t *frame) {
    // Send EOI (End of Interrupt) to PIC
    if (frame->int_no >= 40) {
        // Slave PIC (IRQ 8-15)
        outb(0xA0, 0x20);
    }
    // Master PIC (IRQ 0-7 or cascaded from slave)
    outb(0x20, 0x20);

    // Handle specific IRQs
    switch (frame->int_no) {
        case IRQ_TIMER:
            // Timer tick - handled silently for now
            break;

        case IRQ_KEYBOARD:
            // Keyboard interrupt
            console_print("[IRQ] Keyboard interrupt received\n");
            // TODO: Read scancode from port 0x60
            break;

        default:
            // Unknown IRQ
            console_print("[IRQ] Unhandled IRQ: ");
            console_print_hex(frame->int_no - IRQ_BASE);
            console_print("\n");
            break;
    }
}
