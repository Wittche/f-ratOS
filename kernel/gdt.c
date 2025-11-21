/**
 * AuroraOS Kernel - GDT Implementation
 *
 * Manages Global Descriptor Table for x86_64 segmentation
 */

#include "gdt.h"
#include "console.h"

// GDT entries and pointer
gdt_entry_t gdt[GDT_ENTRIES];  // Exported for TSS
static gdt_ptr_t gdt_ptr;

/**
 * Set a GDT entry
 *
 * @param num Entry number (0-4)
 * @param base Base address (mostly ignored in 64-bit mode)
 * @param limit Segment limit (mostly ignored in 64-bit mode)
 * @param access Access flags
 * @param gran Granularity flags
 */
void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit,
                  uint8_t access, uint8_t gran) {
    if (num >= GDT_ENTRIES) {
        return;
    }

    // Set base address
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;

    // Set limit
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    // Set flags
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

/**
 * Initialize GDT
 *
 * Sets up a flat memory model with kernel code/data segments
 * In 64-bit long mode, segmentation is mostly disabled, but we still
 * need proper GDT entries for code/data segments.
 */
void gdt_init(void) {
    console_print("[GDT] Initializing Global Descriptor Table...\n");

    // Calculate GDT pointer
    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base = (uint64_t)&gdt;

    // Entry 0: Null descriptor (required by x86 architecture)
    gdt_set_gate(GDT_NULL, 0, 0, 0, 0);

    // Entry 1: Kernel Code Segment (64-bit)
    // Base = 0, Limit = 0xFFFFF (ignored in 64-bit)
    // Access: Present, Ring 0, Code, Executable, Readable
    // Gran: 64-bit long mode, 4KB granularity
    gdt_set_gate(GDT_KERNEL_CODE,
                 0,                    // Base (ignored in 64-bit)
                 0xFFFFF,              // Limit (ignored in 64-bit)
                 GDT_KERNEL_CODE_ACCESS,
                 GDT_GRAN_GRANULARITY | GDT_GRAN_64BIT);

    // Entry 2: Kernel Data Segment (64-bit)
    // Base = 0, Limit = 0xFFFFF (ignored in 64-bit)
    // Access: Present, Ring 0, Data, Writable
    // Gran: 64-bit compatible, 4KB granularity
    gdt_set_gate(GDT_KERNEL_DATA,
                 0,                    // Base (ignored in 64-bit)
                 0xFFFFF,              // Limit (ignored in 64-bit)
                 GDT_KERNEL_DATA_ACCESS,
                 GDT_GRAN_GRANULARITY | GDT_GRAN_32BIT);

    // Entry 3: User Code Segment (32-bit compatibility mode)
    // For 32-bit processes in compatibility mode
    gdt_set_gate(GDT_USER_CODE32,
                 0,
                 0xFFFFF,
                 GDT_USER_CODE_ACCESS,
                 GDT_GRAN_GRANULARITY | GDT_GRAN_32BIT);

    // Entry 4: User Data Segment
    // Used by both 32-bit and 64-bit user processes
    gdt_set_gate(GDT_USER_DATA,
                 0,
                 0xFFFFF,
                 GDT_USER_DATA_ACCESS,
                 GDT_GRAN_GRANULARITY | GDT_GRAN_32BIT);

    // Entry 5: User Code Segment (64-bit long mode)
    // Used by SYSRET for 64-bit processes
    gdt_set_gate(GDT_USER_CODE64,
                 0,
                 0xFFFFF,
                 GDT_USER_CODE_ACCESS,
                 GDT_GRAN_GRANULARITY | GDT_GRAN_64BIT);

    // Load GDT and update segment registers
    gdt_load();

    console_print("[GDT] Loaded with ");
    console_print_dec(GDT_ENTRIES);
    console_print(" entries\n");
    console_print("[GDT] Kernel CS=");
    console_print_hex(KERNEL_CODE_SELECTOR);
    console_print(", DS=");
    console_print_hex(KERNEL_DATA_SELECTOR);
    console_print("\n");
}

/**
 * Load GDT and reload segment registers
 */
void gdt_load(void) {
    // Load GDT using LGDT instruction (implemented in assembly)
    gdt_load_asm(&gdt_ptr);

    // Reload segment registers with new selectors (implemented in assembly)
    gdt_reload_segments();
}
