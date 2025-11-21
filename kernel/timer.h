/**
 * AuroraOS Kernel - Programmable Interval Timer (PIT)
 *
 * 8253/8254 PIT driver for system timing and scheduling
 */

#ifndef _KERNEL_TIMER_H_
#define _KERNEL_TIMER_H_

#include "types.h"

// PIT I/O ports
#define PIT_CHANNEL0    0x40    // Channel 0 data port (system timer)
#define PIT_CHANNEL1    0x41    // Channel 1 data port (unused)
#define PIT_CHANNEL2    0x42    // Channel 2 data port (PC speaker)
#define PIT_COMMAND     0x43    // Mode/Command register

// PIT command byte format
// Bits 7-6: Channel select (00=0, 01=1, 10=2, 11=read-back)
// Bits 5-4: Access mode (01=lobyte, 10=hibyte, 11=lobyte/hibyte)
// Bits 3-1: Operating mode (000-101, mode 2 and 3 most common)
// Bit 0:    BCD/Binary mode (0=binary, 1=BCD)

#define PIT_CHANNEL0_SELECT  0x00
#define PIT_CHANNEL1_SELECT  0x40
#define PIT_CHANNEL2_SELECT  0x80

#define PIT_ACCESS_LATCH     0x00  // Latch count value
#define PIT_ACCESS_LOBYTE    0x10  // Access low byte only
#define PIT_ACCESS_HIBYTE    0x20  // Access high byte only
#define PIT_ACCESS_LOHI      0x30  // Access low byte, then high byte

#define PIT_MODE_0           0x00  // Interrupt on terminal count
#define PIT_MODE_1           0x02  // Hardware re-triggerable one-shot
#define PIT_MODE_2           0x04  // Rate generator
#define PIT_MODE_3           0x06  // Square wave generator
#define PIT_MODE_4           0x08  // Software triggered strobe
#define PIT_MODE_5           0x0A  // Hardware triggered strobe

#define PIT_BINARY_MODE      0x00
#define PIT_BCD_MODE         0x01

// PIT frequency
#define PIT_BASE_FREQUENCY   1193182  // 1.193182 MHz

// Common timer frequencies
#define TIMER_FREQ_1000HZ    1000     // 1000 Hz = 1ms tick
#define TIMER_FREQ_100HZ     100      // 100 Hz = 10ms tick
#define TIMER_FREQ_50HZ      50       // 50 Hz = 20ms tick
#define TIMER_FREQ_18HZ      18       // 18.2 Hz (default BIOS)

// Timer statistics
typedef struct {
    uint64_t ticks;           // Total timer ticks since boot
    uint64_t frequency;       // Timer frequency in Hz
    uint64_t milliseconds;    // Milliseconds since boot
    uint64_t seconds;         // Seconds since boot
} timer_stats_t;

// Timer callback function type
typedef void (*timer_callback_t)(void);

// Timer initialization
void timer_init(uint32_t frequency);

// Timer control
void timer_set_frequency(uint32_t frequency);
uint32_t timer_get_frequency(void);

// Time getters
uint64_t timer_get_ticks(void);
uint64_t timer_get_milliseconds(void);
uint64_t timer_get_seconds(void);

// Delay functions
void timer_sleep(uint32_t milliseconds);
void timer_wait_ticks(uint32_t ticks);

// Timer callback registration
void timer_register_callback(timer_callback_t callback);

// Timer IRQ handler (called from IDT)
void timer_irq_handler(void);

// Statistics
void timer_print_stats(void);

#endif // _KERNEL_TIMER_H_
