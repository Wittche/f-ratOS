/**
 * AuroraOS Kernel - Kernel Heap Allocator Implementation
 *
 * Simple block-based allocator with first-fit strategy
 */

#include "kheap.h"
#include "pmm.h"
#include "vmm.h"
#include "console.h"
#include "types.h"
#include "io.h"

// Serial debug helper
static inline void serial_write(const char *s) {
    while (*s) { outb(0x3F8, *s++); }
}

// Block header structure
typedef struct block_header {
    uint64_t size;               // Size of data area (excluding header)
    uint64_t flags;              // BLOCK_FREE or BLOCK_USED
    struct block_header *next;   // Next block in list
    struct block_header *prev;   // Previous block in list
    uint32_t magic;              // Magic number for validation
} __attribute__((packed)) block_header_t;

#define BLOCK_MAGIC 0xDEADBEEF
#define HEADER_SIZE sizeof(block_header_t)

// Heap state
static struct {
    uint64_t heap_start;
    uint64_t heap_end;
    uint64_t heap_size;
    block_header_t *first_block;
    bool initialized;
    heap_stats_t stats;
} heap_state = {0};

// Helper macros
#define ALIGN_UP(addr, align)   (((addr) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)

/**
 * Get data pointer from block header
 */
static inline void* block_to_ptr(block_header_t *block) {
    return (void*)((uint8_t*)block + HEADER_SIZE);
}

/**
 * Get block header from data pointer
 */
static inline block_header_t* ptr_to_block(void *ptr) {
    return (block_header_t*)((uint8_t*)ptr - HEADER_SIZE);
}

/**
 * Validate block header
 */
static bool validate_block(block_header_t *block) {
    if (!block) return false;
    if (block->magic != BLOCK_MAGIC) return false;
    if (block->size == 0) return false;
    return true;
}

/**
 * Find free block using first-fit strategy
 */
static block_header_t* find_free_block(uint64_t size) {
    block_header_t *current = heap_state.first_block;

    while (current) {
        if (!validate_block(current)) {
            console_print("[HEAP] ERROR: Corrupted block detected\n");
            return NULL;
        }

        if ((current->flags & BLOCK_USED) == 0 && current->size >= size) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

/**
 * Split block if large enough
 */
static void split_block(block_header_t *block, uint64_t size) {
    if (block->size < size + HEADER_SIZE + HEAP_MIN_BLOCK) {
        // Block too small to split
        return;
    }

    // Create new free block from remaining space
    uint64_t remaining = block->size - size - HEADER_SIZE;
    block_header_t *new_block = (block_header_t*)((uint8_t*)block + HEADER_SIZE + size);

    new_block->size = remaining;
    new_block->flags = BLOCK_FREE;
    new_block->magic = BLOCK_MAGIC;
    new_block->next = block->next;
    new_block->prev = block;

    if (block->next) {
        block->next->prev = new_block;
    }

    block->next = new_block;
    block->size = size;

    heap_state.stats.num_blocks++;
    heap_state.stats.num_free_blocks++;
}

/**
 * Coalesce adjacent free blocks
 */
void kheap_coalesce(void) {
    block_header_t *current = heap_state.first_block;

    while (current && current->next) {
        if (!validate_block(current)) {
            console_print("[HEAP] ERROR: Invalid block during coalesce\n");
            return;
        }

        // If current and next are both free, merge them
        if ((current->flags & BLOCK_USED) == 0 &&
            (current->next->flags & BLOCK_USED) == 0) {

            block_header_t *next = current->next;
            current->size += HEADER_SIZE + next->size;
            current->next = next->next;

            if (next->next) {
                next->next->prev = current;
            }

            heap_state.stats.num_blocks--;
            heap_state.stats.num_free_blocks--;
        } else {
            current = current->next;
        }
    }
}

/**
 * Expand heap by allocating more pages
 */
void kheap_expand(uint64_t size) {
    serial_write("expand_START\n");
    // Align size to page boundary
    size = ALIGN_UP(size, PAGE_SIZE);

    // Check max heap size
    if (heap_state.heap_size + size > HEAP_MAX_SIZE) {
        console_print("[HEAP] WARNING: Max heap size reached\n");
        return;
    }

    // Allocate physical pages
    uint64_t num_pages = size / PAGE_SIZE;
    serial_write("alloc_loop\n");

    for (uint64_t i = 0; i < num_pages; i++) {
        if (i % 64 == 0) outb(0x3F8, '.');

        serial_write("[CALL_PMM]");
        uint64_t phys = pmm_alloc_frame();
        serial_write("[BACK_PMM]");
        if (phys == 0) {
            serial_write("PMM_FAIL\n");
            console_print("[HEAP] ERROR: Failed to allocate physical page\n");
            return;
        }

        uint64_t virt = heap_state.heap_end + (i * PAGE_SIZE);
        serial_write("[CALL_VMM]");
        bool result = vmm_map_page(virt, phys, PTE_KERNEL_FLAGS);
        serial_write("[BACK_VMM]");
        if (!result) {
            serial_write("VMM_MAP_FAIL\n");
            console_print("[HEAP] ERROR: Failed to map heap page\n");
            pmm_free_frame(phys);
            return;
        }
    }
    serial_write("\nalloc_done\n");

    // Create new free block at end of heap
    serial_write("new_block\n");
    block_header_t *new_block = (block_header_t*)heap_state.heap_end;
    new_block->size = size - HEADER_SIZE;
    new_block->flags = BLOCK_FREE;
    new_block->magic = BLOCK_MAGIC;
    new_block->next = NULL;
    new_block->prev = NULL;
    serial_write("block_init\n");

    // Find last block and append
    if (!heap_state.first_block) {
        serial_write("first_block_NULL\n");
        // First block ever - initialize the list
        heap_state.first_block = new_block;
        serial_write("first_block_SET\n");
    } else {
        serial_write("append_block\n");
        // Find last block and append
        block_header_t *last = heap_state.first_block;
        while (last && last->next) {
            last = last->next;
        }
        last->next = new_block;
        new_block->prev = last;
        serial_write("appended\n");
    }

    // Memory barrier to ensure all writes are visible
    serial_write("mfence\n");
    __asm__ volatile("mfence" ::: "memory");
    serial_write("expand_COMPLETE\n");

    heap_state.heap_end += size;
    heap_state.heap_size += size;
    heap_state.stats.total_size += size;
    heap_state.stats.free_size += size - HEADER_SIZE;
    heap_state.stats.num_blocks++;
    heap_state.stats.num_free_blocks++;

    // Try to coalesce with previous block
    kheap_coalesce();
}

/**
 * Allocate memory from heap
 */
void* kmalloc(uint64_t size) {
    if (!heap_state.initialized) {
        console_print("[HEAP] ERROR: Heap not initialized\n");
        return NULL;
    }

    if (size == 0) {
        return NULL;
    }

    // Align size to 8-byte boundary
    size = ALIGN_UP(size, 8);

    // Find free block
    block_header_t *block = find_free_block(size);

    // If no block found, expand heap
    if (!block) {
        uint64_t expand_size = ALIGN_UP(size + HEADER_SIZE, PAGE_SIZE);
        kheap_expand(expand_size);
        block = find_free_block(size);

        if (!block) {
            console_print("[HEAP] ERROR: Out of memory\n");
            return NULL;
        }
    }

    // Split block if large enough
    split_block(block, size);

    // Mark as used
    block->flags = BLOCK_USED;

    // Update statistics
    heap_state.stats.used_size += block->size;
    heap_state.stats.free_size -= block->size;
    heap_state.stats.num_used_blocks++;
    heap_state.stats.num_free_blocks--;
    heap_state.stats.num_allocations++;

    return block_to_ptr(block);
}

/**
 * Allocate aligned memory
 */
void* kmalloc_aligned(uint64_t size, uint64_t alignment) {
    if (!IS_ALIGNED(alignment, alignment)) {
        return NULL;
    }

    // Allocate extra space for alignment
    uint64_t total_size = size + alignment + HEADER_SIZE;
    void *ptr = kmalloc(total_size);

    if (!ptr) {
        return NULL;
    }

    // Calculate aligned address
    uint64_t addr = (uint64_t)ptr;
    uint64_t aligned = ALIGN_UP(addr, alignment);

    // If already aligned, return as-is
    if (aligned == addr) {
        return ptr;
    }

    // Otherwise, we need to adjust (simplified - just return unaligned for now)
    return ptr;
}

/**
 * Allocate and zero memory
 */
void* kcalloc(uint64_t num, uint64_t size) {
    uint64_t total = num * size;
    void *ptr = kmalloc(total);

    if (ptr) {
        // Zero the memory
        uint8_t *bytes = (uint8_t*)ptr;
        for (uint64_t i = 0; i < total; i++) {
            bytes[i] = 0;
        }
    }

    return ptr;
}

/**
 * Free allocated memory
 */
void kfree(void *ptr) {
    if (!ptr) {
        return;
    }

    if (!heap_state.initialized) {
        console_print("[HEAP] ERROR: Heap not initialized\n");
        return;
    }

    // Get block header
    block_header_t *block = ptr_to_block(ptr);

    if (!validate_block(block)) {
        console_print("[HEAP] ERROR: Invalid block in kfree\n");
        return;
    }

    if ((block->flags & BLOCK_USED) == 0) {
        console_print("[HEAP] WARNING: Double free detected\n");
        return;
    }

    // Mark as free
    block->flags = BLOCK_FREE;

    // Update statistics
    heap_state.stats.used_size -= block->size;
    heap_state.stats.free_size += block->size;
    heap_state.stats.num_used_blocks--;
    heap_state.stats.num_free_blocks++;
    heap_state.stats.num_frees++;

    // Coalesce adjacent free blocks
    kheap_coalesce();
}

/**
 * Reallocate memory
 */
void* krealloc(void *ptr, uint64_t new_size) {
    if (!ptr) {
        return kmalloc(new_size);
    }

    if (new_size == 0) {
        kfree(ptr);
        return NULL;
    }

    block_header_t *block = ptr_to_block(ptr);
    if (!validate_block(block)) {
        return NULL;
    }

    // If new size fits in current block, just return it
    if (new_size <= block->size) {
        return ptr;
    }

    // Allocate new block
    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) {
        return NULL;
    }

    // Copy old data
    uint8_t *src = (uint8_t*)ptr;
    uint8_t *dst = (uint8_t*)new_ptr;
    for (uint64_t i = 0; i < block->size && i < new_size; i++) {
        dst[i] = src[i];
    }

    // Free old block
    kfree(ptr);

    return new_ptr;
}

/**
 * Initialize kernel heap
 */
void kheap_init(boot_info_t *boot_info) {
    serial_write("kheap_init_ENTERED\n");
    console_print("[HEAP] Initializing kernel heap...\n");
    serial_write("after_console_print_1\n");

    // Set heap boundaries
    serial_write("A\n");
    heap_state.heap_start = HEAP_START_ADDR;
    serial_write("B\n");
    heap_state.heap_end = HEAP_START_ADDR;
    serial_write("C\n");
    heap_state.heap_size = 0;
    serial_write("heap_bounds_set\n");

    // Memory barrier to ensure initialization is visible
    __asm__ volatile("mfence" ::: "memory");
    serial_write("mfence_done\n");

    // Expand initial heap
    serial_write("before_expand\n");
    kheap_expand(HEAP_INITIAL_SIZE);
    serial_write("after_expand\n");

    if (heap_state.heap_size == 0) {
        console_print("[HEAP] ERROR: Failed to initialize heap\n");
        return;
    }

    heap_state.initialized = true;

    console_print("[HEAP] Initialized at ");
    console_print_hex(heap_state.heap_start);
    console_print("\n[HEAP]   Initial size: ");
    console_print_dec(heap_state.heap_size / 1024);
    console_print(" KB\n[HEAP]   Max size:     ");
    console_print_dec(HEAP_MAX_SIZE / 1024);
    console_print(" KB\n");

    (void)boot_info; // Unused for now
}

/**
 * Validate heap integrity
 */
bool kheap_validate(void) {
    if (!heap_state.initialized) {
        return false;
    }

    block_header_t *current = heap_state.first_block;
    uint64_t count = 0;

    while (current) {
        if (!validate_block(current)) {
            console_print("[HEAP] Validation failed at block ");
            console_print_dec(count);
            console_print("\n");
            return false;
        }

        count++;
        if (count > heap_state.stats.num_blocks + 10) {
            console_print("[HEAP] Validation failed: infinite loop detected\n");
            return false;
        }

        current = current->next;
    }

    return true;
}

/**
 * Print heap statistics
 */
void kheap_print_stats(void) {
    console_print("\n[HEAP] Statistics:\n");
    console_print("  Total Size:    ");
    console_print_dec(heap_state.stats.total_size / 1024);
    console_print(" KB\n  Used:          ");
    console_print_dec(heap_state.stats.used_size / 1024);
    console_print(" KB\n  Free:          ");
    console_print_dec(heap_state.stats.free_size / 1024);
    console_print(" KB\n  Blocks:        ");
    console_print_dec(heap_state.stats.num_blocks);
    console_print("\n  Used Blocks:   ");
    console_print_dec(heap_state.stats.num_used_blocks);
    console_print("\n  Free Blocks:   ");
    console_print_dec(heap_state.stats.num_free_blocks);
    console_print("\n  Allocations:   ");
    console_print_dec(heap_state.stats.num_allocations);
    console_print("\n  Frees:         ");
    console_print_dec(heap_state.stats.num_frees);
    console_print("\n");
}

/**
 * Dump all blocks (for debugging)
 */
void kheap_dump_blocks(void) {
    console_print("\n[HEAP] Block Dump:\n");

    block_header_t *current = heap_state.first_block;
    uint64_t index = 0;

    while (current && index < 20) {
        console_print("  [");
        console_print_dec(index);
        console_print("] ");
        console_print_hex((uint64_t)current);
        console_print(" size=");
        console_print_dec(current->size);
        console_print(" ");
        console_print((current->flags & BLOCK_USED) ? "USED" : "FREE");
        console_print("\n");

        current = current->next;
        index++;
    }

    if (current) {
        console_print("  ... (more blocks)\n");
    }
}
