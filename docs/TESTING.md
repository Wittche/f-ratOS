# Testing AuroraOS

This guide explains how to test the AuroraOS kernel.

## Prerequisites

### Install QEMU

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install qemu-system-x86
```

**Fedora/RHEL:**
```bash
sudo dnf install qemu-system-x86
```

**macOS:**
```bash
brew install qemu
```

**Arch Linux:**
```bash
sudo pacman -S qemu-system-x86
```

## Building the Kernel

```bash
cd /path/to/AuroraOS
make clean
make kernel
```

**Output:**
- `build/kernel.elf` - ELF format (4.5KB)
- `build/kernel.bin` - Raw binary (3KB)

## Running in QEMU

### Method 1: Using Makefile (Recommended)

```bash
make run-bios
```

This will:
1. Build the kernel if needed
2. Launch QEMU with kernel.elf
3. Display output in terminal

### Method 2: Manual QEMU Command

```bash
qemu-system-x86_64 \
    -kernel build/kernel.elf \
    -m 256M \
    -serial stdio \
    -display none
```

**Parameters:**
- `-kernel build/kernel.elf` - Load kernel in ELF format
- `-m 256M` - Allocate 256MB RAM
- `-serial stdio` - Redirect serial to terminal
- `-display none` - No graphical window

### Method 3: With Display (for GUI testing later)

```bash
qemu-system-x86_64 \
    -kernel build/kernel.elf \
    -m 256M \
    -serial stdio
```

## Expected Behavior

### Current Status (Phase 1)

⚠️ **The kernel will crash!** This is expected because:

1. **Missing boot_info**: Kernel expects a boot_info pointer from bootloader
2. **NULL pointer dereference**: QEMU passes NULL, kernel tries to read it
3. **No bootloader**: We're directly loading kernel without UEFI bootloader

**Expected output:**
```
=====================================
   AuroraOS Kernel v0.1
  Hybrid Kernel - XNU Inspired
=====================================

[BOOT] Validating boot information...
[ERROR] Boot info is NULL!
```

Or more likely: **Triple fault / QEMU exits immediately**

### Why This Happens

```c
void kernel_main(boot_info_t *boot_info) {
    // boot_info is NULL when loaded by QEMU -kernel
    if (boot_info->magic != AURORA_BOOT_MAGIC) {  // ← NULL dereference!
        // CRASH!
    }
}
```

## Debugging

### Enable QEMU Debugging

```bash
qemu-system-x86_64 \
    -kernel build/kernel.elf \
    -m 256M \
    -serial stdio \
    -d int,cpu_reset \
    -D qemu-debug.log
```

**Debug flags:**
- `-d int` - Log interrupts
- `-d cpu_reset` - Log CPU resets
- `-D qemu-debug.log` - Save logs to file

### GDB Debugging

**Terminal 1: Start QEMU in debug mode**
```bash
qemu-system-x86_64 \
    -kernel build/kernel.elf \
    -m 256M \
    -s -S
```

**Terminal 2: Connect GDB**
```bash
gdb build/kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

### Examine Kernel Binary

```bash
# View sections
readelf -l build/kernel.elf

# Disassemble
objdump -d build/kernel.elf > kernel.asm

# Check symbols
nm build/kernel.elf

# Verify entry point
readelf -h build/kernel.elf | grep Entry
```

## Next Steps

To get the kernel actually running, you need:

1. **Add NULL check** in kernel_main:
```c
void kernel_main(boot_info_t *boot_info) {
    console_init(NULL, 80, 25, 0);  // Initialize console first
    console_clear();

    if (!boot_info || boot_info->magic != AURORA_BOOT_MAGIC) {
        console_print("WARNING: No valid boot_info\n");
        console_print("Running in test mode...\n");
        // Continue with test mode
    }
}
```

2. **Test mode implementation** - Allow kernel to run without bootloader

3. **Complete UEFI bootloader** - For full boot sequence

## Troubleshooting

### QEMU not found
```bash
# Check if installed
which qemu-system-x86_64

# If not, install (see Prerequisites above)
```

### "Invalid kernel header" error
- **Problem**: Using `kernel.bin` instead of `kernel.elf`
- **Solution**: Use `make run-bios` or manually specify `kernel.elf`

### Kernel crashes immediately
- **Expected!** See "Expected Behavior" section above
- Kernel needs NULL safety checks

### No output
- Make sure `-serial stdio` is included
- Check that console_init() is called first

---

*Last updated: 2025-11-21*
