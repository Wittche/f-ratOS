/**
 * AuroraOS Multiboot Header
 * Allows kernel to be loaded by GRUB and QEMU
 */

.section .multiboot
.align 4

/* Multiboot header constants */
.set MULTIBOOT_MAGIC,        0x1BADB002
.set MULTIBOOT_FLAGS,        0x00000003  /* ALIGN + MEMINFO */
.set MULTIBOOT_CHECKSUM,     -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

/* Multiboot header */
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM
