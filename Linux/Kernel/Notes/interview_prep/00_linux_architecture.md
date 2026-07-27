# Linux Architecture

**Interview Importance:** ⭐⭐⭐⭐⭐

---

## What is Linux?

Linux is a monolithic kernel responsible for managing:

- CPU
- Memory
- Processes
- Filesystems
- Networking
- Device Drivers
- IPC
- Security

Think of Linux as a **resource manager**, not just an operating system.

---

## High-Level Architecture

```
                User Space
+--------------------------------------+
| Chrome | Bash | Python | Your App    |
+--------------------------------------+
               |
               | Library Call (glibc)
               |
               v
================ System Call ============
               |
               v
+--------------------------------------+
|            Linux Kernel              |
|--------------------------------------|
| Scheduler                            |
| Memory Manager                       |
| VFS                                  |
| Networking                           |
| IPC                                  |
| Device Drivers                       |
+--------------------------------------+
               |
               v
========================================
| CPU | RAM | SSD | NIC | GPU | USB    |
========================================
```

---

## User Space vs Kernel Space

| User Space | Kernel Space |
|------------|--------------|
| Applications | Linux Kernel |
| Ring 3 | Ring 0 |
| No direct hardware access | Full hardware access |
| Uses system calls | Executes privileged instructions |

**Why separate them?**

- Security
- Stability
- Memory protection
- Prevent application crashes from affecting the whole system

---

## System Call Flow

Applications cannot directly access hardware.

Example:

```
printf()

↓

glibc

↓

write()

↓

System Call

↓

Kernel

↓

TTY Driver

↓

Display
```

Another example:

```
open("file.txt")

↓

System Call

↓

VFS

↓

Filesystem Driver

↓

SSD
```

---

## Major Kernel Subsystems

| Subsystem | Responsibility |
|------------|---------------|
| Scheduler | CPU scheduling & context switching |
| Memory Manager | Virtual memory, paging, mmap() |
| VFS | Common interface for all filesystems |
| IPC | Process communication |
| Networking | TCP/IP, sockets |
| Drivers | Hardware access |

---

## Boot Overview

```
Power On
   ↓
BIOS / UEFI
   ↓
Bootloader (GRUB/U-Boot)
   ↓
Linux Kernel
   ↓
systemd (PID 1)
   ↓
Applications
```

---

## Interrupt vs Exception vs System Call

| Interrupt | Exception | System Call |
|-----------|-----------|-------------|
| Hardware generated | CPU generated | Application generated |
| Keyboard, NIC | Page Fault, Divide by Zero | read(), write(), fork() |

---

## Interview Questions

### Q. Why can't applications directly access hardware?

Applications execute in User Space with restricted privileges. Hardware access is performed through system calls into Kernel Space for security and stability.

---

### Q. Why is Linux called a monolithic kernel?

Core services such as scheduling, memory management, networking, VFS, and drivers execute in kernel space and communicate directly, reducing overhead compared to a microkernel.

---

### Q. Is printf() a system call?

No.

`printf()` is a **glibc library function**.

It eventually invokes the `write()` system call.

---

### Q. Which kernel subsystem handles open()?

```
open()

↓

VFS

↓

Filesystem Driver

↓

Disk Driver
```

---

## Commands

```bash
uname -a          # Kernel version
lsmod             # Loaded modules
lscpu             # CPU information
free -h           # Memory
ps -ef            # Processes
top               # CPU usage
vmstat            # VM statistics
```

---

# Revision Sheet

```
Application
      │
      ▼
glibc
      │
      ▼
System Call
      │
      ▼
Linux Kernel
 ├── Scheduler
 ├── Memory
 ├── VFS
 ├── IPC
 ├── Networking
 └── Drivers
      │
      ▼
Hardware
```

**Remember:**

- Linux = Resource Manager
- Kernel = Ring 0
- Applications = Ring 3
- System Calls = Entry to Kernel
- VFS = Filesystem Abstraction
- Drivers = Hardware Interface
- `printf()` → `write()`
- `malloc()` → `brk()/mmap()`