/**
 * AuroraOS Kernel - User Mode Support
 *
 * Functions for transitioning to user mode (Ring 3)
 */

#ifndef _KERNEL_USERMODE_H_
#define _KERNEL_USERMODE_H_

#include "types.h"

/**
 * Jump to user mode and execute a function
 *
 * This function sets up the user mode stack and jumps to Ring 3.
 * It never returns (unless the user program calls exit).
 *
 * @param entry_point User mode function to execute
 * @param user_stack User mode stack pointer
 */
void jump_to_usermode(void (*entry_point)(void), uint64_t user_stack);

/**
 * Create and start a user mode process
 *
 * This is a higher-level function that:
 * 1. Allocates user stack
 * 2. Sets up TSS
 * 3. Jumps to user mode
 *
 * @param entry_point User mode function to execute
 */
void start_usermode_process(void (*entry_point)(void));

#endif // _KERNEL_USERMODE_H_
