/**
 * AuroraOS Kernel - I/O Port Functions
 *
 * Low-level port I/O for x86_64
 */

#ifndef _KERNEL_IO_H_
#define _KERNEL_IO_H_

#include "types.h"

/**
 * Read a byte from an I/O port
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Write a byte to an I/O port
 */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * Read a word (16-bit) from an I/O port
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Write a word (16-bit) to an I/O port
 */
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * Read a dword (32-bit) from an I/O port
 */
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ __volatile__("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Write a dword (32-bit) to an I/O port
 */
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ __volatile__("outl %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * I/O wait - short delay for old hardware
 */
static inline void io_wait(void) {
    outb(0x80, 0);  // Write to unused port 0x80
}

#endif // _KERNEL_IO_H_
