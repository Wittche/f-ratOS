/**
 * AuroraOS Kernel - TSS Implementation
 *
 * Task State Segment for privilege level switching
 */

#include "tss.h"
#include "console.h"
#include "gdt.h"
#include "types.h"
#include "io.h"

// Global TSS (one per CPU, we only have one CPU for now)
// Initialize to zero at compile time to avoid runtime loop
static tss_t kernel_tss = {0};

// Serial debug helper (COM1 = 0x3F8)
static inline void serial_debug_char(char c) {
    outb(0x3F8, c);
}

static inline void serial_debug_str(const char *s) {
    while (*s) {
        serial_debug_char(*s++);
    }
}

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
    serial_debug_str("set_gdt_entry_start\n");
    // TSS descriptor is at GDT entry 'num'
    // We need to manually set it because it's a system descriptor (16 bytes)

    extern gdt_entry_t gdt[];  // Defined in gdt.c

    serial_debug_str("set_limit_low\n");
    // First 8 bytes (standard descriptor format)
    gdt[num].limit_low = limit & 0xFFFF;
    serial_debug_str("set_base_low\n");
    gdt[num].base_low = base & 0xFFFF;
    serial_debug_str("set_base_mid\n");
    gdt[num].base_mid = (base >> 16) & 0xFF;

    serial_debug_str("set_access\n");
    // Access byte: Present, DPL=0, Type=0x9 (Available 64-bit TSS)
    gdt[num].access = 0x89;

    serial_debug_str("set_granularity\n");
    // Granularity: limit high bits + flags
    gdt[num].granularity = ((limit >> 16) & 0x0F) | 0x00;  // No granularity flag

    serial_debug_str("set_base_high\n");
    gdt[num].base_high = (base >> 24) & 0xFF;

    serial_debug_str("set_extended\n");
    // Second 8 bytes (upper 32 bits of base address)
    // We need to access the next GDT entry as raw bytes
    uint64_t *gdt_extended = (uint64_t*)&gdt[num + 1];
    serial_debug_str("write_extended\n");
    *gdt_extended = base >> 32;  // Upper 32 bits of base
    serial_debug_str("set_gdt_entry_done\n");
}

/**
 * Initialize TSS
 */
void tss_init(void) {
    serial_debug_str("tss_init_start\n");
    console_print("[TSS] Initializing Task State Segment...\n");
    serial_debug_str("after_tss_console_print\n");

    // TSS structure already initialized to zero at compile time
    // No need for runtime clearing loop

    // Set I/O map base to end of TSS (no I/O bitmap)
    serial_debug_str("setting_iomap\n");
    kernel_tss.iomap_base = sizeof(tss_t);
    serial_debug_str("after_iomap\n");

    // RSP0 and IST entries are already zero (compile-time initialization)
    // RSP0 will be set when creating threads
    // IST entries will remain NULL for now (no special interrupt stacks)
    serial_debug_str("tss_fields_ok\n");

    // Add TSS descriptor to GDT
    // TSS takes 2 GDT entries (16 bytes) in 64-bit mode
    // We'll use entries 6 and 7 (0x30 and 0x38)
    serial_debug_str("calc_tss_base\n");
    uint64_t tss_base = (uint64_t)&kernel_tss;
    uint32_t tss_limit = sizeof(tss_t) - 1;
    serial_debug_str("before_set_gdt_entry\n");

    tss_set_gdt_entry(6, tss_base, tss_limit);
    serial_debug_str("after_set_gdt_entry\n");

    // Load TSS into Task Register
    // Selector = 6 * 8 = 0x30
    serial_debug_str("before_tss_load\n");
    tss_load(0x30);
    serial_debug_str("after_tss_load\n");

    console_print("[TSS] Initialized and loaded\n");
    console_print("[TSS]   Base: ");
    console_print_hex(tss_base);
    console_print("\n[TSS]   Limit: ");
    console_print_hex(tss_limit);
    console_print("\n[TSS]   Selector: 0x30\n");
    serial_debug_str("tss_init_complete\n");
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
