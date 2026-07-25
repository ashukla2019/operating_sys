# Part 1 — Linux Architecture

> **Goal:** Build a strong mental model of how Linux is organized before learning Process Management, System Calls, Memory Management, and VFS.
>
> This part is intentionally compact. It introduces only the concepts needed for the rest of the handbook.

---

# Contents

1. What is an Operating System?
2. Linux Architecture
3. User Space vs Kernel Space
4. Linux Boot Process
5. Linux Kernel Components
6. Summary

---

# Chapter 1 — What is an Operating System?

## Why Do We Need an Operating System?

A computer consists of hardware such as:

- CPU
- RAM
- Storage
- Keyboard
- Display
- Network Card

Applications cannot safely access hardware directly.

The Operating System (OS) acts as a **resource manager** between applications and hardware.

It provides:

- Process Management
- Memory Management
- File System
- Device Management
- Networking
- Security

---

## Operating System Architecture

```text
+---------------------------+
|      Applications         |
+---------------------------+
|    Operating System       |
+---------------------------+
| CPU | RAM | Disk | Device |
+---------------------------+
```

Instead of every application controlling hardware directly, all requests go through the Operating System.

---

## Responsibilities of an Operating System

### 1. Process Management

Creates and manages processes.

Examples:

- fork()
- exec()
- Scheduler
- Context Switch

---

### 2. Memory Management

Manages RAM.

Examples:

- Virtual Memory
- Paging
- Page Cache
- Copy-on-Write

---

### 3. File System Management

Provides a uniform way to access files.

Examples:

- open()
- read()
- write()
- close()

---

### 4. Device Management

Controls hardware devices through drivers.

Examples:

- Keyboard
- Mouse
- Disk
- USB
- Network Card

---

### 5. Networking

Provides networking functionality.

Examples:

- TCP/IP
- Socket
- Ethernet
- Wi-Fi

---

### 6. Security

Controls access to system resources.

Examples:

- User IDs (UID)
- Group IDs (GID)
- Permissions
- Capabilities

---

## Complete Flow

```text
Application

↓

Operating System

↓

CPU
Memory
Disk
Network
Devices
```

---

## Interview Questions

### Why do we need an Operating System?

An Operating System manages hardware resources and provides services such as process management, memory management, file systems, networking, and security.

---

### Can an application directly access hardware?

No.

Applications use Operating System services through **System Calls**.

---

# Chapter 2 — Linux Architecture

Linux follows a layered architecture.

```text
+----------------------------------+
|        User Applications         |
+----------------------------------+
|      C Library (glibc)           |
+----------------------------------+
|         System Calls             |
+----------------------------------+
|         Linux Kernel             |
|----------------------------------|
| Process Management               |
| Memory Management                |
| VFS                              |
| IPC                              |
| Networking                       |
| Device Drivers                   |
+----------------------------------+
|           Hardware               |
+----------------------------------+
```

---

## Layer Explanation

### User Applications

Programs like:

- Bash
- Chrome
- VS Code
- Python
- GCC

Applications run in **User Space**.

They cannot directly access hardware.

---

### C Library (glibc)

Most Linux applications do not invoke system calls directly.

Instead they call library functions such as:

```c
open();
read();
write();
printf();
malloc();
```

glibc converts these into system calls.

---

### System Calls

System calls are the entry point into the Linux kernel.

Examples:

- open()
- read()
- write()
- fork()
- execve()
- mmap()

---

### Linux Kernel

The kernel performs privileged operations.

It manages:

- Processes
- Memory
- Files
- Devices
- Networking
- Security

---

### Hardware

Physical devices such as:

- CPU
- RAM
- SSD
- HDD
- GPU
- NIC
- USB

---

## Complete Flow

```text
Application

↓

glibc

↓

System Call

↓

Kernel

↓

Hardware
```

---

## Interview Questions

### What is Linux?

Linux is a monolithic operating system kernel that manages hardware resources and provides services to applications.

---

### Why do applications use glibc?

glibc provides convenient APIs that internally invoke Linux system calls.

---

# Chapter 3 — User Space vs Kernel Space

Linux separates execution into two privilege levels.

- User Space
- Kernel Space

---

## User Space

Applications execute in User Space.

Examples:

- Chrome
- Bash
- VS Code
- Python

Restrictions:

- Cannot access kernel memory
- Cannot execute privileged instructions
- Cannot directly control hardware

---

## Kernel Space

Kernel code executes in Kernel Space.

The kernel has full hardware access.

Responsibilities:

- Scheduling
- Memory Management
- File Systems
- Device Drivers
- Networking

---

## Architecture

```text
+-------------------------+
|      User Space         |
|-------------------------|
| Applications            |
| glibc                   |
+-------------------------+

        System Call

+-------------------------+
|     Kernel Space        |
|-------------------------|
| Linux Kernel            |
| Drivers                 |
+-------------------------+
```

---

## User Mode → Kernel Mode

When an application requires a privileged operation:

```text
Application

↓

System Call

↓

CPU switches to Kernel Mode

↓

Kernel executes request

↓

Return to User Mode
```

This transition is called a **Mode Switch**.

---

## Why Separate User and Kernel Space?

Security

A buggy application cannot corrupt the kernel.

Stability

One crashing application does not crash the operating system.

Protection

Applications cannot access another application's memory.

---

## Interview Questions

### What is the difference between User Space and Kernel Space?

| User Space | Kernel Space |
|------------|--------------|
| Applications execute | Kernel executes |
| Limited privileges | Full privileges |
| Cannot access hardware directly | Can access hardware |

---

### What causes a mode switch?

A **System Call**, interrupt, or exception.

---

# Chapter 4 — Linux Boot Process

The Linux boot process starts when the system is powered on.

---

## Boot Flow

```text
Power On

↓

BIOS / UEFI

↓

Bootloader (GRUB)

↓

Linux Kernel

↓

initramfs

↓

systemd

↓

Applications
```

---

## BIOS / UEFI

Initializes hardware.

Finds a bootable device.

Loads the bootloader.

---

## Bootloader (GRUB)

Loads:

- Linux Kernel
- initramfs

Passes kernel parameters.

---

## Linux Kernel

Initializes:

- Memory
- Scheduler
- Device Drivers
- File Systems

Mounts the root filesystem.

---

## initramfs

Temporary root filesystem.

Used to:

- Load required drivers
- Mount the real root filesystem

---

## systemd

The first user-space process.

PID = 1

Starts:

- Services
- Login manager
- Network
- Applications

---

## Boot Flow Summary

```text
Power On

↓

Firmware

↓

GRUB

↓

Kernel

↓

initramfs

↓

systemd

↓

User Programs
```

---

## Interview Questions

### Which program loads the Linux kernel?

GRUB (or another bootloader).

---

### What is PID 1?

systemd (or another init system).

---

### Why is initramfs needed?

It loads essential drivers and prepares the real root filesystem before the normal filesystem becomes available.

---

# Chapter 5 — Linux Kernel Components

The Linux kernel is divided into multiple subsystems.

```text
Linux Kernel

├── Process Management
├── Memory Management
├── Virtual File System (VFS)
├── IPC
├── Networking
├── Device Drivers
├── Scheduler
└── Security
```

---

## Process Management

Responsible for:

- Process Creation
- Scheduling
- Context Switching
- Signals

Covered in **Part 2**.

---

## Memory Management

Responsible for:

- Virtual Memory
- Page Tables
- Paging
- Page Cache
- Copy-on-Write

Covered in **Part 4**.

---

## Virtual File System (VFS)

Provides a common interface for all filesystems.

Responsible for:

- open()
- read()
- write()
- close()

Covered in **Part 5**.

---

## IPC (Inter-Process Communication)

Allows processes to communicate.

Examples:

- Pipe
- Shared Memory
- Semaphore
- Socket
- Message Queue

Covered in **Part 6**.

---

## Networking

Responsible for:

- TCP/IP
- UDP
- Routing
- Socket Layer

---

## Device Drivers

Communicate with hardware devices.

Examples:

- USB Driver
- Disk Driver
- Network Driver
- GPIO Driver

---

## Scheduler

Responsible for:

- Selecting the next process to run
- Context Switching
- CPU time allocation

Covered in **Part 2**.

---

## Security

Responsible for:

- User IDs
- Group IDs
- Permissions
- Capabilities
- LSM (Linux Security Modules)

---

# Part 1 Summary

```text
Power On

↓

Firmware (BIOS / UEFI)

↓

GRUB

↓

Linux Kernel

↓

systemd

↓

Applications

↓

System Calls

↓

Kernel

↓

Hardware
```

The Linux kernel consists of several major subsystems:

- Process Management
- Memory Management
- Virtual File System (VFS)
- IPC
- Scheduler
- Networking
- Device Drivers
- Security

The following parts of this handbook explain these subsystems in detail, focusing on **how Linux works internally** and the execution flow commonly discussed in Linux Embedded and Kernel interviews.
-------------------------------------------------------------------------------------------  

# Part 2 — Process Management

> **Goal:** Understand how Linux creates, executes, schedules, manages, and destroys processes.
>
> By the end of this part, you should be able to explain exactly what happens when a process is created, runs, waits, receives signals, performs context switching, and exits.

---

# Contents

1. Program vs Process
2. Process Memory Layout
3. Process Creation (`fork()`, `exec()`)
4. Process Control Block (`task_struct`)
5. Process States
6. Context Switching
7. CPU Scheduler (CFS Overview)
8. Signals
9. Zombie, Orphan and Daemon Processes
10. Complete Process Lifecycle

---

# Chapter 1 — Program vs Process

## What is a Program?

A **Program** is a passive executable file stored on disk.

Examples:

- `/bin/ls`
- `/usr/bin/python3`
- `a.out`

A program contains:

- Machine code
- Static data
- Metadata

A program does **not** execute by itself.

---

## What is a Process?

A **Process** is a program that is currently executing.

A process has:

- PID
- Memory
- CPU registers
- Stack
- Heap
- Open files
- Scheduling information

One program can create multiple processes.

Example:

```bash
./server
```

Every execution creates a new process.

---

## Program vs Process

| Program | Process |
|----------|----------|
| Stored on disk | Running in memory |
| Passive | Active |
| No PID | Has PID |
| No CPU state | Has CPU context |
| Doesn't consume CPU | Executes on CPU |

---

## Complete Flow

```text
Program (Disk)

↓

fork()

↓

Process Created

↓

exec()

↓

Running Process

↓

exit()

↓

Destroyed
```

---

## Interview Questions

### Can one program create multiple processes?

Yes.

Each execution creates a separate process with its own PID and address space.

---

### Does a program have a PID?

No.

Only a process has a PID.

---

# Chapter 2 — Process Memory Layout

Every process has its own virtual address space.

```text
High Address
+----------------------+
| Stack                |
+----------------------+
| Memory Mapped Files  |
+----------------------+
| Shared Libraries     |
+----------------------+
| Heap                 |
+----------------------+
| BSS                  |
+----------------------+
| Data                 |
+----------------------+
| Text (Code)          |
+----------------------+
Low Address
```

---

## Text Segment

Contains:

- Executable instructions
- Read-only code

Example:

```c
int main()
{
    return 0;
}
```

---

## Data Segment

Stores initialized global and static variables.

Example:

```c
int x = 10;
```

---

## BSS Segment

Stores uninitialized global and static variables.

Example:

```c
int counter;
```

---

## Heap

Dynamic memory allocated during runtime.

Functions:

```c
malloc()
calloc()
realloc()
free()
```

The heap grows upward.

---

## Stack

Stores:

- Function calls
- Local variables
- Return addresses
- Function parameters

The stack grows downward.

---

## Memory Layout Summary

| Segment | Stores |
|----------|---------|
| Text | Program instructions |
| Data | Initialized globals/statics |
| BSS | Uninitialized globals/statics |
| Heap | Dynamic memory |
| Stack | Local variables and function calls |

---

## Interview Questions

### Where are local variables stored?

Stack.

---

### Where are global variables stored?

Data or BSS segment.

---

### Where does `malloc()` allocate memory?

Heap.

---

# Chapter 3 — Process Creation (`fork()` and `exec()`)

Linux creates new processes using two important system calls.

- `fork()`
- `exec()`

---

## Process Creation Flow

```text
Shell

↓

fork()

↓

Parent
     \
      \
       Child

↓

exec()

↓

New Program

↓

Running
```

---

## `fork()`

Creates a child process.

After `fork()`:

- Parent continues.
- Child continues.
- Both execute the next instruction.

Modern Linux uses **Copy-on-Write (CoW)**, so memory pages are shared until modified.

---

## `exec()`

Replaces the current process image with a new executable.

Example:

```c
execl("/bin/ls","ls",NULL);
```

The process keeps the same PID, but its code, data, heap, and stack are replaced by the new program.

---

## Process Creation Summary

```text
fork()

↓

Child Process

↓

exec()

↓

Load ELF

↓

main()
```

---

## Interview Questions

### Does `fork()` create a new program?

No.

It creates a new process.

---

### Does `exec()` create a new process?

No.

It replaces the current process image.

---

# Chapter 4 — Process Control Block (`task_struct`)

Linux stores information about every process in a kernel data structure called **`task_struct`**.

```text
task_struct

├── PID
├── State
├── Registers
├── Memory Info
├── Scheduling Info
├── Open Files
├── Signals
└── Credentials
```

---

## Why is it needed?

The kernel needs to remember:

- Which process is running?
- Which registers belong to it?
- Which files are open?
- What memory belongs to it?

All this information is stored in the PCB.

---

## Interview Questions

### What is PCB?

A kernel data structure storing all information required to manage a process.

---

### What is PCB called in Linux?

`task_struct`

---

# Chapter 5 — Process States

A process moves through different states during its lifetime.

```text
New

↓

Ready

↓

Running

↓

Waiting

↓

Ready

↓

Running

↓

Terminated
```

---

## Ready

Waiting for CPU.

---

## Running

Currently executing on a CPU.

---

## Waiting (Blocked)

Waiting for an event such as:

- Disk I/O
- Network
- Pipe
- Signal

---

## Terminated

Execution has completed.

---

## Interview Questions

### Difference between Ready and Waiting?

Ready → Waiting for CPU.

Waiting → Waiting for an event.

---

# Chapter 6 — Context Switching

Only one process can run on a CPU core at a time.

The kernel switches between processes using a **Context Switch**.

```text
CPU

↓

Process A

↓

Save Registers

↓

task_struct

↓

Load Registers

↓

Process B
```

---

## Context includes

- Program Counter (PC)
- Stack Pointer (SP)
- CPU Registers
- Process State

---

## Why is it needed?

Allows multiple processes to share the CPU.

---

## Interview Questions

### Does context switching copy process memory?

No.

Only the CPU execution context is saved and restored.

---

# Chapter 7 — CPU Scheduler (Overview)

The scheduler decides which process runs next.

```text
Ready Queue

↓

Scheduler

↓

Running Process
```

Linux uses the **Completely Fair Scheduler (CFS)** for normal processes.

Important concepts:

- Ready Queue
- `vruntime`
- Nice Value
- Time Slice

> Detailed CFS internals are covered later if needed.

---

## Interview Questions

### What is the job of the scheduler?

To select the next runnable process and allocate CPU time.

---

# Chapter 8 — Signals

Signals provide a way to notify a process that an event has occurred.

Common signals:

| Signal | Purpose |
|---------|---------|
| SIGINT | Interrupt (Ctrl+C) |
| SIGTERM | Graceful termination |
| SIGKILL | Forcefully terminate |
| SIGSTOP | Pause process |
| SIGCONT | Resume process |
| SIGCHLD | Child process terminated |

---

## Signal Flow

```text
kill()

↓

Kernel

↓

Target Process

↓

Signal Handler
```

---

## Interview Questions

### Can SIGKILL be ignored?

No.

The kernel always terminates the process.

---

# Chapter 9 — Zombie, Orphan and Daemon Processes

## Zombie Process

A process that has exited, but whose parent has not yet called `wait()`.

```text
Child exits

↓

Zombie

↓

Parent calls wait()

↓

Removed
```

---

## Orphan Process

A child process whose parent exits first.

The orphan is adopted by **systemd (PID 1)**.

---

## Daemon Process

A background process with no controlling terminal.

Examples:

- sshd
- systemd
- cron

---

## Interview Questions

### Why do Zombie processes exist?

So the parent can retrieve the child's exit status.

---

### Who adopts an orphan process?

PID 1 (`systemd` on modern Linux systems).

---

# Chapter 10 — Complete Process Lifecycle

```text
Program (Disk)

↓

fork()

↓

Child Process

↓

Copy-on-Write

↓

exec()

↓

Load ELF

↓

Process Memory Created

↓

Ready Queue

↓

Scheduler

↓

Running

↓

Context Switch

↓

Waiting (I/O)

↓

Running

↓

Signals

↓

exit()

↓

Zombie

↓

wait()

↓

Destroyed
```

---

# Part 2 Summary

You should now understand:

- Difference between a Program and a Process
- Process Memory Layout
- `fork()` and `exec()`
- Process Control Block (`task_struct`)
- Process States
- Context Switching
- CPU Scheduling (CFS overview)
- Signals
- Zombie, Orphan, and Daemon processes
- Complete lifecycle of a Linux process

> **Next:** **Part 3 — System Calls**, where we'll follow the complete execution path of system calls like `open()`, `read()`, `write()`, `mmap()`, and `ioctl()` from user space into the Linux kernel.
> -------------------------------------------------------------------------------------------
> # Part 3 — Linux System Calls

> **Goal:** Understand how an application communicates with the Linux kernel.
>
> This section focuses on the complete execution flow of commonly asked system calls in Linux Embedded and Kernel interviews.

---

# Contents

1. What is a System Call?
2. User Space → Kernel Space
3. System Call Execution Flow
4. open()
5. read()
6. write()
7. close()
8. lseek()
9. stat()
10. ioctl()
11. mmap()
12. fork()
13. execve()
14. wait()
15. System Call Summary

---

# Chapter 1 — What is a System Call?

Applications cannot directly access:

- Disk
- Memory Management
- Device Drivers
- Process Management
- Network Hardware

Instead, they request the Linux kernel to perform privileged operations.

This request is called a **System Call**.

---

## Common System Calls

| Category | Examples |
|----------|----------|
| File | open(), read(), write(), close() |
| Process | fork(), execve(), wait(), exit() |
| Memory | mmap(), brk() |
| Device | ioctl() |
| Networking | socket(), bind(), connect() |

---

## Complete Flow

```text
Application

↓

System Call

↓

Linux Kernel

↓

Hardware
```

---

## Interview Questions

### Why are System Calls required?

Applications run in User Space and cannot perform privileged operations directly.

---

# Chapter 2 — User Space → Kernel Space

When an application invokes a system call:

```text
Application

↓

glibc

↓

syscall

↓

CPU switches to Kernel Mode

↓

Kernel executes request

↓

Return to User Mode
```

This is called a **Mode Switch**.

---

## Steps

1. Application calls API.
2. glibc prepares system call.
3. CPU enters Kernel Mode.
4. Kernel validates arguments.
5. Kernel performs requested operation.
6. Result returned to User Space.

---

## Interview Questions

### Does every System Call perform a Context Switch?

No.

Every system call performs a **Mode Switch**.

A Context Switch happens only if another process is scheduled.

---

# Chapter 3 — System Call Execution Flow

Example:

```c
fd = open("file.txt", O_RDONLY);
```

Internally:

```text
Application

↓

glibc

↓

System Call

↓

Kernel

↓

VFS

↓

Filesystem

↓

Driver

↓

Hardware
```

Every system call follows a similar path.

---

# Chapter 4 — open()

Purpose:

Open a file and return a File Descriptor.

---

## Flow

```text
open()

↓

glibc

↓

syscall

↓

Kernel

↓

VFS

↓

Path Resolution

↓

Dentry Cache

↓

inode

↓

Filesystem (ext4)

↓

Create file object

↓

Allocate File Descriptor

↓

Return fd
```

---

## Important Concepts

- File Descriptor
- file object
- inode
- dentry
- VFS

These are explained in Part 5 (VFS).

---

## Interview Questions

### What does open() return?

A File Descriptor.

---

### Does open() read the file?

No.

It only opens the file and prepares kernel data structures.

---

# Chapter 5 — read()

Purpose:

Read data from an open file.

---

## Flow

```text
read()

↓

Kernel

↓

VFS

↓

Page Cache ?

      │

 YES          NO
 │             │
 │             ▼
 │      Filesystem
 │             │
 │       Block Layer
 │             │
 │       Device Driver
 │             │
 │            DMA
 │             │
 │        Interrupt
 │             │
 └─────────────┘
        │
        ▼
Page Cache

↓

copy_to_user()

↓

Return
```

---

## Important Concepts

- Page Cache
- DMA
- Interrupt
- Block Layer

---

## Interview Questions

### Why is the second read() faster?

Because data is usually already in the **Page Cache**.

---

# Chapter 6 — write()

Purpose:

Write data to a file.

---

## Flow

```text
write()

↓

copy_from_user()

↓

Kernel Buffer

↓

Page Cache

↓

Mark Dirty

↓

Return

↓

Background Writeback

↓

Filesystem

↓

Disk Driver

↓

Disk
```

---

## Important Concept

write() often returns before data is physically written to disk.

The kernel writes dirty pages later using background writeback threads.

---

## Interview Questions

### Does write() immediately write data to disk?

Usually no.

The data first goes to the Page Cache.

---

# Chapter 7 — close()

Purpose:

Close an open file descriptor.

---

## Flow

```text
close()

↓

Kernel

↓

Remove File Descriptor

↓

Decrease Reference Count

↓

If Last Reference

↓

Release file object
```

---

## Interview Questions

### Does close() delete the file?

No.

It only closes the open file descriptor.

---

# Chapter 8 — lseek()

Purpose:

Move the file offset.

---

## Flow

```text
lseek()

↓

Kernel

↓

file object

↓

Update Current Offset

↓

Return
```

---

## Important Point

lseek() changes only the current file position.

It does not read or write data.

---

# Chapter 9 — stat()

Purpose:

Retrieve file metadata.

---

## Flow

```text
stat()

↓

Kernel

↓

VFS

↓

inode

↓

Copy Metadata

↓

User Space
```

---

## Metadata Returned

- Size
- Owner
- Permissions
- Timestamps
- File Type

---

# Chapter 10 — ioctl()

Purpose:

Device-specific control operations.

Unlike read() and write(), ioctl() sends commands to a device driver.

---

## Flow

```text
Application

↓

ioctl()

↓

Kernel

↓

Device Driver

↓

Hardware
```

---

## Examples

- Configure serial port
- Control GPIO
- Camera settings
- SPI configuration

---

## Interview Questions

### Why do we need ioctl()?

Different devices need commands beyond simple read() and write().

---

# Chapter 11 — mmap()

Purpose:

Map a file or anonymous memory into a process's virtual address space.

---

## Flow

```text
mmap()

↓

Kernel

↓

Create Virtual Memory Area

↓

Page Table Updated

↓

Access Memory

↓

Page Fault

↓

Disk

↓

RAM
```

---

## Benefits

- Faster file access
- Shared memory
- Efficient large file handling

---

## Interview Questions

### Does mmap() immediately load the file?

No.

Pages are loaded on demand when first accessed.

---

# Chapter 12 — fork()

Purpose:

Create a child process.

---

## Flow

```text
fork()

↓

Kernel

↓

Create task_struct

↓

Copy Page Tables

↓

Copy-on-Write

↓

Parent

+

Child
```

---

## Important Point

Linux does not immediately copy all memory.

It uses **Copy-on-Write (CoW)**.

---

# Chapter 13 — execve()

Purpose:

Replace the current process image with a new program.

---

## Flow

```text
execve()

↓

Destroy Old Address Space

↓

Load ELF

↓

Map Shared Libraries

↓

Create Stack

↓

Jump to main()
```

---

## Important Point

PID remains the same.

Only the process image changes.

---

# Chapter 14 — wait()

Purpose:

Wait for a child process to terminate.

---

## Flow

```text
Parent

↓

wait()

↓

Child exits

↓

Collect Exit Status

↓

Zombie Removed
```

---

## Interview Questions

### Why is wait() required?

To collect the child's exit status and remove zombie processes.

---

# System Call Summary

| System Call | Purpose |
|-------------|---------|
| open() | Open a file |
| read() | Read file data |
| write() | Write file data |
| close() | Close a file descriptor |
| lseek() | Change file offset |
| stat() | Retrieve file metadata |
| ioctl() | Device-specific operations |
| mmap() | Map memory/file into virtual address space |
| fork() | Create a child process |
| execve() | Replace current process image |
| wait() | Wait for child process |

---

# Part 3 Summary

You should now understand:

- What a System Call is
- User Space → Kernel Space transition
- Mode Switch vs Context Switch
- Internal flow of `open()`, `read()`, `write()`, `close()`, `lseek()`, `stat()`, `ioctl()`, `mmap()`, `fork()`, `execve()`, and `wait()`

> **Next:** **Part 4 — Memory Management**, where you'll learn how Linux translates virtual addresses to physical addresses, handles page faults, manages page tables, TLBs, Copy-on-Write, page cache, and kernel memory allocation.
> ---------------------------------------------------------------------------------
> # Part 4 — Linux Memory Management

> **Goal:** Understand how Linux manages memory from the moment a program accesses an address until the data is fetched from RAM (or disk if necessary).
>
> Memory Management is one of the most important topics in Linux Embedded, Kernel, Storage, and Systems interviews.

---

# Contents

1. Why Virtual Memory?
2. Virtual Address Space
3. Virtual Address → Physical Address
4. MMU (Memory Management Unit)
5. Page Tables
6. TLB (Translation Lookaside Buffer)
7. Paging
8. Page Fault
9. Demand Paging
10. Copy-on-Write (CoW)
11. malloc() Internals
12. Kernel Memory Allocation
13. Memory Mapping (mmap)
14. Memory Management Summary

---

# Chapter 1 — Why Virtual Memory?

Every process thinks it owns the entire memory.

Example:

```text
Process A

0x00000000
     .
     .
0xFFFFFFFF
```

```text
Process B

0x00000000
     .
     .
0xFFFFFFFF
```

Both processes appear to have the same address range, but they actually use different physical memory.

Linux achieves this using **Virtual Memory**.

---

## Benefits

- Process isolation
- Memory protection
- Efficient memory utilization
- Shared libraries
- Larger address space

---

## Interview Questions

### Why is Virtual Memory required?

To isolate processes, simplify programming, and efficiently manage physical memory.

---

# Chapter 2 — Virtual Address Space

Every process has its own virtual address space.

```text
High Address
+----------------------+
| Stack                |
+----------------------+
| mmap Region          |
+----------------------+
| Shared Libraries     |
+----------------------+
| Heap                 |
+----------------------+
| Data                 |
+----------------------+
| Text (Code)          |
+----------------------+
Low Address
```

Each process has its own independent address space.

---

# Chapter 3 — Virtual Address → Physical Address

Applications never use physical addresses directly.

Flow:

```text
CPU

↓

Virtual Address

↓

MMU

↓

Page Table

↓

Physical Address

↓

RAM
```

The CPU generates a **Virtual Address**, and the MMU translates it into a **Physical Address**.

---

## Interview Questions

### Does the CPU generate physical addresses?

No.

The CPU generates virtual addresses.

The MMU performs the translation.

---

# Chapter 4 — MMU (Memory Management Unit)

The **MMU** is hardware inside the CPU responsible for address translation.

Responsibilities:

- Virtual → Physical translation
- Permission checking
- Page protection
- Triggering page faults

---

## MMU Flow

```text
CPU

↓

Virtual Address

↓

MMU

↓

Physical Address
```

Without an MMU, Virtual Memory would not exist.

---

# Chapter 5 — Page Tables

Memory is divided into fixed-size pages.

Example:

```
Page Size = 4 KB
```

Each virtual page maps to a physical frame.

```text
Virtual Page

↓

Page Table

↓

Physical Frame
```

The Page Table stores:

- Physical frame number
- Present bit
- Read/Write permissions
- Execute permission
- Dirty bit
- Accessed bit

---

## Interview Questions

### What does a Page Table store?

Mappings between virtual pages and physical frames, along with access permissions and status bits.

---

# Chapter 6 — TLB (Translation Lookaside Buffer)

Looking up the Page Table for every memory access is slow.

The CPU uses a cache called the **TLB**.

```text
CPU

↓

TLB

↓

Hit ?

│

├── Yes → Physical Address

└── No

↓

Page Table

↓

Update TLB

↓

Physical Address
```

---

## Interview Questions

### Why is TLB needed?

To speed up virtual-to-physical address translation.

---

# Chapter 7 — Paging

Physical memory is divided into fixed-size frames.

Virtual memory is divided into pages.

```text
Virtual Memory

Page 0

Page 1

Page 2

↓

Page Table

↓

Physical Memory

Frame 4

Frame 8

Frame 2
```

Pages can be stored anywhere in physical memory.

---

# Chapter 8 — Page Fault

A Page Fault occurs when a process accesses a page that is not currently in RAM.

```text
Access Address

↓

Present?

↓

No

↓

Page Fault

↓

Kernel

↓

Load Page

↓

RAM

↓

Resume Instruction
```

The process resumes after the page is loaded.

---

## Interview Questions

### Is every Page Fault an error?

No.

Many Page Faults are normal, such as Demand Paging.

---

# Chapter 9 — Demand Paging

Linux loads pages only when they are first accessed.

```text
Program Starts

↓

No Pages Loaded

↓

Access Page

↓

Page Fault

↓

Load Required Page

↓

Continue Execution
```

Benefits:

- Faster program startup
- Lower memory usage

---

# Chapter 10 — Copy-on-Write (CoW)

Copy-on-Write is used by `fork()`.

Instead of copying all memory immediately:

```text
Parent

↓

fork()

↓

Parent + Child

↓

Shared Pages

↓

Write?

↓

Copy Page

↓

Continue
```

Memory is copied **only when one process modifies it**.

---

## Interview Questions

### Why is Copy-on-Write efficient?

It avoids unnecessary memory copying after `fork()`.

---

# Chapter 11 — malloc() Internals

When an application calls:

```c
malloc(size);
```

Flow:

```text
Application

↓

malloc()

↓

glibc

↓

brk() or mmap()

↓

Kernel

↓

Page Allocator

↓

RAM
```

Small allocations usually extend the heap using `brk()`.

Large allocations are often handled with `mmap()`.

---

## Interview Questions

### Does malloc() directly allocate RAM?

No.

It requests memory from the kernel using mechanisms such as `brk()` or `mmap()`.

---

# Chapter 12 — Kernel Memory Allocation

The kernel cannot use `malloc()`.

Instead it uses:

| Allocator | Purpose |
|-----------|---------|
| Buddy Allocator | Allocate physical pages |
| Slab Allocator | Allocate frequently used kernel objects |
| kmalloc() | Allocate physically contiguous kernel memory |
| vmalloc() | Allocate virtually contiguous kernel memory |

---

## Simplified Flow

```text
Kernel

↓

kmalloc()

↓

Buddy Allocator

↓

Physical Pages
```

---

# Chapter 13 — Memory Mapping (mmap)

`mmap()` maps a file or anonymous memory into a process's virtual address space.

```text
mmap()

↓

Create Virtual Mapping

↓

Access Memory

↓

Page Fault

↓

Load Page

↓

RAM
```

Benefits:

- Shared memory
- Efficient file access
- Large file handling

---

## Interview Questions

### Does mmap() immediately load a file into RAM?

No.

Pages are loaded on demand when accessed.

---

# Memory Management Summary

```text
Application

↓

Virtual Address

↓

CPU

↓

TLB

↓

Page Table

↓

Physical Address

↓

RAM

↓

Page Missing?

↓

Page Fault

↓

Kernel

↓

Disk

↓

RAM

↓

Continue Execution
```

---

# Part 4 Summary

You should now understand:

- Why Linux uses Virtual Memory
- Virtual Address Space
- MMU
- Page Tables
- TLB
- Paging
- Page Faults
- Demand Paging
- Copy-on-Write
- malloc() internals
- Kernel memory allocation
- mmap()

> **Next:** **Part 5 — Virtual File System (VFS)**, where you'll learn how Linux resolves file paths, manages file descriptors, uses inodes and dentries, interacts with different filesystems (such as ext4 and NFS), and performs file I/O through the VFS layer.
> -------------------------------------------------------------------------------------
> # Part 5 — Virtual File System (VFS)

> **Goal:** Understand how Linux accesses files irrespective of the underlying filesystem (ext4, XFS, NFS, FAT, etc.).
>
> VFS is one of the most frequently asked topics in Linux Embedded, Storage, Kernel, and System Software interviews because almost every file operation (`open()`, `read()`, `write()`) passes through it.

---

# Contents

1. Why VFS?
2. VFS Architecture
3. File Path Resolution
4. Superblock
5. Inode
6. Dentry
7. File Object
8. File Descriptor
9. Opening a File
10. Reading a File
11. Writing a File
12. Closing a File
13. Page Cache
14. Complete File I/O Flow
15. VFS Summary

---

# Chapter 1 — Why VFS?

Linux supports many filesystems.

Examples:

- ext4
- XFS
- FAT32
- NFS
- tmpfs

Applications should not care which filesystem stores the file.

For example,

```c
open("/home/user/file.txt");
```

works the same regardless of the filesystem.

This is possible because of the **Virtual File System (VFS)**.

---

## VFS Layer

```text
Application

↓

System Call

↓

VFS

↓

ext4 / XFS / NFS / FAT
```

The VFS provides a **common interface** to all filesystems.

---

## Interview Questions

### Why do we need VFS?

To provide a uniform interface for accessing different filesystems.

---

# Chapter 2 — VFS Architecture

```text
Application

↓

glibc

↓

System Call

↓

VFS

↓

Filesystem (ext4, NFS, XFS...)

↓

Block Layer

↓

Device Driver

↓

Disk
```

Responsibilities of VFS:

- Path lookup
- File descriptor management
- Common file operations
- Dispatching requests to the correct filesystem

---

# Chapter 3 — File Path Resolution

Suppose the application executes:

```c
open("/home/user/file.txt");
```

Linux resolves the path component by component.

```text
/

↓

home

↓

user

↓

file.txt
```

For every component, VFS searches the corresponding directory entry.

The result is an **inode** representing the file.

---

## Dentry Cache

To speed up repeated path lookups, Linux caches directory entries.

```text
Path

↓

Dentry Cache

↓

Found?

│

├── Yes

└── No

↓

Filesystem Lookup
```

---

## Interview Questions

### Does Linux search the disk every time a file is opened?

No.

Previously resolved path components are often found in the **Dentry Cache**.

---

# Chapter 4 — Superblock

Every mounted filesystem has one **Superblock**.

It stores filesystem-level information.

Examples:

- Filesystem type
- Block size
- Total blocks
- Free blocks
- Root inode

---

## Relationship

```text
Filesystem

↓

Superblock

↓

Inodes
```

There is typically **one superblock per mounted filesystem**.

---

## Interview Questions

### What does a Superblock represent?

The metadata describing an entire filesystem.

---

# Chapter 5 — Inode

An **inode** describes a file.

It does **not** store the filename.

It stores:

- File size
- Owner
- Permissions
- Timestamps
- Block addresses
- File type

---

## Relationship

```text
Filename

↓

Dentry

↓

Inode

↓

Disk Blocks
```

---

## Interview Questions

### Does an inode store the filename?

No.

The filename is stored in the directory entry (dentry).

---

# Chapter 6 — Dentry

A **Dentry (Directory Entry)** connects a filename to an inode.

Example:

```text
notes.txt

↓

Dentry

↓

Inode
```

The dentry also speeds up path lookup using the **Dentry Cache**.

---

## Interview Questions

### What is the purpose of a dentry?

To map a filename to an inode.

---

# Chapter 7 — File Object

When a file is opened, Linux creates a **file object**.

It stores information specific to that open instance.

Examples:

- Current file offset
- Open mode
- File operations
- Reference count

---

## Relationship

```text
File Descriptor

↓

File Object

↓

Inode
```

Every call to `open()` creates a new file object, even if the same file is opened multiple times.

---

## Interview Questions

### What does the file object store?

Information about an open file instance, including the current file position.

---

# Chapter 8 — File Descriptor

A **File Descriptor (FD)** is a small integer returned by `open()`.

Example:

```c
int fd = open("notes.txt", O_RDONLY);
```

```text
fd = 3
```

The File Descriptor indexes the process's file descriptor table.

---

## Flow

```text
Process

↓

File Descriptor Table

↓

File Object

↓

Inode
```

---

## Interview Questions

### What is a File Descriptor?

An integer identifying an open file within a process.

---

# Chapter 9 — Opening a File

Complete flow:

```text
Application

↓

open()

↓

System Call

↓

VFS

↓

Path Resolution

↓

Dentry Cache

↓

Inode

↓

Filesystem

↓

Create File Object

↓

Allocate File Descriptor

↓

Return fd
```

---

## Objects Created

- File Descriptor
- File Object

Objects Used

- Dentry
- Inode
- Superblock

---

# Chapter 10 — Reading a File

Flow:

```text
read(fd)

↓

File Descriptor

↓

File Object

↓

Inode

↓

Address Space

↓

Page Cache

↓

Page Present?

│

├── Yes

│      ↓

│ copy_to_user()

│

└── No

↓

Filesystem

↓

Block Layer

↓

Device Driver

↓

DMA

↓

Disk

↓

Page Cache

↓

copy_to_user()
```

---

## Interview Questions

### Why is the second read() usually faster?

Because the required data is often already in the **Page Cache**.

---

# Chapter 11 — Writing a File

Flow:

```text
write(fd)

↓

copy_from_user()

↓

Page Cache

↓

Dirty Page

↓

Return

↓

Writeback Thread

↓

Filesystem

↓

Block Layer

↓

Device Driver

↓

Disk
```

---

## Important Point

`write()` usually returns before the data reaches the disk.

The kernel writes dirty pages later.

---

## Interview Questions

### Why does write() return before writing to disk?

Because Linux buffers writes in the **Page Cache** for better performance.

---

# Chapter 12 — Closing a File

Flow:

```text
close(fd)

↓

Remove File Descriptor

↓

Decrease Reference Count

↓

Last Reference?

│

├── No

└── Yes

↓

Destroy File Object
```

---

# Chapter 13 — Page Cache

The **Page Cache** stores recently accessed file data in RAM.

```text
Disk

↓

Page Cache

↓

Application
```

Benefits:

- Faster reads
- Faster writes
- Reduced disk access

---

## Interview Questions

### Is the Page Cache part of VFS?

The Page Cache is closely integrated with the VFS and the memory management subsystem. VFS uses it to cache file data.

---

# Chapter 14 — Complete File I/O Flow

## open()

```text
Application

↓

open()

↓

VFS

↓

Path Resolution

↓

Dentry

↓

Inode

↓

File Object

↓

File Descriptor
```

---

## read()

```text
Application

↓

read()

↓

FD

↓

File Object

↓

Page Cache

↓

Filesystem

↓

Driver

↓

Disk
```

---

## write()

```text
Application

↓

write()

↓

Page Cache

↓

Dirty Pages

↓

Writeback

↓

Disk
```

---

# VFS Object Relationship

This is the **most important diagram** for interviews.

```text
Filesystem

↓

Superblock

↓

Directory

↓

Dentry

↓

Inode

↓

File Object

↓

File Descriptor

↓

Application
```

Remember:

| Object | Purpose |
|----------|---------|
| Superblock | Filesystem metadata |
| Inode | File metadata |
| Dentry | Filename → inode mapping |
| File Object | Open file instance |
| File Descriptor | Handle used by the process |

---

# Part 5 Summary

You should now understand:

- Why VFS exists
- How Linux resolves a pathname
- Superblock
- Inode
- Dentry
- File Object
- File Descriptor
- Complete `open()`, `read()`, `write()`, and `close()` flows
- How Page Cache improves performance

> **Next:** **Part 6 — Inter-Process Communication (IPC)**, covering Pipes, FIFOs, Shared Memory, Semaphores, Message Queues, and Sockets, including when to use each mechanism and how the Linux kernel implements communication between processes.
> ------------------------------------------------------------------------------------
> # Part 6 — Inter-Process Communication (IPC)

> **Goal:** Understand how processes communicate with each other in Linux.
>
> Since each process has its own address space, processes cannot directly access each other's memory. Linux provides several IPC mechanisms for exchanging data and synchronizing execution.

---

# Contents

1. Why IPC?
2. Pipe
3. Named Pipe (FIFO)
4. Message Queue
5. Shared Memory
6. Semaphore
7. Socket
8. Signal
9. Choosing the Right IPC
10. IPC Summary

---

# Chapter 1 — Why IPC?

Each process has its own virtual memory.

```text
Process A

Memory A

----------------------------

Process B

Memory B
```

Process A cannot directly access Process B's memory.

Linux provides **Inter-Process Communication (IPC)** mechanisms to exchange data safely.

---

## Common IPC Mechanisms

| IPC | Purpose |
|------|----------|
| Pipe | Parent ↔ Child communication |
| FIFO | Communication between unrelated processes |
| Message Queue | Message-based communication |
| Shared Memory | Fastest data sharing |
| Semaphore | Synchronization |
| Socket | Local or Network communication |
| Signal | Event notification |

---

# Chapter 2 — Pipe

A **Pipe** is the simplest IPC mechanism.

It provides **one-way communication** between related processes.

Usually used between:

- Parent
- Child

---

## Flow

```text
Parent Process

Write End

======== PIPE ========>

Read End

Child Process
```

---

## Characteristics

- One-way communication
- Byte stream
- Parent-child processes
- Exists only while processes are running

---

## System Calls

```c
pipe()
read()
write()
close()
```

---

## Interview Questions

### Can unrelated processes use an unnamed pipe?

No.

Unnamed pipes are generally used only between related processes.

---

# Chapter 3 — Named Pipe (FIFO)

A FIFO behaves like a pipe but exists as a file in the filesystem.

```text
Process A

↓

FIFO File

↓

Process B
```

---

## Characteristics

- One-way communication
- Unrelated processes can communicate
- Exists in filesystem
- Can be opened by multiple processes

---

## System Calls

```c
mkfifo()
open()
read()
write()
close()
```

---

## Interview Questions

### Difference between Pipe and FIFO?

| Pipe | FIFO |
|------|------|
| Parent-child | Any process |
| Exists in memory | Exists as a file |
| Created with pipe() | Created with mkfifo() |

---

# Chapter 4 — Message Queue

Processes communicate by sending discrete messages.

Instead of reading raw bytes, processes exchange complete messages.

---

## Flow

```text
Process A

↓

Send Message

↓

Kernel Message Queue

↓

Receive Message

↓

Process B
```

---

## Characteristics

- Messages have boundaries
- Supports priorities
- Kernel manages queue
- Asynchronous communication

---

## System Calls

```c
msgget()
msgsnd()
msgrcv()
msgctl()
```

---

## Interview Questions

### Why use Message Queues instead of Pipes?

Message Queues preserve individual messages, while Pipes provide only a continuous byte stream.

---

# Chapter 5 — Shared Memory

Shared Memory allows multiple processes to map the same physical memory.

```text
            Shared Memory

        +------------------+
        |                  |
        |      Data        |
        |                  |
        +------------------+
           ▲            ▲
           │            │
      Process A    Process B
```

---

## Characteristics

- Fastest IPC mechanism
- No data copying after mapping
- Large data transfer
- Requires synchronization

---

## System Calls

```c
shmget()
shmat()
shmdt()
shmctl()
```

---

## Interview Questions

### Why is Shared Memory the fastest IPC?

Because processes access the same memory directly without repeatedly copying data through the kernel.

---

# Chapter 6 — Semaphore

Shared Memory provides data sharing.

Semaphores provide synchronization.

They prevent multiple processes from modifying shared data simultaneously.

---

## Flow

```text
Process A

↓

Acquire Semaphore

↓

Critical Section

↓

Release Semaphore

↓

Process B
```

---

## Uses

- Mutual exclusion
- Resource protection
- Producer-Consumer
- Shared Memory synchronization

---

## System Calls

```c
semget()
semop()
semctl()
```

---

## Interview Questions

### Does a Semaphore transfer data?

No.

It only synchronizes access to shared resources.

---

# Chapter 7 — Socket

Sockets allow communication:

- Between processes on the same machine
- Between processes on different machines

---

## Local Socket

```text
Process A

↓

UNIX Socket

↓

Process B
```

---

## Network Socket

```text
Client

↓

TCP/IP

↓

Server
```

---

## System Calls

```c
socket()
bind()
listen()
accept()
connect()
send()
recv()
close()
```

---

## Interview Questions

### Can sockets communicate over a network?

Yes.

Sockets support both local (UNIX domain) and network (TCP/UDP) communication.

---

# Chapter 8 — Signal

Signals are used to notify a process that an event has occurred.

Signals carry very little information—they are mainly used as notifications.

---

## Flow

```text
Process A

↓

kill()

↓

Kernel

↓

Signal

↓

Process B
```

---

## Common Signals

| Signal | Purpose |
|----------|----------|
| SIGINT | Interrupt |
| SIGTERM | Graceful termination |
| SIGKILL | Force terminate |
| SIGSTOP | Pause process |
| SIGCONT | Continue execution |
| SIGCHLD | Child exited |

---

## Interview Questions

### Are Signals used to transfer data?

No.

Signals are primarily notification mechanisms.

---

# Chapter 9 — Choosing the Right IPC

| IPC | Best Use Case |
|------|---------------|
| Pipe | Parent-child communication |
| FIFO | Unrelated local processes |
| Message Queue | Structured messages |
| Shared Memory | Large and high-speed data sharing |
| Semaphore | Synchronization |
| Socket | Local and network communication |
| Signal | Event notification |

---

# IPC Comparison

| IPC | Data Transfer | Synchronization | Network Support |
|------|---------------|-----------------|----------------|
| Pipe | Yes | No | No |
| FIFO | Yes | No | No |
| Message Queue | Yes | No | No |
| Shared Memory | Yes | Needs Semaphore | No |
| Semaphore | No | Yes | No |
| Socket | Yes | Application Managed | Yes |
| Signal | Notification Only | No | No |

---

# IPC Summary

```text
                 IPC

        ┌────────┼────────┐
        │        │        │
      Pipe     FIFO    Message Queue

        │
        │
   Shared Memory
        │
   Semaphore
        │
      Socket
        │
      Signal
```

---

# Part 6 Summary

You should now understand:

- Why IPC is needed
- Pipe
- FIFO
- Message Queue
- Shared Memory
- Semaphore
- Socket
- Signal
- When to use each IPC mechanism

---

# Complete Linux OS Flow

After completing Parts 1–6, you should be able to explain the following interview flows:

```text
Application
      │
      ▼
System Call
      │
      ▼
Kernel
      │
      ├── Process Management
      ├── Memory Management
      ├── Virtual File System (VFS)
      ├── IPC
      ├── Scheduler
      ├── Device Drivers
      └── Networking
      │
      ▼
Hardware
```

## What You Can Now Explain

- How a process is created (`fork()` → `execve()`)
- How a process is scheduled by Linux
- How a virtual address is translated to a physical address
- How a page fault is handled
- How `malloc()` gets memory
- How `open()`, `read()`, `write()`, and `close()` work internally
- The relationship between **Superblock → Inode → Dentry → File Object → File Descriptor**
- How the Page Cache improves file I/O performance
- How processes communicate using Linux IPC mechanisms

These concepts form the core Linux knowledge expected in most Linux Embedded, Kernel, Device Driver, Storage, BSP, and System Software interviews.
---------------------------------------------------------------------------------
# Part 7 — Complete Linux Execution Flows

> **Goal:** Connect Process Management, System Calls, Memory Management, and VFS together.
>
> This section explains complete end-to-end execution flows that are frequently asked in Linux Embedded, Linux Kernel, Storage, and System Software interviews.

---

# Contents

1. open() Flow
2. read() Flow
3. write() Flow
4. Process Creation Flow
5. Process Scheduling Flow
6. Virtual Address Translation
7. Page Fault Flow
8. mmap() Flow
9. Complete File I/O Flow
10. Linux Request Flow

---

# Flow 1 — open()

```text
Application

↓

open()

↓

glibc

↓

System Call

↓

Kernel

↓

VFS

↓

Path Lookup

↓

Dentry Cache

↓

inode

↓

Filesystem

↓

Create file object

↓

Allocate File Descriptor

↓

Return fd
```

### Important Objects

- File Descriptor
- File Object
- Dentry
- Inode
- Superblock

---

# Flow 2 — read()

```text
Application

↓

read(fd)

↓

Kernel

↓

File Descriptor

↓

File Object

↓

Page Cache ?

│

├── Yes

│

├── copy_to_user()

│

└── Return

│

└── No

↓

Filesystem

↓

Block Layer

↓

Device Driver

↓

DMA

↓

Disk

↓

Interrupt

↓

Page Cache

↓

copy_to_user()

↓

Return
```

---

# Flow 3 — write()

```text
Application

↓

write(fd)

↓

copy_from_user()

↓

Page Cache

↓

Dirty Pages

↓

Return

↓

Background Writeback

↓

Filesystem

↓

Driver

↓

Disk
```

---

# Flow 4 — Process Creation

```text
Application

↓

fork()

↓

Create task_struct

↓

Copy Page Tables

↓

Copy-on-Write

↓

Parent + Child

↓

execve()

↓

Load ELF

↓

Create New Address Space

↓

main()
```

---

# Flow 5 — CPU Scheduling

```text
Ready Queue

↓

Scheduler

↓

Running

↓

Timer Interrupt

↓

Save Registers

↓

task_struct

↓

Load Next Process

↓

Running
```

---

# Flow 6 — Virtual Address Translation

```text
CPU

↓

Virtual Address

↓

TLB

↓

Hit ?

│

├── Yes

│

└── Physical Address

│

└── No

↓

Page Table

↓

Physical Address
```

---

# Flow 7 — Page Fault

```text
Memory Access

↓

Page Present ?

│

├── Yes

│

└── Continue

│

└── No

↓

Page Fault

↓

Kernel

↓

Disk

↓

RAM

↓

Update Page Table

↓

Resume Process
```

---

# Flow 8 — mmap()

```text
mmap()

↓

Create Virtual Mapping

↓

Access Memory

↓

Page Fault

↓

Load Page

↓

RAM

↓

Continue
```

---

# Flow 9 — Complete File I/O

```text
Application

↓

System Call

↓

VFS

↓

File Object

↓

inode

↓

Filesystem

↓

Block Layer

↓

Device Driver

↓

Disk
```

---

# Flow 10 — Complete Linux Request Flow

```text
Application

↓

glibc

↓

System Call

↓

Kernel

├── Process Management
├── Memory Management
├── VFS
├── IPC
├── Networking
├── Device Drivers

↓

Hardware

↓

Interrupt

↓

Kernel

↓

Return to User Space
```

---

# Interview Tips

You should be able to draw these flows on a whiteboard without referring to notes:

- open()
- read()
- write()
- fork() → execve()
- Virtual Address → Physical Address
- Page Fault
- Context Switch
- Complete File I/O
- VFS Object Relationship
- IPC Selection
- ------------------------------------------------------------------------------

