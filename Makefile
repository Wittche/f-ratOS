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
              $(BUILD_DIR)/timer.o \
              $(BUILD_DIR)/keyboard.o \
              $(BUILD_DIR)/pmm.o \
              $(BUILD_DIR)/vmm.o \
              $(BUILD_DIR)/vmm_asm.o \
              $(BUILD_DIR)/kheap.o \
              $(BUILD_DIR)/process.o \
              $(BUILD_DIR)/scheduler.o \
              $(BUILD_DIR)/switch.o \
              $(BUILD_DIR)/tss.o \
              $(BUILD_DIR)/syscall.o \
              $(BUILD_DIR)/syscall_asm.o \
              $(BUILD_DIR)/usermode.o \
              $(BUILD_DIR)/usermode_test.o

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

$(BUILD_DIR)/main.o: $(KERNEL_DIR)/main.c $(KERNEL_DIR)/types.h $(KERNEL_DIR)/boot.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/gdt.h $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/pmm.h $(KERNEL_DIR)/vmm.h $(KERNEL_DIR)/kheap.h $(KERNEL_DIR)/timer.h $(KERNEL_DIR)/keyboard.h $(KERNEL_DIR)/process.h $(KERNEL_DIR)/scheduler.h $(KERNEL_DIR)/syscall.h $(KERNEL_DIR)/tss.h $(KERNEL_DIR)/usermode.h | $(BUILD_DIR)
	@echo "[CC] Compiling kernel main..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/console.o: $(KERNEL_DIR)/console.c $(KERNEL_DIR)/console.h $(KERNEL_DIR)/types.h | $(BUILD_DIR)
	@echo "[CC] Compiling console..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/gdt.o: $(KERNEL_DIR)/gdt.c $(KERNEL_DIR)/gdt.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling GDT..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/gdt_asm.o: $(KERNEL_DIR)/gdt_asm.S | $(BUILD_DIR)
	@echo "[AS] Assembling GDT functions..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/idt.o: $(KERNEL_DIR)/idt.c $(KERNEL_DIR)/idt.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling IDT..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/idt_asm.o: $(KERNEL_DIR)/idt_asm.S | $(BUILD_DIR)
	@echo "[AS] Assembling IDT functions..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/timer.o: $(KERNEL_DIR)/timer.c $(KERNEL_DIR)/timer.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/io.h | $(BUILD_DIR)
	@echo "[CC] Compiling timer..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: $(KERNEL_DIR)/keyboard.c $(KERNEL_DIR)/keyboard.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/io.h | $(BUILD_DIR)
	@echo "[CC] Compiling keyboard..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/pmm.o: $(KERNEL_DIR)/pmm.c $(KERNEL_DIR)/pmm.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling PMM..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/vmm.o: $(KERNEL_DIR)/vmm.c $(KERNEL_DIR)/vmm.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling VMM..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/vmm_asm.o: $(KERNEL_DIR)/vmm_asm.S | $(BUILD_DIR)
	@echo "[AS] Assembling VMM functions..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/kheap.o: $(KERNEL_DIR)/kheap.c $(KERNEL_DIR)/kheap.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling kernel heap..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/process.o: $(KERNEL_DIR)/process.c $(KERNEL_DIR)/process.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling process management..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/scheduler.o: $(KERNEL_DIR)/scheduler.c $(KERNEL_DIR)/scheduler.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling scheduler..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/switch.o: $(KERNEL_DIR)/switch.S | $(BUILD_DIR)
	@echo "[AS] Assembling context switch..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/tss.o: $(KERNEL_DIR)/tss.c $(KERNEL_DIR)/tss.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h $(KERNEL_DIR)/gdt.h | $(BUILD_DIR)
	@echo "[CC] Compiling TSS..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/syscall.o: $(KERNEL_DIR)/syscall.c $(KERNEL_DIR)/syscall.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling syscalls..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/syscall_asm.o: $(KERNEL_DIR)/syscall_asm.S | $(BUILD_DIR)
	@echo "[AS] Assembling syscall handler..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/usermode.o: $(KERNEL_DIR)/usermode.c $(KERNEL_DIR)/usermode.h $(KERNEL_DIR)/types.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling user mode support..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/usermode_test.o: $(KERNEL_DIR)/usermode_test.c $(KERNEL_DIR)/types.h $(KERNEL_DIR)/syscall.h | $(BUILD_DIR)
	@echo "[CC] Compiling user mode test..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

# Create kernel linker script
$(KERNEL_DIR)/linker.ld:
	@echo "Creating kernel linker script..."
	@echo "/* AuroraOS Kernel Linker Script" > $@
	@echo " * For Multiboot2 64-bit kernel" >> $@
	@echo " * Kernel loaded at 1MB (0x100000) physical address" >> $@
	@echo " */" >> $@
	@echo "OUTPUT_FORMAT(elf64-x86-64)" >> $@
	@echo "ENTRY(_start)" >> $@
	@echo "" >> $@
	@echo "SECTIONS" >> $@
	@echo "{" >> $@
	@echo "    /* Kernel starts at 1MB physical address */" >> $@
	@echo "    . = 0x100000;" >> $@
	@echo "" >> $@
	@echo "    /* Multiboot2 header MUST be at the start */" >> $@
	@echo "    .boot ALIGN(8) :" >> $@
	@echo "    {" >> $@
	@echo "        *(.multiboot)" >> $@
	@echo "        *(.text)" >> $@
	@echo "    }" >> $@
	@echo "" >> $@
	@echo "    /* Read-only data - 4KB aligned */" >> $@
	@echo "    .rodata ALIGN(4K) :" >> $@
	@echo "    {" >> $@
	@echo "        *(.rodata*)" >> $@
	@echo "    }" >> $@
	@echo "" >> $@
	@echo "    /* Initialized data - 4KB aligned */" >> $@
	@echo "    .data ALIGN(4K) :" >> $@
	@echo "    {" >> $@
	@echo "        *(.data)" >> $@
	@echo "    }" >> $@
	@echo "" >> $@
	@echo "    /* Uninitialized data (BSS) - 4KB aligned */" >> $@
	@echo "    .bss ALIGN(4K) :" >> $@
	@echo "    {" >> $@
	@echo "        *(.bss)" >> $@
	@echo "        *(COMMON)" >> $@
	@echo "    }" >> $@
	@echo "" >> $@
	@echo "    /* Discard debug and unneeded sections */" >> $@
	@echo "    /DISCARD/ : {" >> $@
	@echo "        *(.eh_frame)" >> $@
	@echo "        *(.eh_frame_hdr)" >> $@
	@echo "        *(.note.GNU-stack)" >> $@
	@echo "        *(.comment)" >> $@
	@echo "    }" >> $@
	@echo "" >> $@
	@echo "    _kernel_end = .;" >> $@
	@echo "}" >> $@

# Create bootable ISO
.PHONY: iso
iso: $(KERNEL_ELF)
	@echo "Creating bootable ISO..."
	@mkdir -p iso/boot/grub
	@cp $(KERNEL_ELF) iso/boot/kernel.elf
	grub-mkrescue -o $(BUILD_DIR)/auroraos.iso iso/
	@echo "ISO created: $(BUILD_DIR)/auroraos.iso"

# Run ISO in QEMU
.PHONY: run-iso
run-iso: iso
	@echo "Running AuroraOS ISO in QEMU..."
	qemu-system-x86_64 -cdrom $(BUILD_DIR)/auroraos.iso -m 256M -serial stdio

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

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)
	rm -f $(KERNEL_DIR)/linker.ld

# Help
.PHONY: help
help:
	@echo "AuroraOS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build bootloader and kernel (default)"
	@echo "  kernel     - Build kernel only"
	@echo "  iso        - Create bootable ISO with GRUB"
	@echo "  run-iso    - Build ISO and run in QEMU"
	@echo "  esp        - Create ESP (EFI System Partition) image"
	@echo "  run        - Run in QEMU with UEFI (auto-creates ESP)"
	@echo "  run-bios   - Run in QEMU with legacy BIOS (Multiboot test)"
	@echo "  clean      - Remove build artifacts"
	@echo "  help       - Show this help"
	@echo ""
	@echo "Requirements:"
	@echo "  - GCC/Clang with x86_64 target"
	@echo "  - NASM assembler"
	@echo "  - QEMU for testing"
	@echo "  - OVMF UEFI firmware for UEFI testing"
