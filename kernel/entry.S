/**
 * AuroraOS Kernel Entry Point (GNU Assembler syntax)
 * Called from bootloader after UEFI exits boot services
 *
 * Entry conditions:
 * - Long mode (64-bit) enabled
 * - Paging enabled with identity mapping
 * - RDI = pointer to boot_info structure
 * - RSI-R15 = undefined
 * - Stack = valid but minimal
 */

.section .text
.code64

.global _start
.extern kernel_main

_start:
    /* Disable interrupts (should already be disabled) */
    cli

    /* Save boot info pointer (RDI is first argument in System V ABI) */
    /* kernel_main will receive it automatically */

    /* Setup stack (use a temporary stack for now) */
    lea stack_top(%rip), %rsp
    xor %rbp, %rbp

    /* Clear RFLAGS */
    pushq $0
    popfq

    /* Call kernel main (boot_info pointer already in RDI) */
    call kernel_main

    /* If kernel_main returns (it shouldn't), halt */
.halt:
    cli
    hlt
    jmp .halt

.section .bss
.align 16
stack_bottom:
    .skip 16384  /* 16KB stack */
stack_top:
