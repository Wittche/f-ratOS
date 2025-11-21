/**
 * AuroraOS UEFI Bootloader
 *
 * Minimal UEFI type definitions and protocols
 * Based on UEFI Specification 2.10
 */

#ifndef _EFI_H_
#define _EFI_H_

// Basic types
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef char               int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;

typedef uint64_t           uintn_t;
typedef int64_t            intn_t;

typedef uint16_t           char16_t;  // UCS-2 character
typedef void               void_t;

// Boolean
typedef uint8_t            bool_t;
#define TRUE               1
#define FALSE              0

// EFI types
typedef uintn_t            efi_status_t;
typedef void*              efi_handle_t;
typedef uint64_t           efi_physical_address_t;
typedef uint64_t           efi_virtual_address_t;

// EFI Status codes
#define EFI_SUCCESS                      0
#define EFI_LOAD_ERROR                   (1ULL | (1ULL << 63))
#define EFI_INVALID_PARAMETER            (2ULL | (1ULL << 63))
#define EFI_UNSUPPORTED                  (3ULL | (1ULL << 63))
#define EFI_BAD_BUFFER_SIZE              (4ULL | (1ULL << 63))
#define EFI_BUFFER_TOO_SMALL             (5ULL | (1ULL << 63))
#define EFI_NOT_READY                    (6ULL | (1ULL << 63))
#define EFI_DEVICE_ERROR                 (7ULL | (1ULL << 63))
#define EFI_WRITE_PROTECTED              (8ULL | (1ULL << 63))
#define EFI_OUT_OF_RESOURCES             (9ULL | (1ULL << 63))
#define EFI_NOT_FOUND                    (14ULL | (1ULL << 63))

// GUID structure
typedef struct {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
} efi_guid_t;

// EFI Table Header
typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
} efi_table_header_t;

// Forward declarations
typedef struct _efi_simple_text_output_protocol efi_simple_text_output_protocol_t;
typedef struct _efi_simple_text_input_protocol efi_simple_text_input_protocol_t;
typedef struct _efi_boot_services efi_boot_services_t;
typedef struct _efi_runtime_services efi_runtime_services_t;

// EFI System Table
typedef struct {
    efi_table_header_t hdr;
    char16_t *firmware_vendor;
    uint32_t firmware_revision;
    efi_handle_t console_in_handle;
    efi_simple_text_input_protocol_t *con_in;
    efi_handle_t console_out_handle;
    efi_simple_text_output_protocol_t *con_out;
    efi_handle_t standard_error_handle;
    efi_simple_text_output_protocol_t *std_err;
    efi_runtime_services_t *runtime_services;
    efi_boot_services_t *boot_services;
    uintn_t number_of_table_entries;
    void *configuration_table;
} efi_system_table_t;

// Simple Text Output Protocol
typedef struct _efi_simple_text_output_protocol {
    void *reset;
    efi_status_t (*output_string)(efi_simple_text_output_protocol_t *this, char16_t *string);
    void *test_string;
    void *query_mode;
    void *set_mode;
    void *set_attribute;
    efi_status_t (*clear_screen)(efi_simple_text_output_protocol_t *this);
    void *set_cursor_position;
    void *enable_cursor;
    void *mode;
} efi_simple_text_output_protocol_t;

// Simple Text Input Protocol
typedef struct _efi_simple_text_input_protocol {
    void *reset;
    void *read_key_stroke;
    void *wait_for_key;
} efi_simple_text_input_protocol_t;

// Memory types
typedef enum {
    EFI_RESERVED_MEMORY_TYPE,
    EFI_LOADER_CODE,
    EFI_LOADER_DATA,
    EFI_BOOT_SERVICES_CODE,
    EFI_BOOT_SERVICES_DATA,
    EFI_RUNTIME_SERVICES_CODE,
    EFI_RUNTIME_SERVICES_DATA,
    EFI_CONVENTIONAL_MEMORY,
    EFI_UNUSABLE_MEMORY,
    EFI_ACPI_RECLAIM_MEMORY,
    EFI_ACPI_MEMORY_NVS,
    EFI_MEMORY_MAPPED_IO,
    EFI_MEMORY_MAPPED_IO_PORT_SPACE,
    EFI_PAL_CODE,
    EFI_PERSISTENT_MEMORY,
    EFI_MAX_MEMORY_TYPE
} efi_memory_type_t;

// Memory descriptor
typedef struct {
    uint32_t type;
    efi_physical_address_t physical_start;
    efi_virtual_address_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
} efi_memory_descriptor_t;

// EFI Boot Services
typedef struct _efi_boot_services {
    efi_table_header_t hdr;

    // Task Priority Services
    void *raise_tpl;
    void *restore_tpl;

    // Memory Services
    efi_status_t (*allocate_pages)(uint32_t type, uint32_t memory_type, uintn_t pages, efi_physical_address_t *memory);
    efi_status_t (*free_pages)(efi_physical_address_t memory, uintn_t pages);
    efi_status_t (*get_memory_map)(uintn_t *memory_map_size, efi_memory_descriptor_t *memory_map,
                                   uintn_t *map_key, uintn_t *descriptor_size, uint32_t *descriptor_version);
    efi_status_t (*allocate_pool)(uint32_t pool_type, uintn_t size, void **buffer);
    efi_status_t (*free_pool)(void *buffer);

    // Event & Timer Services
    void *create_event;
    void *set_timer;
    void *wait_for_event;
    void *signal_event;
    void *close_event;
    void *check_event;

    // Protocol Handler Services
    void *install_protocol_interface;
    void *reinstall_protocol_interface;
    void *uninstall_protocol_interface;
    efi_status_t (*handle_protocol)(efi_handle_t handle, efi_guid_t *protocol, void **interface);
    void *reserved;
    void *register_protocol_notify;
    efi_status_t (*locate_handle)(uint32_t search_type, efi_guid_t *protocol, void *search_key,
                                  uintn_t *buffer_size, efi_handle_t *buffer);
    void *locate_device_path;
    void *install_configuration_table;

    // Image Services
    void *load_image;
    void *start_image;
    efi_status_t (*exit)(efi_handle_t image_handle, efi_status_t exit_status, uintn_t exit_data_size, char16_t *exit_data);
    void *unload_image;
    efi_status_t (*exit_boot_services)(efi_handle_t image_handle, uintn_t map_key);

    // Miscellaneous Services
    void *get_next_monotonic_count;
    void *stall;
    void *set_watchdog_timer;

    // Driver Support Services
    void *connect_controller;
    void *disconnect_controller;

    // Open and Close Protocol Services
    void *open_protocol;
    void *close_protocol;
    void *open_protocol_information;

    // Library Services
    void *protocols_per_handle;
    void *locate_handle_buffer;
    efi_status_t (*locate_protocol)(efi_guid_t *protocol, void *registration, void **interface);
    void *install_multiple_protocol_interfaces;
    void *uninstall_multiple_protocol_interfaces;

    // CRC Services
    void *calculate_crc32;

    // Miscellaneous Services
    void *copy_mem;
    void *set_mem;
    void *create_event_ex;
} efi_boot_services_t;

// EFI Runtime Services
typedef struct _efi_runtime_services {
    efi_table_header_t hdr;

    // Time Services
    void *get_time;
    void *set_time;
    void *get_wakeup_time;
    void *set_wakeup_time;

    // Virtual Memory Services
    void *set_virtual_address_map;
    void *convert_pointer;

    // Variable Services
    void *get_variable;
    void *get_next_variable_name;
    void *set_variable;

    // Miscellaneous Services
    void *get_next_high_mono_count;
    void *reset_system;

    // UEFI 2.0 Capsule Services
    void *update_capsule;
    void *query_capsule_capabilities;

    // Miscellaneous UEFI 2.0 Services
    void *query_variable_info;
} efi_runtime_services_t;

// Graphics Output Protocol (GOP)
typedef struct {
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t reserved_mask;
} efi_pixel_bitmask_t;

typedef enum {
    PIXEL_RGB_RESERVED_8BIT_PER_COLOR,
    PIXEL_BGR_RESERVED_8BIT_PER_COLOR,
    PIXEL_BIT_MASK,
    PIXEL_BLT_ONLY,
    PIXEL_FORMAT_MAX
} efi_graphics_pixel_format_t;

typedef struct {
    uint32_t version;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    efi_graphics_pixel_format_t pixel_format;
    efi_pixel_bitmask_t pixel_information;
    uint32_t pixels_per_scan_line;
} efi_graphics_output_mode_information_t;

typedef struct {
    uint32_t max_mode;
    uint32_t mode;
    efi_graphics_output_mode_information_t *info;
    uintn_t size_of_info;
    efi_physical_address_t frame_buffer_base;
    uintn_t frame_buffer_size;
} efi_graphics_output_protocol_mode_t;

typedef struct {
    void *query_mode;
    void *set_mode;
    void *blt;
    efi_graphics_output_protocol_mode_t *mode;
} efi_graphics_output_protocol_t;

// GOP GUID
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}}

// Simple File System Protocol
typedef struct _efi_file_protocol efi_file_protocol_t;

typedef struct {
    void *open_volume;
} efi_simple_file_system_protocol_t;

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}

// File Protocol
typedef struct _efi_file_protocol {
    uint64_t revision;
    efi_status_t (*open)(efi_file_protocol_t *this, efi_file_protocol_t **new_handle,
                         char16_t *filename, uint64_t open_mode, uint64_t attributes);
    efi_status_t (*close)(efi_file_protocol_t *this);
    void *delete;
    efi_status_t (*read)(efi_file_protocol_t *this, uintn_t *buffer_size, void *buffer);
    void *write;
    void *get_position;
    void *set_position;
    void *get_info;
    void *set_info;
    void *flush;
} efi_file_protocol_t;

// File open modes
#define EFI_FILE_MODE_READ    0x0000000000000001
#define EFI_FILE_MODE_WRITE   0x0000000000000002
#define EFI_FILE_MODE_CREATE  0x8000000000000000

// Allocate types
#define ALLOCATE_ANY_PAGES     0
#define ALLOCATE_MAX_ADDRESS   1
#define ALLOCATE_ADDRESS       2

#endif // _EFI_H_
