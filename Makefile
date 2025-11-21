# AuroraOS Makefile
# Build system for bootloader and kernel

# Toolchain
BOOT_CC = clang          # Use Clang for UEFI bootloader (supports -target)
KERNEL_CC = gcc          # Use GCC for kernel
AS = as                  # Use GNU Assembler (comes with GCC)
LD = ld
OBJCOPY = objcopy

# Directories
BUILD_DIR = build
BOOT_DIR = bootloader/efi
KERNEL_DIR = kernel
ISO_DIR = $(BUILD_DIR)/iso

# Target architecture
ARCH = x86_64

# Compiler flags for UEFI bootloader
EFI_CC_FLAGS = -target $(ARCH)-unknown-windows \
               -ffreestanding \
               -fno-stack-protector \
               -fno-stack-check \
               -fshort-wchar \
               -mno-red-zone \
               -std=c11 \
               -I$(BOOT_DIR) \
               -Wall -Wextra

# Linker flags for UEFI bootloader
EFI_LD_FLAGS = -target $(ARCH)-unknown-windows \
               -nostdlib \
               -Wl,-entry:efi_main \
               -Wl,-subsystem:efi_application \
               -fuse-ld=lld

# Compiler flags for kernel
KERNEL_CC_FLAGS = -ffreestanding \
                  -nostdlib \
                  -nostdinc \
                  -fno-builtin \
                  -fno-stack-protector \
                  -fno-pic \
                  -fno-pie \
                  -mno-red-zone \
                  -mcmodel=kernel \
                  -m64 \
                  -std=c11 \
                  -I$(KERNEL_DIR) \
                  -Wall -Wextra -Werror \
                  -O2

# Assembler flags for kernel (GNU as)
KERNEL_AS_FLAGS = --64

# Linker flags for kernel
KERNEL_LD_FLAGS = -n \
                  -T $(KERNEL_DIR)/linker.ld \
                  -nostdlib

# Output files
BOOTLOADER_EFI = $(BUILD_DIR)/BOOTX64.EFI
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
ISO_IMAGE = $(BUILD_DIR)/auroraos.iso

# Object files
BOOT_OBJS = $(BUILD_DIR)/boot.o
KERNEL_OBJS = $(BUILD_DIR)/multiboot.o \
              $(BUILD_DIR)/entry.o \
              $(BUILD_DIR)/main.o \
              $(BUILD_DIR)/console.o \
              $(BUILD_DIR)/gdt.o \
              $(BUILD_DIR)/gdt_asm.o \
              $(BUILD_DIR)/idt.o \
              $(BUILD_DIR)/idt_asm.o \
              $(BUILD_DIR)/pmm.o \
              $(BUILD_DIR)/vmm.o \
              $(BUILD_DIR)/vmm_asm.o \
              $(BUILD_DIR)/kheap.o \
              $(BUILD_DIR)/timer.o \
              $(BUILD_DIR)/keyboard.o \
              $(BUILD_DIR)/process.o \
              $(BUILD_DIR)/scheduler.o \
              $(BUILD_DIR)/switch.o \
              $(BUILD_DIR)/syscall.o \
              $(BUILD_DIR)/syscall_asm.o \
              $(BUILD_DIR)/kthread_test.o

# Default target
.PHONY: all
all: $(BOOTLOADER_EFI) $(KERNEL_BIN)
	@echo "Build complete!"
	@echo "Bootloader: $(BOOTLOADER_EFI)"
	@echo "Kernel:     $(KERNEL_BIN)"

# Build only kernel (skip bootloader)
.PHONY: kernel
kernel: $(KERNEL_BIN)
	@echo "Kernel build complete!"
	@echo "Kernel: $(KERNEL_BIN)"

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build UEFI bootloader
$(BOOTLOADER_EFI): $(BOOT_OBJS) | $(BUILD_DIR)
	@echo "[LD] Linking UEFI bootloader..."
	$(BOOT_CC) $(EFI_LD_FLAGS) -o $@ $^

$(BUILD_DIR)/boot.o: $(BOOT_DIR)/boot_simple.c $(BOOT_DIR)/efi.h | $(BUILD_DIR)
	@echo "[CC] Compiling bootloader (simplified)..."
	$(BOOT_CC) $(EFI_CC_FLAGS) -c $< -o $@

# Build kernel
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "[OBJCOPY] Creating kernel binary..."
	$(OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(KERNEL_DIR)/linker.ld
	@echo "[LD] Linking kernel..."
	$(LD) $(KERNEL_LD_FLAGS) -o $@ $(KERNEL_OBJS)

$(BUILD_DIR)/multiboot.o: $(KERNEL_DIR)/multiboot.S | $(BUILD_DIR)
	@echo "[AS] Assembling multiboot header..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/entry.o: $(KERNEL_DIR)/entry.S | $(BUILD_DIR)
	@echo "[AS] Assembling kernel entry..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/main.o: $(KERNEL_DIR)/main.c $(KERNEL_DIR)/types.h $(KERNEL_DIR)/boot.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling kernel main..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/console.o: $(KERNEL_DIR)/console.c $(KERNEL_DIR)/console.h $(KERNEL_DIR)/types.h | $(BUILD_DIR)
	@echo "[CC] Compiling console..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/gdt.o: $(KERNEL_DIR)/gdt.c $(KERNEL_DIR)/gdt.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling GDT..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/gdt_asm.o: $(KERNEL_DIR)/gdt_asm.S | $(BUILD_DIR)
	@echo "[AS] Assembling GDT loader..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/idt.o: $(KERNEL_DIR)/idt.c $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/timer.h $(KERNEL_DIR)/keyboard.h | $(BUILD_DIR)
	@echo "[CC] Compiling IDT..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/idt_asm.o: $(KERNEL_DIR)/idt_asm.S | $(BUILD_DIR)
	@echo "[AS] Assembling IDT stubs..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/pmm.o: $(KERNEL_DIR)/pmm.c $(KERNEL_DIR)/pmm.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/boot.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling PMM..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/vmm.o: $(KERNEL_DIR)/vmm.c $(KERNEL_DIR)/vmm.h $(KERNEL_DIR)/pmm.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/boot.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling VMM..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/vmm_asm.o: $(KERNEL_DIR)/vmm_asm.S | $(BUILD_DIR)
	@echo "[AS] Assembling VMM functions..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/kheap.o: $(KERNEL_DIR)/kheap.c $(KERNEL_DIR)/kheap.h $(KERNEL_DIR)/pmm.h $(KERNEL_DIR)/vmm.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling kernel heap..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/timer.o: $(KERNEL_DIR)/timer.c $(KERNEL_DIR)/timer.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/scheduler.h | $(BUILD_DIR)
	@echo "[CC] Compiling timer driver..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: $(KERNEL_DIR)/keyboard.c $(KERNEL_DIR)/keyboard.h $(KERNEL_DIR)/io.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling keyboard driver..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/process.o: $(KERNEL_DIR)/process.c $(KERNEL_DIR)/process.h $(KERNEL_DIR)/kheap.h $(KERNEL_DIR)/vmm.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling process management..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/scheduler.o: $(KERNEL_DIR)/scheduler.c $(KERNEL_DIR)/scheduler.h $(KERNEL_DIR)/process.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling scheduler..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/switch.o: $(KERNEL_DIR)/switch.S | $(BUILD_DIR)
	@echo "[AS] Assembling context switch..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/syscall.o: $(KERNEL_DIR)/syscall.c $(KERNEL_DIR)/syscall.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/process.h $(KERNEL_DIR)/scheduler.h $(KERNEL_DIR)/timer.h $(KERNEL_DIR)/types.h | $(BUILD_DIR)
	@echo "[CC] Compiling system call interface..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/syscall_asm.o: $(KERNEL_DIR)/syscall_asm.S | $(BUILD_DIR)
	@echo "[AS] Assembling system call entry..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/kthread_test.o: $(KERNEL_DIR)/kthread_test.c $(KERNEL_DIR)/kthread_test.h $(KERNEL_DIR)/process.h $(KERNEL_DIR)/scheduler.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/timer.h $(KERNEL_DIR)/types.h | $(BUILD_DIR)
	@echo "[CC] Compiling kernel thread tests..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

# Create kernel linker script
$(KERNEL_DIR)/linker.ld:
	@echo "Creating kernel linker script..."
	@echo "OUTPUT_FORMAT(elf64-x86-64)" > $@
	@echo "ENTRY(_start)" >> $@
	@echo "" >> $@
	@echo "SECTIONS" >> $@
	@echo "{" >> $@
	@echo "    . = 0x100000;" >> $@
	@echo "" >> $@
	@echo "    .text : {" >> $@
	@echo "        *(.multiboot)" >> $@
	@echo "        *(.text)" >> $@
	@echo "    }" >> $@
	@echo "" >> $@
	@echo "    .rodata : {" >> $@
	@echo "        *(.rodata*)" >> $@
	@echo "    }" >> $@
	@echo "" >> $@
	@echo "    .data : {" >> $@
	@echo "        *(.data)" >> $@
	@echo "    }" >> $@
	@echo "" >> $@
	@echo "    .bss : {" >> $@
	@echo "        *(.bss)" >> $@
	@echo "        *(COMMON)" >> $@
	@echo "    }" >> $@
	@echo "" >> $@
	@echo "    /DISCARD/ : {" >> $@
	@echo "        *(.eh_frame)" >> $@
	@echo "        *(.comment)" >> $@
	@echo "    }" >> $@
	@echo "}" >> $@

# Create bootable ISO (future)
.PHONY: iso
iso: $(BOOTLOADER_EFI) $(KERNEL_BIN)
	@echo "ISO creation not yet implemented"

# Create ESP (EFI System Partition) image
.PHONY: esp
esp: $(BOOTLOADER_EFI) $(KERNEL_BIN)
	@echo "Creating ESP image..."
	@bash scripts/create_esp.sh

# Run with QEMU (UEFI)
.PHONY: run
run: esp
	@echo "Running AuroraOS in QEMU (UEFI mode)..."
	@echo "Note: Requires OVMF UEFI firmware"
	qemu-system-x86_64 \
		-bios /usr/share/ovmf/OVMF.fd \
		-drive format=raw,file=$(BUILD_DIR)/esp.img \
		-m 256M \
		-serial stdio

# Run with QEMU (use ELF format - simpler for testing)
.PHONY: run-bios
run-bios: $(KERNEL_ELF)
	@echo "Running kernel in QEMU (direct ELF load for testing)..."
	@echo "Note: This is a basic test - kernel expects boot_info but gets multiboot info"
	qemu-system-x86_64 \
		-kernel $(KERNEL_ELF) \
		-m 256M \
		-serial stdio

# Quick test - Build and run in one command
.PHONY: test
test: all
	@echo ""
	@echo "=========================================="
	@echo "  Quick Test - Running AuroraOS Kernel"
	@echo "=========================================="
	@echo ""
	@echo "Build Summary:"
	@echo "  Bootloader: $$(ls -lh $(BOOTLOADER_EFI) | awk '{print $$5}')"
	@echo "  Kernel:     $$(ls -lh $(KERNEL_BIN) | awk '{print $$5}')"
	@echo ""
	@echo "Starting QEMU..."
	@echo "Press Ctrl+C to exit"
	@echo ""
	@qemu-system-x86_64 \
		-kernel $(KERNEL_ELF) \
		-m 256M \
		-serial stdio \
		-display curses \
		2>&1 || (echo ""; echo "ERROR: QEMU not found!"; echo "Install: sudo apt-get install qemu-system-x86"; false)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)
	rm -f $(KERNEL_DIR)/linker.ld

# Help
.PHONY: help
help:
	@echo "=========================================="
	@echo "      AuroraOS Build System v0.1"
	@echo "=========================================="
	@echo ""
	@echo "Build Targets:"
	@echo "  all        - Build bootloader and kernel (default)"
	@echo "  kernel     - Build kernel only"
	@echo "  clean      - Remove all build artifacts"
	@echo ""
	@echo "Test Targets:"
	@echo "  test       - Build everything and run quick test (recommended)"
	@echo "  run-bios   - Run kernel with legacy Multiboot (simple test)"
	@echo "  run        - Run with UEFI bootloader (requires OVMF)"
	@echo ""
	@echo "Advanced Targets:"
	@echo "  esp        - Create ESP (EFI System Partition) image"
	@echo "  iso        - Create bootable ISO (not yet implemented)"
	@echo ""
	@echo "Quick Start:"
	@echo "  make test  - Build and test in one command!"
	@echo ""
	@echo "Requirements:"
	@echo "  Build:  GCC, Clang, GNU Binutils (as, ld, objcopy)"
	@echo "  Test:   qemu-system-x86 (install: apt install qemu-system-x86)"
	@echo "  UEFI:   OVMF firmware (optional, for full UEFI boot)"
	@echo ""
	@echo "Project Status:"
	@echo "  ✅ UEFI Bootloader (boot_simple.c)"
	@echo "  ✅ Kernel Entry (entry.S)"
	@echo "  ✅ VGA Console (console.c)"
	@echo "  ✅ GDT (Global Descriptor Table)"
	@echo "  ✅ IDT (Interrupt Descriptor Table)"
	@echo "  ✅ PIC (8259 Interrupt Controller)"
	@echo "  ⏳ PMM (Physical Memory Manager)"
	@echo "  ⏳ VMM (Virtual Memory Manager)"
