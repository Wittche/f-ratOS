/**
 * AuroraOS Kernel - System Call Interface
 *
 * System call definitions and handler (x86_64 SYSCALL/SYSRET)
 */

#ifndef _KERNEL_SYSCALL_H_
#define _KERNEL_SYSCALL_H_

#include "types.h"

// System call numbers (inspired by POSIX/Linux)
#define SYSCALL_EXIT        0   // exit(int status)
#define SYSCALL_WRITE       1   // write(int fd, const void *buf, size_t count)
#define SYSCALL_READ        2   // read(int fd, void *buf, size_t count)
#define SYSCALL_OPEN        3   // open(const char *path, int flags)
#define SYSCALL_CLOSE       4   // close(int fd)
#define SYSCALL_GETPID      5   // getpid()
#define SYSCALL_FORK        6   // fork()
#define SYSCALL_EXEC        7   // exec(const char *path, char **argv)
#define SYSCALL_WAIT        8   // wait(int *status)
#define SYSCALL_KILL        9   // kill(pid_t pid, int sig)
#define SYSCALL_SLEEP       10  // sleep(uint32_t ms)
#define SYSCALL_YIELD       11  // yield()
#define SYSCALL_MMAP        12  // mmap(void *addr, size_t len, int prot, int flags)
#define SYSCALL_MUNMAP      13  // munmap(void *addr, size_t len)
#define SYSCALL_BRK         14  // brk(void *addr)
#define SYSCALL_SBRK        15  // sbrk(intptr_t increment)

// Maximum syscall number
#define SYSCALL_MAX         15

// System call return values
#define SYSCALL_SUCCESS     0
#define SYSCALL_ERROR       (-1)

// Error codes (simplified errno)
#define ESUCCESS    0   // Success
#define EINVAL      1   // Invalid argument
#define ENOSYS      2   // Function not implemented
#define EBADF       3   // Bad file descriptor
#define ENOMEM      4   // Out of memory
#define EACCES      5   // Permission denied
#define ENOENT      6   // No such file or directory
#define EIO         7   // I/O error
#define EAGAIN      8   // Try again
#define EBUSY       9   // Device or resource busy

// File descriptor constants
#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

// System call handler function type
typedef int64_t (*syscall_handler_t)(uint64_t arg1, uint64_t arg2, uint64_t arg3,
                                     uint64_t arg4, uint64_t arg5, uint64_t arg6);

// System call context (saved on syscall entry)
typedef struct {
    uint64_t rax;   // Syscall number / return value
    uint64_t rdi;   // Arg 1
    uint64_t rsi;   // Arg 2
    uint64_t rdx;   // Arg 3
    uint64_t r10;   // Arg 4
    uint64_t r8;    // Arg 5
    uint64_t r9;    // Arg 6
    uint64_t rcx;   // Return RIP (saved by SYSCALL)
    uint64_t r11;   // Return RFLAGS (saved by SYSCALL)
} syscall_context_t;

// System call initialization
void syscall_init(void);

// System call dispatcher (called from assembly)
int64_t syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                        uint64_t arg3, uint64_t arg4, uint64_t arg5, uint64_t arg6);

// Assembly syscall entry point
extern void syscall_entry(void);

// Statistics
void syscall_print_stats(void);

#endif // _KERNEL_SYSCALL_H_
