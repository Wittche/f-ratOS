/**
 * AuroraOS Kernel - Main Entry Point
 *
 * This is the first C code executed after the bootloader
 * transfers control to the kernel.
 */

#include "types.h"
#include "boot.h"
#include "console.h"

// Forward declarations
static void print_memory_map(boot_info_t *info);

/**
 * Kernel Main Entry Point
 * Called from entry.asm after bootloader
 *
 * @param boot_info Pointer to boot information structure
 */
void kernel_main(boot_info_t *boot_info) {
    // Initialize early console (VGA text mode fallback)
    // TODO: Use framebuffer if available
    console_init(NULL, 80, 25, 0);
    console_clear();

    // Print banner
    console_print("=====================================\n");
    console_print("      AuroraOS Kernel v0.1\n");
    console_print("  Hybrid Kernel - XNU Inspired\n");
    console_print("=====================================\n\n");

    // Validate boot info
    console_print("[BOOT] Validating boot information...\n");
    if (!boot_info) {
        console_print("[ERROR] Boot info is NULL!\n");
        goto halt;
    }

    if (boot_info->magic != AURORA_BOOT_MAGIC) {
        console_print("[ERROR] Invalid boot magic: ");
        console_print_hex(boot_info->magic);
        console_print("\n");
        goto halt;
    }

    console_print("[OK] Boot info validated\n");

    // Print boot information
    console_print("\n[BOOT] Boot Information:\n");
    console_print("  Kernel Physical Base: ");
    console_print_hex(boot_info->kernel_physical_base);
    console_print("\n  Kernel Virtual Base:  ");
    console_print_hex(boot_info->kernel_virtual_base);
    console_print("\n  Kernel Size:          ");
    console_print_hex(boot_info->kernel_size);
    console_print(" bytes\n");

    // Print memory map
    console_print("\n[BOOT] Memory Map:\n");
    print_memory_map(boot_info);

    // Print graphics info if available
    if (boot_info->graphics_info) {
        graphics_info_t *gfx = boot_info->graphics_info;
        console_print("\n[BOOT] Graphics Information:\n");
        console_print("  Framebuffer: ");
        console_print_hex(gfx->framebuffer_base);
        console_print("\n  Resolution:  ");
        console_print_dec(gfx->horizontal_resolution);
        console_print("x");
        console_print_dec(gfx->vertical_resolution);
        console_print("\n");
    }

    // Initialize kernel subsystems
    console_print("\n[KERNEL] Initializing subsystems...\n");

    // TODO: Initialize GDT
    console_print("  [ ] GDT (Global Descriptor Table)\n");

    // TODO: Initialize IDT
    console_print("  [ ] IDT (Interrupt Descriptor Table)\n");

    // TODO: Initialize physical memory manager
    console_print("  [ ] PMM (Physical Memory Manager)\n");

    // TODO: Initialize virtual memory/paging
    console_print("  [ ] VMM (Virtual Memory Manager)\n");

    // TODO: Initialize heap
    console_print("  [ ] Kernel Heap\n");

    // TODO: Initialize Mach layer
    console_print("  [ ] Mach Microkernel Layer\n");

    // TODO: Initialize BSD layer
    console_print("  [ ] BSD Layer\n");

    // TODO: Initialize scheduler
    console_print("  [ ] Scheduler\n");

    console_print("\n[KERNEL] Initialization incomplete - halting\n");
    console_print("(This is expected for initial stub)\n");

halt:
    console_print("\n[HALT] System halted\n");
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

/**
 * Print memory map entries
 */
static void print_memory_map(boot_info_t *info) {
    if (!info->memory_map || info->memory_map_size == 0) {
        console_print("  No memory map available\n");
        return;
    }

    uint64_t num_entries = info->memory_map_size / info->memory_map_descriptor_size;
    console_print("  Entries: ");
    console_print_dec(num_entries);
    console_print("\n");

    // Print first few entries
    memory_descriptor_t *desc = info->memory_map;
    uint64_t count = 0;

    for (uint64_t i = 0; i < num_entries && count < 10; i++) {
        // Get pointer to descriptor (accounting for variable size)
        memory_descriptor_t *entry = (memory_descriptor_t*)((uint8_t*)desc + (i * info->memory_map_descriptor_size));

        console_print("    ");
        console_print_hex(entry->physical_start);
        console_print(" - ");
        console_print_hex(entry->physical_start + (entry->number_of_pages * 4096));
        console_print(" Type=");
        console_print_dec(entry->type);
        console_print("\n");

        count++;
    }

    if (num_entries > 10) {
        console_print("  ... (");
        console_print_dec(num_entries - 10);
        console_print(" more entries)\n");
    }
}
