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

// Static page table buffers for initial identity mapping
// These break the chicken-and-egg problem: we need page tables to set up
// identity mapping, but we need identity mapping to access PMM-allocated page tables.
// After initial setup with these static buffers, we can use PMM normally.
// NOTE: We use 2MB huge pages (PD level), so NO PT arrays needed! Saves 32KB and 4096 loop iterations
static page_table_t static_pml4 __attribute__((aligned(4096)));
static page_table_t static_pdpt __attribute__((aligned(4096)));
static page_table_t static_pd __attribute__((aligned(4096)));

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
        // DISABLED: Zeroing intermediate tables causes stack corruption
        // Only PT (leaf tables) need zeroing, intermediate PDPT is okay with garbage
        serial_debug_str("before_pdpt_zero\n");
        pdpt = (page_table_t*)pdpt_phys;
        serial_debug_str("pdpt_zero_skipped\n");
        // for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        //     pdpt->entries[i] = 0;
        // }
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
        // DISABLED: Zeroing intermediate tables causes stack corruption
        // Only PT (leaf tables) need zeroing, intermediate PD is okay with garbage
        serial_debug_str("before_pd_zero\n");
        pd = (page_table_t*)pd_phys;
        serial_debug_str("pd_zero_skipped\n");
        // serial_debug_str("pd_zero_loop_start\n");
        // for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        //     pd->entries[i] = 0;
        // }
        // serial_debug_str("pd_zero_loop_end\n");
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
        // DISABLED: 512-iteration loop causes stack overflow!
        // PMM should ideally return zeroed pages, but for now we accept garbage in unused entries.
        // Only entries we explicitly map will be set, unmapped entries remain garbage (but not accessed).
        serial_debug_str("before_pt_zero\n");
        pt = (page_table_t*)pt_phys;
        serial_debug_str("pt_zero_skipped\n");
        // for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        //     pt->entries[i] = 0;
        // }
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
    serial_debug_str("using_static_page_tables\n");

    // PHASE 1: Set up initial identity mapping using STATIC page table buffers
    // This solves the chicken-and-egg problem: we need page tables to create
    // identity mapping, but PMM-allocated page tables need identity mapping to be accessed!

    // NOTE: Static buffers are in .bss section, bootloader already zeroed them!
    // NO need to zero again - just use them directly. This eliminates 1536 loop iterations!
    serial_debug_str("static_buffers_already_zero\n");

    // Get physical addresses of static buffers (they're in kernel .bss)
    serial_debug_str("get_static_addrs\n");
    uint64_t pml4_phys = (uint64_t)&static_pml4;
    uint64_t pdpt_phys = (uint64_t)&static_pdpt;
    uint64_t pd_phys = (uint64_t)&static_pd;
    serial_debug_str("got_static_addrs\n");

    // Build identity mapping manually for first 16MB (0x0 - 0xFFFFFF)
    // Using 2MB HUGE PAGES to avoid PT arrays and 4096-iteration loops!
    // Structure: PML4[0] -> PDPT[0] -> PD[0-7] (each PD entry = 2MB huge page)

    // PML4[0] -> PDPT
    serial_debug_str("setup_pml4_entry\n");
    static_pml4.entries[0] = pte_create(pdpt_phys, PTE_PRESENT | PTE_WRITE);

    // PDPT[0] -> PD
    serial_debug_str("setup_pdpt_entry\n");
    static_pdpt.entries[0] = pte_create(pd_phys, PTE_PRESENT | PTE_WRITE);

    // PD[0-7] = 2MB huge pages (0-16MB)
    // Each PD entry points directly to a 2MB physical region (no PT needed!)
    serial_debug_str("setup_huge_pages\n");

    // Unroll loop manually to debug which write fails
    serial_debug_str("hp0\n");
    static_pd.entries[0] = pte_create(0 * 2 * 1024 * 1024, PTE_PRESENT | PTE_WRITE | PTE_HUGE);
    serial_debug_str("hp1\n");
    static_pd.entries[1] = pte_create(1 * 2 * 1024 * 1024, PTE_PRESENT | PTE_WRITE | PTE_HUGE);
    serial_debug_str("hp2\n");
    static_pd.entries[2] = pte_create(2 * 2 * 1024 * 1024, PTE_PRESENT | PTE_WRITE | PTE_HUGE);
    serial_debug_str("hp3\n");
    static_pd.entries[3] = pte_create(3 * 2 * 1024 * 1024, PTE_PRESENT | PTE_WRITE | PTE_HUGE);
    serial_debug_str("hp4\n");
    static_pd.entries[4] = pte_create(4 * 2 * 1024 * 1024, PTE_PRESENT | PTE_WRITE | PTE_HUGE);
    serial_debug_str("hp5\n");
    static_pd.entries[5] = pte_create(5 * 2 * 1024 * 1024, PTE_PRESENT | PTE_WRITE | PTE_HUGE);
    serial_debug_str("hp6\n");
    static_pd.entries[6] = pte_create(6 * 2 * 1024 * 1024, PTE_PRESENT | PTE_WRITE | PTE_HUGE);
    serial_debug_str("hp7\n");
    static_pd.entries[7] = pte_create(7 * 2 * 1024 * 1024, PTE_PRESENT | PTE_WRITE | PTE_HUGE);
    serial_debug_str("huge_pages_setup\n");

    // Set up VMM state
    kernel_pml4 = &static_pml4;
    vmm_state.pml4_physical = pml4_phys;
    vmm_state.page_tables_allocated = 3; // PML4 + PDPT + PD (no PTs!)
    vmm_state.kernel_pages = 4096; // 16MB = 4096 pages
    vmm_initialized = true;
    serial_debug_str("vmm_state_initialized\n");

    // Load the new page tables (activate identity mapping)
    serial_debug_str("loading_cr3\n");
    vmm_load_cr3(pml4_phys);
    serial_debug_str("cr3_loaded\n");

    console_print("[VMM] Identity mapping active (16MB)\n");

    // PHASE 2: Now we can use vmm_map_range for additional mappings
    // since identity mapping is active and PMM allocations are accessible

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
