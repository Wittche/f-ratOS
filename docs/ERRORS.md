# AuroraOS - Error Documentation

This file tracks all significant errors encountered during development, their root causes, and solutions.

## Format

Each entry follows this structure:
```
### [Date] Error Title
**Phase**: [Development phase]
**Component**: [Affected component]
**Description**: [What happened]
**Root Cause**: [Why it happened]
**Solution**: [How it was fixed]
**Commit**: [Relevant commit hash]
**Prevention**: [How to avoid in future]
---
```

## Error Log

---

## Tips and Lessons Learned

### General
- Always initialize all struct fields to prevent undefined behavior
- Use volatile for MMIO (Memory-Mapped I/O) registers
- Test each component incrementally before integration

### UEFI Development
- UEFI uses UCS-2 (wide characters) for strings
- EFI_HANDLE and EFI_SYSTEM_TABLE are critical - never lose them
- GOP (Graphics Output Protocol) must be acquired before ExitBootServices

### Kernel Development
- Never dereference NULL pointers - always check returns
- Stack alignment is critical (16-byte aligned on x86_64)
- Interrupts must be disabled during critical sections

### Memory Management
- Page tables must be page-aligned (4KB boundary)
- Always invalidate TLB after page table modifications
- Keep track of physical/virtual address translations

---

*Last updated: 2025-11-21*
