# Building AuroraOS

This document explains how to build and test AuroraOS.

## Prerequisites

### Required Tools

1. **GCC/Clang** with x86_64 target support
   ```bash
   # On Debian/Ubuntu
   sudo apt install gcc clang lld

   # On macOS
   brew install llvm
   ```

2. **NASM** assembler
   ```bash
   # On Debian/Ubuntu
   sudo apt install nasm

   # On macOS
   brew install nasm
   ```

3. **GNU Make**
   ```bash
   # Usually pre-installed on most systems
   make --version
   ```

4. **QEMU** for testing (optional but recommended)
   ```bash
   # On Debian/Ubuntu
   sudo apt install qemu-system-x86

   # On macOS
   brew install qemu
   ```

5. **OVMF** UEFI firmware for QEMU (optional, for UEFI testing)
   ```bash
   # On Debian/Ubuntu
   sudo apt install ovmf

   # On macOS
   brew install qemu  # Includes UEFI firmware
   ```

## Building

### Build Everything

```bash
cd AuroraOS
make
```

This will:
- Compile the UEFI bootloader (`build/BOOTX64.EFI`)
- Compile the kernel (`build/kernel.bin`)

### Build Individual Components

```bash
# Build only bootloader
make build/BOOTX64.EFI

# Build only kernel
make build/kernel.bin
```

### Clean Build

```bash
make clean
```

## Running

### Run in QEMU (UEFI mode)

```bash
make run
```

**Note**: This requires OVMF UEFI firmware to be installed.

### Run in QEMU (Legacy BIOS mode)

For quick kernel testing without UEFI:

```bash
make run-bios
```

This directly loads the kernel binary, skipping the bootloader.

## Testing on Real Hardware

**WARNING**: Testing on real hardware can potentially damage your system. Only proceed if you know what you're doing.

### Create Bootable USB

1. Build the project:
   ```bash
   make
   ```

2. Create ESP (EFI System Partition) on USB drive:
   ```bash
   # DANGER: Replace /dev/sdX with your USB drive!
   sudo mkfs.vfat -F 32 /dev/sdX1
   sudo mkdir -p /mnt/usb
   sudo mount /dev/sdX1 /mnt/usb
   ```

3. Copy bootloader:
   ```bash
   sudo mkdir -p /mnt/usb/EFI/BOOT
   sudo cp build/BOOTX64.EFI /mnt/usb/EFI/BOOT/
   sudo cp build/kernel.bin /mnt/usb/
   sudo umount /mnt/usb
   ```

4. Boot from USB drive in UEFI mode

## Troubleshooting

### "fatal error: 'efi.h' file not found"

Make sure you're building from the AuroraOS root directory.

### "ld: cannot find -lc"

You're using the system linker instead of the bare-metal linker. Make sure you're using the correct KERNEL_LD_FLAGS.

### "clang: error: unsupported option '-target'"

Your clang version might be too old. Try using GCC instead or update clang.

### QEMU doesn't start (UEFI mode)

Make sure OVMF is installed:
```bash
# Ubuntu/Debian
sudo apt install ovmf

# macOS - included with QEMU
brew install qemu
```

## Development Tips

### Debugging with QEMU + GDB

1. Start QEMU in debug mode:
   ```bash
   qemu-system-x86_64 -kernel build/kernel.bin -s -S
   ```

2. In another terminal, start GDB:
   ```bash
   gdb build/kernel.elf
   (gdb) target remote localhost:1234
   (gdb) break kernel_main
   (gdb) continue
   ```

### Serial Output

QEMU forwards serial output to stdio by default. Add debug prints:

```c
// In kernel code
console_print("Debug: value = ");
console_print_hex(value);
console_print("\n");
```

### Viewing Assembly

```bash
objdump -d build/kernel.elf > kernel.asm
```

## CI/CD (Future)

GitHub Actions workflow will be added to automatically:
- Build on every push
- Run tests in QEMU
- Create release artifacts

---

*Last updated: 2025-11-21*
