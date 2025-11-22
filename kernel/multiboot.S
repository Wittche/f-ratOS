/**
 * AuroraOS Multiboot2 Header
 * Allows 64-bit kernel to be loaded by GRUB2 and QEMU
 */

.section .multiboot
.align 8

/* Multiboot2 header constants */
.set MULTIBOOT2_MAGIC,           0xE85250D6
.set MULTIBOOT2_ARCHITECTURE,    0          /* i386 (also used for x86_64) */
.set MULTIBOOT2_HEADER_LENGTH,   (multiboot_header_end - multiboot_header_start)
.set MULTIBOOT2_CHECKSUM,        -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCHITECTURE + MULTIBOOT2_HEADER_LENGTH)

/* Multiboot2 header */
multiboot_header_start:
    .long MULTIBOOT2_MAGIC
    .long MULTIBOOT2_ARCHITECTURE
    .long MULTIBOOT2_HEADER_LENGTH
    .long MULTIBOOT2_CHECKSUM

    /* Information request tag (optional) */
    .align 8
    .short 1    /* type = 1 (information request) */
    .short 0    /* flags */
    .long 16    /* size */
    .long 4     /* request memory map */
    .long 6     /* request memory basic info */

    /* Framebuffer tag (optional) */
    .align 8
    .short 5    /* type = 5 (framebuffer) */
    .short 0    /* flags */
    .long 20    /* size */
    .long 1024  /* width */
    .long 768   /* height */
    .long 32    /* depth */

    /* End tag (required) */
    .align 8
    .short 0    /* type = 0 (end) */
    .short 0    /* flags */
    .long 8     /* size */
multiboot_header_end:
