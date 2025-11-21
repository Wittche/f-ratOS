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
