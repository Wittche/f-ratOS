/**
 * AuroraOS Kernel - Virtual Memory Manager Implementation
 *
 * Implements x86_64 4-level paging with higher-half kernel
 */

#include "vmm.h"
#include "pmm.h"
#include "console.h"
#include "types.h"
#include "boot.h"
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

// Global page table pointers
static page_table_t *kernel_pml4 = NULL;
static bool vmm_initialized = false;

// VMM state
static struct {
    uint64_t pml4_physical;
    uint64_t mapped_pages;
    uint64_t kernel_pages;
    uint64_t page_tables_allocated;
} vmm_state = {0};

// Helper macros
#define ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))
#define ALIGN_UP(addr, align)   (((addr) + (align) - 1) & ~((align) - 1))
#define IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)

#define PTE_ADDR_MASK  0x000FFFFFFFFFF000ULL  // Physical address bits
#define PTE_FLAGS_MASK 0xFFF0000000000FFFULL  // Flag bits

// Extract physical address from PTE
static inline uint64_t pte_get_addr(pte_t pte) {
    return pte & PTE_ADDR_MASK;
}

// Create PTE from address and flags
static inline pte_t pte_create(uint64_t phys_addr, uint64_t flags) {
    return (phys_addr & PTE_ADDR_MASK) | (flags & PTE_FLAGS_MASK);
}

/**
 * Parse virtual address into components
 */
virt_addr_t vmm_parse_address(uint64_t addr) {
    virt_addr_t vaddr = {0};

    vaddr.offset     = (addr >> 0)  & 0xFFF;  // 12 bits
    vaddr.pt_index   = (addr >> 12) & 0x1FF;  // 9 bits
    vaddr.pd_index   = (addr >> 21) & 0x1FF;  // 9 bits
    vaddr.pdpt_index = (addr >> 30) & 0x1FF;  // 9 bits
    vaddr.pml4_index = (addr >> 39) & 0x1FF;  // 9 bits
    vaddr.sign_ext   = (addr >> 48) & 0xFFFF; // 16 bits

    return vaddr;
}

/**
 * Construct virtual address from components
 */
uint64_t vmm_construct_address(virt_addr_t *vaddr) {
    uint64_t addr = 0;

    addr |= ((uint64_t)vaddr->offset     << 0);
    addr |= ((uint64_t)vaddr->pt_index   << 12);
    addr |= ((uint64_t)vaddr->pd_index   << 21);
    addr |= ((uint64_t)vaddr->pdpt_index << 30);
    addr |= ((uint64_t)vaddr->pml4_index << 39);

    // Sign extend bit 47
    if (addr & (1ULL << 47)) {
        addr |= 0xFFFF000000000000ULL;
    }

    return addr;
}

/**
 * Flush TLB (Translation Lookaside Buffer)
 */
void vmm_flush_tlb(void) {
    __asm__ __volatile__(
        "movq %%cr3, %%rax\n"
        "movq %%rax, %%cr3\n"
        ::: "rax"
    );
}

/**
 * Flush single TLB entry
 */
void vmm_flush_tlb_single(uint64_t virt_addr) {
    __asm__ __volatile__(
        "invlpg (%0)"
        :: "r"(virt_addr)
        : "memory"
    );
}

/**
 * Get page table entry for virtual address
 * Creates intermediate tables if needed
 *
 * @param virt_addr Virtual address
 * @param create If true, create missing page tables
 * @return Pointer to PTE or NULL if not present and create=false
 */
pte_t* vmm_get_pte(uint64_t virt_addr, bool create) {
    if (!vmm_initialized) {
        serial_debug_str("get_pte_not_init\n");
        console_print("[DEBUG] vmm_get_pte: VMM not initialized!\n");
        return NULL;
    }

    virt_addr_t vaddr = vmm_parse_address(virt_addr);

    // Walk PML4
    pte_t *pml4_entry = &kernel_pml4->entries[vaddr.pml4_index];
    page_table_t *pdpt;

    if (!(*pml4_entry & PTE_PRESENT)) {
        if (!create) return NULL;

        // Allocate new PDPT
        serial_debug_str("alloc_pdpt\n");
        uint64_t pdpt_phys = pmm_alloc_frame();
        serial_debug_str("pdpt_allocated\n");
        if (pdpt_phys == 0) return NULL;

        *pml4_entry = pte_create(pdpt_phys, PTE_KERNEL_FLAGS);
        vmm_state.page_tables_allocated++;

        // Zero out the new table
        serial_debug_str("before_pdpt_zero\n");
        pdpt = (page_table_t*)pdpt_phys;
        for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
            pdpt->entries[i] = 0;
        }
        serial_debug_str("after_pdpt_zero\n");
    } else {
        pdpt = (page_table_t*)pte_get_addr(*pml4_entry);
    }

    // Walk PDPT
    pte_t *pdpt_entry = &pdpt->entries[vaddr.pdpt_index];
    page_table_t *pd;

    if (!(*pdpt_entry & PTE_PRESENT)) {
        if (!create) return NULL;

        // Allocate new PD
        serial_debug_str("alloc_pd\n");
        uint64_t pd_phys = pmm_alloc_frame();
        if (pd_phys == 0) return NULL;

        *pdpt_entry = pte_create(pd_phys, PTE_KERNEL_FLAGS);
        vmm_state.page_tables_allocated++;

        // Zero out the new table
        serial_debug_str("before_pd_zero\n");
        pd = (page_table_t*)pd_phys;
        serial_debug_str("pd_zero_loop_start\n");
        for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
            pd->entries[i] = 0;
        }
        serial_debug_str("pd_zero_loop_end\n");
        serial_debug_str("after_pd_zero\n");
    } else {
        pd = (page_table_t*)pte_get_addr(*pdpt_entry);
    }

    // Walk PD
    serial_debug_str("before_pd_walk\n");
    pte_t *pd_entry = &pd->entries[vaddr.pd_index];
    serial_debug_str("after_pd_entry_get\n");
    page_table_t *pt;

    serial_debug_str("checking_pd_present\n");
    if (!(*pd_entry & PTE_PRESENT)) {
        serial_debug_str("pd_not_present\n");
        if (!create) {
            serial_debug_str("create_false_return\n");
            return NULL;
        }

        // Allocate new PT
        serial_debug_str("alloc_pt\n");
        serial_debug_str("before_pt_pmm_call\n");
        uint64_t pt_phys = pmm_alloc_frame();
        serial_debug_str("after_pt_pmm_call\n");
        if (pt_phys == 0) {
            serial_debug_str("pt_alloc_failed\n");
            return NULL;
        }
        serial_debug_str("pt_alloc_success\n");

        serial_debug_str("before_pt_pte_create\n");
        *pd_entry = pte_create(pt_phys, PTE_KERNEL_FLAGS);
        serial_debug_str("after_pt_pte_create\n");
        vmm_state.page_tables_allocated++;
        serial_debug_str("pt_count_incremented\n");

        // Zero out the new table
        serial_debug_str("before_pt_zero\n");
        pt = (page_table_t*)pt_phys;
        serial_debug_str("pt_zero_loop_start\n");
        for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
            pt->entries[i] = 0;
        }
        serial_debug_str("pt_zero_loop_end\n");
        serial_debug_str("after_pt_zero\n");
    } else {
        serial_debug_str("pd_already_present\n");
        pt = (page_table_t*)pte_get_addr(*pd_entry);
    }
    serial_debug_str("pt_walk_complete\n");

    // Return pointer to final PT entry
    return &pt->entries[vaddr.pt_index];
}

/**
 * Map a single page
 *
 * @param virt_addr Virtual address (will be page-aligned)
 * @param phys_addr Physical address (will be page-aligned)
 * @param flags Page flags (PTE_PRESENT, PTE_WRITE, etc.)
 * @return true on success, false on failure
 */
bool vmm_map_page(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
    // Align addresses
    virt_addr = ALIGN_DOWN(virt_addr, PAGE_SIZE);
    phys_addr = ALIGN_DOWN(phys_addr, PAGE_SIZE);

    // Get or create PTE
    pte_t *pte = vmm_get_pte(virt_addr, true);
    if (!pte) {
        return false;
    }

    // Check if already mapped
    if (*pte & PTE_PRESENT) {
        // Already mapped - update flags
        *pte = pte_create(phys_addr, flags | PTE_PRESENT);
    } else {
        // New mapping
        *pte = pte_create(phys_addr, flags | PTE_PRESENT);
        vmm_state.mapped_pages++;
    }

    // Flush TLB for this page
    vmm_flush_tlb_single(virt_addr);

    return true;
}

/**
 * Unmap a single page
 */
bool vmm_unmap_page(uint64_t virt_addr) {
    virt_addr = ALIGN_DOWN(virt_addr, PAGE_SIZE);

    pte_t *pte = vmm_get_pte(virt_addr, false);
    if (!pte || !(*pte & PTE_PRESENT)) {
        return false;
    }

    *pte = 0;
    vmm_state.mapped_pages--;

    vmm_flush_tlb_single(virt_addr);
    return true;
}

/**
 * Get physical address for virtual address
 */
uint64_t vmm_get_physical(uint64_t virt_addr) {
    uint64_t offset = virt_addr & 0xFFF;
    virt_addr = ALIGN_DOWN(virt_addr, PAGE_SIZE);

    pte_t *pte = vmm_get_pte(virt_addr, false);
    if (!pte || !(*pte & PTE_PRESENT)) {
        return 0;
    }

    return pte_get_addr(*pte) + offset;
}

/**
 * Map a range of pages
 */
bool vmm_map_range(uint64_t virt_addr, uint64_t phys_addr, uint64_t size, uint64_t flags) {
    uint64_t virt_end = virt_addr + size;

    // Align start down and end up
    virt_addr = ALIGN_DOWN(virt_addr, PAGE_SIZE);
    phys_addr = ALIGN_DOWN(phys_addr, PAGE_SIZE);
    virt_end = ALIGN_UP(virt_end, PAGE_SIZE);

    for (uint64_t v = virt_addr, p = phys_addr; v < virt_end; v += PAGE_SIZE, p += PAGE_SIZE) {
        if (!vmm_map_page(v, p, flags)) {
            return false;
        }
    }

    return true;
}

/**
 * Unmap a range of pages
 */
bool vmm_unmap_range(uint64_t virt_addr, uint64_t size) {
    uint64_t virt_end = virt_addr + size;

    virt_addr = ALIGN_DOWN(virt_addr, PAGE_SIZE);
    virt_end = ALIGN_UP(virt_end, PAGE_SIZE);

    for (uint64_t v = virt_addr; v < virt_end; v += PAGE_SIZE) {
        vmm_unmap_page(v);
    }

    return true;
}

/**
 * Initialize Virtual Memory Manager
 */
void vmm_init(boot_info_t *boot_info) {
    serial_debug_str("vmm_init_start\n");
    serial_debug_str("vmm_console_print_skipped\n");
    serial_debug_str("after_vmm_console_print\n");

    // DEBUG: Before allocation
    serial_debug_str("before_alloc_pml4_msg\n");
    serial_debug_str("alloc_pml4_msg_skipped\n");
    serial_debug_str("after_alloc_pml4_msg\n");

    // Allocate PML4 (top-level page table)
    serial_debug_str("calling_pmm_alloc_frame\n");
    uint64_t pml4_phys = pmm_alloc_frame();
    serial_debug_str("after_pmm_alloc_frame\n");

    // DEBUG: After allocation
    serial_debug_str("before_pmm_result_msg\n");
    serial_debug_str("pmm_result_msg_skipped\n");
    serial_debug_str("after_pmm_result_msg\n");

    if (pml4_phys == 0) {
        serial_debug_str("pml4_alloc_failed\n");
        console_print("[VMM] ERROR: Failed to allocate PML4\n");
        return;
    }

    serial_debug_str("before_pml4_setup\n");
    kernel_pml4 = (page_table_t*)pml4_phys;
    vmm_state.pml4_physical = pml4_phys;
    vmm_state.page_tables_allocated = 1;
    serial_debug_str("after_pml4_setup\n");

    // DEBUG: Before zeroing
    serial_debug_str("before_pml4_zero_msg\n");
    serial_debug_str("pml4_zero_msg_skipped\n");
    serial_debug_str("after_pml4_zero_msg\n");

    // Zero out PML4
    serial_debug_str("before_pml4_zero_loop\n");
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        kernel_pml4->entries[i] = 0;
    }
    serial_debug_str("after_pml4_zero_loop\n");

    // DEBUG: After zeroing
    serial_debug_str("before_zeroed_msg\n");
    serial_debug_str("pml4_zeroed_msg_skipped\n");
    serial_debug_str("after_zeroed_msg\n");

    serial_debug_str("before_pml4_print\n");
    serial_debug_str("pml4_print_skipped\n");
    serial_debug_str("after_pml4_print\n");

    serial_debug_str("before_vmm_init_flag\n");
    vmm_initialized = true;
    serial_debug_str("after_vmm_init_flag\n");

    // DEBUG: VMM initialized flag set
    serial_debug_str("before_init_flag_msg\n");
    serial_debug_str("init_flag_msg_skipped\n");
    serial_debug_str("after_init_flag_msg\n");

    // Identity map first 16MB (all available physical memory)
    // This allows page tables allocated by PMM to be accessible
    // since PMM can allocate from anywhere in physical memory
    serial_debug_str("before_identity_map_msg\n");
    serial_debug_str("identity_map_msg_skipped\n");
    serial_debug_str("after_identity_map_msg\n");
    serial_debug_str("before_vmm_map_range_call\n");
    if (!vmm_map_range(0x0, 0x0, 16 * 1024 * 1024, PTE_KERNEL_FLAGS)) {
        serial_debug_str("identity_map_failed\n");
        console_print("[VMM] ERROR: Failed to identity map\n");
        return;
    }
    serial_debug_str("after_vmm_map_range_call\n");
    serial_debug_str("identity_complete_msg_skipped\n");
    serial_debug_str("after_identity_complete_msg\n");
    vmm_state.kernel_pages += 4096; // 16MB = 4096 pages
    serial_debug_str("after_kernel_pages_update\n");

    // Map kernel to higher-half (if boot_info available)
    serial_debug_str("before_boot_info_check\n");
    if (boot_info && boot_info->magic == AURORA_BOOT_MAGIC) {
        serial_debug_str("boot_info_valid\n");
        uint64_t kernel_size = boot_info->kernel_size;
        uint64_t kernel_phys = boot_info->kernel_physical_base;
        uint64_t kernel_virt = KERNEL_VIRTUAL_BASE;

        serial_debug_str("before_kernel_map_msg\n");
        serial_debug_str("kernel_map_msg_skipped\n");
        serial_debug_str("after_kernel_map_msg\n");

        serial_debug_str("before_kernel_map_range\n");
        if (!vmm_map_range(kernel_virt, kernel_phys, kernel_size, PTE_KERNEL_FLAGS)) {
            serial_debug_str("kernel_map_failed\n");
            console_print("[VMM] ERROR: Failed to map kernel\n");
            return;
        }
        serial_debug_str("after_kernel_map_range\n");

        uint64_t kernel_pages = (kernel_size + PAGE_SIZE - 1) / PAGE_SIZE;
        vmm_state.kernel_pages += kernel_pages;
        serial_debug_str("kernel_map_complete\n");
    } else {
        serial_debug_str("boot_info_invalid_test_mode\n");
        // Test mode: Map kernel at 1MB
        serial_debug_str("test_mode_msg_skipped\n");
        uint64_t kernel_size = 1024 * 1024; // Assume 1MB kernel
        serial_debug_str("before_test_map_range\n");
        if (!vmm_map_range(KERNEL_PHYSICAL_BASE, KERNEL_PHYSICAL_BASE,
                          kernel_size, PTE_KERNEL_FLAGS)) {
            serial_debug_str("test_map_failed\n");
            console_print("[VMM] ERROR: Failed to map kernel\n");
            return;
        }
        serial_debug_str("after_test_map_range\n");
        vmm_state.kernel_pages += 256; // 1MB = 256 pages
    }

    // Setup recursive mapping (last PML4 entry points to itself)
    serial_debug_str("before_recursive_map_msg\n");
    serial_debug_str("recursive_map_msg_skipped\n");
    serial_debug_str("after_recursive_map_msg\n");
    serial_debug_str("before_recursive_map_set\n");
    kernel_pml4->entries[RECURSIVE_SLOT] = pte_create(pml4_phys, PTE_KERNEL_FLAGS);
    serial_debug_str("after_recursive_map_set\n");

    serial_debug_str("before_vmm_complete_msg\n");
    serial_debug_str("vmm_complete_msg_skipped\n");
    serial_debug_str("after_vmm_complete_msg\n");
    serial_debug_str("vmm_stats_skipped\n");
    serial_debug_str("vmm_init_complete\n");
}

/**
 * Print VMM statistics
 */
void vmm_print_stats(void) {
    console_print("\n[VMM] Statistics:\n");
    console_print("  PML4 Physical:     ");
    console_print_hex(vmm_state.pml4_physical);
    console_print("\n  Page Tables:       ");
    console_print_dec(vmm_state.page_tables_allocated);
    console_print("\n  Total Pages:       ");
    console_print_dec(vmm_state.mapped_pages);
    console_print("\n  Kernel Pages:      ");
    console_print_dec(vmm_state.kernel_pages);
    console_print("\n  Virtual Memory:    ");
    console_print_dec(vmm_state.mapped_pages * 4);
    console_print(" KB\n");
}
