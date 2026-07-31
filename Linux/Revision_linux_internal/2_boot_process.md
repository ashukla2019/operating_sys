# Part 2 – Linux Boot Process (Interview Revision)

## Goal

Understand how Linux starts from pressing the power button until `main()` of a user application executes.

---

# Complete Boot Flow

```text
Power ON
    │
    ▼
BIOS / UEFI
    │
    ▼
Bootloader (GRUB)
    │
    ▼
Linux Kernel
    │
    ▼
Kernel Initialization
    │
    ▼
initramfs
    │
    ▼
Mount Root Filesystem
    │
    ▼
PID 1 (systemd/init)
    │
    ▼
System Services
    │
    ▼
Login
    │
    ▼
Shell
    │
    ▼
Application
```

---

# Step 1 — Power ON

Power button pressed.

CPU immediately begins execution from a predefined reset address.

CPU does **not** know about Linux.

It simply starts executing firmware.

```text
Power
   │
   ▼
CPU Reset
   │
   ▼
Firmware
```

---

# Step 2 — BIOS / UEFI

Firmware initializes hardware.

Responsibilities

```text
Initialize CPU

Initialize RAM

Detect Keyboard

Detect Disk

Initialize PCI Devices

Initialize USB

Hardware Self Test (POST)
```

Finally,

```text
Find boot device

Load bootloader
```

Interview Question

> BIOS vs UEFI?

```text
BIOS
Older firmware

UEFI
Modern firmware
Faster
Supports GPT
Secure Boot
```

---

# Step 3 — Bootloader (GRUB)

GRUB loads Linux into memory.

Responsibilities

```text
Load Linux Kernel

Load initramfs

Pass Kernel Parameters

Transfer control to Kernel
```

Flow

```text
Disk

↓

GRUB

↓

Kernel Image

↓

initramfs

↓

Kernel Starts
```

Common kernel parameters

```text
root=

ro

rw

quiet

init=
```

---

# Step 4 — Linux Kernel Starts

Kernel is decompressed.

Then initialization begins.

Major tasks

```text
Initialize CPU

Initialize Scheduler

Initialize Memory

Initialize Interrupts

Initialize Drivers

Initialize VFS

Initialize Networking

Initialize Filesystems
```

At this stage,

```text
Kernel is alive

User space not started yet
```

---

# Step 5 — initramfs

Temporary root filesystem stored in RAM.

Purpose

```text
Load required drivers

Detect storage

Locate real root filesystem

Mount root filesystem
```

Flow

```text
Kernel

↓

initramfs

↓

Load Drivers

↓

Find Root Disk

↓

Switch Root
```

Without initramfs,

many systems cannot boot because storage drivers may not yet be available.

---

# Step 6 — Mount Root Filesystem

Kernel mounts the actual filesystem.

Examples

```text
ext4

xfs

btrfs

NFS
```

After mounting,

```text
/

becomes available.
```

---

# Step 7 — Start PID 1

Kernel executes

```text
/sbin/init
```

Today this is usually

```text
systemd
```

PID

```text
1
```

Responsibilities

```text
Start Services

Mount Remaining Filesystems

Configure Network

Start Login Manager

Launch User Space
```

Interview Question

> Why is PID 1 special?

Answer

```text
First user-space process.

Ancestor of almost every process.

Responsible for system startup.
```

---

# Step 8 — Start Services

Examples

```text
udev

NetworkManager

sshd

cron

dbus

systemd-journald
```

Services initialize the operating system.

---

# Step 9 — Login

User logs in using

```text
Console

SSH

GUI
```

Authentication occurs.

Shell starts.

Examples

```text
bash

zsh

sh
```

---

# Step 10 — Application Starts

Example

```bash
./app
```

Flow

```text
Shell

↓

fork()

↓

execve()

↓

Program Loaded

↓

main()
```

---

# Process Creation Flow

```text
Shell

↓

fork()

↓

Child Process

↓

execve()

↓

ELF Loader

↓

main()
```

Important system calls

```text
fork()

execve()

wait()

exit()
```

---

# Memory After Boot

```text
+------------------------+
| User Applications      |
+------------------------+
| Shared Libraries       |
+------------------------+
| Heap                   |
+------------------------+
| Stack                  |
+------------------------+

System Call Boundary

+------------------------+
| Linux Kernel           |
+------------------------+
```

---

# Boot Components Summary

| Component | Responsibility |
|------------|----------------|
| BIOS / UEFI | Initialize Hardware |
| GRUB | Load Kernel |
| Kernel | Initialize OS |
| initramfs | Load Drivers |
| Root Filesystem | Permanent Storage |
| systemd | Start User Space |
| Shell | Execute Commands |
| Application | User Program |

---

# Complete Boot Dependency Chain

```text
Power
   ↓
CPU Reset
   ↓
BIOS / UEFI
   ↓
GRUB
   ↓
Kernel
   ↓
Memory Initialization
   ↓
Scheduler
   ↓
Drivers
   ↓
initramfs
   ↓
Root Filesystem
   ↓
systemd (PID 1)
   ↓
Services
   ↓
Login
   ↓
Shell
   ↓
fork()
   ↓
execve()
   ↓
Application
```

---

# Interview Questions

Know these answers immediately.

```text
✓ Explain Linux boot process.

✓ BIOS vs UEFI?

✓ What is GRUB?

✓ What does bootloader load?

✓ What is initramfs?

✓ Why do we need initramfs?

✓ What is root filesystem?

✓ Why is PID 1 special?

✓ What does systemd do?

✓ What happens after execve()?

✓ How does main() finally execute?
```

---

# One-Minute Revision

```text
Power
   ↓
BIOS / UEFI
   ↓
GRUB
   ↓
Linux Kernel
   ↓
initramfs
   ↓
Root Filesystem
   ↓
systemd (PID 1)
   ↓
Services
   ↓
Login
   ↓
Shell
   ↓
fork()
   ↓
execve()
   ↓
main()
```

> **Remember:** Every Linux system follows this sequence before your application starts executing.
