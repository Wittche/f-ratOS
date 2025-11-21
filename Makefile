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
KERNEL_OBJS = $(BUILD_DIR)/entry.o \
              $(BUILD_DIR)/main.o \
              $(BUILD_DIR)/console.o

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

$(BUILD_DIR)/boot.o: $(BOOT_DIR)/boot.c $(BOOT_DIR)/efi.h | $(BUILD_DIR)
	@echo "[CC] Compiling bootloader..."
	$(BOOT_CC) $(EFI_CC_FLAGS) -c $< -o $@

# Build kernel
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "[OBJCOPY] Creating kernel binary..."
	$(OBJCOPY) -O binary $< $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(KERNEL_DIR)/linker.ld
	@echo "[LD] Linking kernel..."
	$(LD) $(KERNEL_LD_FLAGS) -o $@ $(KERNEL_OBJS)

$(BUILD_DIR)/entry.o: $(KERNEL_DIR)/entry.S | $(BUILD_DIR)
	@echo "[AS] Assembling kernel entry..."
	$(AS) $(KERNEL_AS_FLAGS) $< -o $@

$(BUILD_DIR)/main.o: $(KERNEL_DIR)/main.c $(KERNEL_DIR)/types.h $(KERNEL_DIR)/boot.h $(KERNEL_DIR)/console.h | $(BUILD_DIR)
	@echo "[CC] Compiling kernel main..."
	$(KERNEL_CC) $(KERNEL_CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/console.o: $(KERNEL_DIR)/console.c $(KERNEL_DIR)/console.h $(KERNEL_DIR)/types.h | $(BUILD_DIR)
	@echo "[CC] Compiling console..."
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

# Run with QEMU (UEFI)
.PHONY: run
run: $(BOOTLOADER_EFI) $(KERNEL_BIN)
	@echo "Running AuroraOS in QEMU (UEFI mode)..."
	@echo "Note: Requires OVMF UEFI firmware"
	qemu-system-x86_64 \
		-bios /usr/share/ovmf/OVMF.fd \
		-drive format=raw,file=$(BUILD_DIR)/esp.img \
		-m 256M \
		-serial stdio

# Run with QEMU (legacy BIOS - for testing basic kernel)
.PHONY: run-bios
run-bios: $(KERNEL_BIN)
	@echo "Running kernel in QEMU (legacy BIOS mode for testing)..."
	qemu-system-x86_64 \
		-kernel $(KERNEL_BIN) \
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
	@echo "  clean      - Remove build artifacts"
	@echo "  run        - Run in QEMU with UEFI"
	@echo "  run-bios   - Run in QEMU with legacy BIOS"
	@echo "  iso        - Create bootable ISO (not implemented)"
	@echo "  help       - Show this help"
	@echo ""
	@echo "Requirements:"
	@echo "  - GCC/Clang with x86_64 target"
	@echo "  - NASM assembler"
	@echo "  - QEMU for testing"
	@echo "  - OVMF UEFI firmware for UEFI testing"
