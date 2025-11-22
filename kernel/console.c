/**
 * AuroraOS Kernel - Console Implementation
 * Simple text console for early kernel output
 * Outputs to both VGA text mode and serial port (COM1)
 */

#include "console.h"
#include "types.h"
#include "serial.h"

// VGA text mode constants
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// Serial port configuration
#define CONSOLE_SERIAL_PORT SERIAL_COM1
#define CONSOLE_BAUD_DIVISOR 1  // 115200 baud

// Console state
static struct {
    uint16_t *buffer;      // Framebuffer or VGA memory
    uint32_t width;
    uint32_t height;
    uint32_t row;
    uint32_t col;
    uint8_t color;
    bool is_vga;           // Using VGA text mode?
} console;

// VGA color codes
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// Make VGA color attribute
static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

// Make VGA entry
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// Initialize console
void console_init(void *framebuffer, uint32_t width, uint32_t height, uint32_t pitch) {
    (void)pitch;  // Unused for now, suppress warning

    // Initialize serial port first for early debug output
    serial_init(CONSOLE_SERIAL_PORT, CONSOLE_BAUD_DIVISOR);
    serial_write_string(CONSOLE_SERIAL_PORT, "\r\n=== AuroraOS Serial Console ===\r\n");

    if (framebuffer && width > 0 && height > 0) {
        // Framebuffer mode (not implemented yet)
        console.buffer = (uint16_t*)framebuffer;
        console.width = width;
        console.height = height;
        console.is_vga = false;
    } else {
        // VGA text mode fallback
        console.buffer = (uint16_t*)VGA_MEMORY;
        console.width = VGA_WIDTH;
        console.height = VGA_HEIGHT;
        console.is_vga = true;
    }

    console.row = 0;
    console.col = 0;
    console.color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);  // Beyaz yazı

    // Immediate test - write directly to VGA memory to verify it works
    if (console.is_vga) {
        volatile uint16_t *vga = (volatile uint16_t*)VGA_MEMORY;
        vga[0] = vga_entry('O', console.color);
        vga[1] = vga_entry('K', console.color);
        vga[2] = vga_entry('!', console.color);
    }

    serial_write_string(CONSOLE_SERIAL_PORT, "Console initialized\r\n");
}

// Clear console
void console_clear(void) {
    if (!console.buffer) return;

    for (uint32_t y = 0; y < console.height; y++) {
        for (uint32_t x = 0; x < console.width; x++) {
            const uint32_t index = y * console.width + x;
            console.buffer[index] = vga_entry(' ', console.color);
        }
    }

    console.row = 0;
    console.col = 0;
}

// Scroll console up one line
static void console_scroll(void) {
    if (!console.buffer) return;

    // Move all lines up
    for (uint32_t y = 1; y < console.height; y++) {
        for (uint32_t x = 0; x < console.width; x++) {
            const uint32_t src = y * console.width + x;
            const uint32_t dst = (y - 1) * console.width + x;
            console.buffer[dst] = console.buffer[src];
        }
    }

    // Clear last line
    const uint32_t last_line = (console.height - 1) * console.width;
    for (uint32_t x = 0; x < console.width; x++) {
        console.buffer[last_line + x] = vga_entry(' ', console.color);
    }

    console.row = console.height - 1;
}

// Put character at current position
static void console_putchar_at(char c, uint8_t color, uint32_t x, uint32_t y) {
    if (!console.buffer) return;
    const uint32_t index = y * console.width + x;
    console.buffer[index] = vga_entry(c, color);
}

// Put character and advance cursor
static void console_putchar(char c) {
    // Always write to serial port (works even if VGA buffer is not available)
    serial_write_byte(CONSOLE_SERIAL_PORT, c);

    if (!console.buffer) return;

    if (c == '\n') {
        console.col = 0;
        if (++console.row >= console.height) {
            console_scroll();
        }
        return;
    }

    if (c == '\r') {
        console.col = 0;
        return;
    }

    if (c == '\t') {
        console.col = (console.col + 8) & ~7;
        if (console.col >= console.width) {
            console.col = 0;
            if (++console.row >= console.height) {
                console_scroll();
            }
        }
        return;
    }

    console_putchar_at(c, console.color, console.col, console.row);

    if (++console.col >= console.width) {
        console.col = 0;
        if (++console.row >= console.height) {
            console_scroll();
        }
    }
}

// Print string
void console_print(const char *str) {
    if (!str) return;

    while (*str) {
        console_putchar(*str++);
    }
}

// Print hex number
void console_print_hex(uint64_t num) {
    const char *digits = "0123456789ABCDEF";
    char buffer[20];
    int i = 0;

    buffer[i++] = '0';
    buffer[i++] = 'x';

    if (num == 0) {
        buffer[i++] = '0';
    } else {
        char temp[16];
        int j = 0;

        while (num > 0) {
            temp[j++] = digits[num & 0xF];
            num >>= 4;
        }

        while (j > 0) {
            buffer[i++] = temp[--j];
        }
    }

    buffer[i] = '\0';
    console_print(buffer);
}

// Print decimal number
void console_print_dec(uint64_t num) {
    char buffer[32];
    int i = 0;

    if (num == 0) {
        console_putchar('0');
        return;
    }

    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    while (i > 0) {
        console_putchar(buffer[--i]);
    }
}

// Set text color
void console_set_color(uint32_t fg, uint32_t bg) {
    if (console.is_vga) {
        console.color = vga_entry_color((uint8_t)fg, (uint8_t)bg);
    } else {
        // TODO: Framebuffer color
        console.color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }
}
