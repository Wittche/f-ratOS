/**
 * AuroraOS Kernel - PS/2 Keyboard Driver
 *
 * 8042 PS/2 Controller keyboard driver with scancode translation
 */

#ifndef _KERNEL_KEYBOARD_H_
#define _KERNEL_KEYBOARD_H_

#include "types.h"

// 8042 PS/2 Controller ports
#define KEYBOARD_DATA_PORT    0x60  // Data port (read/write)
#define KEYBOARD_STATUS_PORT  0x64  // Status port (read)
#define KEYBOARD_COMMAND_PORT 0x64  // Command port (write)

// Status register bits
#define KEYBOARD_STATUS_OUTPUT_FULL  0x01  // Output buffer full (data available)
#define KEYBOARD_STATUS_INPUT_FULL   0x02  // Input buffer full (can't write)
#define KEYBOARD_STATUS_SYSTEM       0x04  // System flag
#define KEYBOARD_STATUS_COMMAND      0x08  // Command (1) or data (0)
#define KEYBOARD_STATUS_TIMEOUT      0x40  // Timeout error
#define KEYBOARD_STATUS_PARITY       0x80  // Parity error

// Keyboard buffer size
#define KEYBOARD_BUFFER_SIZE 256

// Special key scancodes (Scancode Set 1)
#define KEY_ESCAPE       0x01
#define KEY_BACKSPACE    0x0E
#define KEY_TAB          0x0F
#define KEY_ENTER        0x1C
#define KEY_LCTRL        0x1D
#define KEY_LSHIFT       0x2A
#define KEY_RSHIFT       0x36
#define KEY_LALT         0x38
#define KEY_SPACE        0x39
#define KEY_CAPSLOCK     0x3A
#define KEY_F1           0x3B
#define KEY_F2           0x3C
#define KEY_F3           0x3D
#define KEY_F4           0x3E
#define KEY_F5           0x3F
#define KEY_F6           0x40
#define KEY_F7           0x41
#define KEY_F8           0x42
#define KEY_F9           0x43
#define KEY_F10          0x44
#define KEY_NUMLOCK      0x45
#define KEY_SCROLLLOCK   0x46
#define KEY_F11          0x57
#define KEY_F12          0x58

// Key release bit (scancode | 0x80)
#define KEY_RELEASE_MASK 0x80

// Keyboard state flags
#define KBD_FLAG_LSHIFT    (1 << 0)
#define KBD_FLAG_RSHIFT    (1 << 1)
#define KBD_FLAG_LCTRL     (1 << 2)
#define KBD_FLAG_LALT      (1 << 3)
#define KBD_FLAG_CAPSLOCK  (1 << 4)
#define KBD_FLAG_NUMLOCK   (1 << 5)
#define KBD_FLAG_SCROLLLOCK (1 << 6)

// Keyboard event structure
typedef struct {
    uint8_t scancode;     // Raw scancode
    char ascii;           // Translated ASCII character (0 if non-printable)
    bool pressed;         // true = key pressed, false = key released
    uint8_t flags;        // Modifier flags (shift, ctrl, alt, etc.)
} keyboard_event_t;

// Keyboard statistics
typedef struct {
    uint64_t total_scancodes;   // Total scancodes received
    uint64_t total_keypresses;  // Total key presses
    uint64_t total_releases;    // Total key releases
    uint64_t buffer_overruns;   // Buffer overflow count
} keyboard_stats_t;

// Keyboard initialization
void keyboard_init(void);

// Keyboard IRQ handler (called from IDT)
void keyboard_irq_handler(void);

// Key reading functions
bool keyboard_has_key(void);
char keyboard_getchar(void);
keyboard_event_t keyboard_get_event(void);

// Keyboard state query
bool keyboard_is_shift_pressed(void);
bool keyboard_is_ctrl_pressed(void);
bool keyboard_is_alt_pressed(void);
bool keyboard_is_capslock_on(void);

// Keyboard buffer management
void keyboard_flush_buffer(void);
uint32_t keyboard_buffer_count(void);

// LED control
void keyboard_set_leds(bool capslock, bool numlock, bool scrolllock);

// Statistics
void keyboard_print_stats(void);

#endif // _KERNEL_KEYBOARD_H_
