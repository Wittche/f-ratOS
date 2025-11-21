/**
 * AuroraOS Kernel - Type Definitions
 */

#ifndef _KERNEL_TYPES_H_
#define _KERNEL_TYPES_H_

// Basic integer types
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

// Size types
typedef unsigned long      size_t;
typedef signed long        ssize_t;
typedef unsigned long      uintptr_t;
typedef signed long        intptr_t;

// Boolean
typedef int                bool;
#define true               1
#define false              0

// NULL pointer
#define NULL               ((void*)0)

// Kernel result codes
typedef int32_t            kern_return_t;
#define KERN_SUCCESS       0
#define KERN_FAILURE       (-1)
#define KERN_INVALID_ARG   (-2)
#define KERN_NO_MEMORY     (-3)
#define KERN_NOT_FOUND     (-4)

#endif // _KERNEL_TYPES_H_
