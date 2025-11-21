/**
 * AuroraOS Kernel - Global Descriptor Table (GDT)
 *
 * Defines structures and functions for x86_64 segmentation
 */

#ifndef _KERNEL_GDT_H_
#define _KERNEL_GDT_H_

#include "types.h"

// GDT entry structure (8 bytes)
typedef struct {
    uint16_t limit_low;      // Limit bits 0-15
    uint16_t base_low;       // Base bits 0-15
    uint8_t  base_mid;       // Base bits 16-23
    uint8_t  access;         // Access flags
    uint8_t  granularity;    // Granularity and limit bits 16-19
    uint8_t  base_high;      // Base bits 24-31
} __attribute__((packed)) gdt_entry_t;

// GDT pointer structure (used by LGDT instruction)
typedef struct {
    uint16_t limit;          // Size of GDT - 1
    uint64_t base;           // Base address of GDT
} __attribute__((packed)) gdt_ptr_t;

// Number of GDT entries
#define GDT_ENTRIES 5

// GDT entry indices
#define GDT_NULL        0  // Null descriptor (required)
#define GDT_KERNEL_CODE 1  // Kernel code segment (0x08)
#define GDT_KERNEL_DATA 2  // Kernel data segment (0x10)
#define GDT_USER_CODE   3  // User code segment (0x18) - future
#define GDT_USER_DATA   4  // User data segment (0x20) - future

// Segment selector values (index * 8)
#define KERNEL_CODE_SELECTOR 0x08
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR   0x18
#define USER_DATA_SELECTOR   0x20

// Access byte flags
#define GDT_ACCESS_PRESENT      0x80  // Present bit
#define GDT_ACCESS_PRIV_RING0   0x00  // Privilege level 0 (kernel)
#define GDT_ACCESS_PRIV_RING3   0x60  // Privilege level 3 (user)
#define GDT_ACCESS_CODE_DATA    0x10  // Code or Data segment
#define GDT_ACCESS_EXECUTABLE   0x08  // Executable (code segment)
#define GDT_ACCESS_DIRECTION    0x04  // Direction/Conforming
#define GDT_ACCESS_RW           0x02  // Readable (code) / Writable (data)
#define GDT_ACCESS_ACCESSED     0x01  // Accessed bit (set by CPU)

// Granularity byte flags
#define GDT_GRAN_GRANULARITY    0x80  // Granularity (1 = 4KB blocks)
#define GDT_GRAN_32BIT          0x40  // Size (1 = 32-bit protected mode)
#define GDT_GRAN_64BIT          0x20  // Long mode (1 = 64-bit long mode)

// Common access patterns
#define GDT_KERNEL_CODE_ACCESS  (GDT_ACCESS_PRESENT | GDT_ACCESS_PRIV_RING0 | \
                                 GDT_ACCESS_CODE_DATA | GDT_ACCESS_EXECUTABLE | \
                                 GDT_ACCESS_RW)

#define GDT_KERNEL_DATA_ACCESS  (GDT_ACCESS_PRESENT | GDT_ACCESS_PRIV_RING0 | \
                                 GDT_ACCESS_CODE_DATA | GDT_ACCESS_RW)

#define GDT_USER_CODE_ACCESS    (GDT_ACCESS_PRESENT | GDT_ACCESS_PRIV_RING3 | \
                                 GDT_ACCESS_CODE_DATA | GDT_ACCESS_EXECUTABLE | \
                                 GDT_ACCESS_RW)

#define GDT_USER_DATA_ACCESS    (GDT_ACCESS_PRESENT | GDT_ACCESS_PRIV_RING3 | \
                                 GDT_ACCESS_CODE_DATA | GDT_ACCESS_RW)

// Function prototypes
void gdt_init(void);
void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit,
                  uint8_t access, uint8_t gran);
void gdt_load(void);

// Assembly functions (defined in gdt_asm.S)
extern void gdt_load_asm(gdt_ptr_t *gdt_ptr);
extern void gdt_reload_segments(void);

#endif // _KERNEL_GDT_H_
