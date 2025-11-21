/**
 * AuroraOS Kernel - Main Entry Point
 *
 * This is the first C code executed after the bootloader
 * transfers control to the kernel.
 */

#include "types.h"
#include "boot.h"
#include "console.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "timer.h"
#include "keyboard.h"
#include "process.h"
#include "scheduler.h"
#include "syscall.h"
#include "kthread_test.h"
#include "tss.h"
#include "usermode.h"

// User mode test program (defined in usermode_test.c)
extern void usermode_test_program(void);

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

    // Check if we have valid boot info
    bool has_boot_info = false;
    if (!boot_info) {
        console_print("[WARNING] Boot info is NULL\n");
        console_print("[INFO] Running in TEST MODE (no bootloader)\n");
    } else if (boot_info->magic != AURORA_BOOT_MAGIC) {
        console_print("[WARNING] Invalid boot magic: ");
        console_print_hex(boot_info->magic);
        console_print("\n[INFO] Running in TEST MODE\n");
    } else {
        console_print("[OK] Boot info validated\n");
        has_boot_info = true;
    }

    // Print boot information if available
    if (has_boot_info) {
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
    } else {
        console_print("\n[TEST MODE] No boot information available\n");
        console_print("[TEST MODE] Kernel loaded by QEMU or debugger\n");
    }

    // Print graphics info if available
    if (has_boot_info && boot_info->graphics_info) {
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

    // Initialize GDT (must be done before IDT and TSS)
    gdt_init();
    console_print("  [OK] GDT (Global Descriptor Table)\n");

    // Initialize TSS (must be done after GDT)
    tss_init();
    console_print("  [OK] TSS (Task State Segment)\n");

    // Initialize IDT
    idt_init();
    console_print("  [OK] IDT (Interrupt Descriptor Table)\n");

    // Initialize Physical Memory Manager
    pmm_init(boot_info);
    console_print("  [OK] PMM (Physical Memory Manager)\n");

    // Initialize Virtual Memory Manager
    vmm_init(boot_info);
    console_print("  [OK] VMM (Virtual Memory Manager)\n");

    // Initialize Kernel Heap
    kheap_init(boot_info);
    console_print("  [OK] Kernel Heap\n");

    // Initialize Timer (PIT)
    timer_init(TIMER_FREQ_1000HZ);  // 1000 Hz = 1ms tick
    console_print("  [OK] Timer (PIT)\n");

    // Initialize Keyboard (PS/2)
    keyboard_init();
    console_print("  [OK] Keyboard (PS/2)\n");

    // Initialize Process Management
    process_init();
    console_print("  [OK] Process Management\n");

    // Initialize Scheduler
    scheduler_init();
    console_print("  [OK] Scheduler\n");

    // Initialize System Call Interface
    syscall_init();
    console_print("  [OK] System Call Interface\n");

    console_print("\n[KERNEL] All subsystems initialized!\n\n");

    // TODO: Initialize Mach layer
    console_print("  [ ] Mach Microkernel Layer (TODO)\n");

    // TODO: Initialize BSD layer
    console_print("  [ ] BSD Layer (TODO)\n\n");

    console_print("=====================================\n");
    console_print("  AuroraOS Kernel Ready!\n");
    console_print("=====================================\n\n");

    // Start user mode test program
    // This function will not return - jumps to Ring 3
    start_usermode_process(usermode_test_program);

    // Should never reach here
    console_print("\n[ERROR] Returned from kthread_test_start()!\n");
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
