/**
 * AuroraOS Kernel - User Mode Support Implementation
 *
 * Ring 0 to Ring 3 privilege level transition
 */

#include "usermode.h"
#include "console.h"
#include "kheap.h"
#include "tss.h"
#include "gdt.h"

/**
 * Jump to user mode using IRETQ
 *
 * IRETQ stack frame (from bottom to top):
 *   SS      - User data segment
 *   RSP     - User stack pointer
 *   RFLAGS  - Flags (with IF set)
 *   CS      - User code segment
 *   RIP     - Entry point
 *
 * This function is implemented in assembly to have precise control
 * over the stack and registers.
 */
__attribute__((naked))
void jump_to_usermode(void (*entry_point)(void) __attribute__((unused)),
                      uint64_t user_stack __attribute__((unused))) {
    __asm__ __volatile__(
        // RDI = entry_point (from caller)
        // RSI = user_stack (from caller)

        // Clear all general purpose registers (for security)
        "xor %%rax, %%rax\n"
        "xor %%rbx, %%rbx\n"
        "xor %%rcx, %%rcx\n"
        "xor %%rdx, %%rdx\n"
        // RDI and RSI will be cleared after we use them
        "xor %%rbp, %%rbp\n"
        "xor %%r8, %%r8\n"
        "xor %%r9, %%r9\n"
        "xor %%r10, %%r10\n"
        "xor %%r11, %%r11\n"
        "xor %%r12, %%r12\n"
        "xor %%r13, %%r13\n"
        "xor %%r14, %%r14\n"
        "xor %%r15, %%r15\n"

        // Set up IRETQ stack frame
        // Push SS (user data segment with RPL=3)
        "push %[user_ds]\n"

        // Push RSP (user stack)
        "push %%rsi\n"

        // Push RFLAGS (0x202 = IF flag set, reserved bit 1 always set)
        "push $0x202\n"

        // Push CS (user code segment with RPL=3)
        "push %[user_cs]\n"

        // Push RIP (entry point)
        "push %%rdi\n"

        // Clear RDI and RSI now that we've used them
        "xor %%rdi, %%rdi\n"
        "xor %%rsi, %%rsi\n"

        // Set data segment registers to user data segment
        "mov %[user_ds], %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"

        // Jump to user mode
        "iretq\n"

        : // No outputs
        : [user_cs] "i" (USER_CODE64_SELECTOR | 3),  // RPL = 3
          [user_ds] "i" (USER_DATA_SELECTOR | 3)      // RPL = 3
        : "memory"
    );
}

/**
 * Start a user mode process
 */
void start_usermode_process(void (*entry_point)(void)) {
    console_print("\n[USERMODE] Creating user mode process...\n");

    // Allocate user mode stack (64KB)
    uint64_t user_stack_size = 65536;
    void *user_stack_base = kmalloc(user_stack_size);
    if (!user_stack_base) {
        console_print("[USERMODE] ERROR: Failed to allocate user stack\n");
        return;
    }

    // Stack grows down, so pointer is at the top
    uint64_t user_stack = (uint64_t)user_stack_base + user_stack_size - 16;

    console_print("[USERMODE] User stack allocated: ");
    console_print_hex((uint64_t)user_stack_base);
    console_print(" - ");
    console_print_hex(user_stack);
    console_print("\n");

    // Allocate kernel stack for when we return from user mode (via syscall/interrupt)
    uint64_t kernel_stack_size = 8192;
    void *kernel_stack_base = kmalloc(kernel_stack_size);
    if (!kernel_stack_base) {
        console_print("[USERMODE] ERROR: Failed to allocate kernel stack\n");
        kfree(user_stack_base);
        return;
    }

    uint64_t kernel_stack = (uint64_t)kernel_stack_base + kernel_stack_size;

    console_print("[USERMODE] Kernel stack allocated: ");
    console_print_hex((uint64_t)kernel_stack_base);
    console_print(" - ");
    console_print_hex(kernel_stack);
    console_print("\n");

    // Set TSS kernel stack (for syscall/interrupt returns)
    tss_set_kernel_stack(kernel_stack);

    console_print("[USERMODE] TSS updated with kernel stack\n");
    console_print("[USERMODE] Entry point: ");
    console_print_hex((uint64_t)entry_point);
    console_print("\n");

    console_print("[USERMODE] Jumping to Ring 3...\n\n");
    console_print("=====================================\n");
    console_print("  USER MODE STARTING\n");
    console_print("=====================================\n\n");

    // Jump to user mode
    // This function NEVER returns
    jump_to_usermode(entry_point, user_stack);

    // Should never reach here
    console_print("[USERMODE] ERROR: Returned from user mode!\n");
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
