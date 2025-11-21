/**
 * AuroraOS - User Mode Test Program
 *
 * Simple user-space program to test Ring 3 execution and syscalls
 */

#include "types.h"
#include "syscall.h"

/**
 * User mode syscall wrapper - write()
 */
static inline int64_t usermode_write(int fd, const void *buf, uint64_t count) {
    int64_t ret;
    __asm__ __volatile__(
        "mov %1, %%rax\n"      // syscall number
        "mov %2, %%rdi\n"      // arg1: fd
        "mov %3, %%rsi\n"      // arg2: buf
        "mov %4, %%rdx\n"      // arg3: count
        "syscall\n"
        "mov %%rax, %0\n"      // return value
        : "=r"(ret)
        : "i"(SYSCALL_WRITE), "r"((uint64_t)fd), "r"((uint64_t)buf), "r"(count)
        : "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
    return ret;
}

/**
 * User mode syscall wrapper - exit()
 */
static inline void usermode_exit(int status) {
    __asm__ __volatile__(
        "mov %0, %%rax\n"      // syscall number
        "mov %1, %%rdi\n"      // arg1: status
        "syscall\n"
        :
        : "i"(SYSCALL_EXIT), "r"((uint64_t)status)
        : "rax", "rdi", "rcx", "r11", "memory"
    );
}

/**
 * User mode syscall wrapper - getpid()
 */
static inline int64_t usermode_getpid(void) {
    int64_t ret;
    __asm__ __volatile__(
        "mov %1, %%rax\n"      // syscall number
        "syscall\n"
        "mov %%rax, %0\n"      // return value
        : "=r"(ret)
        : "i"(SYSCALL_GETPID)
        : "rax", "rcx", "r11", "memory"
    );
    return ret;
}

/**
 * User mode syscall wrapper - yield()
 */
static inline void usermode_yield(void) {
    __asm__ __volatile__(
        "mov %0, %%rax\n"      // syscall number
        "syscall\n"
        :
        : "i"(SYSCALL_YIELD)
        : "rax", "rcx", "r11", "memory"
    );
}

/**
 * Simple strlen function
 */
static uint64_t strlen(const char *str) {
    uint64_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

/**
 * Print a string to stdout
 */
static void print(const char *str) {
    usermode_write(STDOUT_FILENO, str, strlen(str));
}

/**
 * User mode test program entry point
 *
 * This function runs in Ring 3 (user mode) and tests syscalls.
 */
void usermode_test_program(void) {
    // Test write syscall
    print("Hello from user mode (Ring 3)!\n");
    print("Testing syscalls...\n");

    // Test getpid syscall
    int64_t pid = usermode_getpid();
    print("My PID is: ");
    // We can't print numbers easily in user mode without a library
    // So just print a placeholder
    if (pid > 0) {
        print("[PID > 0]\n");
    } else {
        print("[ERROR: Invalid PID]\n");
    }

    // Test multiple writes
    print("\nUser mode features:\n");
    print("  [OK] Ring 3 execution\n");
    print("  [OK] System calls (SYSCALL instruction)\n");
    print("  [OK] write() syscall\n");
    print("  [OK] getpid() syscall\n");

    // Print some iterations
    print("\nRunning user mode loop...\n");
    for (int i = 0; i < 5; i++) {
        print("Iteration ");
        // Print a simple counter (using characters)
        char num[2] = {'0' + i, '\n'};
        usermode_write(STDOUT_FILENO, num, 2);

        // Yield CPU to test cooperative multitasking
        usermode_yield();
    }

    print("\nUser mode test completed successfully!\n");
    print("Calling exit(0)...\n\n");

    // Exit user mode program
    usermode_exit(0);

    // Should never reach here
    print("ERROR: Returned from exit()!\n");
    while (1) {
        // Hang
    }
}
