/* Multiboot2 Header - SIMPLIFIED */
.section .multiboot
.align 8

multiboot_header_start:
    /* Required: Magic + Architecture + Length + Checksum */
    .long 0xe85250d6                /* Magic */
    .long 0                         /* i386 protected mode */
    .long multiboot_header_end - multiboot_header_start
    .long -(0xe85250d6 + 0 + (multiboot_header_end - multiboot_header_start))

    /* End tag - REQUIRED */
    .align 8
    .short 0                        /* Type: end */
    .short 0                        /* Flags */
    .long 8                         /* Size */

multiboot_header_end:
