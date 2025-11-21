/**
 * AuroraOS Kernel - Scheduler Implementation
 *
 * Preemptive round-robin task scheduler
 */

#include "scheduler.h"
#include "process.h"
#include "console.h"
#include "timer.h"
#include "types.h"

// Scheduler state
static struct {
    bool running;
    bool initialized;
    sched_policy_t policy;
    thread_t *ready_queue_head;
    thread_t *ready_queue_tail;
    uint32_t ready_count;
    sched_stats_t stats;
} sched_state = {0};

// External function to set current thread (from process.c)
extern void thread_set_current(thread_t *thread);

/**
 * Add thread to ready queue
 */
void scheduler_add_thread(thread_t *thread) {
    if (!thread || thread->state != TASK_STATE_READY) {
        return;
    }

    // Add to end of ready queue
    thread->next = NULL;
    thread->prev = sched_state.ready_queue_tail;

    if (sched_state.ready_queue_tail) {
        sched_state.ready_queue_tail->next = thread;
    } else {
        sched_state.ready_queue_head = thread;
    }

    sched_state.ready_queue_tail = thread;
    sched_state.ready_count++;
}

/**
 * Remove thread from ready queue
 */
void scheduler_remove_thread(thread_t *thread) {
    if (!thread) {
        return;
    }

    // Remove from ready queue
    if (thread->prev) {
        thread->prev->next = thread->next;
    } else {
        sched_state.ready_queue_head = thread->next;
    }

    if (thread->next) {
        thread->next->prev = thread->prev;
    } else {
        sched_state.ready_queue_tail = thread->prev;
    }

    thread->next = NULL;
    thread->prev = NULL;
    sched_state.ready_count--;
}

/**
 * Get next thread to run (round-robin)
 */
static thread_t* scheduler_pick_next(void) {
    if (!sched_state.ready_queue_head) {
        return NULL;  // No threads ready
    }

    // Pick first thread from queue (round-robin)
    thread_t *next = sched_state.ready_queue_head;

    // Remove from queue
    scheduler_remove_thread(next);

    return next;
}

/**
 * Perform context switch
 */
static void scheduler_switch_to(thread_t *next) {
    if (!next) {
        return;
    }

    thread_t *current = thread_get_current();

    // If switching to same thread, do nothing
    if (current == next) {
        thread_set_state(next, TASK_STATE_RUNNING);
        return;
    }

    // Update states
    if (current) {
        if (current->state == TASK_STATE_RUNNING) {
            thread_set_state(current, TASK_STATE_READY);
            scheduler_add_thread(current);  // Re-add to ready queue
        }
    }

    thread_set_state(next, TASK_STATE_RUNNING);
    thread_set_current(next);

    // Reset time slice
    next->time_slice = SCHED_DEFAULT_TIME_SLICE;

    // Increment switch counter
    sched_state.stats.total_switches++;

    // Perform actual context switch (if there was a previous thread)
    if (current && current != next) {
        switch_context(&current->context, &next->context);
    }
}

/**
 * Schedule next thread
 */
static void scheduler_schedule(void) {
    if (!sched_state.running || !sched_state.initialized) {
        return;
    }

    // Pick next thread
    thread_t *next = scheduler_pick_next();

    if (!next) {
        // No ready threads - this shouldn't happen with idle process
        console_print("[SCHED] WARNING: No ready threads!\n");
        return;
    }

    // Switch to it
    scheduler_switch_to(next);
}

/**
 * Scheduler tick (called from timer interrupt)
 */
void scheduler_tick(void) {
    if (!sched_state.running || !sched_state.initialized) {
        return;
    }

    sched_state.stats.total_ticks++;

    thread_t *current = thread_get_current();
    if (!current) {
        // No current thread, schedule one
        scheduler_schedule();
        return;
    }

    // Update runtime
    current->total_runtime++;

    // Decrement time slice
    if (current->time_slice > 0) {
        current->time_slice--;
    }

    // If time slice expired, reschedule
    if (current->time_slice == 0) {
        scheduler_schedule();
    }
}

/**
 * Voluntary yield CPU
 */
void scheduler_yield(void) {
    if (!sched_state.running) {
        return;
    }

    thread_t *current = thread_get_current();
    if (current) {
        current->time_slice = 0;  // Force reschedule
    }

    scheduler_schedule();
}

/**
 * Start scheduler
 */
void scheduler_start(void) {
    if (!sched_state.initialized) {
        console_print("[SCHED] ERROR: Scheduler not initialized\n");
        return;
    }

    console_print("[SCHED] Starting scheduler...\n");
    sched_state.running = true;

    // Do initial schedule
    scheduler_schedule();

    console_print("[SCHED] Scheduler started\n");
}

/**
 * Stop scheduler
 */
void scheduler_stop(void) {
    console_print("[SCHED] Stopping scheduler...\n");
    sched_state.running = false;
}

/**
 * Check if scheduler is running
 */
bool scheduler_is_running(void) {
    return sched_state.running;
}

/**
 * Set scheduling policy
 */
void scheduler_set_policy(sched_policy_t policy) {
    sched_state.policy = policy;
}

/**
 * Get scheduling policy
 */
sched_policy_t scheduler_get_policy(void) {
    return sched_state.policy;
}

/**
 * Get scheduler statistics
 */
sched_stats_t scheduler_get_stats(void) {
    return sched_state.stats;
}

/**
 * Print scheduler statistics
 */
void scheduler_print_stats(void) {
    console_print("\n[SCHED] Statistics:\n");
    console_print("  Policy:          ");
    switch (sched_state.policy) {
        case SCHED_POLICY_ROUND_ROBIN:
            console_print("Round-Robin");
            break;
        case SCHED_POLICY_PRIORITY:
            console_print("Priority");
            break;
        case SCHED_POLICY_FIFO:
            console_print("FIFO");
            break;
        default:
            console_print("Unknown");
            break;
    }
    console_print("\n");

    console_print("  Status:          ");
    console_print(sched_state.running ? "RUNNING" : "STOPPED");
    console_print("\n");

    console_print("  Ready threads:   ");
    console_print_dec(sched_state.ready_count);
    console_print("\n");

    console_print("  Total switches:  ");
    console_print_dec(sched_state.stats.total_switches);
    console_print("\n");

    console_print("  Total ticks:     ");
    console_print_dec(sched_state.stats.total_ticks);
    console_print("\n");

    thread_t *current = thread_get_current();
    if (current) {
        console_print("  Current thread:  TID ");
        console_print_dec(current->tid);
        console_print(" (");
        console_print(task_state_to_string(current->state));
        console_print(")\n");
        console_print("  Time slice left: ");
        console_print_dec(current->time_slice);
        console_print(" ticks\n");
    } else {
        console_print("  Current thread:  None\n");
    }
}

/**
 * Initialize scheduler
 */
void scheduler_init(void) {
    console_print("[SCHED] Initializing scheduler...\n");

    // Reset state
    sched_state.running = false;
    sched_state.policy = SCHED_POLICY_ROUND_ROBIN;
    sched_state.ready_queue_head = NULL;
    sched_state.ready_queue_tail = NULL;
    sched_state.ready_count = 0;
    sched_state.stats.total_switches = 0;
    sched_state.stats.total_ticks = 0;
    sched_state.stats.idle_ticks = 0;

    sched_state.initialized = true;

    console_print("[SCHED] Scheduler initialized\n");
    console_print("[SCHED]   Policy: Round-Robin\n");
    console_print("[SCHED]   Time slice: ");
    console_print_dec(SCHED_DEFAULT_TIME_SLICE);
    console_print(" ms\n");
}
