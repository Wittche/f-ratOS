/**
 * AuroraOS Kernel - Task State Segment (TSS)
 *
 * TSS for x86_64 privilege level switching and kernel stack management
 */

#ifndef _KERNEL_TSS_H_
#define _KERNEL_TSS_H_

#include "types.h"

/**
 * TSS structure for x86_64
 *
 * In 64-bit mode, TSS is much simpler than 32-bit:
 * - No hardware task switching
 * - Used only for kernel stack pointer (RSP0) and IST
 * - Required for privilege level changes (Ring 3 -> Ring 0)
 */
typedef struct {
    uint32_t reserved0;

    // Privilege level 0-2 stack pointers
    uint64_t rsp0;       // Kernel stack pointer (used when switching from Ring 3)
    uint64_t rsp1;       // Not used in most systems
    uint64_t rsp2;       // Not used in most systems

    uint64_t reserved1;

    // Interrupt Stack Table (IST)
    // These are alternative stacks for specific interrupts
    uint64_t ist1;       // IST stack 1
    uint64_t ist2;       // IST stack 2
    uint64_t ist3;       // IST stack 3
    uint64_t ist4;       // IST stack 4
    uint64_t ist5;       // IST stack 5
    uint64_t ist6;       // IST stack 6
    uint64_t ist7;       // IST stack 7

    uint64_t reserved2;
    uint16_t reserved3;

    // I/O Map Base Address (offset from TSS base)
    uint16_t iomap_base;
} __attribute__((packed)) tss_t;

// Initialize TSS
void tss_init(void);

// Set kernel stack for current CPU
void tss_set_kernel_stack(uint64_t stack);

// Get TSS pointer (for debugging)
tss_t* tss_get(void);

#endif // _KERNEL_TSS_H_
