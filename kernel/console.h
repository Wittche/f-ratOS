/**
 * AuroraOS Kernel - Console Output
 * Early console for kernel debugging
 */

#ifndef _KERNEL_CONSOLE_H_
#define _KERNEL_CONSOLE_H_

#include "types.h"

// Initialize console (framebuffer or serial)
void console_init(void *framebuffer, uint32_t width, uint32_t height, uint32_t pitch);

// Print string to console
void console_print(const char *str);

// Print hex number
void console_print_hex(uint64_t num);

// Print decimal number
void console_print_dec(uint64_t num);

// Clear console
void console_clear(void);

// Set text color (RGB or attribute)
void console_set_color(uint32_t fg, uint32_t bg);

#endif // _KERNEL_CONSOLE_H_
