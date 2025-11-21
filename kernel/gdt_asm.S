/**
 * AuroraOS Kernel - GDT Assembly Functions
 *
 * Low-level GDT loading and segment register management
 */

.section .text
.code64

# Load GDT
# Takes pointer to gdt_ptr_t structure in RDI
.global gdt_load_asm
gdt_load_asm:
    lgdt (%rdi)
    ret

# Reload segment registers
# Must be called after loading GDT to update CS, DS, ES, SS
.global gdt_reload_segments
gdt_reload_segments:
    # Reload data segments (DS, ES, FS, GS, SS)
    # Use kernel data selector (0x10)
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Reload code segment (CS) using far return
    # We need to do a far return to change CS
    # Push new CS (0x08 = kernel code selector)
    pushq $0x08

    # Push return address (label .reload_cs)
    leaq .reload_cs(%rip), %rax
    pushq %rax

    # Far return - pops CS and RIP
    lretq

.reload_cs:
    # CS is now reloaded, return to caller
    ret
