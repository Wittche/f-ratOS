/**
 * AuroraOS Kernel - Kernel Heap Allocator
 *
 * Dynamic memory allocation for kernel
 * Implements kmalloc/kfree with block-based allocation
 */

#ifndef _KERNEL_KHEAP_H_
#define _KERNEL_KHEAP_H_

#include "types.h"
#include "boot.h"

// Heap configuration
#define HEAP_START_ADDR   0x200000      // 2MB (after kernel)
#define HEAP_INITIAL_SIZE (16 * 1024)   // 16KB initial heap (faster boot on low RAM)
#define HEAP_MAX_SIZE     (16 * 1024 * 1024) // 16MB max heap
#define HEAP_MIN_BLOCK    16             // Minimum block size

// Block header flags
#define BLOCK_FREE  0x0
#define BLOCK_USED  0x1

// Heap statistics
typedef struct {
    uint64_t total_size;        // Total heap size
    uint64_t used_size;         // Used memory
    uint64_t free_size;         // Free memory
    uint64_t num_blocks;        // Total blocks
    uint64_t num_free_blocks;   // Free blocks
    uint64_t num_used_blocks;   // Used blocks
    uint64_t num_allocations;   // Total allocations
    uint64_t num_frees;         // Total frees
} heap_stats_t;

// Heap initialization
void kheap_init(boot_info_t *boot_info);

// Memory allocation functions
void* kmalloc(uint64_t size);
void* kmalloc_aligned(uint64_t size, uint64_t alignment);
void* kcalloc(uint64_t num, uint64_t size);
void* krealloc(void *ptr, uint64_t new_size);
void  kfree(void *ptr);

// Heap management
void kheap_expand(uint64_t size);
void kheap_coalesce(void);

// Debugging and statistics
void kheap_print_stats(void);
void kheap_dump_blocks(void);
bool kheap_validate(void);

#endif // _KERNEL_KHEAP_H_
