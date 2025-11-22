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

    /* DEBUG: Write 'A' to VGA (we reached _start) */
    movl $0xB8000, %edi
    movb $'A', (%edi)
    movb $0x0F, 1(%edi)

    /* Save Multiboot2 info */
    /* Use absolute addressing - load address to register first */
    movl $multiboot_info_ptr, %edi
    movl %ebx, (%edi)

    /* DEBUG: Write 'B' to VGA (saved multiboot info) */
    movl $0xB8000, %edi
    movb $'B', 2(%edi)
    movb $0x0F, 3(%edi)

    /* Setup initial stack (16KB) */
    movl $stack_top, %esp
    movl $0, %ebp

    /* Clear EFLAGS */
    pushl $0
    popf

    /* DEBUG: Write 'C' to VGA (about to check long mode) */
    movl $0xB8000, %edi
    movb $'C', 4(%edi)
    movb $0x0F, 5(%edi)

    /* Check for long mode support */
    call check_long_mode
    testl %eax, %eax
    jz .no_long_mode

    /* DEBUG: Write 'D' to VGA (long mode supported) */
    movl $0xB8000, %edi
    movb $'D', 6(%edi)
    movb $0x0F, 7(%edi)

    /* Setup paging for long mode */
    call setup_page_tables

    /* DEBUG: Write 'E' to VGA (page tables setup) */
    movl $0xB8000, %edi
    movb $'E', 8(%edi)
    movb $0x0F, 9(%edi)

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

    /* DEBUG: Write 'F' to VGA (about to enable paging) */
    movl $0xB8000, %edi
    movb $'F', 10(%edi)
    movb $0x0F, 11(%edi)

    /* Enable paging and protected mode */
    movl %cr0, %eax
    orl $0x80000001, %eax /* Set PG and PE bits */
    movl %eax, %cr0

    /* DEBUG: Write 'G' to VGA (paging enabled) */
    movl $0xB8000, %edi
    movb $'G', 12(%edi)
    movb $0x0F, 13(%edi)

    /* Load 64-bit GDT */
    /* Load address into register first, then use register-indirect addressing */
    movl $gdt64_pointer, %eax
    lgdt (%eax)

    /* DEBUG: Write 'H' to VGA (GDT loaded) */
    movl $0xB8000, %edi
    movb $'H', 14(%edi)
    movb $0x0F, 15(%edi)

    /* Jump to 64-bit code */
    /* Manual far jump encoding to avoid assembler issues */
    /* Opcode: EA <offset:4 bytes> <selector:2 bytes> */
    .byte 0xEA
    .long long_mode_start
    .word 0x08

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
    /* DEBUG: Write 'I' to VGA (in 64-bit mode!) */
    movq $0xB8000, %rdi
    movb $'I', 16(%rdi)
    movb $0x0F, 17(%rdi)

    /* Load 64-bit data segment */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    /* DEBUG: Write 'J' to VGA (segments loaded) */
    movq $0xB8000, %rdi
    movb $'J', 18(%rdi)
    movb $0x0F, 19(%rdi)

    /* Setup 64-bit stack */
    movq $stack_top, %rsp
    xorq %rbp, %rbp

    /* DEBUG: Write 'K' to VGA (stack ready) */
    movq $0xB8000, %rdi
    movb $'K', 20(%rdi)
    movb $0x0F, 21(%rdi)

    /* Prepare argument for kernel_main */
    /* RDI = boot_info pointer (NULL for now, we'll parse Multiboot2 later) */
    xorq %rdi, %rdi

    /* DEBUG: Write 'L' to VGA (calling kernel_main) */
    movq $0xB8000, %rdi
    movb $'L', 22(%rdi)
    movb $0x0F, 23(%rdi)
    xorq %rdi, %rdi  /* restore RDI */

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
