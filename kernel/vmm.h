/**
 * AuroraOS Kernel - Virtual Memory Manager (VMM)
 *
 * Manages virtual memory, paging, and address space mapping
 * Implements x86_64 4-level paging (PML4, PDPT, PD, PT)
 */

#ifndef _KERNEL_VMM_H_
#define _KERNEL_VMM_H_

#include "types.h"
#include "boot.h"

// Page size constants
#define PAGE_SIZE       4096
#define PAGE_SHIFT      12
#define ENTRIES_PER_TABLE 512

// Page table entry flags
#define PTE_PRESENT     (1ULL << 0)   // Page is present in memory
#define PTE_WRITE       (1ULL << 1)   // Page is writable
#define PTE_USER        (1ULL << 2)   // Page is accessible from user mode
#define PTE_WRITETHROUGH (1ULL << 3)  // Write-through caching
#define PTE_CACHE_DISABLE (1ULL << 4) // Cache disabled
#define PTE_ACCESSED    (1ULL << 5)   // Page has been accessed
#define PTE_DIRTY       (1ULL << 6)   // Page has been written to
#define PTE_HUGE        (1ULL << 7)   // 2MB/1GB page (PD/PDPT level)
#define PTE_GLOBAL      (1ULL << 8)   // Global page (not flushed on CR3 reload)
#define PTE_NX          (1ULL << 63)  // No-execute bit

// Common flag combinations
#define PTE_KERNEL_FLAGS (PTE_PRESENT | PTE_WRITE)
#define PTE_USER_FLAGS   (PTE_PRESENT | PTE_WRITE | PTE_USER)

// Virtual memory layout
#define KERNEL_VIRTUAL_BASE  0xFFFFFFFF80000000ULL  // -2GB (higher-half kernel)
#define KERNEL_PHYSICAL_BASE 0x100000ULL            // 1MB (where kernel is loaded)

// Recursive mapping slot (last PML4 entry maps to itself)
#define RECURSIVE_SLOT      511
#define RECURSIVE_BASE      0xFFFF800000000000ULL

// Page table structure (4KB aligned)
typedef struct {
    uint64_t entries[ENTRIES_PER_TABLE];
} __attribute__((aligned(4096))) page_table_t;

// Page table entry structure
typedef uint64_t pte_t;

// Virtual address breakdown
typedef struct {
    uint16_t offset;    // Bits 0-11:  Page offset (4KB)
    uint16_t pt_index;  // Bits 12-20: Page Table index
    uint16_t pd_index;  // Bits 21-29: Page Directory index
    uint16_t pdpt_index;// Bits 30-38: PDPT index
    uint16_t pml4_index;// Bits 39-47: PML4 index
    uint16_t sign_ext;  // Bits 48-63: Sign extension
} virt_addr_t;

// VMM statistics
typedef struct {
    uint64_t total_virtual_pages;
    uint64_t mapped_pages;
    uint64_t kernel_pages;
    uint64_t user_pages;
    uint64_t total_page_tables;
} vmm_stats_t;

// VMM initialization
void vmm_init(boot_info_t *boot_info);

// Page table management
pte_t* vmm_get_pte(uint64_t virt_addr, bool create);
bool vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);
bool vmm_unmap_page(uint64_t virt_addr);
uint64_t vmm_get_physical(uint64_t virt_addr);

// Address space management
bool vmm_map_range(uint64_t virt_addr, uint64_t phys_addr, uint64_t size, uint64_t flags);
bool vmm_unmap_range(uint64_t virt_addr, uint64_t size);

// TLB management
void vmm_flush_tlb(void);
void vmm_flush_tlb_single(uint64_t virt_addr);

// Utility functions
virt_addr_t vmm_parse_address(uint64_t addr);
uint64_t vmm_construct_address(virt_addr_t *vaddr);
void vmm_print_stats(void);

// Assembly functions
extern void vmm_load_cr3(uint64_t pml4_phys);
extern uint64_t vmm_get_cr3(void);
extern void vmm_enable_paging(void);

#endif // _KERNEL_VMM_H_
