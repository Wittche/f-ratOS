# AuroraOS Architecture

## Overview

AuroraOS is a modern operating system inspired by macOS, featuring a hybrid kernel architecture that combines the best aspects of microkernel and monolithic designs.

## Kernel Architecture

### Hybrid Kernel Design (XNU-inspired)

```
┌─────────────────────────────────────────┐
│           User Space                     │
│  ┌──────────┐  ┌──────────┐  ┌────────┐│
│  │   Apps   │  │ Daemons  │  │  GUI   ││
│  └──────────┘  └──────────┘  └────────┘│
└─────────────────────────────────────────┘
         │                 │
         │   System Calls  │
         ▼                 ▼
┌─────────────────────────────────────────┐
│        BSD Layer (Kernel Space)         │
│  ┌──────────────────────────────────┐   │
│  │  POSIX API, VFS, Network Stack   │   │
│  └──────────────────────────────────┘   │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│    Mach Microkernel (Core Services)     │
│  ┌────────┐ ┌────────┐ ┌─────────────┐ │
│  │  IPC   │ │ VM/MM  │ │  Scheduler  │ │
│  └────────┘ └────────┘ └─────────────┘ │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│         Hardware Abstraction            │
└─────────────────────────────────────────┘
```

### Mach Layer (Microkernel Core)

The Mach layer provides fundamental kernel services:

**1. Tasks and Threads**
- Tasks: Resource containers (address space, IPC ports)
- Threads: Execution units within tasks
- Preemptive scheduling with priorities

**2. Virtual Memory (VM)**
- Demand paging
- Copy-on-Write (COW)
- Memory objects
- Page replacement policies

**3. IPC (Inter-Process Communication)**
- Port-based messaging
- Synchronous/asynchronous message passing
- Port rights (send, receive, send-once)
- Message queues

**4. Time Services**
- High-resolution timers
- Clock management

### BSD Layer

Built on top of Mach, provides Unix compatibility:

**1. VFS (Virtual File System)**
- File system abstraction
- Multiple FS support (HFS+, FAT32, ext2)
- Mount points
- File descriptors

**2. POSIX API**
- Standard Unix system calls
- Process management (fork, exec, wait)
- Signal handling
- File I/O

**3. Network Stack**
- TCP/IP implementation
- Socket API
- Network drivers

**4. Device Framework**
- Device drivers
- I/O Kit-like driver model

## Boot Process

### Phase 1: UEFI Firmware
```
1. Power-On Self Test (POST)
2. Initialize UEFI firmware
3. Locate EFI System Partition (ESP)
4. Load bootloader (boot.efi) from ESP
```

### Phase 2: Bootloader (boot.efi)
```
1. Initialize UEFI console output
2. Load kernel binary from disk
3. Parse kernel ELF headers
4. Allocate memory for kernel
5. Setup page tables for kernel
6. Create kernel boot info structure
7. Exit UEFI Boot Services
8. Jump to kernel entry point
```

### Phase 3: Kernel Initialization
```
1. Initialize console/serial
2. Setup GDT/IDT
3. Initialize memory management
   - Physical memory manager
   - Virtual memory/paging
   - Kernel heap
4. Initialize Mach services
   - Thread system
   - IPC ports
   - Scheduler
5. Initialize BSD layer
   - VFS
   - Device framework
6. Mount root filesystem
7. Load init process (launchd)
8. Enter multitasking mode
```

## Memory Layout

### Physical Memory
```
0x00000000 - 0x000FFFFF : Real mode (1MB) - Reserved
0x00100000 - 0x003FFFFF : Kernel code/data (3MB)
0x00400000 - 0x00FFFFFF : Kernel heap (12MB)
0x01000000 - 0xXXXXXXXX : Available memory
```

### Virtual Memory (x86_64)
```
0x0000000000000000 - 0x00007FFFFFFFFFFF : User space (128TB)
0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF : Kernel space (128TB)
  0xFFFF800000000000 : Direct physical mapping
  0xFFFF880000000000 : Kernel heap
  0xFFFFFFFF80000000 : Kernel code/data
```

## GUI Architecture

### Display System

```
┌──────────────────────────────────────┐
│         Applications                  │
│  Use UIKit-like framework             │
└──────────────────────────────────────┘
              │
              ▼
┌──────────────────────────────────────┐
│      UI Framework (libui)            │
│  Widgets, layouts, event handling     │
└──────────────────────────────────────┘
              │
              ▼
┌──────────────────────────────────────┐
│   Compositor (WindowServer)          │
│  - Window management                  │
│  - Compositing                        │
│  - Event routing                      │
└──────────────────────────────────────┘
              │
              ▼
┌──────────────────────────────────────┐
│    Display Driver (GOP/VBE)          │
│  Frame buffer, mode setting           │
└──────────────────────────────────────┘
```

### Compositor (Quartz-inspired)

**Responsibilities:**
1. Window server - manages all windows
2. Compositing - combines window contents
3. Event queue - keyboard, mouse events
4. Drawing primitives - 2D graphics

**Window Management:**
- Each window has backing store (buffer)
- Compositor blends windows to screen
- Only compositor writes to framebuffer

## File System

### HFS+ Inspired Design

**Features:**
- Journaling for crash recovery
- Extended attributes (metadata)
- Hard and symbolic links
- Unicode filenames (UTF-8)
- POSIX permissions

**Structure:**
```
- Superblock (volume header)
- Allocation bitmap
- Catalog file (B-tree of files/directories)
- Extents overflow file
- Attributes file
- Journal
```

## Process Model

### Mach Tasks vs BSD Processes

**Mach Task:**
- Container for resources
- Address space
- IPC port space
- Collection of threads

**BSD Process:**
- Built on top of Mach task
- POSIX semantics (PID, PPID, UID, GID)
- Signal handling
- File descriptors
- Working directory

### Thread Scheduling

**Priority Levels:**
- Real-time (0-63): Time-critical
- Normal (64-127): User threads
- Background (128-255): Low priority

**Scheduling Algorithm:**
- Multi-level feedback queues
- Round-robin within priority
- Priority boosting for interactive processes

## IPC System

### Port-Based Messaging

**Port Types:**
1. **Port Set**: Collection of ports
2. **Port**: Message queue endpoint
3. **Port Rights**:
   - SEND: Can send messages
   - RECEIVE: Can receive messages
   - SEND_ONCE: Single-use send right

**Message Structure:**
```c
typedef struct {
    mach_msg_header_t header;
    mach_msg_body_t body;
    // payload data
} mach_msg_t;
```

**Operations:**
- `mach_msg_send()`: Send message to port
- `mach_msg_receive()`: Receive from port
- `mach_msg()`: Combined send/receive

## System Services

### LaunchD-inspired Init System

**Responsibilities:**
1. First user-space process (PID 1)
2. Launch system daemons
3. Manage services lifecycle
4. Handle service dependencies
5. Socket activation
6. User session management

**Service Definition:**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<plist>
    <dict>
        <key>Label</key>
        <string>com.aurora.example</string>
        <key>ProgramArguments</key>
        <array>
            <string>/usr/bin/example</string>
        </array>
        <key>RunAtLoad</key>
        <true/>
    </dict>
</plist>
```

## Security Model

### Capabilities
- Fine-grained permissions
- Inherited through IPC
- Checked at kernel level

### Sandboxing
- Process isolation
- Restricted file system access
- Limited IPC permissions

---

*This document is a living architecture guide and will be updated as development progresses.*
