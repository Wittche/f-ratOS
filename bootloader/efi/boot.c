/**
 * AuroraOS UEFI Bootloader
 *
 * Entry point for the operating system
 * Loads the kernel and transfers control
 */

#include "efi.h"

// Global EFI pointers
static efi_handle_t g_image_handle;
static efi_system_table_t *g_system_table;
static efi_simple_text_output_protocol_t *g_console;

// Helper: Print a string to EFI console
static void print(const char16_t *str) {
    if (g_console && g_console->output_string) {
        g_console->output_string(g_console, (char16_t *)str);
    }
}

// Helper: Print ASCII string (convert to UCS-2)
static void print_ascii(const char *str) {
    char16_t buffer[256];
    int i = 0;

    while (str[i] && i < 255) {
        buffer[i] = (char16_t)str[i];
        i++;
    }
    buffer[i] = 0;

    print(buffer);
}

// Helper: Print hex number
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

// Helper: Compare two GUIDs
static bool_t guid_equal(efi_guid_t *a, efi_guid_t *b) {
    if (a->data1 != b->data1) return FALSE;
    if (a->data2 != b->data2) return FALSE;
    if (a->data3 != b->data3) return FALSE;
    for (int i = 0; i < 8; i++) {
        if (a->data4[i] != b->data4[i]) return FALSE;
    }
    return TRUE;
}

// Helper: Wait for key press
static void wait_for_key(void) {
    print(u"\r\nPress any key to continue...\r\n");

    // Simple busy wait - not ideal but works
    for (volatile uint64_t i = 0; i < 100000000; i++);
}

// Get memory map from UEFI
static efi_status_t get_memory_map(efi_memory_descriptor_t **map, uintn_t *map_size,
                                   uintn_t *map_key, uintn_t *descriptor_size) {
    efi_status_t status;
    efi_boot_services_t *bs = g_system_table->boot_services;

    *map_size = 0;
    uint32_t desc_version;

    // First call to get size
    status = bs->get_memory_map(map_size, NULL, map_key, descriptor_size, &desc_version);
    if (status != EFI_BUFFER_TOO_SMALL) {
        return status;
    }

    // Add extra space for new allocations
    *map_size += (*descriptor_size * 2);

    // Allocate buffer
    status = bs->allocate_pool(EFI_LOADER_DATA, *map_size, (void **)map);
    if (status != EFI_SUCCESS) {
        return status;
    }

    // Get actual memory map
    status = bs->get_memory_map(map_size, *map, map_key, descriptor_size, &desc_version);
    if (status != EFI_SUCCESS) {
        bs->free_pool(*map);
        return status;
    }

    return EFI_SUCCESS;
}

// Load kernel from file (stub - to be implemented)
static efi_status_t load_kernel(void **kernel_entry, uint64_t *kernel_size) {
    print_ascii("Loading kernel from disk...\r\n");

    // TODO: Implement actual kernel loading from file
    // For now, this is a stub that will be expanded later

    print_ascii("Kernel loading not yet implemented\r\n");
    *kernel_entry = (void *)0x100000;  // Placeholder
    *kernel_size = 0;

    return EFI_SUCCESS;
}

// Boot information structure to pass to kernel
typedef struct {
    uint64_t magic;                      // Magic number for validation
    efi_memory_descriptor_t *memory_map;
    uintn_t memory_map_size;
    uintn_t memory_map_descriptor_size;
    efi_graphics_output_protocol_mode_t *graphics_mode;
    void *acpi_table;                    // ACPI RSDP pointer
} aurora_boot_info_t;

#define AURORA_BOOT_MAGIC 0x41555230524F0000ULL  // "AUR\0RO\0\0"

/**
 * UEFI Entry Point
 * Called by UEFI firmware when bootloader starts
 */
efi_status_t efi_main(efi_handle_t image_handle, efi_system_table_t *system_table) {
    efi_status_t status;

    // Save global pointers
    g_image_handle = image_handle;
    g_system_table = system_table;
    g_console = system_table->con_out;

    // Clear screen
    if (g_console->clear_screen) {
        g_console->clear_screen(g_console);
    }

    // Print welcome banner
    print(u"=====================================\r\n");
    print(u"   AuroraOS UEFI Bootloader v0.1\r\n");
    print(u"=====================================\r\n\r\n");

    // Print firmware information
    print(u"UEFI Firmware: ");
    if (system_table->firmware_vendor) {
        print(system_table->firmware_vendor);
    }
    print(u"\r\nRevision: ");
    print_hex(system_table->firmware_revision);
    print(u"\r\n\r\n");

    // Get memory map
    print_ascii("Getting memory map...\r\n");
    efi_memory_descriptor_t *memory_map = NULL;
    uintn_t memory_map_size = 0;
    uintn_t map_key = 0;
    uintn_t descriptor_size = 0;

    status = get_memory_map(&memory_map, &memory_map_size, &map_key, &descriptor_size);
    if (status != EFI_SUCCESS) {
        print_ascii("ERROR: Failed to get memory map! Status: ");
        print_hex(status);
        print(u"\r\n");
        wait_for_key();
        return status;
    }

    print_ascii("Memory map obtained: ");
    print_hex(memory_map_size / descriptor_size);
    print_ascii(" entries\r\n");

    // Get Graphics Output Protocol
    print_ascii("Locating Graphics Output Protocol...\r\n");
    efi_guid_t gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    efi_graphics_output_protocol_t *gop = NULL;

    status = system_table->boot_services->locate_protocol(&gop_guid, NULL, (void **)&gop);
    if (status == EFI_SUCCESS && gop) {
        print_ascii("GOP found - Framebuffer: ");
        print_hex(gop->mode->frame_buffer_base);
        print(u"\r\n");
        print_ascii("Resolution: ");
        print_hex(gop->mode->info->horizontal_resolution);
        print(u"x");
        print_hex(gop->mode->info->vertical_resolution);
        print(u"\r\n\r\n");
    } else {
        print_ascii("WARNING: GOP not found, graphics may not work\r\n\r\n");
    }

    // Load kernel
    void *kernel_entry = NULL;
    uint64_t kernel_size = 0;

    status = load_kernel(&kernel_entry, &kernel_size);
    if (status != EFI_SUCCESS) {
        print_ascii("ERROR: Failed to load kernel!\r\n");
        wait_for_key();
        return status;
    }

    // Prepare boot info structure
    aurora_boot_info_t boot_info;
    boot_info.magic = AURORA_BOOT_MAGIC;
    boot_info.memory_map = memory_map;
    boot_info.memory_map_size = memory_map_size;
    boot_info.memory_map_descriptor_size = descriptor_size;
    boot_info.graphics_mode = gop ? gop->mode : NULL;
    boot_info.acpi_table = NULL;  // TODO: Find ACPI table

    print_ascii("Boot info prepared\r\n");
    print_ascii("Kernel entry point: ");
    print_hex((uint64_t)kernel_entry);
    print(u"\r\n\r\n");

    // Exit Boot Services
    print_ascii("Exiting UEFI Boot Services...\r\n");
    status = system_table->boot_services->exit_boot_services(image_handle, map_key);
    if (status != EFI_SUCCESS) {
        print_ascii("ERROR: Failed to exit boot services! Status: ");
        print_hex(status);
        print(u"\r\n");

        // Try to get memory map again
        system_table->boot_services->free_pool(memory_map);
        status = get_memory_map(&memory_map, &memory_map_size, &map_key, &descriptor_size);
        if (status != EFI_SUCCESS) {
            return status;
        }

        // Retry
        status = system_table->boot_services->exit_boot_services(image_handle, map_key);
        if (status != EFI_SUCCESS) {
            return status;
        }
    }

    // We are now in runtime mode - no more boot services!
    // Jump to kernel

    // TODO: Implement actual kernel jump
    // For now, just halt
    while (1) {
        __asm__ __volatile__("hlt");
    }

    return EFI_SUCCESS;
}
