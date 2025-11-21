/**
 * AuroraOS Kernel - Keyboard Driver Implementation
 *
 * PS/2 keyboard driver with Scancode Set 1 translation
 */

#include "keyboard.h"
#include "io.h"
#include "console.h"
#include "types.h"

// Scancode Set 1 translation table (US QWERTY layout)
// Index = scancode, Value = ASCII character (unshifted)
static const char scancode_to_ascii[128] = {
    0,    0,   '1',  '2',  '3',  '4',  '5',  '6',   // 0x00-0x07
    '7',  '8', '9',  '0',  '-',  '=',  '\b', '\t',  // 0x08-0x0F (backspace, tab)
    'q',  'w', 'e',  'r',  't',  'y',  'u',  'i',   // 0x10-0x17
    'o',  'p', '[',  ']',  '\n', 0,    'a',  's',   // 0x18-0x1F (enter, ctrl)
    'd',  'f', 'g',  'h',  'j',  'k',  'l',  ';',   // 0x20-0x27
    '\'', '`', 0,    '\\', 'z',  'x',  'c',  'v',   // 0x28-0x2F (left shift)
    'b',  'n', 'm',  ',',  '.',  '/',  0,    '*',   // 0x30-0x37 (right shift)
    0,    ' ', 0,    0,    0,    0,    0,    0,     // 0x38-0x3F (alt, capslock, F1-F4)
    0,    0,   0,    0,    0,    0,    0,    '7',   // 0x40-0x47 (F5-F10, numlock, scroll, numpad)
    '8',  '9', '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F (numpad)
    '2',  '3', '0',  '.',  0,    0,    0,    0,     // 0x50-0x57 (numpad, F11)
    0,    0,   0,    0,    0,    0,    0,    0,     // 0x58-0x5F (F12)
    0,    0,   0,    0,    0,    0,    0,    0,     // 0x60-0x67
    0,    0,   0,    0,    0,    0,    0,    0,     // 0x68-0x6F
    0,    0,   0,    0,    0,    0,    0,    0,     // 0x70-0x77
    0,    0,   0,    0,    0,    0,    0,    0      // 0x78-0x7F
};

// Scancode Set 1 translation table (shifted)
static const char scancode_to_ascii_shifted[128] = {
    0,    0,   '!',  '@',  '#',  '$',  '%',  '^',   // 0x00-0x07
    '&',  '*', '(',  ')',  '_',  '+',  '\b', '\t',  // 0x08-0x0F
    'Q',  'W', 'E',  'R',  'T',  'Y',  'U',  'I',   // 0x10-0x17
    'O',  'P', '{',  '}',  '\n', 0,    'A',  'S',   // 0x18-0x1F
    'D',  'F', 'G',  'H',  'J',  'K',  'L',  ':',   // 0x20-0x27
    '"',  '~', 0,    '|',  'Z',  'X',  'C',  'V',   // 0x28-0x2F
    'B',  'N', 'M',  '<',  '>',  '?',  0,    '*',   // 0x30-0x37
    0,    ' ', 0,    0,    0,    0,    0,    0,     // 0x38-0x3F
    0,    0,   0,    0,    0,    0,    0,    '7',   // 0x40-0x47
    '8',  '9', '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F
    '2',  '3', '0',  '.',  0,    0,    0,    0,     // 0x50-0x57
    0,    0,   0,    0,    0,    0,    0,    0,     // 0x58-0x5F
    0,    0,   0,    0,    0,    0,    0,    0,     // 0x60-0x67
    0,    0,   0,    0,    0,    0,    0,    0,     // 0x68-0x6F
    0,    0,   0,    0,    0,    0,    0,    0,     // 0x70-0x77
    0,    0,   0,    0,    0,    0,    0,    0      // 0x78-0x7F
};

// Keyboard state
static struct {
    uint8_t buffer[KEYBOARD_BUFFER_SIZE];
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t count;
    uint8_t flags;
    bool initialized;
    keyboard_stats_t stats;
} kbd_state = {0};

/**
 * Translate scancode to ASCII character
 */
static char scancode_to_char(uint8_t scancode) {
    if (scancode >= 128) {
        return 0;  // Extended or release code
    }

    bool shift_pressed = (kbd_state.flags & (KBD_FLAG_LSHIFT | KBD_FLAG_RSHIFT)) != 0;
    bool caps_on = (kbd_state.flags & KBD_FLAG_CAPSLOCK) != 0;

    char ch;
    if (shift_pressed) {
        ch = scancode_to_ascii_shifted[scancode];
    } else {
        ch = scancode_to_ascii[scancode];
    }

    // Apply caps lock to letters only
    if (caps_on && ch >= 'a' && ch <= 'z') {
        ch = ch - 'a' + 'A';
    } else if (caps_on && ch >= 'A' && ch <= 'Z' && !shift_pressed) {
        // If caps is on but shift is not pressed, we got uppercase from table
        // Keep it uppercase (this handles the shifted table case)
    }

    return ch;
}

/**
 * Add character to keyboard buffer
 */
static void keyboard_buffer_push(char ch) {
    if (kbd_state.count >= KEYBOARD_BUFFER_SIZE) {
        kbd_state.stats.buffer_overruns++;
        return;  // Buffer full
    }

    kbd_state.buffer[kbd_state.write_pos] = (uint8_t)ch;
    kbd_state.write_pos = (kbd_state.write_pos + 1) % KEYBOARD_BUFFER_SIZE;
    kbd_state.count++;
}

/**
 * Get character from keyboard buffer
 */
static char keyboard_buffer_pop(void) {
    if (kbd_state.count == 0) {
        return 0;  // Buffer empty
    }

    char ch = (char)kbd_state.buffer[kbd_state.read_pos];
    kbd_state.read_pos = (kbd_state.read_pos + 1) % KEYBOARD_BUFFER_SIZE;
    kbd_state.count--;
    return ch;
}

/**
 * Keyboard IRQ handler (IRQ 1)
 * Called from IDT when keyboard interrupt fires
 */
void keyboard_irq_handler(void) {
    if (!kbd_state.initialized) {
        return;
    }

    // Read scancode from keyboard
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    kbd_state.stats.total_scancodes++;

    // Check if this is a key release (bit 7 set)
    bool released = (scancode & KEY_RELEASE_MASK) != 0;
    uint8_t key = scancode & ~KEY_RELEASE_MASK;

    if (released) {
        kbd_state.stats.total_releases++;

        // Handle modifier key releases
        switch (key) {
            case KEY_LSHIFT:
                kbd_state.flags &= ~KBD_FLAG_LSHIFT;
                break;
            case KEY_RSHIFT:
                kbd_state.flags &= ~KBD_FLAG_RSHIFT;
                break;
            case KEY_LCTRL:
                kbd_state.flags &= ~KBD_FLAG_LCTRL;
                break;
            case KEY_LALT:
                kbd_state.flags &= ~KBD_FLAG_LALT;
                break;
        }
    } else {
        // Key pressed
        kbd_state.stats.total_keypresses++;

        // Handle modifier keys
        switch (key) {
            case KEY_LSHIFT:
                kbd_state.flags |= KBD_FLAG_LSHIFT;
                break;
            case KEY_RSHIFT:
                kbd_state.flags |= KBD_FLAG_RSHIFT;
                break;
            case KEY_LCTRL:
                kbd_state.flags |= KBD_FLAG_LCTRL;
                break;
            case KEY_LALT:
                kbd_state.flags |= KBD_FLAG_LALT;
                break;
            case KEY_CAPSLOCK:
                // Toggle caps lock
                kbd_state.flags ^= KBD_FLAG_CAPSLOCK;
                keyboard_set_leds(
                    (kbd_state.flags & KBD_FLAG_CAPSLOCK) != 0,
                    (kbd_state.flags & KBD_FLAG_NUMLOCK) != 0,
                    (kbd_state.flags & KBD_FLAG_SCROLLLOCK) != 0
                );
                break;
            case KEY_NUMLOCK:
                kbd_state.flags ^= KBD_FLAG_NUMLOCK;
                keyboard_set_leds(
                    (kbd_state.flags & KBD_FLAG_CAPSLOCK) != 0,
                    (kbd_state.flags & KBD_FLAG_NUMLOCK) != 0,
                    (kbd_state.flags & KBD_FLAG_SCROLLLOCK) != 0
                );
                break;
            case KEY_SCROLLLOCK:
                kbd_state.flags ^= KBD_FLAG_SCROLLLOCK;
                keyboard_set_leds(
                    (kbd_state.flags & KBD_FLAG_CAPSLOCK) != 0,
                    (kbd_state.flags & KBD_FLAG_NUMLOCK) != 0,
                    (kbd_state.flags & KBD_FLAG_SCROLLLOCK) != 0
                );
                break;
            default:
                // Translate and buffer printable characters
                char ch = scancode_to_char(key);
                if (ch != 0) {
                    keyboard_buffer_push(ch);

                    // Echo to console (can be disabled later)
                    char str[2] = {ch, '\0'};
                    console_print(str);
                }
                break;
        }
    }

    // Note: EOI is sent by IDT IRQ handler
}

/**
 * Check if keyboard has data available
 */
bool keyboard_has_key(void) {
    return kbd_state.count > 0;
}

/**
 * Get character from keyboard (blocking)
 */
char keyboard_getchar(void) {
    // Wait for key to be available
    while (!keyboard_has_key()) {
        __asm__ __volatile__("hlt");  // Wait for interrupt
    }
    return keyboard_buffer_pop();
}

/**
 * Get keyboard event (not implemented yet)
 */
keyboard_event_t keyboard_get_event(void) {
    keyboard_event_t event = {0};
    // TODO: Implement full event system
    return event;
}

/**
 * Query modifier key states
 */
bool keyboard_is_shift_pressed(void) {
    return (kbd_state.flags & (KBD_FLAG_LSHIFT | KBD_FLAG_RSHIFT)) != 0;
}

bool keyboard_is_ctrl_pressed(void) {
    return (kbd_state.flags & KBD_FLAG_LCTRL) != 0;
}

bool keyboard_is_alt_pressed(void) {
    return (kbd_state.flags & KBD_FLAG_LALT) != 0;
}

bool keyboard_is_capslock_on(void) {
    return (kbd_state.flags & KBD_FLAG_CAPSLOCK) != 0;
}

/**
 * Flush keyboard buffer
 */
void keyboard_flush_buffer(void) {
    kbd_state.read_pos = 0;
    kbd_state.write_pos = 0;
    kbd_state.count = 0;
}

/**
 * Get number of characters in buffer
 */
uint32_t keyboard_buffer_count(void) {
    return kbd_state.count;
}

/**
 * Set keyboard LEDs
 */
void keyboard_set_leds(bool capslock, bool numlock, bool scrolllock) {
    uint8_t led_byte = 0;

    if (scrolllock) led_byte |= 0x01;
    if (numlock)    led_byte |= 0x02;
    if (capslock)   led_byte |= 0x04;

    // Wait for keyboard to be ready
    while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_INPUT_FULL) {
        // Wait
    }

    // Send LED command
    outb(KEYBOARD_DATA_PORT, 0xED);

    // Wait for ACK
    while (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL)) {
        // Wait
    }
    inb(KEYBOARD_DATA_PORT);  // Read ACK

    // Send LED byte
    while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_INPUT_FULL) {
        // Wait
    }
    outb(KEYBOARD_DATA_PORT, led_byte);

    // Wait for ACK
    while (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL)) {
        // Wait
    }
    inb(KEYBOARD_DATA_PORT);  // Read ACK
}

/**
 * Initialize keyboard driver
 */
void keyboard_init(void) {
    console_print("[KBD] Initializing PS/2 keyboard driver...\n");

    // Reset state
    kbd_state.read_pos = 0;
    kbd_state.write_pos = 0;
    kbd_state.count = 0;
    kbd_state.flags = 0;
    kbd_state.stats.total_scancodes = 0;
    kbd_state.stats.total_keypresses = 0;
    kbd_state.stats.total_releases = 0;
    kbd_state.stats.buffer_overruns = 0;

    // Flush any pending data
    while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL) {
        inb(KEYBOARD_DATA_PORT);
    }

    kbd_state.initialized = true;

    console_print("[KBD] PS/2 keyboard initialized\n");
    console_print("[KBD] Buffer size: ");
    console_print_dec(KEYBOARD_BUFFER_SIZE);
    console_print(" characters\n");
    console_print("[KBD] Layout: US QWERTY (Scancode Set 1)\n");
}

/**
 * Print keyboard statistics
 */
void keyboard_print_stats(void) {
    if (!kbd_state.initialized) {
        console_print("[KBD] Not initialized\n");
        return;
    }

    console_print("\n[KBD] Statistics:\n");
    console_print("  Total Scancodes: ");
    console_print_dec(kbd_state.stats.total_scancodes);
    console_print("\n  Key Presses:     ");
    console_print_dec(kbd_state.stats.total_keypresses);
    console_print("\n  Key Releases:    ");
    console_print_dec(kbd_state.stats.total_releases);
    console_print("\n  Buffer Count:    ");
    console_print_dec(kbd_state.count);
    console_print("/");
    console_print_dec(KEYBOARD_BUFFER_SIZE);
    console_print("\n  Buffer Overruns: ");
    console_print_dec(kbd_state.stats.buffer_overruns);
    console_print("\n");

    console_print("  Modifiers: ");
    if (kbd_state.flags & KBD_FLAG_LSHIFT) console_print("LSHIFT ");
    if (kbd_state.flags & KBD_FLAG_RSHIFT) console_print("RSHIFT ");
    if (kbd_state.flags & KBD_FLAG_LCTRL) console_print("CTRL ");
    if (kbd_state.flags & KBD_FLAG_LALT) console_print("ALT ");
    if (kbd_state.flags & KBD_FLAG_CAPSLOCK) console_print("CAPS ");
    if (kbd_state.flags & KBD_FLAG_NUMLOCK) console_print("NUM ");
    if (kbd_state.flags & KBD_FLAG_SCROLLLOCK) console_print("SCROLL ");
    console_print("\n");
}
