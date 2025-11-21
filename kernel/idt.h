/**
 * AuroraOS Kernel - Interrupt Descriptor Table (IDT)
 *
 * Defines structures and functions for managing x86_64 interrupts
 */

#ifndef _KERNEL_IDT_H_
#define _KERNEL_IDT_H_

#include "types.h"

// IDT entry structure (16 bytes on x86_64)
typedef struct {
    uint16_t offset_low;    // Offset bits 0-15
    uint16_t selector;      // Code segment selector
    uint8_t  ist;           // Interrupt Stack Table (bits 0-2), rest reserved
    uint8_t  type_attr;     // Type and attributes
    uint16_t offset_mid;    // Offset bits 16-31
    uint32_t offset_high;   // Offset bits 32-63
    uint32_t reserved;      // Reserved (must be zero)
} __attribute__((packed)) idt_entry_t;

// IDT pointer structure (used by LIDT instruction)
typedef struct {
    uint16_t limit;         // Size of IDT - 1
    uint64_t base;          // Base address of IDT
} __attribute__((packed)) idt_ptr_t;

// Number of IDT entries (256 for x86_64)
#define IDT_ENTRIES 256

// IDT entry type attributes
#define IDT_TYPE_INTERRUPT_GATE 0x8E  // Present, DPL=0, Type=Interrupt Gate
#define IDT_TYPE_TRAP_GATE      0x8F  // Present, DPL=0, Type=Trap Gate
#define IDT_TYPE_TASK_GATE      0x85  // Present, DPL=0, Type=Task Gate

// CPU exception numbers
#define EXCEPTION_DIVIDE_BY_ZERO        0
#define EXCEPTION_DEBUG                 1
#define EXCEPTION_NMI                   2
#define EXCEPTION_BREAKPOINT            3
#define EXCEPTION_OVERFLOW              4
#define EXCEPTION_BOUND_RANGE_EXCEEDED  5
#define EXCEPTION_INVALID_OPCODE        6
#define EXCEPTION_DEVICE_NOT_AVAILABLE  7
#define EXCEPTION_DOUBLE_FAULT          8
#define EXCEPTION_COPROCESSOR_SEGMENT   9
#define EXCEPTION_INVALID_TSS           10
#define EXCEPTION_SEGMENT_NOT_PRESENT   11
#define EXCEPTION_STACK_SEGMENT_FAULT   12
#define EXCEPTION_GENERAL_PROTECTION    13
#define EXCEPTION_PAGE_FAULT            14
// 15 reserved
#define EXCEPTION_X87_FLOATING_POINT    16
#define EXCEPTION_ALIGNMENT_CHECK       17
#define EXCEPTION_MACHINE_CHECK         18
#define EXCEPTION_SIMD_FLOATING_POINT   19
#define EXCEPTION_VIRTUALIZATION        20
// 21-29 reserved
#define EXCEPTION_SECURITY              30
// 31 reserved

// Hardware IRQ numbers (remapped to 32-47)
#define IRQ_BASE      32
#define IRQ_TIMER     (IRQ_BASE + 0)
#define IRQ_KEYBOARD  (IRQ_BASE + 1)
#define IRQ_CASCADE   (IRQ_BASE + 2)
#define IRQ_COM2      (IRQ_BASE + 3)
#define IRQ_COM1      (IRQ_BASE + 4)
#define IRQ_LPT2      (IRQ_BASE + 5)
#define IRQ_FLOPPY    (IRQ_BASE + 6)
#define IRQ_LPT1      (IRQ_BASE + 7)
#define IRQ_RTC       (IRQ_BASE + 8)
#define IRQ_AVAILABLE1 (IRQ_BASE + 9)
#define IRQ_AVAILABLE2 (IRQ_BASE + 10)
#define IRQ_AVAILABLE3 (IRQ_BASE + 11)
#define IRQ_MOUSE     (IRQ_BASE + 12)
#define IRQ_FPU       (IRQ_BASE + 13)
#define IRQ_PRIMARY_ATA (IRQ_BASE + 14)
#define IRQ_SECONDARY_ATA (IRQ_BASE + 15)

// Interrupt stack frame (pushed by CPU and our stub)
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) interrupt_frame_t;

// Function prototypes
void idt_init(void);
void idt_set_gate(uint8_t num, uint64_t handler, uint16_t selector, uint8_t type_attr);
void idt_load(void);

// Exception handler (implemented in idt.c)
void exception_handler(interrupt_frame_t *frame);

// IRQ handler (implemented in idt.c)
void irq_handler(interrupt_frame_t *frame);

// Assembly interrupt stubs (defined in idt_asm.S)
// CPU exceptions (0-31)
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

// Hardware IRQs (32-47)
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

#endif // _KERNEL_IDT_H_
