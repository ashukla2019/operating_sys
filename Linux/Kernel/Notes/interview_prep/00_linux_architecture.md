# Linux Architecture

## What is Linux?

Linux is a monolithic kernel that manages CPU, memory, devices, filesystems, networking and processes.

Responsibilities:
- Process Scheduling
- Memory Management
- VFS
- Networking
- IPC
- Drivers

---

## Architecture

Application
    ↓
glibc
    ↓
System Call
    ↓
Kernel
    ├── Scheduler
    ├── Memory Manager
    ├── VFS
    ├── Networking
    └── Drivers
    ↓
Hardware

---

## User Space vs Kernel Space

User Space
- Applications
- No direct hardware access
- Runs in Ring 3

Kernel Space
- Kernel
- Full hardware access
- Runs in Ring 0

Interview:
Q. Why separate them?
A. Security, stability, memory protection.

---

## System Call

Purpose:
Bridge between application and kernel.

Example:

printf()
    ↓
write()
    ↓
syscall
    ↓
Kernel

Examples:
- open()
- read()
- write()
- mmap()
- fork()
- clone()

Interview:
Difference between library call and system call?

---

## Major Kernel Subsystems

Scheduler
Memory Manager
VFS
Networking
Drivers
IPC

(Each covered in later chapters.)