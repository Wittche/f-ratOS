/**
 * AuroraOS Kernel - Timer Driver Implementation
 *
 * PIT (Programmable Interval Timer) driver for system timing
 */

#include "timer.h"
#include "io.h"
#include "console.h"
#include "types.h"
#include "scheduler.h"

// Timer state
static struct {
    uint64_t ticks;
    uint32_t frequency;
    bool initialized;
    timer_callback_t callback;
} timer_state = {0};

/**
 * Set PIT frequency
 *
 * @param frequency Desired frequency in Hz
 */
void timer_set_frequency(uint32_t frequency) {
    if (frequency == 0 || frequency > PIT_BASE_FREQUENCY) {
        console_print("[TIMER] ERROR: Invalid frequency\n");
        return;
    }

    // Calculate divisor: PIT_BASE_FREQUENCY / desired_frequency
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;

    // Clamp divisor to valid range (1-65535)
    if (divisor < 1) divisor = 1;
    if (divisor > 65535) divisor = 65535;

    // Send command byte:
    // - Channel 0
    // - Access mode: lobyte/hibyte
    // - Mode 3: Square wave generator
    // - Binary mode
    uint8_t command = PIT_CHANNEL0_SELECT | PIT_ACCESS_LOHI | PIT_MODE_3 | PIT_BINARY_MODE;
    outb(PIT_COMMAND, command);

    // Send divisor (low byte, then high byte)
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    timer_state.frequency = frequency;
}

/**
 * Get current timer frequency
 */
uint32_t timer_get_frequency(void) {
    return timer_state.frequency;
}

/**
 * Get total ticks since boot
 */
uint64_t timer_get_ticks(void) {
    return timer_state.ticks;
}

/**
 * Get milliseconds since boot
 */
uint64_t timer_get_milliseconds(void) {
    if (timer_state.frequency == 0) return 0;
    return (timer_state.ticks * 1000) / timer_state.frequency;
}

/**
 * Get seconds since boot
 */
uint64_t timer_get_seconds(void) {
    if (timer_state.frequency == 0) return 0;
    return timer_state.ticks / timer_state.frequency;
}

/**
 * Timer IRQ handler (called from IDT IRQ 0)
 * This is called every timer tick
 */
void timer_irq_handler(void) {
    if (!timer_state.initialized) {
        return;
    }

    // Increment tick counter
    timer_state.ticks++;

    // Call scheduler tick
    scheduler_tick();

    // Call registered callback if any
    if (timer_state.callback) {
        timer_state.callback();
    }

    // Note: EOI (End of Interrupt) is handled by the IDT IRQ handler
}

/**
 * Register a callback function to be called on each timer tick
 */
void timer_register_callback(timer_callback_t callback) {
    timer_state.callback = callback;
}

/**
 * Sleep for specified milliseconds
 *
 * @param milliseconds Time to sleep in milliseconds
 *
 * Note: This is a busy-wait implementation. Will be replaced with
 * proper sleep when we have process scheduling.
 */
void timer_sleep(uint32_t milliseconds) {
    if (!timer_state.initialized || timer_state.frequency == 0) {
        return;
    }

    uint64_t start = timer_get_milliseconds();
    uint64_t target = start + milliseconds;

    // Busy wait until target time reached
    while (timer_get_milliseconds() < target) {
        __asm__ __volatile__("hlt");  // Halt until next interrupt
    }
}

/**
 * Wait for specified number of ticks
 */
void timer_wait_ticks(uint32_t ticks) {
    if (!timer_state.initialized) {
        return;
    }

    uint64_t start = timer_state.ticks;
    uint64_t target = start + ticks;

    while (timer_state.ticks < target) {
        __asm__ __volatile__("hlt");
    }
}

/**
 * Initialize PIT timer
 *
 * @param frequency Desired timer frequency in Hz (default: 1000 Hz = 1ms)
 */
void timer_init(uint32_t frequency) {
    console_print("[TIMER] Initializing Programmable Interval Timer...\n");

    // Validate frequency
    if (frequency == 0) {
        frequency = TIMER_FREQ_1000HZ;  // Default to 1000 Hz (1ms tick)
    }

    // Reset state
    timer_state.ticks = 0;
    timer_state.frequency = 0;
    timer_state.callback = NULL;

    // Set PIT frequency
    timer_set_frequency(frequency);

    timer_state.initialized = true;

    console_print("[TIMER] Initialized at ");
    console_print_dec(frequency);
    console_print(" Hz (");

    uint32_t tick_ms = 1000 / frequency;
    console_print_dec(tick_ms);
    console_print("ms per tick)\n");

    console_print("[TIMER] IRQ 0 will fire every ");
    console_print_dec(tick_ms);
    console_print("ms\n");
}

/**
 * Print timer statistics
 */
void timer_print_stats(void) {
    if (!timer_state.initialized) {
        console_print("[TIMER] Not initialized\n");
        return;
    }

    console_print("\n[TIMER] Statistics:\n");
    console_print("  Frequency:     ");
    console_print_dec(timer_state.frequency);
    console_print(" Hz\n");

    console_print("  Total Ticks:   ");
    console_print_dec(timer_state.ticks);
    console_print("\n");

    console_print("  Uptime:        ");
    console_print_dec(timer_get_seconds());
    console_print(".");
    console_print_dec(timer_get_milliseconds() % 1000);
    console_print(" seconds\n");

    console_print("  Milliseconds:  ");
    console_print_dec(timer_get_milliseconds());
    console_print(" ms\n");
}
