/* Multiboot2 Header for GRUB */
/* This MUST be in the first 32KB of the kernel binary */

.section .multiboot
.align 8

/* Multiboot2 Header Start */
multiboot_header_start:
    .long 0xe85250d6                        /* Magic number (Multiboot2) */
    .long 0                                 /* Architecture: i386 protected mode */
    .long multiboot_header_end - multiboot_header_start  /* Header length */
    .long -(0xe85250d6 + 0 + (multiboot_header_end - multiboot_header_start))  /* Checksum */

    /* Information Request Tag */
    .align 8
    .short 1                                /* Type: Information request */
    .short 0                                /* Flags: required */
    .long info_request_end - info_request_start  /* Size */
info_request_start:
    .long 3                                 /* Request: Module info */
    .long 4                                 /* Request: Basic memory info */
    .long 6                                 /* Request: Memory map */
    .long 8                                 /* Request: Framebuffer info */
info_request_end:

    /* Entry Address Tag - Tell GRUB exactly where to jump */
    .align 8
    .short 3                                /* Type: Entry address */
    .short 0                                /* Flags: required */
    .long 12                                /* Size */
    .long _start                            /* Entry point symbol */

    /* Framebuffer Tag (optional) */
    .align 8
    .short 5                                /* Type: Framebuffer */
    .short 1                                /* Flags: optional */
    .long 20                                /* Size */
    .long 0                                 /* Width (0 = no preference) */
    .long 0                                 /* Height (0 = no preference) */
    .long 32                                /* Depth (32 bits per pixel) */

    /* End Tag */
    .align 8
    .short 0                                /* Type: End tag */
    .short 0                                /* Flags */
    .long 8                                 /* Size */

multiboot_header_end:
