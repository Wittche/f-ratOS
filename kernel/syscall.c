/**
 * AuroraOS Kernel - System Call Implementation
 *
 * System call dispatcher and handlers
 */

#include "syscall.h"
#include "console.h"
#include "process.h"
#include "scheduler.h"
#include "timer.h"
#include "types.h"

// MSR (Model Specific Register) addresses for SYSCALL/SYSRET
#define MSR_STAR    0xC0000081  // Segment selectors
#define MSR_LSTAR   0xC0000082  // SYSCALL entry point (64-bit)
#define MSR_CSTAR   0xC0000083  // SYSCALL entry point (compatibility)
#define MSR_SFMASK  0xC0000084  // RFLAGS mask

// Syscall statistics
static struct {
    uint64_t total_syscalls;
    uint64_t syscall_counts[SYSCALL_MAX + 1];
    bool initialized;
} syscall_state = {0};

/**
 * Write MSR
 */
static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ __volatile__("wrmsr" :: "c"(msr), "a"(low), "d"(high));
}

/**
 * Read MSR
 */
static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

/**
 * sys_exit - Terminate calling process
 */
static int64_t sys_exit(uint64_t status, uint64_t arg2, uint64_t arg3,
                        uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;

    console_print("[SYSCALL] exit(");
    console_print_dec((int)status);
    console_print(")\n");

    process_exit((int)status);
    return 0;  // Never reached
}

/**
 * sys_write - Write to file descriptor
 */
static int64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count,
                         uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    (void)arg4; (void)arg5; (void)arg6;

    // Only support stdout/stderr for now
    if (fd != STDOUT_FILENO && fd != STDERR_FILENO) {
        return -EBADF;
    }

    if (!buf) {
        return -EINVAL;
    }

    // Write to console
    const char *str = (const char*)buf;
    for (uint64_t i = 0; i < count; i++) {
        char ch[2] = {str[i], '\0'};
        console_print(ch);
    }

    return (int64_t)count;
}

/**
 * sys_read - Read from file descriptor
 */
static int64_t sys_read(uint64_t fd, uint64_t buf, uint64_t count,
                        uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    (void)arg4; (void)arg5; (void)arg6;

    // Only support stdin for now
    if (fd != STDIN_FILENO) {
        return -EBADF;
    }

    if (!buf) {
        return -EINVAL;
    }

    // TODO: Implement proper keyboard input buffering
    (void)count;  // Suppress unused warning
    return -ENOSYS;  // Not implemented yet
}

/**
 * sys_getpid - Get process ID
 */
static int64_t sys_getpid(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                          uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;

    process_t *proc = process_get_current();
    if (proc) {
        return (int64_t)proc->pid;
    }
    return -1;
}

/**
 * sys_yield - Yield CPU to another thread
 */
static int64_t sys_yield(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                         uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;

    scheduler_yield();
    return 0;
}

/**
 * sys_sleep - Sleep for milliseconds
 */
static int64_t sys_sleep(uint64_t milliseconds, uint64_t arg2, uint64_t arg3,
                         uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;

    timer_sleep((uint32_t)milliseconds);
    return 0;
}

/**
 * Unimplemented syscall handler
 */
static int64_t sys_unimplemented(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                 uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;

    console_print("[SYSCALL] ERROR: Unimplemented syscall\n");
    return -ENOSYS;
}

/**
 * System call table
 */
static syscall_handler_t syscall_table[SYSCALL_MAX + 1] = {
    [SYSCALL_EXIT]   = sys_exit,
    [SYSCALL_WRITE]  = sys_write,
    [SYSCALL_READ]   = sys_read,
    [SYSCALL_OPEN]   = sys_unimplemented,
    [SYSCALL_CLOSE]  = sys_unimplemented,
    [SYSCALL_GETPID] = sys_getpid,
    [SYSCALL_FORK]   = sys_unimplemented,
    [SYSCALL_EXEC]   = sys_unimplemented,
    [SYSCALL_WAIT]   = sys_unimplemented,
    [SYSCALL_KILL]   = sys_unimplemented,
    [SYSCALL_SLEEP]  = sys_sleep,
    [SYSCALL_YIELD]  = sys_yield,
    [SYSCALL_MMAP]   = sys_unimplemented,
    [SYSCALL_MUNMAP] = sys_unimplemented,
    [SYSCALL_BRK]    = sys_unimplemented,
    [SYSCALL_SBRK]   = sys_unimplemented,
};

/**
 * System call dispatcher
 * Called from assembly syscall entry point
 */
int64_t syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                        uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6) {
    if (!syscall_state.initialized) {
        return -ENOSYS;
    }

    // Update statistics
    syscall_state.total_syscalls++;

    // Validate syscall number
    if (syscall_num > SYSCALL_MAX) {
        console_print("[SYSCALL] ERROR: Invalid syscall number: ");
        console_print_dec(syscall_num);
        console_print("\n");
        return -EINVAL;
    }

    syscall_state.syscall_counts[syscall_num]++;

    // Get handler
    syscall_handler_t handler = syscall_table[syscall_num];
    if (!handler) {
        return -ENOSYS;
    }

    // Call handler
    int64_t result = handler(arg1, arg2, arg3, arg4, arg5, arg6);

    return result;
}

/**
 * Initialize system call interface
 */
void syscall_init(void) {
    console_print("[SYSCALL] Initializing system call interface...\n");

    // Reset statistics
    syscall_state.total_syscalls = 0;
    for (int i = 0; i <= SYSCALL_MAX; i++) {
        syscall_state.syscall_counts[i] = 0;
    }

    // Set up MSRs for SYSCALL/SYSRET
    // STAR: Segment selectors
    // Bits 63:48 - Kernel CS (0x08) and SS (0x10) base
    // Bits 47:32 - User CS (0x18) and SS (0x20) base
    uint64_t star = ((uint64_t)0x08 << 32) | ((uint64_t)0x18 << 48);
    wrmsr(MSR_STAR, star);

    // LSTAR: Entry point for SYSCALL (64-bit mode)
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

    // SFMASK: RFLAGS mask (clear IF on syscall entry)
    wrmsr(MSR_SFMASK, 0x200);  // Clear IF (interrupt flag)

    // Enable SYSCALL/SYSRET in EFER
    // This is typically already enabled, but let's make sure
    // EFER MSR is 0xC0000080, bit 0 is SCE (System Call Extensions)
    uint64_t efer = rdmsr(0xC0000080);
    efer |= 0x01;  // Set SCE bit
    wrmsr(0xC0000080, efer);

    syscall_state.initialized = true;

    console_print("[SYSCALL] System calls initialized\n");
    console_print("[SYSCALL]   Entry point: ");
    console_print_hex((uint64_t)syscall_entry);
    console_print("\n[SYSCALL]   Syscalls available: ");
    console_print_dec(SYSCALL_MAX + 1);
    console_print("\n");
}

/**
 * Print syscall statistics
 */
void syscall_print_stats(void) {
    if (!syscall_state.initialized) {
        console_print("[SYSCALL] Not initialized\n");
        return;
    }

    console_print("\n[SYSCALL] Statistics:\n");
    console_print("  Total syscalls:  ");
    console_print_dec(syscall_state.total_syscalls);
    console_print("\n\n");

    console_print("  Syscall breakdown:\n");

    const char *syscall_names[] = {
        "exit", "write", "read", "open", "close",
        "getpid", "fork", "exec", "wait", "kill",
        "sleep", "yield", "mmap", "munmap", "brk", "sbrk"
    };

    for (int i = 0; i <= SYSCALL_MAX; i++) {
        if (syscall_state.syscall_counts[i] > 0) {
            console_print("    ");
            console_print_dec(i);
            console_print(" (");
            console_print(syscall_names[i]);
            console_print("): ");
            console_print_dec(syscall_state.syscall_counts[i]);
            console_print("\n");
        }
    }
}
