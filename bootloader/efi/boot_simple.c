/**
 * AuroraOS UEFI Bootloader - Simplified Version
 *
 * Minimal bootloader to test kernel
 * Loads pre-loaded kernel and jumps to it
 */

#include "efi.h"

// Define NULL if not defined
#ifndef NULL
#define NULL ((void*)0)
#endif

// Include kernel boot structures
typedef struct {
    uint32_t type;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
} memory_descriptor_t;

typedef struct {
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixels_per_scan_line;
    uint32_t pixel_format;
    uint64_t framebuffer_base;
    uint64_t framebuffer_size;
} graphics_info_t;

typedef struct {
    uint64_t magic;
    memory_descriptor_t *memory_map;
    uint64_t memory_map_size;
    uint64_t memory_map_descriptor_size;
    graphics_info_t *graphics_info;
    void *acpi_rsdp;
    uint64_t kernel_physical_base;
    uint64_t kernel_virtual_base;
    uint64_t kernel_size;
} boot_info_t;

#define AURORA_BOOT_MAGIC 0x41555230524F0000ULL

// Global variables
static efi_system_table_t *g_st = NULL;
static efi_simple_text_output_protocol_t *g_con = NULL;

// Helper functions
static void print(const char16_t *str) {
    if (g_con && g_con->output_string) {
        g_con->output_string(g_con, (char16_t *)str);
    }
}

static void print_hex(uint64_t num) {
    char16_t buffer[20];
    const char16_t *hex = u"0123456789ABCDEF";
    int i = 0;

    buffer[i++] = '0';
    buffer[i++] = 'x';

    if (num == 0) {
        buffer[i++] = '0';
    } else {
        char16_t temp[16];
        int j = 0;
        while (num > 0) {
            temp[j++] = hex[num & 0xF];
            num >>= 4;
        }
        while (j > 0) {
            buffer[i++] = temp[--j];
        }
    }

    buffer[i] = 0;
    print(buffer);
}

// Kernel entry point type
typedef void (*kernel_entry_fn)(boot_info_t *boot_info);

/**
 * UEFI Entry Point
 */
efi_status_t efi_main(efi_handle_t image_handle, efi_system_table_t *system_table) {
    efi_status_t status;

    g_st = system_table;
    g_con = system_table->con_out;

    // Clear screen
    if (g_con->clear_screen) {
        g_con->clear_screen(g_con);
    }

    // Banner
    print(u"=====================================\r\n");
    print(u"   AuroraOS UEFI Bootloader v0.1\r\n");
    print(u"   (Simplified Test Version)\r\n");
    print(u"=====================================\r\n\r\n");

    // Get memory map
    print(u"Getting memory map...\r\n");

    efi_memory_descriptor_t *memory_map = NULL;
    uintn_t map_size = 0;
    uintn_t map_key = 0;
    uintn_t desc_size = 0;
    uint32_t desc_version = 0;

    // First call to get size
    status = system_table->boot_services->get_memory_map(
        &map_size, NULL, &map_key, &desc_size, &desc_version);

    if (status != EFI_BUFFER_TOO_SMALL) {
        print(u"ERROR: get_memory_map failed\r\n");
        while (1) __asm__("hlt");
    }

    // Allocate buffer (add extra space)
    map_size += desc_size * 2;
    status = system_table->boot_services->allocate_pool(
        EFI_LOADER_DATA, map_size, (void **)&memory_map);

    if (status != EFI_SUCCESS) {
        print(u"ERROR: allocate_pool failed\r\n");
        while (1) __asm__("hlt");
    }

    // Get actual map
    status = system_table->boot_services->get_memory_map(
        &map_size, memory_map, &map_key, &desc_size, &desc_version);

    if (status != EFI_SUCCESS) {
        print(u"ERROR: get_memory_map (2nd) failed\r\n");
        while (1) __asm__("hlt");
    }

    print(u"Memory map obtained: ");
    print_hex(map_size / desc_size);
    print(u" entries\r\n");

    // Get Graphics Output Protocol (GOP)
    print(u"Getting Graphics Output Protocol...\r\n");

    efi_guid_t gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_graphics_output_protocol_t *gop = NULL;

    status = system_table->boot_services->locate_protocol(
        &gop_guid, NULL, (void **)&gop);

    graphics_info_t gfx_info;
    graphics_info_t *gfx_info_ptr = NULL;

    if (status == EFI_SUCCESS && gop && gop->mode && gop->mode->info) {
        print(u"GOP found!\r\n");
        print(u"  Framebuffer: ");
        print_hex(gop->mode->frame_buffer_base);
        print(u"\r\n  Resolution: ");
        print_hex(gop->mode->info->horizontal_resolution);
        print(u"x");
        print_hex(gop->mode->info->vertical_resolution);
        print(u"\r\n");

        // Fill graphics info
        gfx_info.framebuffer_base = gop->mode->frame_buffer_base;
        gfx_info.framebuffer_size = gop->mode->frame_buffer_size;
        gfx_info.horizontal_resolution = gop->mode->info->horizontal_resolution;
        gfx_info.vertical_resolution = gop->mode->info->vertical_resolution;
        gfx_info.pixels_per_scan_line = gop->mode->info->pixels_per_scan_line;
        gfx_info.pixel_format = gop->mode->info->pixel_format;
        gfx_info_ptr = &gfx_info;
    } else {
        print(u"WARNING: GOP not available\r\n");
    }

    // Prepare boot_info structure
    print(u"\r\nPreparing boot_info structure...\r\n");

    boot_info_t boot_info;
    boot_info.magic = AURORA_BOOT_MAGIC;
    boot_info.memory_map = (memory_descriptor_t *)memory_map;
    boot_info.memory_map_size = map_size;
    boot_info.memory_map_descriptor_size = desc_size;
    boot_info.graphics_info = gfx_info_ptr;
    boot_info.acpi_rsdp = NULL;  // TODO: Find ACPI table
    boot_info.kernel_physical_base = 0x100000;  // 1MB
    boot_info.kernel_virtual_base = 0x100000;   // Identity mapped
    boot_info.kernel_size = 0;  // Unknown for now

    print(u"Boot info magic: ");
    print_hex(boot_info.magic);
    print(u"\r\n");

    // Kernel entry point
    // Entry is at 0x10000C (_start symbol, after multiboot header)
    kernel_entry_fn kernel_main = (kernel_entry_fn)0x10000C;

    print(u"Kernel entry point: ");
    print_hex((uint64_t)kernel_main);
    print(u"\r\n\r\n");

    // Exit boot services
    print(u"Exiting UEFI Boot Services...\r\n");

    status = system_table->boot_services->exit_boot_services(image_handle, map_key);

    if (status != EFI_SUCCESS) {
        // Map key might have changed, try again
        system_table->boot_services->free_pool(memory_map);

        map_size = 0;
        system_table->boot_services->get_memory_map(
            &map_size, NULL, &map_key, &desc_size, &desc_version);

        map_size += desc_size * 2;
        system_table->boot_services->allocate_pool(
            EFI_LOADER_DATA, map_size, (void **)&memory_map);

        system_table->boot_services->get_memory_map(
            &map_size, memory_map, &map_key, &desc_size, &desc_version);

        boot_info.memory_map = (memory_descriptor_t *)memory_map;
        boot_info.memory_map_size = map_size;

        status = system_table->boot_services->exit_boot_services(image_handle, map_key);

        if (status != EFI_SUCCESS) {
            // Can't print error here - boot services are partially disabled
            while (1) __asm__("hlt");
        }
    }

    // We are now in runtime mode!
    // No more boot services available

    // Jump to kernel
    kernel_main(&boot_info);

    // Should never return
    while (1) {
        __asm__ __volatile__("hlt");
    }

    return EFI_SUCCESS;
}
