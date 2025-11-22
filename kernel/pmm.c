/**
 * AuroraOS Kernel - Physical Memory Manager Implementation
 *
 * Bitmap-based physical page frame allocator
 */

#include "pmm.h"
#include "console.h"
#include "io.h"

// Serial debug helper (COM1 = 0x3F8)
static inline void serial_debug_char(char c) {
    outb(0x3F8, c);
}

static inline void serial_debug_str(const char *s) {
    while (*s) {
        serial_debug_char(*s++);
    }
}

// Bitmap to track page allocation (1 = allocated, 0 = free)
// Each bit represents one 4KB page
// 1MB bitmap = supports up to 32GB RAM (1MB * 8 bits * 4KB)
#define BITMAP_SIZE (1024 * 1024)  // 1MB
static uint8_t page_bitmap[BITMAP_SIZE];

// PMM state
static struct {
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t used_pages;
    uint64_t highest_page;     // Highest usable page number
    bool initialized;
} pmm_state = {0};

// Helper: Set a bit in the bitmap
static inline void bitmap_set(uint64_t bit) {
    page_bitmap[bit / 8] |= (1 << (bit % 8));
}

// Helper: Clear a bit in the bitmap
static inline void bitmap_clear(uint64_t bit) {
    page_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

// Helper: Test a bit in the bitmap
static inline bool bitmap_test(uint64_t bit) {
    return (page_bitmap[bit / 8] & (1 << (bit % 8))) != 0;
}

/**
 * Initialize PMM with boot memory map
 */
void pmm_init(boot_info_t *boot_info) {
    serial_debug_str("pmm_init_start\n");
    console_print("[PMM] Initializing Physical Memory Manager...\n");
    serial_debug_str("after_pmm_console_print\n");

    // Clear bitmap (all pages initially marked as allocated)
    // WARNING: This is a 1MB loop that might crash like TSS did!
    serial_debug_str("before_bitmap_clear\n");
    for (uint64_t i = 0; i < BITMAP_SIZE; i++) {
        page_bitmap[i] = 0xFF;
    }
    serial_debug_str("after_bitmap_clear\n");

    serial_debug_str("clearing_pmm_state\n");
    pmm_state.total_pages = 0;
    pmm_state.free_pages = 0;
    pmm_state.used_pages = 0;
    pmm_state.highest_page = 0;
    serial_debug_str("after_pmm_state\n");

    // Check if we have boot info
    serial_debug_str("checking_boot_info\n");
    if (!boot_info || !boot_info->memory_map || boot_info->memory_map_size == 0) {
        serial_debug_str("boot_info_null_path\n");
        console_print("[PMM] WARNING: No memory map available\n");
        serial_debug_str("after_warning\n");
        console_print("[PMM] Using default 16MB memory assumption\n");
        serial_debug_str("after_default_msg\n");

        // Default: Mark first 16MB as available (except first 1MB)
        serial_debug_str("calc_default_pages\n");
        uint64_t default_pages = (16 * 1024 * 1024) / PAGE_SIZE;  // 16MB = 4096 pages
        uint64_t reserved_pages = (1 * 1024 * 1024) / PAGE_SIZE;  // First 1MB reserved

        serial_debug_str("set_total_pages\n");
        pmm_state.total_pages = default_pages;
        pmm_state.highest_page = default_pages;

        // Mark pages 256-4095 as free (1MB-16MB)
        serial_debug_str("before_mark_free\n");
        for (uint64_t i = reserved_pages; i < default_pages; i++) {
            bitmap_clear(i);
            pmm_state.free_pages++;
        }
        serial_debug_str("after_mark_free\n");

        serial_debug_str("calc_used_pages\n");
        pmm_state.used_pages = pmm_state.total_pages - pmm_state.free_pages;
        pmm_state.initialized = true;

        serial_debug_str("before_init_msg\n");
        console_print("[PMM] Initialized with default memory layout\n");
        serial_debug_str("before_print_stats\n");
        pmm_print_stats();
        serial_debug_str("pmm_init_done\n");
        return;
    }

    // Parse UEFI memory map
    console_print("[PMM] Parsing memory map...\n");

    uint64_t num_entries = boot_info->memory_map_size / boot_info->memory_map_descriptor_size;
    memory_descriptor_t *mmap = boot_info->memory_map;

    console_print("[PMM] Memory map entries: ");
    console_print_dec(num_entries);
    console_print("\n");

    // First pass: Find total memory and highest page
    for (uint64_t i = 0; i < num_entries; i++) {
        memory_descriptor_t *entry = (memory_descriptor_t*)
            ((uint8_t*)mmap + (i * boot_info->memory_map_descriptor_size));

        uint64_t start_page = ADDR_TO_PAGE(entry->physical_start);
        uint64_t num_pages = entry->number_of_pages;
        uint64_t end_page = start_page + num_pages;

        if (end_page > pmm_state.highest_page) {
            pmm_state.highest_page = end_page;
        }

        // Only count conventional memory as total
        if (entry->type == MEMORY_TYPE_AVAILABLE) {
            pmm_state.total_pages += num_pages;
        }
    }

    console_print("[PMM] Highest page: ");
    console_print_hex(pmm_state.highest_page);
    console_print("\n");

    // Check if we have enough bitmap space
    uint64_t required_bitmap_bytes = (pmm_state.highest_page + 7) / 8;
    if (required_bitmap_bytes > BITMAP_SIZE) {
        console_print("[PMM] WARNING: Not enough bitmap space\n");
        console_print("[PMM] Required: ");
        console_print_dec(required_bitmap_bytes);
        console_print(" bytes, Available: ");
        console_print_dec(BITMAP_SIZE);
        console_print(" bytes\n");
        pmm_state.highest_page = BITMAP_SIZE * 8;
    }

    // Second pass: Mark available pages as free
    for (uint64_t i = 0; i < num_entries; i++) {
        memory_descriptor_t *entry = (memory_descriptor_t*)
            ((uint8_t*)mmap + (i * boot_info->memory_map_descriptor_size));

        // Only mark conventional (available) memory as free
        if (entry->type != MEMORY_TYPE_AVAILABLE) {
            continue;
        }

        uint64_t start_page = ADDR_TO_PAGE(entry->physical_start);
        uint64_t num_pages = entry->number_of_pages;

        // Skip if beyond our bitmap
        if (start_page >= pmm_state.highest_page) {
            continue;
        }

        // Limit to bitmap size
        if (start_page + num_pages > pmm_state.highest_page) {
            num_pages = pmm_state.highest_page - start_page;
        }

        // Mark pages as free
        for (uint64_t page = start_page; page < start_page + num_pages; page++) {
            bitmap_clear(page);
            pmm_state.free_pages++;
        }
    }

    // Reserve first 1MB (BIOS, VGA, etc.)
    uint64_t reserved_pages = (1 * 1024 * 1024) / PAGE_SIZE;
    for (uint64_t page = 0; page < reserved_pages; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            if (pmm_state.free_pages > 0) {
                pmm_state.free_pages--;
            }
        }
    }

    // Reserve kernel memory (1MB - 2MB range)
    uint64_t kernel_start_page = ADDR_TO_PAGE(0x100000);  // 1MB
    uint64_t kernel_end_page = ADDR_TO_PAGE(0x200000);    // 2MB
    for (uint64_t page = kernel_start_page; page < kernel_end_page; page++) {
        if (!bitmap_test(page)) {
            bitmap_set(page);
            if (pmm_state.free_pages > 0) {
                pmm_state.free_pages--;
            }
        }
    }

    pmm_state.used_pages = pmm_state.total_pages - pmm_state.free_pages;
    pmm_state.initialized = true;

    console_print("[PMM] Initialization complete\n");
    pmm_print_stats();
}

/**
 * Allocate a single physical page frame
 */
uint64_t pmm_alloc_frame(void) {
    console_print("[DEBUG] pmm_alloc_frame: Entry\n");

    if (!pmm_state.initialized) {
        console_print("[DEBUG] PMM not initialized!\n");
        return 0;
    }

    if (pmm_state.free_pages == 0) {
        console_print("[DEBUG] PMM: Out of memory!\n");
        return 0;  // Out of memory
    }

    console_print("[DEBUG] PMM: Searching for free page...\n");

    // Find first free page (first-fit)
    for (uint64_t page = 0; page < pmm_state.highest_page; page++) {
        if (!bitmap_test(page)) {
            console_print("[DEBUG] PMM: Found free page ");
            console_print_dec(page);
            console_print("\n");

            bitmap_set(page);
            pmm_state.free_pages--;
            pmm_state.used_pages++;

            uint64_t addr = PAGE_TO_ADDR(page);
            console_print("[DEBUG] PMM: Returning address ");
            console_print_hex(addr);
            console_print("\n");

            return addr;
        }
    }

    console_print("[DEBUG] PMM: No free pages found!\n");
    return 0;  // No free pages found
}

/**
 * Allocate multiple contiguous physical page frames
 */
uint64_t pmm_alloc_frames(uint64_t count) {
    if (!pmm_state.initialized || count == 0) {
        return 0;
    }

    if (pmm_state.free_pages < count) {
        return 0;  // Not enough memory
    }

    // Find contiguous free pages
    uint64_t start_page = 0;
    uint64_t found_count = 0;

    for (uint64_t page = 0; page < pmm_state.highest_page; page++) {
        if (!bitmap_test(page)) {
            if (found_count == 0) {
                start_page = page;
            }
            found_count++;

            if (found_count == count) {
                // Found enough contiguous pages, allocate them
                for (uint64_t i = 0; i < count; i++) {
                    bitmap_set(start_page + i);
                }
                pmm_state.free_pages -= count;
                pmm_state.used_pages += count;
                return PAGE_TO_ADDR(start_page);
            }
        } else {
            found_count = 0;
        }
    }

    return 0;  // Not enough contiguous pages
}

/**
 * Free a physical page frame
 */
void pmm_free_frame(uint64_t addr) {
    if (!pmm_state.initialized) {
        return;
    }

    uint64_t page = ADDR_TO_PAGE(addr);
    if (page >= pmm_state.highest_page) {
        return;
    }

    if (bitmap_test(page)) {
        bitmap_clear(page);
        pmm_state.free_pages++;
        pmm_state.used_pages--;
    }
}

/**
 * Free multiple contiguous physical page frames
 */
void pmm_free_frames(uint64_t addr, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        pmm_free_frame(addr + (i * PAGE_SIZE));
    }
}

/**
 * Mark a physical page as used
 */
void pmm_mark_used(uint64_t addr) {
    if (!pmm_state.initialized) {
        return;
    }

    uint64_t page = ADDR_TO_PAGE(addr);
    if (page >= pmm_state.highest_page) {
        return;
    }

    if (!bitmap_test(page)) {
        bitmap_set(page);
        if (pmm_state.free_pages > 0) {
            pmm_state.free_pages--;
        }
        pmm_state.used_pages++;
    }
}

/**
 * Mark multiple pages as used
 */
void pmm_mark_used_range(uint64_t addr, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        pmm_mark_used(addr + (i * PAGE_SIZE));
    }
}

/**
 * Check if a physical page is allocated
 */
bool pmm_is_allocated(uint64_t addr) {
    if (!pmm_state.initialized) {
        return true;
    }

    uint64_t page = ADDR_TO_PAGE(addr);
    if (page >= pmm_state.highest_page) {
        return true;
    }

    return bitmap_test(page);
}

/**
 * Get PMM statistics
 */
void pmm_get_stats(pmm_stats_t *stats) {
    if (!stats) {
        return;
    }

    stats->total_pages = pmm_state.total_pages;
    stats->free_pages = pmm_state.free_pages;
    stats->used_pages = pmm_state.used_pages;
    stats->reserved_pages = pmm_state.total_pages - pmm_state.free_pages - pmm_state.used_pages;
    stats->total_memory = pmm_state.total_pages * PAGE_SIZE;
    stats->free_memory = pmm_state.free_pages * PAGE_SIZE;
}

/**
 * Get total physical memory
 */
uint64_t pmm_get_total_memory(void) {
    return pmm_state.total_pages * PAGE_SIZE;
}

/**
 * Get free physical memory
 */
uint64_t pmm_get_free_memory(void) {
    return pmm_state.free_pages * PAGE_SIZE;
}

/**
 * Print PMM statistics
 */
void pmm_print_stats(void) {
    console_print("[PMM] Memory Statistics:\n");
    console_print("  Total Pages: ");
    console_print_dec(pmm_state.total_pages);
    console_print(" (");
    console_print_dec(pmm_state.total_pages * PAGE_SIZE / 1024 / 1024);
    console_print(" MB)\n");

    console_print("  Free Pages:  ");
    console_print_dec(pmm_state.free_pages);
    console_print(" (");
    console_print_dec(pmm_state.free_pages * PAGE_SIZE / 1024 / 1024);
    console_print(" MB)\n");

    console_print("  Used Pages:  ");
    console_print_dec(pmm_state.used_pages);
    console_print(" (");
    console_print_dec(pmm_state.used_pages * PAGE_SIZE / 1024 / 1024);
    console_print(" MB)\n");
}
