# AuroraOS

A modern, macOS-inspired operating system with hybrid kernel architecture.

## Architecture

- **Hybrid Kernel**: Combines Mach-like microkernel with BSD components
- **UEFI Bootloader**: Modern EFI boot system
- **GUI Framework**: Quartz/Aqua-inspired compositor and window manager
- **POSIX Compatible**: BSD layer provides POSIX API compatibility

## Project Structure

```
AuroraOS/
├── bootloader/efi/       # UEFI bootloader (boot.efi)
├── kernel/               # Kernel source
│   ├── mach/            # Microkernel (IPC, VM, threads)
│   ├── bsd/             # BSD layer (VFS, POSIX, network)
│   ├── libkern/         # Kernel libraries
│   └── osfmk/           # OS fundamentals
├── drivers/              # Device drivers
├── userspace/            # User-space libraries and daemons
├── gui/                  # GUI framework and compositor
├── docs/                 # Documentation
└── tools/                # Build tools
```

## Development Status

**Phase 1: Bootloader + Minimal Kernel** (In Progress)
- [x] Project structure
- [x] UEFI bootloader implementation (simplified)
- [x] Kernel stub with NULL safety
- [x] Basic console output (VGA text mode)
- [x] Build system (Makefile)
- [x] ESP image creation
- [ ] Interrupt handling
- [ ] Memory management (PMM/VMM)

## Build Requirements

### Required
- **GCC** (for kernel compilation)
- **Clang** (for UEFI bootloader with `-target` support)
- **LLD** (LLVM linker, comes with Clang)
- **GNU Binutils** (as, ld, objcopy)
- **GNU Make**

### Optional (for testing)
- **QEMU** with x86_64 support
- **OVMF** UEFI firmware (`/usr/share/ovmf/OVMF.fd`)

### Installation (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential clang lld qemu-system-x86 ovmf
```

## Building

### Build Everything
```bash
make all
```

This builds:
- `build/BOOTX64.EFI` - UEFI bootloader (5KB)
- `build/kernel.bin` - Kernel binary (3KB)
- `build/kernel.elf` - Kernel with debug symbols

### Build Kernel Only
```bash
make kernel
```

### Create ESP Image
```bash
make esp
```

Creates `build/esp.img` - A bootable FAT32 image with UEFI directory structure.

### Run in QEMU (UEFI)
```bash
make run
```

This automatically:
1. Builds bootloader and kernel
2. Creates ESP image
3. Launches QEMU with OVMF firmware

**Note:** Requires OVMF firmware installed at `/usr/share/ovmf/OVMF.fd`

### Clean Build
```bash
make clean
```

## Testing

See [docs/TESTING.md](docs/TESTING.md) for detailed testing instructions.

### Quick Test
```bash
make run
```

Expected output:
```
=====================================
   AuroraOS UEFI Bootloader v0.1
   (Simplified Test Version)
=====================================

UEFI Firmware: EDK II
Revision: 0x...

Getting memory map...
Memory map obtained: XX entries
Getting Graphics Output Protocol...
GOP found!
  Framebuffer: 0x...
  Resolution: 1024x768

Preparing boot_info structure...
Boot info magic: 0x41555230524F0000
Kernel entry point: 0x10000C

Exiting UEFI Boot Services...

=====================================
      AuroraOS Kernel v0.1
  Hybrid Kernel - XNU Inspired
=====================================

[BOOT] Validating boot information...
[OK] Boot info validated

[BOOT] Boot Information:
  Kernel Physical Base: 0x100000
  ...

[KERNEL] Initializing subsystems...
  [ ] GDT (Global Descriptor Table)
  ...

[HALT] System halted
```

## Documentation

- [ARCHITECTURE.md](docs/ARCHITECTURE.md) - System architecture
- [BOOTLOADER.md](docs/BOOTLOADER.md) - UEFI bootloader details
- [TESTING.md](docs/TESTING.md) - Testing procedures
- [ERRORS.md](docs/ERRORS.md) - Error documentation (5 errors documented)

## License

MIT License

## References

- Apple Darwin/XNU Kernel
- UEFI Specification
- OSDev Wiki
