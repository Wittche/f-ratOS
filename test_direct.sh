#!/bin/bash
# Direct QEMU test with multiboot

qemu-system-x86_64 \
    -kernel build/kernel.elf \
    -m 256M \
    -serial mon:stdio \
    -display none \
    -no-reboot \
    -no-shutdown
