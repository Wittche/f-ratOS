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

**Phase 1: Core Infrastructure** ✅ (Complete)
- [x] Project structure
- [x] UEFI bootloader implementation (simplified)
- [x] Kernel entry point (64-bit long mode)
- [x] VGA console output
- [x] GDT (Global Descriptor Table)
- [x] IDT (Interrupt Descriptor Table)
- [x] PIC (8259) remapping
- [x] Exception and IRQ handlers
- [x] Build system (Makefile)

**Phase 2: Memory Management** ✅ (Complete)
- [x] Physical Memory Manager (PMM)
- [x] Virtual Memory Manager (VMM)
- [x] Kernel heap allocator

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

## Quick Start

**Build and Test in One Command:**
```bash
make test
```

This will:
1. Build bootloader (BOOTX64.EFI - 5KB)
2. Build kernel (kernel.bin - 19KB)
3. Launch QEMU for testing
4. Show kernel output in VGA console

**Or use individual commands:**

## Building

### Build Everything
```bash
make all
```

This builds:
- `build/BOOTX64.EFI` - UEFI bootloader (5KB)
- `build/kernel.bin` - Kernel binary (19KB)
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
      AuroraOS Kernel v0.1
  Hybrid Kernel - XNU Inspired
=====================================

[BOOT] Validating boot information...
[WARNING] Boot info is NULL
[INFO] Running in TEST MODE (no bootloader)

[KERNEL] Initializing subsystems...
[GDT] Initializing Global Descriptor Table...
[GDT] Loaded with 5 entries
[GDT] Kernel CS=0x8, DS=0x10
  [OK] GDT (Global Descriptor Table)

[IDT] Initializing Interrupt Descriptor Table...
[IDT] Loaded with 256 entries
[IDT] Exceptions: 0-31, IRQs: 32-47
  [OK] IDT (Interrupt Descriptor Table)

[PMM] Initializing Physical Memory Manager...
[PMM] Bitmap allocator initialized
  [OK] PMM (Physical Memory Manager)

[VMM] Initializing Virtual Memory Manager...
[VMM] PML4 allocated, identity mapping first 4MB
  [OK] VMM (Virtual Memory Manager)

[HEAP] Initializing kernel heap...
[HEAP] Initialized at 0x200000 (1MB initial)
  [OK] Kernel Heap

  [ ] Mach Microkernel Layer
  [ ] BSD Layer
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
