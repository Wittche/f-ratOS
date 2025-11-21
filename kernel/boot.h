/**
 * AuroraOS Kernel - Boot Information
 * Structure passed from bootloader to kernel
 */

#ifndef _KERNEL_BOOT_H_
#define _KERNEL_BOOT_H_

#include "types.h"

#define AURORA_BOOT_MAGIC 0x41555230524F0000ULL  // "AUR\0RO\0\0"

// Memory descriptor (matches EFI format)
typedef struct {
    uint32_t type;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
} memory_descriptor_t;

// Graphics mode information
typedef struct {
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixels_per_scan_line;
    uint32_t pixel_format;  // 0=RGB, 1=BGR
    uint64_t framebuffer_base;
    uint64_t framebuffer_size;
} graphics_info_t;

// Boot information passed from bootloader
typedef struct {
    uint64_t magic;                       // Magic number for validation
    memory_descriptor_t *memory_map;       // Physical memory map
    uint64_t memory_map_size;              // Size in bytes
    uint64_t memory_map_descriptor_size;   // Size of each descriptor
    graphics_info_t *graphics_info;        // Graphics mode info (or NULL)
    void *acpi_rsdp;                       // ACPI RSDP pointer (or NULL)
    uint64_t kernel_physical_base;         // Physical address of kernel
    uint64_t kernel_virtual_base;          // Virtual address of kernel
    uint64_t kernel_size;                  // Size of kernel in bytes
} boot_info_t;

#endif // _KERNEL_BOOT_H_
