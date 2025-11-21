/**
 * AuroraOS Kernel - Physical Memory Manager (PMM)
 *
 * Manages physical memory pages using a bitmap allocator
 */

#ifndef _KERNEL_PMM_H_
#define _KERNEL_PMM_H_

#include "types.h"
#include "boot.h"

// Page size (4KB standard for x86_64)
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12  // log2(PAGE_SIZE)

// Align address up to page boundary
#define PAGE_ALIGN_UP(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

// Align address down to page boundary
#define PAGE_ALIGN_DOWN(addr) ((addr) & ~(PAGE_SIZE - 1))

// Get page frame number from address
#define ADDR_TO_PAGE(addr) ((addr) >> PAGE_SHIFT)

// Get address from page frame number
#define PAGE_TO_ADDR(page) ((page) << PAGE_SHIFT)

// Memory region types (from UEFI/E820)
#define MEMORY_TYPE_AVAILABLE       1  // EfiConventionalMemory
#define MEMORY_TYPE_RESERVED        2  // EfiReservedMemoryType
#define MEMORY_TYPE_ACPI_RECLAIM    3  // EfiACPIReclaimMemory
#define MEMORY_TYPE_ACPI_NVS        4  // EfiACPIMemoryNVS
#define MEMORY_TYPE_BAD             5  // EfiBadMemory

// PMM statistics
typedef struct {
    uint64_t total_pages;      // Total physical pages
    uint64_t free_pages;       // Currently free pages
    uint64_t used_pages;       // Currently used pages
    uint64_t reserved_pages;   // Reserved by firmware/kernel
    uint64_t total_memory;     // Total memory in bytes
    uint64_t free_memory;      // Free memory in bytes
} pmm_stats_t;

// Function prototypes

/**
 * Initialize physical memory manager
 * Parses boot memory map and sets up bitmap allocator
 *
 * @param boot_info Boot information structure from bootloader
 */
void pmm_init(boot_info_t *boot_info);

/**
 * Allocate a single physical page frame
 *
 * @return Physical address of allocated page, or 0 if out of memory
 */
uint64_t pmm_alloc_frame(void);

/**
 * Allocate multiple contiguous physical page frames
 *
 * @param count Number of pages to allocate
 * @return Physical address of first page, or 0 if not enough contiguous memory
 */
uint64_t pmm_alloc_frames(uint64_t count);

/**
 * Free a previously allocated physical page frame
 *
 * @param addr Physical address of page to free
 */
void pmm_free_frame(uint64_t addr);

/**
 * Free multiple contiguous physical page frames
 *
 * @param addr Physical address of first page
 * @param count Number of pages to free
 */
void pmm_free_frames(uint64_t addr, uint64_t count);

/**
 * Mark a physical page as used (reserved)
 *
 * @param addr Physical address of page to mark as used
 */
void pmm_mark_used(uint64_t addr);

/**
 * Mark multiple physical pages as used (reserved)
 *
 * @param addr Physical address of first page
 * @param count Number of pages to mark
 */
void pmm_mark_used_range(uint64_t addr, uint64_t count);

/**
 * Check if a physical page is allocated
 *
 * @param addr Physical address to check
 * @return true if allocated, false if free
 */
bool pmm_is_allocated(uint64_t addr);

/**
 * Get PMM statistics
 *
 * @param stats Pointer to stats structure to fill
 */
void pmm_get_stats(pmm_stats_t *stats);

/**
 * Get total physical memory in bytes
 *
 * @return Total memory size
 */
uint64_t pmm_get_total_memory(void);

/**
 * Get free physical memory in bytes
 *
 * @return Free memory size
 */
uint64_t pmm_get_free_memory(void);

/**
 * Print PMM status to console (for debugging)
 */
void pmm_print_stats(void);

#endif // _KERNEL_PMM_H_
