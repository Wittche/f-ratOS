#!/bin/bash
# Quick QEMU test script for AuroraOS
# Tests the bootable ISO in QEMU

if [ ! -f "build/auroraos.iso" ]; then
    echo "ERROR: build/auroraos.iso doesn't exist!"
    echo "Run ./build_and_test.sh first to create the ISO"
    exit 1
fi

echo "Starting AuroraOS in QEMU..."
echo "Press Ctrl+C to exit QEMU"
echo ""

qemu-system-x86_64 \
    -cdrom build/auroraos.iso \
    -m 256M \
    -serial stdio \
    -vga std
