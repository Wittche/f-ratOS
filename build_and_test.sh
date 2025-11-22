#!/bin/bash
# AuroraOS - Build and Test Script
# This script rebuilds the kernel and creates a fresh bootable ISO

set -e  # Exit on any error

echo "========================================="
echo "AuroraOS Build & Test Script"
echo "========================================="
echo ""

# Step 1: Clean previous build
echo "[1/4] Cleaning previous build..."
make clean
echo ""

# Step 2: Build kernel
echo "[2/4] Building kernel..."
make kernel
echo ""

# Step 3: Create bootable ISO
echo "[3/4] Creating bootable ISO..."
make iso
echo ""

# Step 4: Verify ISO was created
echo "[4/4] Verifying ISO..."
if [ -f "build/auroraos.iso" ]; then
    echo "SUCCESS! ISO created:"
    ls -lh build/auroraos.iso
    file build/auroraos.iso
    echo ""
    echo "========================================="
    echo "BUILD SUCCESSFUL!"
    echo "========================================="
    echo ""
    echo "To test your kernel, run ONE of these commands:"
    echo ""
    echo "  1. Test in QEMU (recommended):"
    echo "     qemu-system-x86_64 -cdrom build/auroraos.iso -m 256M -serial stdio"
    echo ""
    echo "  2. Or use the Makefile shortcut:"
    echo "     make run-iso"
    echo ""
    echo "  3. To write to USB and boot on real hardware:"
    echo "     sudo dd if=build/auroraos.iso of=/dev/sdX bs=4M status=progress"
    echo "     (Replace /dev/sdX with your USB device!)"
    echo ""
else
    echo "ERROR: ISO file was not created!"
    exit 1
fi
