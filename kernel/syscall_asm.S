/**
 * AuroraOS Kernel - System Call Entry Point
 *
 * Assembly glue for SYSCALL/SYSRET instructions
 */

.section .text
.code64

#
# void syscall_entry(void);
#
# Entry point for SYSCALL instruction
# This is loaded into the LSTAR MSR
#
# On entry:
#   RAX = syscall number
#   RDI = arg1
#   RSI = arg2
#   RDX = arg3
#   R10 = arg4 (R10 instead of RCX because SYSCALL uses RCX)
#   R8  = arg5
#   R9  = arg6
#   RCX = return RIP (saved by SYSCALL instruction)
#   R11 = return RFLAGS (saved by SYSCALL instruction)
#
.global syscall_entry
syscall_entry:
    # SYSCALL instruction has already:
    # - Saved user RIP to RCX
    # - Saved user RFLAGS to R11
    # - Loaded RIP from LSTAR MSR (jumped here)
    # - Loaded CS from STAR[47:32]
    # - Loaded SS from STAR[47:32] + 8
    # - Cleared RFLAGS bits according to SFMASK (disabled interrupts)

    # Save return context (RCX and R11 are clobbered by function calls)
    pushq %rcx              # Save return RIP
    pushq %r11              # Save return RFLAGS

    # Save other registers that might be clobbered
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # Prepare arguments for syscall_handler
    # C function signature:
    #   int64_t syscall_handler(uint64_t syscall_num, uint64_t arg1,
    #                          uint64_t arg2, uint64_t arg3,
    #                          uint64_t arg4, uint64_t arg5, uint64_t arg6)
    #
    # System V ABI calling convention:
    #   RDI = syscall_num
    #   RSI = arg1
    #   RDX = arg2
    #   RCX = arg3
    #   R8  = arg4
    #   R9  = arg5
    #   [rsp] = arg6
    #
    # Current register state:
    #   RAX = syscall_num
    #   RDI = arg1
    #   RSI = arg2
    #   RDX = arg3
    #   R10 = arg4
    #   R8  = arg5
    #   R9  = arg6
    #
    # Need to shuffle arguments (do in reverse order to avoid clobbering)
    pushq %r9               # arg6 on stack
    movq %r8, %r9           # arg5: R8 -> R9
    movq %r10, %r8          # arg4: R10 -> R8
    movq %rdx, %rcx         # arg3: RDX -> RCX
    movq %rsi, %rdx         # arg2: RSI -> RDX
    movq %rdi, %rsi         # arg1: RDI -> RSI
    movq %rax, %rdi         # syscall_num: RAX -> RDI

    # Call C handler
    call syscall_handler

    # Return value is in RAX
    # Clean up arg6 from stack
    addq $8, %rsp

    # Restore callee-saved registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx

    # Restore return context
    popq %r11               # Restore RFLAGS for SYSRET
    popq %rcx               # Restore RIP for SYSRET

    # Return to caller using SYSRET
    # SYSRET will:
    # - Load RIP from RCX
    # - Load RFLAGS from R11
    # - Load CS from STAR[63:48] + 16
    # - Load SS from STAR[63:48] + 8
    # - Set RFLAGS.IF
    sysretq
