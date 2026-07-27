# Linux Architecture
**File:** `00_Linux_Architecture.md`

> **Target Audience:** Senior Embedded Linux C/C++ Engineers
>
> **Interview Importance:** ⭐⭐⭐⭐⭐
>
> **Prerequisites:** Basic C Programming
>
> **Estimated Reading Time:** 2-3 Hours

---

# Chapter Objectives

After completing this chapter, you should be able to answer:

- Why do we need an Operating System?
- What is Linux?
- Why is Linux divided into User Space and Kernel Space?
- What is the Linux Kernel?
- What are the major components of Linux?
- How does an application communicate with hardware?
- What happens when a program executes?
- How do all Linux subsystems fit together?

This chapter serves as the **foundation** for every remaining chapter in this handbook.

---

# Learning Roadmap

```
Computer
     │
     ▼
Hardware
     │
     ▼
Operating System
     │
     ▼
Linux Kernel
     │
     ▼
Kernel Subsystems
     │
     ▼
Applications
```

Everything else (Processes, Memory, VFS, IPC, Networking, Drivers) is simply one part of this picture.

---

# 1. Why Do We Need an Operating System?

Imagine there is **no operating system**.

Your application wants to print:

```c
printf("Hello");
```

Without an OS, your application would need to:

- Configure CPU
- Configure RAM
- Initialize display controller
- Locate video memory
- Write pixels manually
- Handle interrupts
- Manage keyboard
- Handle scheduling
- Access storage directly
- Manage network hardware

Every application would have to do this.

That would be nearly impossible.

Instead, Linux does all this work.

Applications simply request services.

```
Application
      │
      ▼
 Linux Kernel
      │
      ▼
 Hardware
```

The Operating System hides hardware complexity.

---

# Real Life Analogy

Imagine a restaurant.

You don't walk into the kitchen and cook your own food.

Instead:

```
Customer
    │
    ▼
 Waiter
    │
    ▼
 Kitchen
```

Linux acts like the waiter.

Applications request services.

Linux communicates with hardware.

---

# Responsibilities of an Operating System

An Operating System is responsible for managing every hardware resource.

Major responsibilities include:

```
CPU Management

Memory Management

Storage Management

File Systems

Network

Security

Scheduling

Device Drivers

Interrupt Handling

Process Management

Thread Management

IPC
```

Every chapter in this handbook explains one of these responsibilities.

---

# Example

Suppose Chrome needs to save a downloaded file.

Chrome never talks directly to the SSD.

Instead:

```
Chrome
    │
open()
write()
close()
    │
    ▼
Linux Kernel
    │
    ▼
Filesystem
    │
    ▼
SSD Driver
    │
    ▼
SSD Hardware
```

Applications **never own hardware**.

Linux owns hardware.

---

# Interview Question

**Q:** Why do we need an Operating System?

Expected Answer:

- Hardware abstraction
- Resource management
- Security
- Scheduling
- Memory management
- Device management
- File systems
- Networking

---

# 2. What is Linux?

Many beginners say:

> Linux is an Operating System.

This is not completely correct.

Linux is actually **the Kernel**.

The complete operating system consists of:

```
Applications

Libraries

Shell

Utilities

Compiler

Linux Kernel
```

Example:

```
Ubuntu

Debian

Fedora

RHEL
```

All of these use the Linux Kernel.

---

# Linux Distribution

A Linux Distribution consists of:

```
+----------------------------------+
| Applications                     |
+----------------------------------+

+----------------------------------+
| Shell (bash, zsh)                |
+----------------------------------+

+----------------------------------+
| Libraries (glibc)                |
+----------------------------------+

+----------------------------------+
| Linux Kernel                     |
+----------------------------------+

+----------------------------------+
| Hardware                         |
+----------------------------------+
```

The Kernel is only one component.

---

# What Does Linux Kernel Actually Do?

The kernel manages everything.

```
CPU

Memory

Disk

USB

Network

Processes

Threads

Filesystem

Security

Drivers
```

Whenever an application needs something,

it asks the kernel.

---

# Kernel Responsibilities

The kernel performs:

- Process Scheduling
- Memory Allocation
- Virtual Memory
- Interrupt Handling
- Device Driver Management
- Networking
- IPC
- Filesystem Management
- Security
- Power Management

Without the kernel, applications cannot safely use hardware.

---

# 3. Linux Design Philosophy

Linux follows a few simple but powerful principles.

---

## 1. Everything is a File

Linux represents many resources as files.

Examples:

```
Regular File

Directory

Socket

Pipe

Device

Terminal
```

Examples:

```
/dev/sda

/dev/ttyUSB0

/dev/null

/proc/cpuinfo

/sys
```

This provides a consistent interface.

Instead of learning different APIs,

everything is accessed using familiar file operations.

```
open()

read()

write()

close()
```

This philosophy greatly simplifies software development.

---

## 2. Small Programs

Linux encourages writing programs that do one job well.

Example:

```
grep

sort

wc

awk

sed
```

Instead of writing one huge application,

Linux combines many small programs.

Example:

```
cat log.txt

↓

grep ERROR

↓

sort

↓

uniq

↓

wc
```

This modular design is one reason Linux is so powerful.

---

## 3. Everything Can Be Combined

Programs communicate using pipes.

```
Program A

↓

Pipe

↓

Program B

↓

Pipe

↓

Program C
```

We'll study Pipes in the IPC chapter.

---

# Why These Principles Matter

These design choices make Linux:

- Modular
- Stable
- Maintainable
- Easy to automate
- Easy to debug

---

# Interview Question

Why is "Everything is a File" considered one of Linux's biggest strengths?

Expected discussion:

- Unified interface
- Simpler APIs
- Reusable tools
- Consistent programming model

---

# 4. High-Level Linux Architecture

The complete Linux software stack looks like this.

```
+-----------------------------------------------------+
|                 User Applications                   |
| Chrome | GCC | Bash | Python | Your Program         |
+-----------------------------------------------------+

+-----------------------------------------------------+
|             Standard Libraries (glibc)              |
+-----------------------------------------------------+

================ System Call Interface ================

+-----------------------------------------------------+
|                  Linux Kernel                       |
|                                                     |
| Process Scheduler                                   |
| Memory Manager                                      |
| Virtual File System                                 |
| Networking Stack                                    |
| IPC                                                 |
| Device Drivers                                      |
| Security                                            |
+-----------------------------------------------------+

================ Hardware Interface ===================

+-----------------------------------------------------+
| CPU | RAM | SSD | NIC | GPU | USB | UART | SPI      |
+-----------------------------------------------------+
```

This is **the single most important diagram** in the entire handbook.

Every future chapter simply zooms into one of these boxes.

---

# Example Flow

Suppose your program executes:

```c
printf("Hello World");
```

The simplified execution path is:

```
Application

↓

printf()

↓

glibc

↓

write()

↓

System Call

↓

Linux Kernel

↓

Terminal Driver

↓

Display Hardware

↓

Monitor
```

Notice that the application never directly communicates with the display.

Everything goes through the kernel.

---

# 5. Hardware Layer

Linux ultimately controls physical hardware.

The major hardware components are:

```
CPU

RAM

SSD / HDD

GPU

Network Card

USB Controller

UART

SPI

I2C

Timers

Interrupt Controller
```

Every one of these devices is controlled through a **device driver**.

Applications do **not** communicate directly with hardware registers.

Instead:

```
Application

↓

System Call

↓

Kernel

↓

Device Driver

↓

Hardware Register

↓

Physical Device
```

This separation allows applications to remain portable across different hardware platforms.

---

# Key Takeaways (Part 1)

- Linux is a **resource manager** for CPU, memory, storage, devices, and networking.
- The Linux **kernel** is only one part of a complete Linux distribution.
- Applications interact with hardware through the kernel—not directly.
- Linux follows design principles such as **"Everything is a File"** and building small, composable programs.
- The high-level architecture consists of:
  - User Applications
  - Standard Libraries
  - System Call Interface
  - Linux Kernel
  - Hardware
- Understanding this architecture is essential before studying processes, memory management, VFS, IPC, networking, or debugging.

---

# Linux Architecture (Part 2)

---

# 6. User Space vs Kernel Space

One of the most frequently asked interview topics.

Understanding this concept makes Process Management, Memory Management, System Calls, VFS and Debugging much easier.

---

# Why Separate User Space and Kernel Space?

Imagine every application had complete access to hardware.

Suppose a calculator application contains a bug.

Without protection it could:

- Format your SSD
- Corrupt RAM
- Disable interrupts
- Crash the CPU
- Read passwords from other applications

This would make the system completely unstable.

To prevent this, modern operating systems separate execution into two worlds.

```
+------------------------------+
|        User Space            |
|------------------------------|
| Chrome                       |
| Firefox                      |
| Bash                         |
| Python                       |
| Your Program                 |
+------------------------------+

          System Calls

+------------------------------+
|       Kernel Space           |
|------------------------------|
| Scheduler                    |
| Memory Manager               |
| VFS                          |
| Networking                   |
| Device Drivers               |
+------------------------------+
```

Applications execute in User Space.

The operating system executes in Kernel Space.

---

# User Space

User Space is where applications execute.

Examples:

```
Chrome

VS Code

Firefox

Bash

Python

Your C Program

MySQL

Redis
```

Applications execute with limited privileges.

They **cannot directly**:

- Read physical memory
- Access disk sectors
- Configure page tables
- Disable interrupts
- Execute privileged CPU instructions
- Access hardware registers
- Schedule other processes

Instead, they request services from the kernel.

---

# Characteristics of User Space

```
Limited Privileges

Protected Memory

Application Crash
     │
     ▼
Usually only that process dies

No direct hardware access

Cannot access kernel memory

Cannot access another process's memory
```

This isolation improves:

- Stability
- Security
- Reliability

---

# Kernel Space

Kernel Space is where Linux itself executes.

Kernel code has unrestricted access to the machine.

It can access:

```
CPU Registers

Physical Memory

DMA

Interrupt Controller

Disk Controller

Network Card

GPU

USB Controller

Timers

Page Tables
```

The kernel decides how hardware should be shared.

---

# Responsibilities of Kernel Space

Kernel performs:

```
Scheduling

Memory Management

Interrupt Handling

Filesystem

Networking

IPC

Drivers

Security

Power Management
```

Every request from applications eventually reaches one of these kernel subsystems.

---

# Memory Isolation

Each process has its own virtual address space.

Example:

```
Process A

Virtual Address

0x1000

↓

Physical Page A


---------------------------

Process B

Virtual Address

0x1000

↓

Physical Page B
```

Notice:

Both processes use the same virtual address.

But they map to different physical memory.

This is why one process cannot corrupt another process.

We'll study this in detail in the Memory Management chapter.

---

# What Happens if an Application Tries Illegal Access?

Example:

```c
int *p = (int *)0xFFFFFFFFFFFF;

*p = 100;
```

CPU detects illegal access.

```
Application

↓

Invalid Address

↓

MMU

↓

Page Fault

↓

Kernel

↓

SIGSEGV

↓

Application Terminates
```

Result:

```
Segmentation Fault
```

Linux protects the system.

---

# Interview Question

Why does a Segmentation Fault occur?

Expected Answer:

The process attempted to access memory that:

- Doesn't exist
- Doesn't belong to it
- Doesn't have required permissions

The CPU raises an exception.

The kernel converts it into SIGSEGV.

---

# Benefits of User/Kernel Separation

```
Security

Stability

Memory Protection

Fault Isolation

Controlled Hardware Access

Multi-user Support
```

Without this separation Linux would not be secure.

---

# 7. CPU Privilege Levels

Modern CPUs provide different privilege levels.

These are commonly called **Rings**.

```
Ring 0

Highest Privilege

Kernel


Ring 3

Lowest Privilege

Applications
```

Linux primarily uses:

```
Ring 0 → Kernel

Ring 3 → User Applications
```

---

# Why Rings?

Suppose an application executes:

```
Disable Interrupts

Modify Page Table

Write Disk Controller

Configure MMU
```

These instructions are privileged.

If executed from Ring 3:

CPU immediately generates an exception.

Kernel decides what to do.

---

# Privileged Instructions

Examples:

```
Change Page Table

Disable Interrupt

Configure MMU

Access Control Registers

Load Kernel Registers
```

Applications cannot execute these.

Only the kernel can.

---

# User Mode vs Kernel Mode

```
User Mode

↓

Restricted

↓

System Call

↓

Kernel Mode

↓

Hardware Access

↓

Return

↓

User Mode
```

Notice:

Applications enter kernel mode only temporarily.

After the request is complete,

control returns to user mode.

---

# 8. System Call Interface

This is one of the most important Linux concepts.

Applications cannot directly call kernel functions.

Instead they use **System Calls**.

```
Application

↓

System Call

↓

Linux Kernel

↓

Hardware
```

Think of a system call as the official entrance to the kernel.

---

# Examples of System Calls

```
open()

close()

read()

write()

fork()

execve()

wait()

socket()

connect()

mmap()

ioctl()
```

Nearly every Linux application uses hundreds of system calls.

---

# Example

```c
fd = open("data.txt", O_RDONLY);
```

Execution Flow:

```
Application

↓

glibc

↓

open()

↓

System Call

↓

Kernel

↓

VFS

↓

Filesystem Driver

↓

SSD

↓

Return File Descriptor
```

The application never communicates directly with the SSD.

---

# System Call Life Cycle

```
Application

↓

Library Function

↓

CPU switches to Kernel Mode

↓

Kernel validates request

↓

Kernel executes operation

↓

Kernel returns result

↓

CPU returns User Mode

↓

Application resumes
```

This transition happens millions of times every second on a busy system.

---

# Why Not Call Kernel Functions Directly?

Reasons:

```
Security

Permission Checking

Memory Validation

Scheduling

Resource Accounting

Hardware Independence
```

System calls provide a safe interface.

---

# 9. Standard Libraries (glibc)

Many programmers think:

```
printf()

↓

Kernel
```

This is incorrect.

The actual path is:

```
Application

↓

glibc

↓

System Call

↓

Kernel
```

glibc acts as a wrapper.

---

# Example

Program:

```c
printf("Hello");
```

Simplified flow:

```
printf()

↓

Format String

↓

Buffer

↓

write()

↓

System Call

↓

Kernel

↓

Terminal Driver

↓

Display
```

Notice:

`printf()` itself is **not** a system call.

Eventually it calls `write()`.

---

# Why Use glibc?

Advantages:

```
Portable

Standard API

Optimized

Buffering

Compatibility

Error Handling
```

Applications rarely invoke raw system calls directly.

---

# Common Interview Questions

### Q1. Why is User Space separated from Kernel Space?

### Q2. What is Kernel Mode?

### Q3. What is User Mode?

### Q4. Why can't applications access hardware directly?

### Q5. What is a System Call?

### Q6. Is printf() a system call?

### Q7. What happens during a system call?

### Q8. What causes a Segmentation Fault?

### Q9. What is glibc?

### Q10. Why do we need standard libraries?

---

# Key Takeaways

- Applications execute in **User Space** with limited privileges.
- The Linux kernel executes in **Kernel Space** with full hardware access.
- CPU privilege levels prevent user applications from executing privileged instructions.
- **System calls** are the only supported interface between applications and the kernel.
- Standard libraries such as **glibc** provide convenient APIs and internally invoke system calls.
- Memory protection and privilege separation are fundamental to Linux security and stability.

---

# What's Next? (Part 3)

In Part 3, we'll study the **major Linux kernel subsystems**, including:

- Process Management
- Memory Management
- Virtual File System (VFS)
- Networking Stack
- Device Drivers
- Interrupts and Exceptions
- Boot Flow Overview

By the end of Part 3, you'll understand how all of these components work together inside the Linux kernel.

---------
# Linux Architecture (Part 3)

---

# 10. Linux Kernel Overview

The Linux kernel is the **core component** of the operating system.

It sits between **applications** and **hardware**.

```
                User Space
+---------------------------------------+
| Chrome  Bash  GCC  Python  MySQL      |
+---------------------------------------+

            System Calls

+---------------------------------------+
|            Linux Kernel               |
+---------------------------------------+

               Hardware

+---------------------------------------+
| CPU RAM SSD NIC GPU USB UART SPI      |
+---------------------------------------+
```

The kernel is responsible for:

- CPU Scheduling
- Memory Management
- Filesystem Management
- Networking
- Device Drivers
- Interrupt Handling
- Security
- IPC
- Power Management

Without the kernel, applications cannot safely use hardware.

---

# Is Linux Kernel a Single Program?

Yes.

After booting, the kernel is loaded into memory and remains resident.

Unlike applications,

it is **never swapped out**.

The kernel is a privileged program that serves every process in the system.

---

# Why Is It Called a Kernel?

Think of Linux like a city.

```
Applications

↓

Need Resources

↓

Kernel

↓

Allocates Resources

↓

Hardware
```

Every request passes through the kernel.

It is literally the **core** of the operating system.

---

# Major Linux Kernel Subsystems

The Linux kernel is divided into several major subsystems.

```
Linux Kernel

│

├── Process Management

├── Memory Management

├── Virtual File System

├── Device Drivers

├── Networking Stack

├── IPC

├── Security

└── Architecture Specific Code
```

Each subsystem has a well-defined responsibility.

---

# 11. Process Management

One of Linux's primary responsibilities is executing programs.

Example:

```
Terminal

↓

./my_app

↓

Kernel Creates Process

↓

Scheduler Executes It

↓

Program Runs
```

Every running program becomes a **Process**.

---

## Responsibilities

Process Management handles:

- Process creation
- Process termination
- Scheduling
- Context switching
- Signals
- Parent-child relationships
- Process states

---

## Process Lifecycle

```
Program

↓

fork()

↓

New Process

↓

exec()

↓

Running

↓

Exit

↓

Zombie

↓

wait()

↓

Destroyed
```

We'll study every step in detail in **01_Process_Management.md**.

---

# Process States

Typical process states:

```
Running

Ready

Sleeping

Stopped

Zombie
```

Linux constantly moves processes between these states.

---

# Real Example

Suppose Chrome, VS Code and Spotify are running.

```
Chrome

VS Code

Spotify

Terminal
```

The scheduler rapidly switches the CPU among them.

To the user, all appear to run simultaneously.

---

# Interview Question

**Why can a single-core CPU appear to run multiple applications simultaneously?**

Expected Answer:

The scheduler performs **context switching** so quickly that users perceive concurrent execution.

---

# 12. Memory Management

Memory is one of the kernel's most important responsibilities.

Applications should never worry about:

- Physical RAM
- Memory allocation
- Virtual addresses
- Page tables

The kernel handles everything.

---

## Responsibilities

```
Virtual Memory

Paging

Memory Allocation

Page Cache

Copy-on-Write

Memory Mapping

Swap

OOM Killer
```

---

# Virtual Memory

Every process believes it owns the entire memory.

Example:

```
Process A

0x400000

↓

Physical Page 10


----------------------


Process B

0x400000

↓

Physical Page 82
```

Same virtual address.

Different physical pages.

This is possible because of virtual memory.

---

# Why Virtual Memory?

Benefits:

- Process isolation
- Security
- Simpler programming
- Efficient memory usage
- Shared libraries
- Memory mapping

---

# Example

When you call:

```c
malloc(1024);
```

Your program does **not** receive physical RAM directly.

Instead:

```
malloc()

↓

glibc

↓

brk()/mmap()

↓

Kernel

↓

Virtual Address Returned
```

Actual physical memory may be allocated later when first accessed (demand paging).

---

# Interview Question

Does `malloc()` directly allocate physical memory?

Answer:

No.

It allocates virtual memory.

Physical pages are typically mapped by the kernel when they are first accessed.

---

# 13. Virtual File System (VFS)

Linux supports many filesystems.

Examples:

```
ext4

XFS

NFS

FAT32

tmpfs

procfs

sysfs
```

Without VFS,

every application would need filesystem-specific code.

---

# Problem Without VFS

Imagine opening files like this:

```
open_ext4()

open_xfs()

open_nfs()

open_fat()
```

Applications would become extremely complicated.

---

# Linux Solution

Linux introduces the **Virtual File System**.

```
Application

↓

open()

↓

VFS

↓

Filesystem Driver

↓

Disk
```

Applications always call:

```
open()

read()

write()

close()
```

VFS translates these requests to the correct filesystem implementation.

---

# Advantages

```
Filesystem Independence

Code Reuse

Cleaner Design

Easy Addition of New Filesystems
```

---

# Example

Whether the file is stored on:

```
SSD

USB

NFS Server

RAM Disk
```

Applications use the same APIs.

---

# 14. Networking Stack

Linux contains a complete networking implementation.

Major protocols:

```
Ethernet

ARP

IPv4

IPv6

ICMP

TCP

UDP
```

Applications use sockets.

---

# Networking Flow

```
Application

↓

Socket API

↓

TCP/UDP

↓

IP

↓

Ethernet

↓

NIC Driver

↓

Network Card
```

Every packet follows this path.

---

# Responsibilities

Networking subsystem handles:

- Packet transmission
- Packet reception
- Routing
- TCP
- UDP
- Congestion control
- Buffer management
- Checksums

---

# Example

Browser requests:

```
https://example.com
```

Simplified path:

```
Browser

↓

Socket

↓

TCP

↓

IP

↓

Ethernet

↓

NIC Driver

↓

Network Card
```

---

# 15. Device Drivers

Every hardware device requires software to control it.

That software is called a **Device Driver**.

---

# Why Drivers?

Every hardware device is different.

A USB controller is completely different from:

- GPU
- SSD
- UART
- SPI
- Network Card

Applications should not need to understand hardware details.

Drivers hide hardware-specific implementation.

---

# Driver Flow

```
Application

↓

read()

↓

Kernel

↓

Driver

↓

Hardware Register

↓

Physical Device
```

---

# Examples

```
USB Driver

Ethernet Driver

NVMe Driver

GPU Driver

UART Driver

SPI Driver

I2C Driver
```

---

# Character vs Block Devices

Linux broadly classifies devices as:

### Character Devices

Examples:

```
Keyboard

Mouse

UART

Serial Port
```

Characteristics:

- Sequential data
- Byte-oriented
- Usually not seekable

---

### Block Devices

Examples:

```
SSD

HDD

NVMe

USB Storage
```

Characteristics:

- Fixed-size blocks
- Random access
- Filesystems reside on them

---

# Interview Question

What is the difference between a character device and a block device?

Expected Answer:

Character devices transfer data as a stream of bytes, while block devices transfer fixed-size blocks and support random access.

---

# Relationship Between Kernel Subsystems

Everything in Linux is interconnected.

```
Application

↓

System Call

↓

Kernel

├── Process Management
├── Memory Management
├── VFS
├── Networking
├── IPC
└── Device Drivers

↓

Hardware
```

No subsystem works in isolation.

Example:

```
read()

↓

VFS

↓

Filesystem

↓

Page Cache

↓

Memory Manager

↓

Device Driver

↓

Disk
```

One simple API may involve multiple kernel subsystems.

---

# Key Takeaways

- The Linux kernel is the central component of the operating system.
- Major kernel subsystems include:
  - Process Management
  - Memory Management
  - Virtual File System (VFS)
  - Networking
  - Device Drivers
  - IPC
- Processes are scheduled by the kernel.
- Virtual memory gives each process its own address space.
- VFS provides a common interface for different filesystems.
- Device drivers abstract hardware details.
- Most system calls interact with multiple kernel subsystems before completing.

---

# What's Next? (Part 4)

In the final part of this chapter, we'll cover:

- Interrupts
- Exceptions
- Boot Process Overview
- Complete end-to-end execution flow (`printf()`, `open()`, `read()`)
- How all kernel subsystems work together
- Senior interview questions
- Chapter summary