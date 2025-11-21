/**
 * AuroraOS Kernel - Task Scheduler
 *
 * Preemptive multitasking scheduler with round-robin algorithm
 */

#ifndef _KERNEL_SCHEDULER_H_
#define _KERNEL_SCHEDULER_H_

#include "types.h"
#include "process.h"

// Scheduler policies
typedef enum {
    SCHED_POLICY_ROUND_ROBIN,    // Simple round-robin
    SCHED_POLICY_PRIORITY,        // Priority-based
    SCHED_POLICY_FIFO,            // First-in-first-out
} sched_policy_t;

// Scheduler configuration
#define SCHED_DEFAULT_TIME_SLICE  10  // 10ms default time slice

// Scheduler statistics
typedef struct {
    uint64_t total_switches;      // Total context switches
    uint64_t total_ticks;         // Total scheduler ticks
    uint64_t idle_ticks;          // Ticks spent in idle
} sched_stats_t;

// Scheduler initialization
void scheduler_init(void);

// Scheduler control
void scheduler_start(void);
void scheduler_stop(void);
bool scheduler_is_running(void);

// Thread scheduling
void scheduler_add_thread(thread_t *thread);
void scheduler_remove_thread(thread_t *thread);
void scheduler_yield(void);  // Voluntary yield CPU

// Called by timer interrupt
void scheduler_tick(void);

// Policy management
void scheduler_set_policy(sched_policy_t policy);
sched_policy_t scheduler_get_policy(void);

// Statistics
void scheduler_print_stats(void);
sched_stats_t scheduler_get_stats(void);

// Context switching (implemented in assembly)
extern void switch_context(cpu_context_t *old_ctx, cpu_context_t *new_ctx);

#endif // _KERNEL_SCHEDULER_H_
