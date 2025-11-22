/* AuroraOS Kernel Entry Point */
/* Called from GRUB Multiboot2 after loading kernel */
/*
 * Entry conditions:
 * - 32-bit protected mode
 * - Paging disabled
 * - EAX = Multiboot2 magic (0x36d76289)
 * - EBX = Physical address of Multiboot2 info structure
 */

.section .text
.code32

.global _start
.extern kernel_main

_start:
    /* Disable interrupts */
    cli

    /* Save Multiboot2 info */
    movl %ebx, multiboot_info_ptr

    /* Setup initial stack (16KB) */
    movl $stack_top, %esp
    movl $0, %ebp

    /* Clear EFLAGS */
    pushl $0
    popf

    /* Check for long mode support */
    call check_long_mode
    testl %eax, %eax
    jz .no_long_mode

    /* Setup paging for long mode */
    call setup_page_tables

    /* Enable PAE (Physical Address Extension) */
    movl %cr4, %eax
    orl $0x20, %eax       /* Set PAE bit */
    movl %eax, %cr4

    /* Load CR3 with PML4 address */
    movl $pml4_table, %eax
    movl %eax, %cr3

    /* Enable long mode in EFER MSR */
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax      /* Set LM bit */
    wrmsr

    /* Enable paging and protected mode */
    movl %cr0, %eax
    orl $0x80000001, %eax /* Set PG and PE bits */
    movl %eax, %cr0

    /* Load 64-bit GDT */
    lgdt (gdt64_pointer)

    /* Jump to 64-bit code */
    ljmp $0x08, $long_mode_start

.no_long_mode:
    /* Halt if no long mode support */
    cli
    hlt
    jmp .no_long_mode

/* Check if CPU supports long mode */
check_long_mode:
    /* Check for CPUID support */
    pushfl
    popl %eax
    movl %eax, %ecx
    xorl $0x200000, %eax
    pushl %eax
    popfl
    pushfl
    popl %eax
    pushl %ecx
    popfl
    xorl %ecx, %eax
    jz .no_cpuid

    /* Check for extended CPUID */
    movl $0x80000000, %eax
    cpuid
    cmpl $0x80000001, %eax
    jb .no_cpuid

    /* Check for long mode */
    movl $0x80000001, %eax
    cpuid
    testl $0x20000000, %edx  /* Long mode bit */
    jz .no_cpuid

    movl $1, %eax
    ret

.no_cpuid:
    movl $0, %eax
    ret

/* Setup page tables for identity mapping */
setup_page_tables:
    /* Clear page tables (3 tables x 4KB each = 12KB total) */
    movl $pml4_table, %edi
    movl $0, %eax
    movl $3072, %ecx      /* 3 * 4096 / 4 = 3072 dwords */
    rep stosl

    /* Setup PML4: PML4[0] -> PDPT */
    movl $pml4_table, %edi
    movl $pdp_table, %eax
    orl $0x03, %eax       /* Present, writable */
    movl %eax, (%edi)

    /* Setup PDPT: PDPT[0] -> PD */
    movl $pdp_table, %edi
    movl $pd_table, %eax
    orl $0x03, %eax       /* Present, writable */
    movl %eax, (%edi)

    /* Setup PD: Map first 2MB with huge pages */
    movl $pd_table, %edi
    movl $0x00000083, %eax  /* 0x00000000, present, writable, huge page */
    movl $512, %ecx
.map_pd:
    movl %eax, (%edi)
    addl $0x200000, %eax    /* Next 2MB */
    addl $8, %edi
    loop .map_pd

    ret

/* 64-bit code */
.code64
long_mode_start:
    /* Load 64-bit data segment */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    /* Setup 64-bit stack */
    movq $stack_top, %rsp
    xorq %rbp, %rbp

    /* Prepare argument for kernel_main */
    /* RDI = boot_info pointer (NULL for now, we'll parse Multiboot2 later) */
    xorq %rdi, %rdi

    /* Call kernel main */
    call kernel_main

    /* Halt if kernel_main returns */
.halt:
    cli
    hlt
    jmp .halt

/* Temporary GDT for 64-bit mode */
.section .data
.align 16
gdt64:
    .quad 0x0000000000000000    /* Null descriptor */
    .quad 0x00AF9A000000FFFF    /* Code segment (64-bit) */
    .quad 0x00AF92000000FFFF    /* Data segment (64-bit) */
gdt64_end:

gdt64_pointer:
    .word gdt64_end - gdt64 - 1
    .long gdt64           /* Use .long (4 bytes) for 32-bit mode lgdt */
    .word 0               /* Padding to align if needed */

/* Multiboot info pointer storage */
.section .bss
.align 4
multiboot_info_ptr:
    .long 0

/* Stack */
.align 16
stack_bottom:
    .space 16384  /* 16KB stack */
stack_top:

/* Page tables (identity mapping for first 1GB) */
.align 4096
pml4_table:
    .space 4096
pdp_table:
    .space 4096
pd_table:
    .space 4096
