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

### [2025-11-21] QEMU PVH ELF Note Error - Missing Multiboot Header (AuroraOS)
**Phase**: Phase 1 - QEMU testing and boot protocol
**Component**: Kernel ELF file - Missing Multiboot/PVH boot protocol header
**Description**: After successfully building the kernel with NULL safety, attempting to boot it in QEMU using `-kernel` flag resulted in PVH ELF Note error.

**Error**:
```
qemu-system-x86_64: Error loading uncompressed kernel without PVH ELF Note
make: *** [Makefile:175: run-bios] Error 1
```

**Root Cause**:
QEMU's `-kernel` flag can boot kernels using multiple protocols:
1. **Multiboot** - Legacy boot protocol (used by GRUB)
2. **PVH** - Modern paravirtualized boot protocol (Linux)
3. **ELF with boot notes** - Various ELF note sections

Our kernel.elf had NONE of these! We had created a `kernel/multiboot.S` file with the Multiboot header, but:
1. It was never added to the build system
2. Even after adding it, the linker placed it at the WRONG location (end of file instead of beginning)

**Investigation Process**:
1. **First attempt**: Added multiboot.o to KERNEL_OBJS in Makefile ✅
2. **First attempt**: Added build rule for multiboot.o ✅
3. **First attempt**: Created separate .multiboot section in linker script ❌

   Result: Multiboot header was placed at offset 0xcb4 (end of file), OUTSIDE the LOAD segment!

   ```bash
   $ readelf -S build/kernel.elf
   [ 1] .multiboot   PROGBITS   0000000000100000  00000cb4  # Wrong offset!
   [ 2] .text        PROGBITS   0000000000100000  000000c0  # Actual code start

   $ od -Ax -tx4z -j 0xcb4 -N 12 build/kernel.elf
   000cb4 1badb002 00000003 e4524ffb  # Multiboot header found BUT at wrong location!
   ```

4. **Problem identified**: The .multiboot section was at the same Virtual Address (0x100000) as .text, but at a different file offset. The LOAD segment started at 0xc0 (.text), so .multiboot at 0xcb4 was not loaded into memory!

5. **Solution**: Place .multiboot INSIDE the .text section, BEFORE the actual code:
   ```ld
   .text : {
       *(.multiboot)   # Multiboot header FIRST
       *(.text)        # Then actual code
   }
   ```

**Solution Applied**:
Modified Makefile linker script generation to include .multiboot inside .text section:

```makefile
# Before (WRONG - separate section):
.multiboot : {
    *(.multiboot)
}

.text : {
    *(.text)
}

# After (CORRECT - inside .text):
.text : {
    *(.multiboot)   # Header first
    *(.text)        # Code follows
}
```

**Verification**:
```bash
$ od -Ax -tx4z -j 0xc0 -N 12 build/kernel.elf
0000c0 1badb002 00000003 e4524ffb  # Multiboot magic at LOAD segment start! ✅

Multiboot header breakdown:
- 0x1badb002 = Multiboot magic number
- 0x00000003 = Flags (ALIGN | MEMINFO)
- 0xe4524ffb = Checksum (-(magic + flags))
```

**Result**: ✅ Kernel ELF now has proper Multiboot header at the beginning of the LOAD segment, making it bootable by QEMU and GRUB

**Debugging Tools Used**:
```bash
# Check ELF sections and their offsets
readelf -S build/kernel.elf

# Check program headers (LOAD segments)
readelf -l build/kernel.elf

# Verify multiboot magic bytes at specific offset
od -Ax -tx4z -j <offset> -N 12 build/kernel.elf

# Check section to segment mapping
readelf -l build/kernel.elf | grep "Section to Segment"
```

**Commit**: 3b0e898

**Prevention**:
- **Multiboot header must be in first LOAD segment** - QEMU/GRUB read it from there
- **Use linker script carefully** - separate sections can end up at wrong file offsets
- **Verify with readelf -l** - check that multiboot section is in first LOAD segment
- **Check file offsets vs virtual addresses** - they can differ!
- **Test incrementally** - verify multiboot header location before testing boot

**Key Lesson**: When creating bootable kernel ELF files:
1. Multiboot header must be within first 8KB of kernel image
2. It must be part of a LOAD segment (not orphaned)
3. File offset matters as much as virtual address
4. Placing it inside .text section ensures it's loaded properly
5. Always verify with `readelf -l` and `od` before testing

**Related Files Modified**:
- `Makefile`: Added multiboot.o to KERNEL_OBJS (as FIRST object)
- `Makefile`: Added build rule for multiboot.o
- `Makefile`: Modified linker script to place .multiboot inside .text section
- `kernel/multiboot.S`: Already existed but wasn't integrated

**Boot Protocol References**:
- Multiboot Specification: https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
- PVH Boot Protocol: https://xenbits.xen.org/docs/unstable/misc/pvh.html
- QEMU Direct Kernel Boot: https://qemu.readthedocs.io/en/latest/system/linuxboot.html

---

### [2025-11-21] QEMU Multiboot 1 Limitation - 64-bit Kernel Not Supported (AuroraOS)
**Phase**: Phase 1 - QEMU testing with Multiboot
**Component**: Boot protocol - Multiboot 1 vs 64-bit kernel incompatibility
**Description**: After successfully adding Multiboot header and verifying its correct placement, QEMU still refused to boot the kernel with a 32-bit vs 64-bit error.

**Error**:
```
qemu-system-x86_64: Cannot load x86-64 image, give a 32bit one.
make: *** [Makefile:181: run-bios] Error 1
```

**Root Cause**:
This is a **fundamental limitation** of the Multiboot 1 protocol:

1. **Multiboot 1 Specification**: Always starts kernel in **32-bit protected mode**
2. **Our Kernel**: Written for **64-bit long mode** (x86_64 architecture)
3. **QEMU's -kernel flag**: When using Multiboot 1, expects 32-bit ELF or requires a 32-bit entry point

**Why This Happens**:
```
Multiboot 1 boot flow:
GRUB/QEMU → 32-bit protected mode → Kernel entry point
              ^^^^^^^^^^^^^^^^^^^
              Our kernel expects 64-bit long mode here!

Our kernel entry.S:
.code64              # ← Declared as 64-bit code
_start:
    cli              # This is 64-bit instruction
    lea stack_top(%rip), %rsp  # 64-bit addressing
```

The Multiboot 1 header is correct and properly positioned, but the protocol itself cannot directly boot a 64-bit kernel on QEMU.

**Solutions & Workarounds**:

**Option 1: Add 32-bit Bootstrap (Complex)**
Create a 32-bit entry point that:
1. Receives control in 32-bit protected mode
2. Sets up long mode (64-bit)
   - Enable PAE (Physical Address Extension)
   - Set up basic page tables
   - Enable long mode in EFER MSR
   - Jump to 64-bit code
3. Call the 64-bit kernel_main

**Pros**: Works with Multiboot 1 and QEMU -kernel
**Cons**: Adds significant complexity, duplicates work done by UEFI bootloader

**Option 2: Upgrade to Multiboot 2 (Modern)**
Multiboot 2 specification supports:
- Native 64-bit entry points
- UEFI boot services
- Better information passing

**Pros**: Clean, modern, proper 64-bit support
**Cons**: Requires rewriting multiboot.S, less universal than Multiboot 1

**Option 3: Use UEFI Bootloader (Recommended)**
Test kernel with the actual UEFI bootloader we're building:
```
bootloader/efi/boot.c → Sets up long mode → kernel_main()
```

**Pros**:
- This is our actual deployment path
- UEFI handles all long mode setup
- Tests the real boot flow
- No workarounds needed

**Cons**: Requires complete bootloader implementation

**Option 4: Real Hardware / Full VM (Future)**
Boot from actual UEFI firmware (OVMF) or real hardware

**Current Status**:
✅ Kernel builds successfully
✅ Multiboot header correctly positioned
✅ Kernel code is valid 64-bit x86_64
❌ Cannot test with QEMU -kernel due to Multiboot 1 limitation
➡️ **Will test with UEFI bootloader instead**

**Decision**:
For AuroraOS, we will:
1. **Keep Multiboot 1 header** for GRUB compatibility (GRUB can handle the transition)
2. **Focus on UEFI bootloader** as primary boot method
3. **Document this limitation** for developers
4. **Consider Multiboot 2** as future enhancement if needed

**Verification That Kernel Is Correct**:
```bash
# Kernel is proper 64-bit ELF
$ file build/kernel.elf
build/kernel.elf: ELF 64-bit LSB executable, x86-64

# Entry point is correct
$ readelf -h build/kernel.elf | grep Entry
Entry point address: 0x100000

# Code is 64-bit
$ objdump -d build/kernel.elf | head -20
100000: fa                    cli
100001: 48 8d 25 f8 4b 00 00  lea    0x4bf8(%rip),%rsp  # 64-bit!
```

**Commit**: (pending - documentation update)

**Prevention & Best Practices**:
- **Understand boot protocol limitations** - Multiboot 1 is 32-bit only
- **Choose boot protocol based on architecture** - Multiboot 2 for native 64-bit
- **UEFI is the modern standard** for 64-bit OS development
- **Test early** to discover incompatibilities before extensive development
- **Document architecture decisions** to avoid confusion later

**Key Lesson**:
Boot protocols and CPU modes must match:
- **Multiboot 1** → 32-bit protected mode → (need manual long mode setup) → 64-bit kernel
- **Multiboot 2** → Can start directly in 64-bit long mode → 64-bit kernel
- **UEFI** → Already in long mode when ExitBootServices called → 64-bit kernel ✅

**Why UEFI Is Better For Our Use Case**:
1. ✅ Native 64-bit support (already in long mode)
2. ✅ Graphics Output Protocol (GOP) for GUI
3. ✅ Memory map services
4. ✅ File system access (can load kernel from FAT32)
5. ✅ Modern standard (post-2010 systems)
6. ✅ Matches our XNU/macOS inspiration (macOS uses EFI)

**References**:
- Multiboot 1 Spec (32-bit): https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
- Multiboot 2 Spec (64-bit): https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
- Intel Long Mode Setup: Intel® 64 and IA-32 Architectures Software Developer's Manual, Volume 3
- OSDev Wiki - Entering Long Mode: https://wiki.osdev.org/Setting_Up_Long_Mode

**Next Steps**:
1. Continue with UEFI bootloader implementation
2. Test kernel with UEFI boot flow
3. Verify kernel works correctly when loaded by bootloader
4. Consider Multiboot 2 as future enhancement if GRUB compatibility needed

---

### [2025-11-21] ESP Image Creation Failed - mkfs.fat Not Installed (AuroraOS)
**Phase**: Phase 2 - Testing and deployment setup
**Component**: scripts/create_esp.sh - FAT32 filesystem creation
**Description**: When attempting to create the ESP (EFI System Partition) image for testing, the script failed at the FAT32 formatting step with mkfs.fat missing error.

**Error**:
```
[INFO] Creating ESP image (64 MB)...
[INFO] Formatting as FAT32...
[ERROR] Failed to format ESP image as FAT32
```

**Root Cause**:
The `mkfs.fat` utility (part of `dosfstools` package) is not installed on the system. This tool is required to format the ESP image as FAT32, which is mandatory for UEFI bootable partitions.

**Investigation**:
```bash
$ which mkfs.fat mkfs.vfat
# Returns: exit code 1 (not found)
```

**Solution Options**:

**Option 1: Install dosfstools (Recommended for real testing)**
```bash
# Debian/Ubuntu
sudo apt-get install dosfstools

# Fedora/RHEL
sudo dnf install dosfstools

# Arch Linux
sudo pacman -S dosfstools
```

**Option 2: Continue with kernel development (Current approach)**
Since OVMF is also not available on this system, focus on kernel feature development instead:
- Implement IDT (Interrupt Descriptor Table)
- Implement GDT (Global Descriptor Table) setup
- Add exception handlers
- Add IRQ handlers
- Implement PMM (Physical Memory Manager)

**Option 3: Use QEMU with kernel directly (Limited testing)**
Test kernel with direct kernel boot (requires Multiboot 2 or 32-bit bootstrap)

**Current Decision**:
✅ **Continue with kernel development** - implement core features (IDT, GDT, interrupts)
⏸️ **Defer ESP testing** - can test later when dosfstools/OVMF are available
📝 **Document for deployment** - update README with dosfstools requirement

**Status**:
- ❌ Cannot create ESP image without mkfs.fat
- ❌ Cannot test with UEFI boot flow yet
- ✅ Can continue developing kernel features
- ✅ Bootloader code is complete and ready
- ➡️ **Focus on kernel feature implementation**

**Prevention**:
- **Document all system dependencies** in README.md with installation commands
- **Add dependency check** at beginning of create_esp.sh script:
  ```bash
  if ! command -v mkfs.fat >/dev/null 2>&1; then
      error "mkfs.fat not found. Install: sudo apt-get install dosfstools"
  fi
  ```
- **CI/CD integration** - ensure build systems have required tools
- **Provide alternative paths** - allow development without full boot testing

**Commit**: (pending)

**Key Lesson**:
OS development has multiple parallel paths:
1. **Bootloader development** ✅ (Complete - boot_simple.c)
2. **Kernel development** 🔄 (In progress - console, entry done)
3. **Testing infrastructure** ❌ (Blocked - needs dosfstools/OVMF)

**You can progress on paths 1 and 2 even when path 3 is blocked.**

**Next Steps**:
1. Implement IDT for interrupt handling
2. Implement GDT setup
3. Add exception handlers (divide by zero, page fault, etc.)
4. Add keyboard IRQ handler
5. Later: Test with UEFI boot when tools available

**Related Dependencies**:
- `dosfstools` - FAT filesystem utilities (mkfs.fat, fsck.fat)
- `OVMF` - Open Virtual Machine Firmware (UEFI firmware for QEMU)
- `qemu-system-x86_64` - QEMU x86_64 emulator ✅ (already installed)

**References**:
- dosfstools: https://github.com/dosfstools/dosfstools
- OVMF: https://github.com/tianocore/edk2
- UEFI Specification: https://uefi.org/specifications

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
