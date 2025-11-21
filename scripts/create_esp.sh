#!/bin/bash
#
# AuroraOS ESP (EFI System Partition) Creator
# Creates a bootable FAT32 image with UEFI bootloader and kernel
#

set -e  # Exit on error

# Configuration
BUILD_DIR="build"
ESP_IMAGE="$BUILD_DIR/esp.img"
ESP_SIZE_MB=64
MOUNT_POINT="/tmp/auroraos_esp_$$"  # Unique mount point

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

# Check if running as root (needed for mount)
if [ "$EUID" -eq 0 ]; then
    USE_SUDO=""
else
    USE_SUDO="sudo"
    info "Will use sudo for mount operations"
fi

# Check required files
info "Checking required files..."

if [ ! -f "$BUILD_DIR/BOOTX64.EFI" ]; then
    error "BOOTX64.EFI not found. Run 'make all' first."
fi

if [ ! -f "$BUILD_DIR/kernel.bin" ]; then
    error "kernel.bin not found. Run 'make all' first."
fi

info "All required files present"

# Create ESP image
info "Creating ESP image ($ESP_SIZE_MB MB)..."

dd if=/dev/zero of="$ESP_IMAGE" bs=1M count=$ESP_SIZE_MB status=none 2>&1 || \
    error "Failed to create ESP image"

info "Formatting as FAT32..."

mkfs.fat -F 32 "$ESP_IMAGE" >/dev/null 2>&1 || \
    error "Failed to format ESP image as FAT32"

# Create mount point
info "Creating mount point..."
$USE_SUDO mkdir -p "$MOUNT_POINT" || error "Failed to create mount point"

# Mount ESP image
info "Mounting ESP image..."
$USE_SUDO mount -o loop "$ESP_IMAGE" "$MOUNT_POINT" || \
    error "Failed to mount ESP image"

# Ensure unmount on exit
trap "$USE_SUDO umount $MOUNT_POINT 2>/dev/null; $USE_SUDO rmdir $MOUNT_POINT 2>/dev/null" EXIT

# Create UEFI directory structure
info "Creating UEFI directory structure..."
$USE_SUDO mkdir -p "$MOUNT_POINT/EFI/BOOT" || \
    error "Failed to create EFI/BOOT directory"

# Copy bootloader
info "Copying bootloader (BOOTX64.EFI)..."
$USE_SUDO cp "$BUILD_DIR/BOOTX64.EFI" "$MOUNT_POINT/EFI/BOOT/BOOTX64.EFI" || \
    error "Failed to copy bootloader"

# Copy kernel
info "Copying kernel (kernel.bin)..."
$USE_SUDO cp "$BUILD_DIR/kernel.bin" "$MOUNT_POINT/kernel.bin" || \
    error "Failed to copy kernel"

# Optional: Copy kernel ELF for debugging
if [ -f "$BUILD_DIR/kernel.elf" ]; then
    info "Copying kernel debug symbols (kernel.elf)..."
    $USE_SUDO cp "$BUILD_DIR/kernel.elf" "$MOUNT_POINT/kernel.elf" 2>/dev/null || \
        warn "Failed to copy kernel.elf (optional)"
fi

# Create a simple boot configuration file (for documentation)
info "Creating boot configuration..."
$USE_SUDO tee "$MOUNT_POINT/boot.txt" >/dev/null <<EOF
AuroraOS Boot Configuration
===========================

Bootloader: EFI/BOOT/BOOTX64.EFI
Kernel:     kernel.bin (at root)

Boot Process:
1. UEFI firmware loads BOOTX64.EFI
2. Bootloader initializes (gets memory map, GOP)
3. Bootloader loads kernel at 0x100000
4. Bootloader exits boot services
5. Bootloader jumps to kernel entry point (0x10000C)
6. Kernel receives control with boot_info structure

Created: $(date)
Build: $BUILD_DIR
EOF

# Sync and unmount
info "Syncing filesystem..."
sync

info "Unmounting ESP image..."
$USE_SUDO umount "$MOUNT_POINT" || warn "Failed to unmount (may already be unmounted)"

# Clean up mount point
$USE_SUDO rmdir "$MOUNT_POINT" 2>/dev/null || true

# Show ESP info
info "ESP image created successfully!"
echo ""
echo "  Image:    $ESP_IMAGE"
echo "  Size:     $ESP_SIZE_MB MB"
echo "  Format:   FAT32"
echo "  Contents:"
echo "    - EFI/BOOT/BOOTX64.EFI (bootloader)"
echo "    - kernel.bin (kernel binary)"
echo "    - boot.txt (boot info)"
echo ""
echo "To test with QEMU (requires OVMF):"
echo "  make run"
echo ""
echo "Or manually:"
echo "  qemu-system-x86_64 \\"
echo "    -bios /usr/share/ovmf/OVMF.fd \\"
echo "    -drive format=raw,file=$ESP_IMAGE \\"
echo "    -m 256M \\"
echo "    -serial stdio"
echo ""

exit 0
