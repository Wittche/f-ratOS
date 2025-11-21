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

### [2025-11-21] Unused Label After NULL Safety Refactoring (AuroraOS)
**Phase**: Phase 1 - NULL safety implementation for test mode
**Component**: kernel/main.c - Error handling refactor
**Description**: After refactoring kernel_main() to handle NULL boot_info gracefully instead of using goto statements, compilation failed with unused label error.

**Error**:
```
kernel/main.c:112:1: error: label 'halt' defined but not used [-Werror=unused-label]
  112 | halt:
      | ^~~~
cc1: all warnings being treated as errors
```

**Root Cause**:
During refactoring to add NULL safety checks, the code was changed from:
```c
// Old code (with goto):
if (!boot_info) {
    console_print("[ERROR] Boot info is NULL!\n");
    goto halt;  // Jump to halt label
}

halt:
    console_print("\n[HALT] System halted\n");
    while (1) { __asm__ __volatile__("hlt"); }
```

To:
```c
// New code (without goto):
if (!boot_info) {
    console_print("[WARNING] Boot info is NULL\n");
    console_print("[INFO] Running in TEST MODE\n");
    // Continue execution, no goto
}

halt:  // ← Label still defined but never used!
    console_print("\n[HALT] System halted\n");
    while (1) { __asm__ __volatile__("hlt"); }
```

The goto statements were removed to allow graceful handling of missing boot_info (test mode), but the `halt:` label was left in place. With `-Werror` (treat warnings as errors), this became a compilation failure.

**Solution**:
Remove the unused label since we no longer use goto for error handling:
```c
// Fixed code:
console_print("\n[KERNEL] Initialization incomplete - halting\n");
console_print("(This is expected for initial stub)\n");

// Halt the system
console_print("\n[HALT] System halted\n");
while (1) {
    __asm__ __volatile__("hlt");
}
```

**Debugging Process**:
1. Identified that NULL safety refactor removed goto statements
2. Noticed halt label was still defined but unreachable
3. Removed label, kept halt loop as natural code flow
4. ✅ Kernel compiled successfully

**Result**: ✅ Kernel builds successfully with NULL safety, can run in test mode without bootloader

**Prevention**:
- **Remove labels when removing goto statements** during refactoring
- Use compiler warnings to catch unused labels early
- Consider using structured error handling instead of goto when possible
- When refactoring control flow, check for orphaned labels

**Commit**: (pending)

**Key Lesson**: When refactoring goto-based error handling to structured code:
1. Remove the goto statements
2. Remove the corresponding labels
3. Ensure code flow naturally reaches cleanup/halt code
4. Verify compilation with `-Werror` enabled

**Related NULL Safety Changes**:
This fix was part of making kernel robust for testing without a bootloader:
- Added `bool has_boot_info` flag to track validity
- Check `!boot_info` before dereferencing
- Check `boot_info->magic != AURORA_BOOT_MAGIC` for validity
- Allow kernel to run in TEST MODE when boot_info is NULL or invalid
- Print appropriate warnings instead of crashing

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
