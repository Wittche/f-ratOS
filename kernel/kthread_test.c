/**
 * AuroraOS Kernel - Kernel Thread Testing
 *
 * Simple kernel threads for testing scheduler and context switching
 */

#include "kthread_test.h"
#include "process.h"
#include "scheduler.h"
#include "console.h"
#include "timer.h"

// Test thread counters
static volatile uint32_t thread_a_count = 0;
static volatile uint32_t thread_b_count = 0;
static volatile uint32_t thread_c_count = 0;

/**
 * Test Thread A - Prints 'A' periodically
 */
static void test_thread_a(void) {
    console_print("[KTHREAD] Thread A started (TID=");
    thread_t *self = thread_get_current();
    if (self) {
        console_print_dec(self->tid);
    }
    console_print(")\n");

    while (1) {
        console_print("A");
        thread_a_count++;

        // Yield to other threads
        scheduler_yield();

        // Sleep a bit to slow down output
        if (thread_a_count % 10 == 0) {
            timer_sleep(100);  // Sleep 100ms every 10 iterations
        }
    }
}

/**
 * Test Thread B - Prints 'B' periodically
 */
static void test_thread_b(void) {
    console_print("[KTHREAD] Thread B started (TID=");
    thread_t *self = thread_get_current();
    if (self) {
        console_print_dec(self->tid);
    }
    console_print(")\n");

    while (1) {
        console_print("B");
        thread_b_count++;

        // Yield to other threads
        scheduler_yield();

        // Sleep a bit to slow down output
        if (thread_b_count % 10 == 0) {
            timer_sleep(100);  // Sleep 100ms every 10 iterations
        }
    }
}

/**
 * Test Thread C - Prints 'C' periodically
 */
static void test_thread_c(void) {
    console_print("[KTHREAD] Thread C started (TID=");
    thread_t *self = thread_get_current();
    if (self) {
        console_print_dec(self->tid);
    }
    console_print(")\n");

    while (1) {
        console_print("C");
        thread_c_count++;

        // Yield to other threads
        scheduler_yield();

        // Sleep a bit to slow down output
        if (thread_c_count % 10 == 0) {
            timer_sleep(100);  // Sleep 100ms every 10 iterations
        }
    }
}

/**
 * Status thread - Prints statistics periodically
 */
static void test_thread_status(void) {
    console_print("[KTHREAD] Status thread started (TID=");
    thread_t *self = thread_get_current();
    if (self) {
        console_print_dec(self->tid);
    }
    console_print(")\n");

    uint32_t iteration = 0;

    while (1) {
        iteration++;

        // Wait 1 second between status updates
        timer_sleep(1000);

        // Print status
        console_print("\n\n[STATUS] Iteration ");
        console_print_dec(iteration);
        console_print(":\n");

        console_print("  Thread A: ");
        console_print_dec(thread_a_count);
        console_print(" iterations\n");

        console_print("  Thread B: ");
        console_print_dec(thread_b_count);
        console_print(" iterations\n");

        console_print("  Thread C: ");
        console_print_dec(thread_c_count);
        console_print(" iterations\n");

        console_print("  Uptime: ");
        console_print_dec(timer_get_seconds());
        console_print(".");
        console_print_dec(timer_get_milliseconds() % 1000);
        console_print(" seconds\n\n");

        // Print scheduler stats every 5 seconds
        if (iteration % 5 == 0) {
            scheduler_print_stats();
        }
    }
}

/**
 * Initialize test kernel threads
 */
void kthread_test_init(void) {
    console_print("[KTHREAD] Initializing test kernel threads...\n");

    // Create test processes with threads
    process_t *proc_a = process_create("test_thread_a", test_thread_a);
    process_t *proc_b = process_create("test_thread_b", test_thread_b);
    process_t *proc_c = process_create("test_thread_c", test_thread_c);
    process_t *proc_status = process_create("status_thread", test_thread_status);

    if (!proc_a || !proc_b || !proc_c || !proc_status) {
        console_print("[KTHREAD] ERROR: Failed to create test processes!\n");
        return;
    }

    console_print("[KTHREAD] Created 4 test processes\n");
    console_print("[KTHREAD]   Process A: PID=");
    console_print_dec(proc_a->pid);
    console_print(", TID=");
    console_print_dec(proc_a->main_thread->tid);
    console_print("\n");

    console_print("[KTHREAD]   Process B: PID=");
    console_print_dec(proc_b->pid);
    console_print(", TID=");
    console_print_dec(proc_b->main_thread->tid);
    console_print("\n");

    console_print("[KTHREAD]   Process C: PID=");
    console_print_dec(proc_c->pid);
    console_print(", TID=");
    console_print_dec(proc_c->main_thread->tid);
    console_print("\n");

    console_print("[KTHREAD]   Status: PID=");
    console_print_dec(proc_status->pid);
    console_print(", TID=");
    console_print_dec(proc_status->main_thread->tid);
    console_print("\n");
}

/**
 * Start the test threads (enable scheduler)
 */
void kthread_test_start(void) {
    console_print("\n[KTHREAD] Starting test threads...\n");
    console_print("[KTHREAD] Scheduler will begin in 3 seconds...\n\n");

    // Wait a bit before starting
    timer_sleep(3000);

    console_print("[KTHREAD] === SCHEDULER STARTING NOW ===\n\n");

    // Start the scheduler - this will begin running threads
    scheduler_start();

    // Should never reach here if scheduler works
    console_print("[KTHREAD] ERROR: Returned from scheduler_start()!\n");
}
