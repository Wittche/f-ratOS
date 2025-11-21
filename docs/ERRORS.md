# AuroraOS - Error Documentation

This file tracks all significant errors encountered during development, their root causes, and solutions.

## Format

Each entry follows this structure:
```
### [Date] Error Title
**Phase**: [Development phase]
**Component**: [Affected component]
**Description**: [What happened]
**Root Cause**: [Why it happened]
**Solution**: [How it was fixed]
**Commit**: [Relevant commit hash]
**Prevention**: [How to avoid in future]
---
```

## Error Log

### [2025-11-21] Boot Hang After "Success! Entering Protected Mode" (Wittche Project)
**Phase**: Previous project (Wittche OS) - Legacy bootloader development
**Component**: Linker script (linker.ld) - Section LMA configuration
**Description**: System successfully loaded kernel, printed "Success! Entering protected mode..." but then hung/froze with no further output. The bootloader appeared to work correctly, but the kernel never executed.

**Symptom Timeline**:
1. ✅ BIOS loads bootloader at 0x7C00
2. ✅ Bootloader reads kernel sectors from disk
3. ✅ Bootloader prints "Loading kernel..."
4. ✅ Bootloader prints "Success! Entering protected mode..."
5. ✅ Bootloader jumps to 0x10000 (expected kernel entry)
6. ❌ System hangs - no kernel output, complete freeze

**Root Cause**:
The linker script only specified `AT(0)` for the `.text` section but **not** for `.rodata`, `.data`, and `.bss` sections. This caused objcopy to use VMA as LMA for those sections:

```ld
.text 0x10000 : AT(0) {     # Good: LMA=0, VMA=0x10000
    *(.text)
}

.rodata : {                  # BAD: No AT(), LMA defaults to VMA!
    *(.rodata*)              # LMA = 0x10000+, VMA = 0x10000+
}
```

When objcopy creates a flat binary, it:
1. Sees `.text` at LMA 0 - places it at offset 0 in binary ✅
2. Sees `.rodata` at LMA 0x10000+ - **adds 64KB padding** to binary! ❌
3. Bootloader loads this padded binary to 0x10000
4. Sections end up at wrong addresses
5. Kernel tries to access `.rodata` strings at wrong address → crash/hang

**Example Problem**:
```
Binary layout WITHOUT proper AT():
[boot.bin: 512 bytes][64KB padding][.text][.rodata][.data]
                     ^^^^^^^^^^^^^^^^^
                     Padding causes misalignment!

When loaded at 0x10000:
- .text at 0x10000 (correct)
- .rodata at 0x20000 (WRONG! Should be after .text)
```

**Solution**:
Specify `AT(ADDR(.section) - 0x10000)` for ALL sections to ensure:
- VMA = 0x10000+ (where code expects to run)
- LMA = 0+ (contiguous in binary file, no padding)

```ld
. = 0x10000;

.text : AT(ADDR(.text) - 0x10000) {
    *(.text)
}

.rodata : AT(ADDR(.rodata) - 0x10000) {
    *(.rodata*)
}

.data : AT(ADDR(.data) - 0x10000) {
    *(.data)
}

.bss : AT(ADDR(.bss) - 0x10000) {
    *(.bss)
}
```

**Debugging Process**:
1. Checked bootloader - working correctly ✅
2. Checked kernel entry point - syntax correct ✅
3. Examined linker script - found AT() only on .text section ❌
4. Realized objcopy was adding padding between sections
5. Fixed all sections with proper AT() expressions

**Commit**: `8682d7f` (Wittche project)
**Prevention**:
- **Always specify LMA for ALL loadable sections** in linker scripts when creating flat binaries
- Use `AT(ADDR(.section) - BASE)` pattern consistently
- Check binary size - if it's unexpectedly large, inspect with `objdump -h kernel.elf`
- Verify section layout: `readelf -l kernel.elf` before objcopy
- Test incrementally: if bootloader works, problem is likely in linker script or kernel code

**Key Lesson**: When bootloader works but kernel doesn't execute:
1. ✅ Check bootloader is loading to correct address
2. ✅ Check kernel entry point assembly
3. ✅ **Check linker script LMA/VMA configuration** ← Most common issue!
4. ✅ Verify binary doesn't have unexpected padding
5. ✅ Use QEMU with `-d int,cpu_reset` for debugging

**Related Reading**:
- LMA vs VMA: https://sourceware.org/binutils/docs/ld/Output-Section-LMA.html
- objcopy behavior: man objcopy (search for "binary")

---

### [2025-11-21] Build Errors: GCC Target Flag and NASM Missing (AuroraOS)
**Phase**: Phase 1 - Build system setup
**Component**: Makefile, toolchain configuration
**Description**: Multiple build errors when attempting to compile AuroraOS for the first time on a system without NASM.

**Error Sequence**:
1. ❌ `gcc: error: unrecognized command-line option '-target'`
2. ❌ `make: nasm: No such file or directory`
3. ❌ `cc1: error: code model kernel does not support PIC mode`
4. ❌ `error: 'validate_boot_info' declared 'static' but never defined`
5. ❌ `error: unused parameter 'pitch'`

**Root Cause**:
Multiple issues in initial build system:
1. **Wrong compiler for bootloader**: Used `CC = gcc` for UEFI bootloader, but `-target` flag is Clang-specific
2. **NASM dependency**: entry.asm required NASM assembler which wasn't installed
3. **PIC mode conflict**: GCC defaulting to PIC mode which conflicts with `-mcmodel=kernel`
4. **Code quality**: Unused forward declarations and parameters with `-Werror`

**Solution**:
1. **Separate toolchains**:
```makefile
BOOT_CC = clang     # For UEFI bootloader (-target support)
KERNEL_CC = gcc     # For kernel
AS = as             # GNU Assembler (comes with GCC)
```

2. **Convert assembly to GNU syntax**:
   - Created `kernel/entry.S` (GNU AS syntax) from `entry.asm` (NASM syntax)
   - Changed flags: `-f elf64` → `--64`
   - Key syntax changes:
     - `section .text` → `.section .text`
     - `global _start` → `.global _start`
     - `[rel symbol]` → `symbol(%rip)`

3. **Fix PIC mode**:
```makefile
KERNEL_CC_FLAGS = ... \
    -fno-pic \          # Disable position-independent code
    -fno-pie \          # Disable position-independent executable
    -mcmodel=kernel \   # Kernel code model
```

4. **Code cleanup**:
   - Removed unused `validate_boot_info()` declaration
   - Added `(void)pitch;` to suppress warning

**Debugging Process**:
1. Identified `-target` is Clang-only → Split compilers
2. NASM missing → Convert to GNU Assembler (universally available)
3. PIC error → Added -fno-pic/-fno-pie flags
4. Warnings → Fixed code quality issues

**Commit**: `370fde8`
**Result**: ✅ Kernel builds successfully (3KB binary)

**Prevention**:
- **Document dependencies** clearly (or avoid them like NASM)
- **Use widely available tools** (GNU as instead of NASM)
- **Separate toolchains** for different components (bootloader vs kernel)
- **Test builds** on fresh systems without assuming installed tools
- **Disable PIC/PIE** explicitly for bare-metal kernel code

**Key Lessons**:
- Clang vs GCC flags are different - document which compiler for which component
- GNU Assembler is more portable than NASM
- `-mcmodel=kernel` requires `-fno-pic -fno-pie`
- Keep build dependencies minimal for portability

---

## Tips and Lessons Learned

### General
- Always initialize all struct fields to prevent undefined behavior
- Use volatile for MMIO (Memory-Mapped I/O) registers
- Test each component incrementally before integration

### UEFI Development
- UEFI uses UCS-2 (wide characters) for strings
- EFI_HANDLE and EFI_SYSTEM_TABLE are critical - never lose them
- GOP (Graphics Output Protocol) must be acquired before ExitBootServices

### Kernel Development
- Never dereference NULL pointers - always check returns
- Stack alignment is critical (16-byte aligned on x86_64)
- Interrupts must be disabled during critical sections
- **Linker scripts**: Always specify AT() for ALL loadable sections when creating flat binaries
- Use `readelf -l kernel.elf` and `objdump -h` to verify section layout before deployment

### Memory Management
- Page tables must be page-aligned (4KB boundary)
- Always invalidate TLB after page table modifications
- Keep track of physical/virtual address translations

---

*Last updated: 2025-11-21*
