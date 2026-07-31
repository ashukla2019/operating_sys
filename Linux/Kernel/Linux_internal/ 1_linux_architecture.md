# Chapter 1 – Linux Architecture

## Objectives
After completing this chapter, you should understand:
- Overall Linux architecture
- User Space vs Kernel Space
- What happens when an application runs
- What is the Linux Kernel
- Why system calls are needed
- Kernel modules
- Monolithic vs Microkernel
- Linux boot process (high level)
- Complete execution flow from application to hardware

---

## What is Linux?
Linux is an operating system kernel created by Linus Torvalds. A complete Linux operating system consists of:
- Linux Kernel
- GNU utilities
- Libraries (glibc, musl, etc.)
- Shell (bash, zsh)
- System services (systemd)
- Applications

Example:
```
Ubuntu
├── Linux Kernel
├── GNU Tools
├── Bash
├── GCC
├── Libraries
└── Applications
```
The kernel is the core of the operating system. Everything eventually goes through the kernel.

---

## High Level Linux Architecture
```
+---------------------------------------+
| Applications                          |
| Chrome, GCC, Vim, Python              |
+---------------------------------------+
| System Libraries                      |
| glibc, libstdc++                      |
+---------------------------------------+
| System Call Interface                 |
+---------------------------------------+
| Linux Kernel                          |
| Process Memory FileSystem Network     |
| Scheduler Drivers IPC Security        |
+---------------------------------------+
| Device Drivers                        |
+---------------------------------------+
| Hardware                              |
| CPU RAM SSD NIC USB GPU               |
+---------------------------------------+
```

---

## Responsibilities of the Kernel
The kernel manages every important hardware resource. Main responsibilities include:
- Process management
- Thread scheduling
- Virtual memory
- Device drivers
- File systems
- Networking
- Security
- Inter-process communication
- Interrupt handling
- Power management

Think of the kernel as the manager of the entire computer. Applications cannot directly access hardware.

---

## User Space vs Kernel Space
Linux separates execution into two areas.

**User Space** — Applications execute here (Chrome, Firefox, Python, GCC, Vim, Games). Applications cannot:
- Access physical memory
- Access hardware directly
- Execute privileged CPU instructions

This protects the operating system.

**Kernel Space** — Kernel code executes here. The kernel has complete access to CPU, RAM, Storage, Network card, USB, Interrupt controller, and MMU. Only trusted kernel code executes here.

---

## Memory Layout
```
CPU
  │
+---------------+
| User Space    |
| Applications  |
+---------------+
  System Calls
  │
+---------------+
| Kernel Space  |
| Linux Kernel  |
+---------------+
  │
Hardware Devices
```

---

## Why Separate User and Kernel Space?
Imagine a buggy application writing random values into RAM.

Without protection: kernel memory gets corrupted, file system gets corrupted, entire OS crashes.
With separation: the application crashes, but the kernel remains safe.

This isolation is one of Linux's biggest strengths.

---

## CPU Modes
Modern CPUs have privilege levels. Simplified: `User Mode → Kernel Mode → Hardware`

**User Mode** — Restricted; cannot execute privileged instructions.
**Kernel Mode** — Full privileges; can access hardware directly.

The CPU switches between these modes during system calls and interrupts.

---

## What is a System Call?
Applications cannot directly perform privileged operations — instead they request the kernel. This request is called a **System Call**.

Example: `printf() → write() → System Call → Kernel → Terminal`

Examples of system calls: `open()`, `read()`, `write()`, `close()`, `fork()`, `execve()`, `socket()`, `connect()`, `mmap()`

---

## Example
```c
#include <unistd.h>

int main()
{
    write(1, "Hello\n", 6);
}
```
Flow: `Application → glibc → write() → System Call → Kernel → Terminal Driver → Screen`

The application never writes directly to the display hardware.

---

## Why Use Libraries?
Instead of invoking system calls manually, applications use libraries.

Example: `printf() → glibc → write() → Kernel`

Benefits: easier programming, portable API, optimized implementations.

---

## Kernel Components
The Linux kernel consists of many subsystems:
```
Linux Kernel
├── Scheduler
├── Memory Manager
├── VFS
├── Device Drivers
├── Networking
├── IPC
├── Security
├── Power Management
├── Block Layer
└── Architecture-specific Code
```
Each subsystem performs a specialized task.

---

## Monolithic Kernel
Linux uses a **Monolithic Kernel** architecture — all major services run inside kernel space.
```
Kernel
├── Scheduler
├── Drivers
├── Memory
├── File Systems
├── Networking
└── IPC
```
**Advantages:** very fast, direct function calls, high performance, low overhead.
**Disadvantages:** a buggy driver can crash the kernel; large code base.

---

## Microkernel
A Microkernel keeps only minimal functionality inside the kernel; everything else runs in user space.
```
Kernel
├── IPC
├── Scheduling
└── Memory

Drivers → User Space → Servers
```
**Advantages:** better isolation, better reliability, easier debugging.
**Disadvantages:** more IPC, slower than monolithic kernels.

---

## Monolithic vs Microkernel
| Feature | Monolithic | Microkernel |
|----------|------------|-------------|
| Performance | High | Lower |
| Drivers | Kernel Space | User Space |
| IPC | Less | More |
| Reliability | Lower | Higher |
| Context Switches | Fewer | More |

Linux chooses performance over maximum isolation.

---

## Loadable Kernel Modules (LKM)
Linux supports loading drivers without rebooting.

Example: `USB Driver → Load Module → Kernel Starts Using Driver`

Commands: `lsmod`, `insmod`, `rmmod`, `modprobe`

Advantages: no reboot, smaller kernel image, easier driver updates.

**Kernel Module Flow:** `Driver.ko → insmod → Kernel → Driver Initialized → Device Ready`

---

## Linux Boot Process (High Level)
```
Power ON → BIOS/UEFI → Bootloader (GRUB) → Linux Kernel → Initramfs
→ systemd (PID 1) → Services → Login → Applications
```
We will study the boot process in detail in a later chapter.

---

## Complete Execution Flow
Suppose you type: `cat notes.txt`

Flow: `cat → glibc → open() → Kernel → VFS → ext4 → Block Layer → Storage Driver → SSD → Data Returned → cat prints file`

Every file access passes through the kernel.

**Another example** — Typing: `ping google.com`

Flow: `ping → socket() → Kernel Network Stack → NIC Driver → Network Card → Internet → Reply → Kernel → Application`

Applications never communicate with hardware directly.

---

## Key Interview Questions

**Why do we need User Space and Kernel Space?**
To protect the operating system and hardware from faulty or malicious applications while allowing controlled access through system calls.

**Why can't applications access hardware directly?**
Direct hardware access could corrupt memory, bypass security, and crash the system. The kernel safely manages all hardware resources.

**What is the Linux Kernel?**
The kernel is the core of the operating system. It manages CPU scheduling, memory, filesystems, networking, device drivers, and communication with hardware.

**What is a system call?**
A controlled interface through which user-space applications request services from the kernel, such as file I/O, process creation, or networking.

**Why does Linux use a monolithic kernel?**
Because direct function calls between kernel subsystems provide higher performance with lower overhead compared to message-passing architectures.

**What is a kernel module?**
A piece of kernel code that can be loaded or unloaded at runtime to add functionality (such as a device driver) without rebuilding or rebooting the kernel.

---

## Summary
In this chapter, we learned:
- Linux architecture
- User Space vs Kernel Space
- CPU privilege levels
- System calls
- Kernel responsibilities
- Linux kernel subsystems
- Monolithic vs Microkernel
- Loadable Kernel Modules
- High-level Linux boot process
- End-to-end execution flow from application to hardware

The next chapter dives into **Process Internals**, where we'll explore `task_struct`, process creation (`fork()`), `exec()`, scheduling, context switching, and process lifecycle in detail.
