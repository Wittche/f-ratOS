#!/bin/bash
#
# AuroraOS ISO Creator
# Creates a bootable ISO with GRUB2 and the kernel
#

set -e  # Exit on error

# Configuration
BUILD_DIR="build"
ISO_DIR="$BUILD_DIR/iso"
ISO_IMAGE="$BUILD_DIR/auroraos.iso"

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

# Check required tools
info "Checking required tools..."

if ! command -v grub-mkrescue &> /dev/null; then
    error "grub-mkrescue not found. Install: sudo apt install grub-pc-bin grub-efi-amd64-bin xorriso"
fi

if ! command -v xorriso &> /dev/null; then
    error "xorriso not found. Install: sudo apt install xorriso"
fi

info "All required tools present"

# Check required files
info "Checking required files..."

if [ ! -f "$BUILD_DIR/kernel.elf" ]; then
    error "kernel.elf not found. Run 'make' first."
fi

if [ ! -f "grub.cfg" ]; then
    error "grub.cfg not found in project root"
fi

info "All required files present"

# Clean and create ISO directory structure
info "Creating ISO directory structure..."

rm -rf "$ISO_DIR"
mkdir -p "$ISO_DIR/boot/grub"

# Copy kernel
info "Copying kernel..."
cp "$BUILD_DIR/kernel.elf" "$ISO_DIR/boot/kernel.elf"

# Copy GRUB config
info "Copying GRUB configuration..."
cp grub.cfg "$ISO_DIR/boot/grub/grub.cfg"

# Create ISO with GRUB
info "Creating bootable ISO with GRUB..."

grub-mkrescue -o "$ISO_IMAGE" "$ISO_DIR" 2>&1 | grep -v "WARNING: Couldn't find" || true

# Check if ISO was created
if [ ! -f "$ISO_IMAGE" ]; then
    error "Failed to create ISO image"
fi

# Show ISO info
ISO_SIZE=$(du -h "$ISO_IMAGE" | cut -f1)

info "ISO image created successfully!"
echo ""
echo "  Image:    $ISO_IMAGE"
echo "  Size:     $ISO_SIZE"
echo "  Format:   Bootable ISO9660 with GRUB2"
echo "  Contents:"
echo "    - boot/kernel.elf (kernel)"
echo "    - boot/grub/grub.cfg (GRUB config)"
echo ""
echo "To test with QEMU:"
echo "  make run-iso"
echo ""
echo "Or manually:"
echo "  qemu-system-x86_64 \\"
echo "    -cdrom $ISO_IMAGE \\"
echo "    -m 256M \\"
echo "    -serial stdio"
echo ""
echo "To boot on real hardware:"
echo "  1. Write to USB: sudo dd if=$ISO_IMAGE of=/dev/sdX bs=4M status=progress"
echo "  2. Boot from USB"
echo ""

exit 0
