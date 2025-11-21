/**
 * AuroraOS Kernel - TSS Implementation
 *
 * Task State Segment for privilege level switching
 */

#include "tss.h"
#include "console.h"
#include "gdt.h"
#include "types.h"

// Global TSS (one per CPU, we only have one CPU for now)
static tss_t kernel_tss;

/**
 * Load TSS into Task Register
 */
static inline void tss_load(uint16_t selector) {
    __asm__ __volatile__("ltr %0" : : "r"(selector));
}

/**
 * Set TSS descriptor in GDT
 *
 * TSS descriptor is 16 bytes in 64-bit mode (double-size descriptor)
 * Format:
 *   Bytes 0-7:  Standard descriptor (limit, base low, flags)
 *   Bytes 8-15: Extended descriptor (base high)
 */
static void tss_set_gdt_entry(uint32_t num, uint64_t base, uint32_t limit) {
    // TSS descriptor is at GDT entry 'num'
    // We need to manually set it because it's a system descriptor (16 bytes)

    extern gdt_entry_t gdt[];  // Defined in gdt.c

    // First 8 bytes (standard descriptor format)
    gdt[num].limit_low = limit & 0xFFFF;
    gdt[num].base_low = base & 0xFFFF;
    gdt[num].base_mid = (base >> 16) & 0xFF;

    // Access byte: Present, DPL=0, Type=0x9 (Available 64-bit TSS)
    gdt[num].access = 0x89;

    // Granularity: limit high bits + flags
    gdt[num].granularity = ((limit >> 16) & 0x0F) | 0x00;  // No granularity flag

    gdt[num].base_high = (base >> 24) & 0xFF;

    // Second 8 bytes (upper 32 bits of base address)
    // We need to access the next GDT entry as raw bytes
    uint64_t *gdt_extended = (uint64_t*)&gdt[num + 1];
    *gdt_extended = base >> 32;  // Upper 32 bits of base
}

/**
 * Initialize TSS
 */
void tss_init(void) {
    console_print("[TSS] Initializing Task State Segment...\n");

    // Clear TSS structure
    for (uint32_t i = 0; i < sizeof(tss_t); i++) {
        ((uint8_t*)&kernel_tss)[i] = 0;
    }

    // Set I/O map base to end of TSS (no I/O bitmap)
    kernel_tss.iomap_base = sizeof(tss_t);

    // RSP0 will be set when creating threads
    // For now, set it to NULL (will be updated per-thread)
    kernel_tss.rsp0 = 0;

    // IST entries are NULL for now (no special interrupt stacks)
    kernel_tss.ist1 = 0;
    kernel_tss.ist2 = 0;
    kernel_tss.ist3 = 0;
    kernel_tss.ist4 = 0;
    kernel_tss.ist5 = 0;
    kernel_tss.ist6 = 0;
    kernel_tss.ist7 = 0;

    // Add TSS descriptor to GDT
    // TSS takes 2 GDT entries (16 bytes) in 64-bit mode
    // We'll use entries 6 and 7 (0x30 and 0x38)
    uint64_t tss_base = (uint64_t)&kernel_tss;
    uint32_t tss_limit = sizeof(tss_t) - 1;

    tss_set_gdt_entry(6, tss_base, tss_limit);

    // Load TSS into Task Register
    // Selector = 6 * 8 = 0x30
    tss_load(0x30);

    console_print("[TSS] Initialized and loaded\n");
    console_print("[TSS]   Base: ");
    console_print_hex(tss_base);
    console_print("\n[TSS]   Limit: ");
    console_print_hex(tss_limit);
    console_print("\n[TSS]   Selector: 0x30\n");
}

/**
 * Set kernel stack for current thread
 *
 * This should be called before switching to user mode or when
 * changing threads, to update the RSP0 value.
 */
void tss_set_kernel_stack(uint64_t stack) {
    kernel_tss.rsp0 = stack;
}

/**
 * Get TSS pointer (for debugging)
 */
tss_t* tss_get(void) {
    return &kernel_tss;
}
