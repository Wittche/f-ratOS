/**
 * AuroraOS Kernel - Kernel Thread Testing
 *
 * Simple kernel threads for testing scheduler and context switching
 */

#ifndef _KERNEL_KTHREAD_TEST_H_
#define _KERNEL_KTHREAD_TEST_H_

#include "types.h"

// Initialize and start test kernel threads
void kthread_test_init(void);

// Start the test threads
void kthread_test_start(void);

#endif // _KERNEL_KTHREAD_TEST_H_
