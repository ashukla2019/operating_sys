# Part 1 – Linux Architecture (Interview Revision)
## Senior Linux Internals (2 Pages)

---

# Goal

Understand the complete Linux architecture from **Application → Hardware**.

Everything in Linux fits into this picture.

```text
Application
      │
      ▼
C Library (glibc)
      │
      ▼
System Call Interface
      │
      ▼
Linux Kernel
      │
 ┌────┼──────────────────────────────────────────────┐
 │    │      │       │       │       │              │
 ▼    ▼      ▼       ▼       ▼       ▼              ▼
Process Memory  VFS  Network  IPC  Scheduler  Drivers
Management              Stack
 │
 ▼
Hardware Abstraction
 │
 ▼
CPU • RAM • Disk • NIC • Devices
```

---

# Linux Layer-by-Layer

## 1. User Space

Runs applications.

Examples

```text
Chrome
Firefox
SSH
Bash
Python
Docker
MySQL
```

Characteristics

- Cannot access hardware directly
- Cannot access kernel memory
- Uses system calls for kernel services
- Process isolation

---

## 2. C Library (glibc)

Applications rarely invoke system calls directly.

Instead,

```text
printf()
malloc()
open()
read()
write()
pthread_create()
```

are library functions.

Flow

```text
Application
      │
      ▼
glibc
      │
      ▼
System Call
```

Example

```c
fd = open("file.txt", O_RDONLY);
```

Internally

```text
glibc
   ↓
syscall()
   ↓
Kernel
```

---

## 3. System Call Interface

Boundary between

```text
User Space
      ↓
Kernel Space
```

Common system calls

```text
open()
close()
read()
write()
mmap()
fork()
execve()
socket()
connect()
ioctl()
poll()
epoll()
```

Interview Question

> Why do we need system calls?

Answer

```text
Applications cannot directly access
kernel memory or hardware.

System calls provide a controlled,
secure interface.
```

---

## 4. Kernel Space

Kernel runs in privileged mode.

Responsibilities

```text
Process Management
Memory Management
Virtual Memory
CPU Scheduling
VFS
Networking
IPC
Drivers
Security
Interrupt Handling
```

Everything eventually reaches the kernel.

---

# Major Kernel Subsystems

## Process Management

Responsible for

```text
Processes
Threads
fork()
exec()
Signals
PID
Context Switching
```

Main structures

```text
task_struct
```

Interview keywords

```text
Scheduler
Context Switch
Thread
Process
PID
```

---

## Memory Management

Responsible for

```text
Virtual Memory
Paging
Page Tables
Page Cache
Anonymous Memory
mmap()
Heap
Stack
```

Main structures

```text
mm_struct
vm_area_struct
page
```

Keywords

```text
Page Fault
TLB
Virtual Address
Physical Address
```

---

## Virtual File System (VFS)

Provides one common interface for every filesystem.

```text
Application
      │
      ▼
VFS
      │
 ┌────┼─────────────┐
 ▼    ▼             ▼
ext4 xfs         tmpfs
```

Important objects

```text
super_block
inode
dentry
file
```

Interview Question

> Why VFS?

Answer

```text
Applications don't care whether the
filesystem is ext4, xfs or NFS.

VFS provides one common API.
```

---

## Networking Stack

Responsible for

```text
Sockets
TCP
UDP
IP
Routing
ARP
NIC Driver
```

Flow

```text
Application

socket()

↓

Socket Layer

↓

TCP / UDP

↓

IP

↓

NIC Driver

↓

Hardware
```

Important structures

```text
socket
sock
sk_buff
net_device
```

---

## Scheduler

Responsible for

```text
Choosing next process

CPU sharing

Fair scheduling
```

Linux scheduler

```text
CFS
```

Concepts

```text
Run Queue
Priority
Time Slice
Context Switch
```

---

## Device Drivers

Responsible for

```text
Keyboard
Disk
USB
PCI
Network Card
GPU
```

Driver exposes operations like

```text
open
read
write
ioctl
poll
mmap
```

---

## IPC

Communication between processes.

Mechanisms

```text
Pipe
FIFO
Shared Memory
Message Queue
Semaphore
Signal
Socket
```

---

# Hardware Layer

Linux finally interacts with

```text
CPU

Memory

Disk

Network Card

USB

PCI Devices

GPU
```

Hardware interrupts notify the kernel.

---

# Complete Request Flow

Example

```text
                 USER SPACE
┌─────────────────────────────────────┐
│                                     │
│          Application                │
│              │                      │
│              ▼                      │
│       libc / syscall wrapper        │
│              │                      │
└──────────────┼──────────────────────┘
               │
               │ syscall instruction
               ▼
════════════════════════════════════════
        USER SPACE → KERNEL SPACE
             privilege transition
════════════════════════════════════════
               │
               ▼
┌─────────────────────────────────────┐
│             KERNEL SPACE             │
│                                     │
│       System Call Entry             │
│              │                      │
│              ▼                      │
│       System Call Dispatcher        │
│              │                      │
│              ▼                      │
│       System Call Handler           │
│              │                      │
│              ▼                      │
│              VFS                    │
│              │                      │
│          Filesystem                 │
│              │                      │
│          Block Layer                │
│              │                      │
│        Device Driver                │
│              │                      │
│          SSD / HDD                  │
│                                     │
└──────────────────┬──────────────────┘
                   │
                   │ return
                   ▼
              USER SPACE

```

Networking example

```text
send()

↓

Socket

↓

TCP

↓

IP

↓

NIC Driver

↓

Ethernet

↓

Network
```

---

# Linux Memory Separation

```text
+-------------------------+
| User Space              |
| Applications            |
+-------------------------+

System Call Boundary

+-------------------------+
| Kernel Space            |
| Linux Kernel            |
+-------------------------+
```

Applications cannot directly access kernel memory.

---

# Interview Dependency Chain

```text
Application
      ↓
glibc
      ↓
System Call
      ↓
Kernel
      ↓
Subsystem
      ↓
Driver
      ↓
Hardware
```

Expanded view

```text
Application
      ↓
glibc
      ↓
System Call
      ↓
Linux Kernel
      ↓
Process Management
Memory Management
VFS
Networking
Scheduler
IPC
Drivers
      ↓
Hardware
```

---

# Senior Interview Questions

You should answer these without hesitation.

```text
✓ User Space vs Kernel Space

✓ Why System Calls?

✓ Why glibc?

✓ What happens after open()?

✓ What happens after read()?

✓ What happens after write()?

✓ Why VFS?

✓ Kernel responsibilities

✓ Major kernel subsystems

✓ Scheduler responsibility

✓ Process vs Thread

✓ Virtual Memory

✓ Why page tables?

✓ Driver responsibilities

✓ Socket to NIC flow

✓ Complete application to hardware flow
```

---

# One-Minute Revision

```text
Application
      ↓
glibc
      ↓
System Call
      ↓
Linux Kernel
      ├── Process Management
      ├── Memory Management
      ├── Scheduler
      ├── VFS
      ├── Networking
      ├── IPC
      └── Device Drivers
              ↓
           Hardware
```

> **Remember:** Every Linux interview question eventually maps to one of these kernel subsystems.
