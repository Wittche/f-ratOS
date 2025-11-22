/**
 * AuroraOS Kernel - Process Management Implementation
 *
 * Process and thread creation, management, and lifecycle
 */

#include "process.h"
#include "kheap.h"
#include "console.h"
#include "vmm.h"
#include "types.h"
#include "scheduler.h"

// Process/Thread ID counters
static pid_t next_pid = 1;
static tid_t next_tid = 1;

// Process and thread lists
static process_t *process_list_head = NULL;
static thread_t *current_thread = NULL;

// Kernel idle process
static process_t *idle_process = NULL;

/**
 * Get current running thread
 */
thread_t* thread_get_current(void) {
    return current_thread;
}

/**
 * Get current running process
 */
process_t* process_get_current(void) {
    if (current_thread) {
        return current_thread->process;
    }
    return NULL;
}

/**
 * Set current running thread
 */
void thread_set_current(thread_t *thread) {
    current_thread = thread;
}

/**
 * Convert task state to string
 */
const char* task_state_to_string(task_state_t state) {
    switch (state) {
        case TASK_STATE_NEW:      return "NEW";
        case TASK_STATE_READY:    return "READY";
        case TASK_STATE_RUNNING:  return "RUNNING";
        case TASK_STATE_BLOCKED:  return "BLOCKED";
        case TASK_STATE_SLEEPING: return "SLEEPING";
        case TASK_STATE_ZOMBIE:   return "ZOMBIE";
        case TASK_STATE_DEAD:     return "DEAD";
        default:                  return "UNKNOWN";
    }
}

/**
 * Set thread state
 */
void thread_set_state(thread_t *thread, task_state_t state) {
    if (thread) {
        thread->state = state;
    }
}

/**
 * Create a new process
 */
process_t* process_create(const char *name, void (*entry_point)(void)) {
    // Allocate process control block
    process_t *proc = (process_t*)kmalloc(sizeof(process_t));
    if (!proc) {
        console_print("[PROC] ERROR: Failed to allocate process\n");
        return NULL;
    }

    // Initialize process
    proc->pid = next_pid++;

    // Copy name
    uint32_t i;
    for (i = 0; i < 63 && name && name[i]; i++) {
        proc->name[i] = name[i];
    }
    proc->name[i] = '\0';

    // Memory management (use kernel page directory for now)
    proc->page_directory = vmm_get_cr3();
    proc->heap_start = NULL;
    proc->heap_end = NULL;

    // Initialize thread info
    proc->main_thread = NULL;
    proc->thread_list = NULL;
    proc->thread_count = 0;

    // Parent/child relationships
    proc->parent = process_get_current();
    proc->children = NULL;

    // Add to process list
    proc->next = process_list_head;
    proc->prev = NULL;
    if (process_list_head) {
        process_list_head->prev = proc;
    }
    process_list_head = proc;

    proc->exit_code = 0;

    // Create main thread if entry point provided
    if (entry_point) {
        proc->main_thread = thread_create(proc, entry_point, 128);
        if (!proc->main_thread) {
            console_print("[PROC] ERROR: Failed to create main thread\n");
            kfree(proc);
            return NULL;
        }
    }

    return proc;
}

/**
 * Create a new thread
 */
thread_t* thread_create(process_t *proc, void (*entry_point)(void), uint32_t priority) {
    if (!proc) {
        return NULL;
    }

    // Allocate thread control block
    thread_t *thread = (thread_t*)kmalloc(sizeof(thread_t));
    if (!thread) {
        console_print("[PROC] ERROR: Failed to allocate thread\n");
        return NULL;
    }

    // Initialize thread
    thread->tid = next_tid++;
    thread->state = TASK_STATE_NEW;
    thread->process = proc;

    // Allocate kernel stack (8KB)
    uint64_t stack_size = 8192;
    thread->stack_size = stack_size;
    thread->stack_base = kmalloc(stack_size);
    if (!thread->stack_base) {
        console_print("[PROC] ERROR: Failed to allocate thread stack\n");
        kfree(thread);
        return NULL;
    }

    thread->kernel_stack = thread->stack_base;

    // Initialize CPU context
    for (uint32_t i = 0; i < sizeof(cpu_context_t); i++) {
        ((uint8_t*)&thread->context)[i] = 0;
    }

    // Set up initial context
    thread->context.rip = (uint64_t)entry_point;
    thread->context.rsp = (uint64_t)thread->stack_base + stack_size - 16;  // Stack grows down
    thread->context.rbp = thread->context.rsp;
    thread->context.rflags = 0x202;  // IF (interrupt enable) flag set
    thread->context.cs = 0x08;       // Kernel code segment
    thread->context.ss = 0x10;       // Kernel data segment
    thread->context.ds = 0x10;
    thread->context.es = 0x10;
    thread->context.fs = 0x10;
    thread->context.gs = 0x10;

    // Scheduling info
    thread->priority = priority;
    thread->time_slice = 10;  // 10 ticks default
    thread->total_runtime = 0;

    // Add to process thread list
    thread->next = proc->thread_list;
    thread->prev = NULL;
    if (proc->thread_list) {
        proc->thread_list->prev = thread;
    }
    proc->thread_list = thread;
    proc->thread_count++;

    // Mark as ready and add to scheduler
    thread->state = TASK_STATE_READY;
    scheduler_add_thread(thread);

    console_print("[PROC] Created thread TID=");
    console_print_dec(thread->tid);
    console_print(" for process PID=");
    console_print_dec(proc->pid);
    console_print("\n");

    return thread;
}

/**
 * Destroy a thread
 */
void thread_destroy(thread_t *thread) {
    if (!thread) {
        return;
    }

    // Remove from process thread list
    if (thread->prev) {
        thread->prev->next = thread->next;
    } else {
        thread->process->thread_list = thread->next;
    }
    if (thread->next) {
        thread->next->prev = thread->prev;
    }

    thread->process->thread_count--;

    // Free stack
    if (thread->stack_base) {
        kfree(thread->stack_base);
    }

    // Free TCB
    kfree(thread);
}

/**
 * Destroy a process
 */
void process_destroy(process_t *proc) {
    if (!proc) {
        return;
    }

    // Destroy all threads
    while (proc->thread_list) {
        thread_destroy(proc->thread_list);
    }

    // Remove from process list
    if (proc->prev) {
        proc->prev->next = proc->next;
    } else {
        process_list_head = proc->next;
    }
    if (proc->next) {
        proc->next->prev = proc->prev;
    }

    // Free PCB
    kfree(proc);
}

/**
 * Exit current thread
 */
void thread_exit(void) {
    thread_t *thread = thread_get_current();
    if (thread) {
        thread_set_state(thread, TASK_STATE_ZOMBIE);
    }
    // Scheduler will clean up
}

/**
 * Exit current process
 */
void process_exit(int exit_code) {
    process_t *proc = process_get_current();
    if (proc) {
        proc->exit_code = exit_code;

        // Mark all threads as zombie
        thread_t *t = proc->thread_list;
        while (t) {
            thread_set_state(t, TASK_STATE_ZOMBIE);
            t = t->next;
        }
    }
    // Scheduler will clean up
}

/**
 * Find process by PID
 */
process_t* process_find_by_pid(pid_t pid) {
    process_t *proc = process_list_head;
    while (proc) {
        if (proc->pid == pid) {
            return proc;
        }
        proc = proc->next;
    }
    return NULL;
}

/**
 * Find thread by TID
 */
thread_t* thread_find_by_tid(tid_t tid) {
    process_t *proc = process_list_head;
    while (proc) {
        thread_t *thread = proc->thread_list;
        while (thread) {
            if (thread->tid == tid) {
                return thread;
            }
            thread = thread->next;
        }
        proc = proc->next;
    }
    return NULL;
}

/**
 * Get first process in list
 */
process_t* process_get_first(void) {
    return process_list_head;
}

/**
 * Get next process
 */
process_t* process_get_next(process_t *proc) {
    return proc ? proc->next : NULL;
}

/**
 * Count total processes
 */
uint32_t process_count(void) {
    uint32_t count = 0;
    process_t *proc = process_list_head;
    while (proc) {
        count++;
        proc = proc->next;
    }
    return count;
}

/**
 * Count total threads
 */
uint32_t thread_count_total(void) {
    uint32_t count = 0;
    process_t *proc = process_list_head;
    while (proc) {
        count += proc->thread_count;
        proc = proc->next;
    }
    return count;
}

/**
 * Print process list
 */
void process_print_list(void) {
    console_print("\n[PROC] Process List:\n");
    console_print("  PID  Threads  State      Name\n");
    console_print("  ---  -------  ---------  ----\n");

    process_t *proc = process_list_head;
    while (proc) {
        console_print("  ");
        console_print_dec(proc->pid);
        console_print("    ");
        console_print_dec(proc->thread_count);
        console_print("        ");

        // Print state of main thread
        if (proc->main_thread) {
            console_print(task_state_to_string(proc->main_thread->state));
        } else {
            console_print("NO_MAIN");
        }

        console_print("     ");
        console_print(proc->name);
        console_print("\n");

        proc = proc->next;
    }

    console_print("\nTotal processes: ");
    console_print_dec(process_count());
    console_print(", Total threads: ");
    console_print_dec(thread_count_total());
    console_print("\n");
}

/**
 * Kernel idle task
 */
static void idle_task(void) {
    while (1) {
        __asm__ __volatile__("hlt");  // Wait for interrupt
    }
}

/**
 * Initialize process management
 */
void process_init(void) {
    console_print("[PROC] Initializing process management...\n");

    // Reset lists
    process_list_head = NULL;
    current_thread = NULL;
    next_pid = 1;
    next_tid = 1;

    // Create idle process (PID 0)
    idle_process = process_create("idle", idle_task);
    if (!idle_process) {
        console_print("[PROC] ERROR: Failed to create idle process\n");
        return;
    }

    console_print("[PROC] Created idle process (PID ");
    console_print_dec(idle_process->pid);
    console_print(")\n");

    console_print("[PROC] Process management initialized\n");
    console_print("[PROC]   Next PID: ");
    console_print_dec(next_pid);
    console_print(", Next TID: ");
    console_print_dec(next_tid);
    console_print("\n");
}
