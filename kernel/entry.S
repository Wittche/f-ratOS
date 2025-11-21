/**
 * AuroraOS Kernel Entry Point (GNU Assembler syntax)
 * Supports both UEFI and Multiboot2 boot
 *
 * Multiboot2 entry conditions:
 * - 32-bit protected mode
 * - EAX = 0x36d76289 (Multiboot2 magic)
 * - EBX = pointer to Multiboot2 information structure
 *
 * UEFI entry conditions:
 * - Long mode (64-bit) enabled
 * - RDI = pointer to boot_info structure
 */

.section .text

/* 32-bit entry point for Multiboot2 */
.code32
.global _start
.extern kernel_main

_start:
    /* Disable interrupts */
    cli

    /* Check if we're already in 64-bit mode (UEFI boot) */
    /* If coming from Multiboot2, we're in 32-bit mode */

    /* Setup temporary GDT for 64-bit mode */
    lgdt gdt64_pointer

    /* Enable PAE (Physical Address Extension) */
    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    /* Setup page tables for identity mapping */
    /* PML4[0] -> PDPT */
    mov $pdpt, %eax
    or $0x3, %eax  /* Present + Writable */
    mov %eax, pml4

    /* PDPT[0] -> PD */
    mov $pd, %eax
    or $0x3, %eax
    mov %eax, pdpt

    /* PD[0] = 2MB page at 0x0 */
    mov $0x83, %eax  /* Present + Writable + Huge Page */
    mov %eax, pd

    /* Load PML4 address into CR3 */
    mov $pml4, %eax
    mov %eax, %cr3

    /* Enable long mode in EFER MSR */
    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax  /* Set LME bit */
    wrmsr

    /* Enable paging */
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    /* Now in compatibility mode, jump to 64-bit code */
    ljmp $0x08, $start64

/* 64-bit entry point */
.code64
start64:
    /* Setup data segments */
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* Setup stack */
    lea stack_top(%rip), %rsp
    xor %rbp, %rbp

    /* Clear RFLAGS */
    pushq $0
    popfq

    /* boot_info pointer is in RDI (or pass NULL for Multiboot2) */
    xor %rdi, %rdi

    /* Call kernel main */
    call kernel_main

    /* If kernel_main returns, halt */
.halt:
    cli
    hlt
    jmp .halt

/* Temporary GDT for 64-bit mode */
.align 16
gdt64:
    .quad 0x0000000000000000    /* Null descriptor */
    .quad 0x00209A0000000000    /* Code segment (64-bit) */
    .quad 0x0000920000000000    /* Data segment */
gdt64_end:

gdt64_pointer:
    .word gdt64_end - gdt64 - 1
    .quad gdt64

/* Page tables (4KB each, aligned) */
.align 4096
pml4:
    .skip 4096
pdpt:
    .skip 4096
pd:
    .skip 4096

/* Stack */
.section .bss
.align 16
stack_bottom:
    .skip 16384  /* 16KB stack */
stack_top:
