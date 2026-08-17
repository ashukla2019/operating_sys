Application → System Call → Physical Device
APPLICATION
   │
   │ calls
   ↓
LIBRARY FUNCTION
(e.g., read(), open(), write())
   │
   │ calls
   ↓
SYSTEM-CALL WRAPPER
   │
   │ prepares system-call number + arguments
   ↓
CPU REGISTERS
   │
   │ wrapper executes
   ↓
SYSTEM-CALL INSTRUCTION
(e.g., syscall)
   │
   │ causes
   ↓
CPU SWITCHES
USER MODE → KERNEL MODE
   │
   │ enters
   ↓
SYSTEM-CALL HANDLER / SYSTEM-CALL INTERFACE
   │
   │ uses system-call number to look up
   ↓
SYSTEM-CALL TABLE
   │
   │ selects/calls
   ↓
CORRESPONDING KERNEL SYSTEM-CALL ROUTINE
(e.g., sys_read(), sys_open(), sys_write())
   │
   │ may call
   ↓
KERNEL SUBSYSTEM
   │
   │ if hardware access is required, calls
   ↓
DEVICE DRIVER
   │
   │ communicates with
   ↓
DEVICE CONTROLLER
   │
   │ controls
   ↓
PHYSICAL DEVICE

Example: read()
Application
   │
   │ calls read(fd, buffer, 100)
   ↓
Library Function: read()
   │
   │ calls/prepares
   ↓
System-Call Wrapper
   │
   │ puts system-call number + arguments
   │ into CPU registers
   ↓
System-Call Instruction: syscall
   │
   │ CPU switches User Mode → Kernel Mode
   ↓
System-Call Handler
   │
   │ uses system-call number
   ↓
System-Call Table
   │
   │ identifies
   ↓
Kernel System-Call Routine: sys_read()
   │
   │ calls
   ↓
Kernel Subsystem / File System
   │
   │ if hardware access is required
   ↓
Device Driver
   │
   │ communicates with
   ↓
Device Controller
   │
   │ controls
   ↓
Physical Device (SSD/HDD)


Important: Not every system call reaches a device driver. Some system calls can be handled entirely inside the kernel. For example, getpid() does not need to access a physical device.
