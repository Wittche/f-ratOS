/**
 * AuroraOS Kernel - IDT Assembly Stubs
 *
 * Low-level interrupt handlers that save state and call C handlers
 */

.section .text
.code64

# Macro: ISR without error code
# CPU exceptions that don't push an error code need a dummy one
.macro ISR_NO_ERR num
.global isr\num
isr\num:
    pushq $0
    pushq $\num
    jmp isr_common_stub
.endm

# Macro: ISR with error code
# CPU exceptions that push an error code
.macro ISR_ERR num
.global isr\num
isr\num:
    pushq $\num
    jmp isr_common_stub
.endm

# Macro: IRQ handler
.macro IRQ num irq_num
.global irq\num
irq\num:
    pushq $0
    pushq $\irq_num
    jmp irq_common_stub
.endm

# CPU Exception stubs (0-31)
ISR_NO_ERR 0
ISR_NO_ERR 1
ISR_NO_ERR 2
ISR_NO_ERR 3
ISR_NO_ERR 4
ISR_NO_ERR 5
ISR_NO_ERR 6
ISR_NO_ERR 7
ISR_ERR    8
ISR_NO_ERR 9
ISR_ERR    10
ISR_ERR    11
ISR_ERR    12
ISR_ERR    13
ISR_ERR    14
ISR_NO_ERR 15
ISR_NO_ERR 16
ISR_ERR    17
ISR_NO_ERR 18
ISR_NO_ERR 19
ISR_NO_ERR 20
ISR_NO_ERR 21
ISR_NO_ERR 22
ISR_NO_ERR 23
ISR_NO_ERR 24
ISR_NO_ERR 25
ISR_NO_ERR 26
ISR_NO_ERR 27
ISR_NO_ERR 28
ISR_NO_ERR 29
ISR_ERR    30
ISR_NO_ERR 31

# Hardware IRQ stubs (32-47)
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

# Common ISR stub - saves state and calls exception handler
isr_common_stub:
    # Save all general-purpose registers
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Call C exception handler (RDI = first argument = stack pointer)
    movq %rsp, %rdi
    call exception_handler

    # Restore all general-purpose registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    # Remove interrupt number and error code
    addq $16, %rsp

    # Return from interrupt
    iretq

# Common IRQ stub - saves state and calls IRQ handler
irq_common_stub:
    # Save all general-purpose registers
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Call C IRQ handler (RDI = first argument = stack pointer)
    movq %rsp, %rdi
    call irq_handler

    # Restore all general-purpose registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax

    # Remove interrupt number and error code
    addq $16, %rsp

    # Return from interrupt
    iretq

# Load IDT function
.global idt_load_asm
idt_load_asm:
    lidt (%rdi)
    ret
