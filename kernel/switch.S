/**
 * AuroraOS Kernel - Context Switch Assembly
 *
 * Low-level context switching for x86_64
 */

.section .text
.code64

#
# void switch_context(cpu_context_t *old_ctx, cpu_context_t *new_ctx);
#
# RDI = old_ctx (pointer to save current context)
# RSI = new_ctx (pointer to load new context)
#
.global switch_context
switch_context:
    # Save current context to old_ctx
    # Skip if old_ctx is NULL
    testq %rdi, %rdi
    jz load_new_context

    # Save general purpose registers
    movq %rax, 0(%rdi)
    movq %rbx, 8(%rdi)
    movq %rcx, 16(%rdi)
    movq %rdx, 24(%rdi)
    movq %rsi, 32(%rdi)
    movq %rdi, 40(%rdi)    # Save RDI itself
    movq %rbp, 48(%rdi)
    movq %rsp, 56(%rdi)
    movq %r8,  64(%rdi)
    movq %r9,  72(%rdi)
    movq %r10, 80(%rdi)
    movq %r11, 88(%rdi)
    movq %r12, 96(%rdi)
    movq %r13, 104(%rdi)
    movq %r14, 112(%rdi)
    movq %r15, 120(%rdi)

    # Save RIP (return address from stack)
    movq (%rsp), %rax
    movq %rax, 128(%rdi)

    # Save RFLAGS
    pushfq
    popq %rax
    movq %rax, 136(%rdi)

    # Save segment registers
    movq %cs, %rax
    movq %rax, 144(%rdi)
    movq %ss, %rax
    movq %rax, 152(%rdi)
    movq %ds, %rax
    movq %rax, 160(%rdi)
    movq %es, %rax
    movq %rax, 168(%rdi)
    movq %fs, %rax
    movq %rax, 176(%rdi)
    movq %gs, %rax
    movq %rax, 184(%rdi)

load_new_context:
    # Load new context from new_ctx
    # RSI already contains new_ctx pointer

    # Restore general purpose registers
    movq 0(%rsi), %rax
    movq 8(%rsi), %rbx
    movq 16(%rsi), %rcx
    movq 24(%rsi), %rdx
    # Skip RSI and RDI for now
    movq 48(%rsi), %rbp
    movq 56(%rsi), %rsp
    movq 64(%rsi), %r8
    movq 72(%rsi), %r9
    movq 80(%rsi), %r10
    movq 88(%rsi), %r11
    movq 96(%rsi), %r12
    movq 104(%rsi), %r13
    movq 112(%rsi), %r14
    movq 120(%rsi), %r15

    # Restore RFLAGS
    movq 136(%rsi), %rax
    pushq %rax
    popfq

    # Restore segment registers (DS, ES, FS, GS)
    # CS and SS are handled by iretq
    movq 160(%rsi), %rax
    movw %ax, %ds
    movq 168(%rsi), %rax
    movw %ax, %es
    movq 176(%rsi), %rax
    movw %ax, %fs
    movq 184(%rsi), %rax
    movw %ax, %gs

    # Push return context for iretq
    # Stack layout for iretq: SS, RSP, RFLAGS, CS, RIP
    movq 152(%rsi), %rax   # SS
    pushq %rax
    movq 56(%rsi), %rax    # RSP
    pushq %rax
    movq 136(%rsi), %rax   # RFLAGS
    pushq %rax
    movq 144(%rsi), %rax   # CS
    pushq %rax
    movq 128(%rsi), %rax   # RIP
    pushq %rax

    # Restore RSI and RDI last
    movq 40(%rsi), %rdi
    movq 32(%rsi), %rsi

    # Return to new context
    iretq
