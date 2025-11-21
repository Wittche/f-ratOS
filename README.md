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
- [ ] UEFI bootloader implementation
- [ ] Kernel stub
- [ ] Basic console output
- [ ] Interrupt handling

## Build Requirements

- GCC cross-compiler (x86_64-elf)
- GNU Make or CMake
- QEMU (for testing)
- GNU-EFI (UEFI development)

## License

MIT License

## References

- Apple Darwin/XNU Kernel
- UEFI Specification
- OSDev Wiki
