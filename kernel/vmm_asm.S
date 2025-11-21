/**
 * AuroraOS Kernel - VMM Assembly Functions
 *
 * Low-level functions for managing paging and CR3 register
 */

.section .text
.code64

# Load CR3 register with PML4 physical address
# void vmm_load_cr3(uint64_t pml4_phys);
.global vmm_load_cr3
vmm_load_cr3:
    # RDI contains pml4_phys (first argument)
    movq %rdi, %cr3
    ret

# Get current CR3 value
# uint64_t vmm_get_cr3(void);
.global vmm_get_cr3
vmm_get_cr3:
    movq %cr3, %rax
    ret

# Enable paging (set CR0.PG bit)
# void vmm_enable_paging(void);
.global vmm_enable_paging
vmm_enable_paging:
    movq %cr0, %rax
    btsq $31, %rax           # Set bit 31 (PG bit)
    movq %rax, %cr0
    ret

# Flush entire TLB by reloading CR3
# void vmm_flush_tlb_all(void);
.global vmm_flush_tlb_all
vmm_flush_tlb_all:
    movq %cr3, %rax
    movq %rax, %cr3
    ret
