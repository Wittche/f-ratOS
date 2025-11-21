/**
 * AuroraOS Kernel - Process and Thread Management
 *
 * Task structures and process management (XNU-inspired)
 */

#ifndef _KERNEL_PROCESS_H_
#define _KERNEL_PROCESS_H_

#include "types.h"

// Process/Thread IDs
typedef uint32_t pid_t;
typedef uint32_t tid_t;

// Process/Thread states
typedef enum {
    TASK_STATE_NEW,         // Task created but not started
    TASK_STATE_READY,       // Ready to run
    TASK_STATE_RUNNING,     // Currently executing
    TASK_STATE_BLOCKED,     // Waiting for I/O or event
    TASK_STATE_SLEEPING,    // Sleeping for timeout
    TASK_STATE_ZOMBIE,      // Terminated, waiting for cleanup
    TASK_STATE_DEAD         // Completely terminated
} task_state_t;

// CPU context for context switching (x86_64)
typedef struct {
    // General purpose registers
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;

    // Instruction pointer and flags
    uint64_t rip;
    uint64_t rflags;

    // Segment selectors
    uint64_t cs, ss, ds, es, fs, gs;

    // FPU/SSE state (simplified)
    // TODO: Add full FXSAVE area (512 bytes)
} __attribute__((packed)) cpu_context_t;

// Thread Control Block (TCB)
typedef struct thread {
    tid_t tid;                      // Thread ID
    task_state_t state;             // Current state
    cpu_context_t context;          // Saved CPU context

    // Stack information
    void *stack_base;               // Stack base address
    uint64_t stack_size;            // Stack size in bytes
    void *kernel_stack;             // Kernel stack

    // Scheduling info
    uint32_t priority;              // Thread priority (0-255)
    uint64_t time_slice;            // Remaining time slice (ticks)
    uint64_t total_runtime;         // Total CPU time used

    // Parent process
    struct process *process;        // Owning process

    // Thread list linkage
    struct thread *next;            // Next thread in list
    struct thread *prev;            // Previous thread in list
} thread_t;

// Process Control Block (PCB)
typedef struct process {
    pid_t pid;                      // Process ID
    char name[64];                  // Process name

    // Memory management
    uint64_t page_directory;        // CR3 value (page table root)
    void *heap_start;               // Heap start address
    void *heap_end;                 // Heap end address

    // Threads
    thread_t *main_thread;          // Main thread
    thread_t *thread_list;          // List of all threads
    uint32_t thread_count;          // Number of threads

    // Parent/child relationships
    struct process *parent;         // Parent process
    struct process *children;       // List of child processes

    // Process list linkage
    struct process *next;           // Next process in list
    struct process *prev;           // Previous process in list

    // Exit status
    int exit_code;                  // Exit code when terminated
} process_t;

// Process/Thread management functions
void process_init(void);

// Process creation and termination
process_t* process_create(const char *name, void (*entry_point)(void));
void process_destroy(process_t *proc);
void process_exit(int exit_code);

// Thread creation and termination
thread_t* thread_create(process_t *proc, void (*entry_point)(void), uint32_t priority);
void thread_destroy(thread_t *thread);
void thread_exit(void);

// Process/Thread queries
process_t* process_get_current(void);
thread_t* thread_get_current(void);
process_t* process_find_by_pid(pid_t pid);
thread_t* thread_find_by_tid(tid_t tid);

// State management
void thread_set_state(thread_t *thread, task_state_t state);
const char* task_state_to_string(task_state_t state);

// Process list iteration
process_t* process_get_first(void);
process_t* process_get_next(process_t *proc);

// Statistics
void process_print_list(void);
uint32_t process_count(void);
uint32_t thread_count_total(void);

#endif // _KERNEL_PROCESS_H_
