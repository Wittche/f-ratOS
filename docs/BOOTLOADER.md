# AuroraOS UEFI Bootloader

This document describes the UEFI bootloader implementation for AuroraOS.

## Overview

AuroraOS uses a UEFI bootloader (`BOOTX64.EFI`) to load the kernel and prepare the boot environment. The bootloader:

1. Initializes UEFI console output
2. Gathers system information (memory map, graphics mode)
3. Prepares boot information structure
4. Exits UEFI boot services
5. Jumps to kernel entry point

## Architecture

```
┌─────────────────┐
│  UEFI Firmware  │
│   (OVMF/Real)   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  BOOTX64.EFI    │  ← Our bootloader
│                 │     (bootloader/efi/boot_simple.c)
│  - Get memory   │
│  - Get GOP      │
│  - Fill boot    │
│    info struct  │
│  - Exit boot    │
│    services     │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   kernel.bin    │  ← Kernel in long mode (64-bit)
│                 │     Entry: 0x10000C (_start)
│  - kernel_main()│
└─────────────────┘
```

## Boot Information Structure

The bootloader passes a `boot_info_t` structure to the kernel:

```c
typedef struct {
    uint64_t magic;                       // 0x41555230524F0000 (validation)
    memory_descriptor_t *memory_map;       // UEFI memory map
    uint64_t memory_map_size;              // Size in bytes
    uint64_t memory_map_descriptor_size;   // Size per descriptor
    graphics_info_t *graphics_info;        // GOP info (or NULL)
    void *acpi_rsdp;                       // ACPI RSDP (future)
    uint64_t kernel_physical_base;         // 0x100000 (1MB)
    uint64_t kernel_virtual_base;          // 0x100000 (identity mapped)
    uint64_t kernel_size;                  // Kernel size
} boot_info_t;
```

## Memory Map

The bootloader provides a UEFI memory map that describes all available physical memory:

| Field | Description |
|-------|-------------|
| `type` | Memory type (EfiConventionalMemory, EfiReservedMemoryType, etc.) |
| `physical_start` | Physical address of memory region |
| `number_of_pages` | Size in 4KB pages |
| `attribute` | Memory attributes (cacheable, etc.) |

## Graphics Information

If Graphics Output Protocol (GOP) is available:

```c
typedef struct {
    uint32_t horizontal_resolution;   // Screen width
    uint32_t vertical_resolution;     // Screen height
    uint32_t pixels_per_scan_line;    // Stride
    uint32_t pixel_format;            // 0=RGB, 1=BGR
    uint64_t framebuffer_base;        // Linear framebuffer address
    uint64_t framebuffer_size;        // Framebuffer size in bytes
} graphics_info_t;
```

## Implementation

### Current Version: boot_simple.c

The simplified bootloader (`boot_simple.c`) assumes the kernel is already loaded at 0x100000:

**Features:**
- ✅ Minimal dependencies (only standard UEFI protocols)
- ✅ Works with pre-loaded kernel (via QEMU or manual load)
- ✅ Full boot_info structure support
- ✅ GOP support for graphics
- ✅ Proper exit of boot services

**Limitations:**
- ❌ Does not load kernel from file system
- ❌ Kernel must be pre-loaded to 0x100000

### Full Version: boot.c (Future)

The full bootloader will include:
- 🔶 Load kernel from ESP (EFI System Partition)
- 🔶 Parse ELF headers
- 🔶 ACPI table location
- 🔶 Configuration file support

## Building the Bootloader

### Requirements

- **Clang** with UEFI target support
- **LLD** linker (LLVM linker)

### Build Command

```bash
# Compile bootloader
clang -target x86_64-unknown-windows \
      -ffreestanding \
      -fno-stack-protector \
      -fshort-wchar \
      -mno-red-zone \
      -c bootloader/efi/boot_simple.c \
      -o build/boot.o

# Link as UEFI application
clang -target x86_64-unknown-windows \
      -nostdlib \
      -Wl,-entry:efi_main \
      -Wl,-subsystem:efi_application \
      -fuse-ld=lld \
      -o build/BOOTX64.EFI \
      build/boot.o
```

Or use the Makefile:

```bash
make all  # Builds both bootloader and kernel
```

## Testing

### Method 1: QEMU with OVMF

Create an ESP image and boot with UEFI firmware:

```bash
# Create ESP FAT32 image
dd if=/dev/zero of=esp.img bs=1M count=64
mkfs.fat -F 32 esp.img

# Mount and copy files
mkdir -p mnt
sudo mount esp.img mnt
sudo mkdir -p mnt/EFI/BOOT
sudo cp build/BOOTX64.EFI mnt/EFI/BOOT/
sudo cp build/kernel.bin mnt/
sudo umount mnt

# Run with OVMF
qemu-system-x86_64 \
    -bios /usr/share/ovmf/OVMF.fd \
    -drive format=raw,file=esp.img \
    -m 256M \
    -serial stdio
```

### Method 2: Real Hardware

1. Format USB drive as FAT32
2. Create directory structure: `EFI/BOOT/`
3. Copy `BOOTX64.EFI` to `EFI/BOOT/`
4. Copy `kernel.bin` to root
5. Boot from USB in UEFI mode

## Boot Flow

1. **UEFI Firmware** loads `BOOTX64.EFI`
2. **Bootloader** calls `efi_main()`
3. **Clear screen** and print banner
4. **Get memory map** via `GetMemoryMap()`
5. **Locate GOP** via `LocateProtocol()`
6. **Fill boot_info** structure
7. **Exit boot services** via `ExitBootServices()`
8. **Jump to kernel** at `0x10000C` (passing `&boot_info`)
9. **Kernel** receives control in 64-bit long mode

## Debugging

### Enable OVMF Debug Output

```bash
qemu-system-x86_64 \
    -bios OVMF_CODE.fd \
    -drive format=raw,file=esp.img \
    -debugcon file:debug.log \
    -global isa-debugcon.iobase=0x402 \
    -serial stdio
```

### Check Bootloader Output

The bootloader prints to both:
- **UEFI Console** (visible in QEMU window)
- **Serial port** (if `-serial stdio` is used)

### Common Issues

**Issue:** "Invalid signature" or "Security Violation"
- **Cause:** Secure Boot enabled
- **Fix:** Disable Secure Boot in UEFI settings

**Issue:** Bootloader not found
- **Cause:** Wrong path or filename
- **Fix:** Ensure file is at `EFI/BOOT/BOOTX64.EFI`

**Issue:** Kernel doesn't start
- **Cause:** Kernel not at 0x100000 or wrong entry point
- **Fix:** Check kernel loading and entry point address

## Memory Layout

```
Physical Memory:
0x00000000 - 0x000FFFFF : BIOS/Real Mode (1MB)
0x00100000 - 0x001FFFFF : Kernel Code & Data (~1MB)
0x00200000 - ...        : Available Memory

Virtual Memory (after kernel sets up paging):
0xFFFFFFFF80000000 - ... : Kernel (higher half)
```

## Future Enhancements

- [ ] Load kernel from file system (Simple File System Protocol)
- [ ] Parse ELF headers for proper loading
- [ ] Locate and parse ACPI tables
- [ ] Configuration file support (boot options)
- [ ] Multi-boot support (select kernel version)
- [ ] Splash screen with logo
- [ ] Encrypted kernel support

## References

- [UEFI Specification 2.10](https://uefi.org/specifications)
- [OSDev UEFI](https://wiki.osdev.org/UEFI)
- [GNU-EFI](https://sourceforge.net/projects/gnu-efi/)
- [TianoCore EDK II](https://github.com/tianocore/edk2)

---

*Last updated: 2025-11-21*
