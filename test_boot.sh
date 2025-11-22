#!/bin/bash
# GRUB boot test with QEMU debugging

echo "Building ISO..."
make clean && make kernel && make iso

echo ""
echo "Running QEMU with debug output..."
echo "Watch for:"
echo "  - CPU resets (triple fault)"
echo "  - Exception/interrupt numbers"
echo "  - Last EIP before reset"
echo ""

qemu-system-x86_64 \
    -cdrom build/auroraos.iso \
    -m 256M \
    -serial stdio \
    -d int,cpu_reset \
    -D qemu_debug.log \
    -no-reboot \
    -no-shutdown

echo ""
echo "=== QEMU Debug Log ==="
tail -50 qemu_debug.log
