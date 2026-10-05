# Chapter 13 – Linux Boot Process

---

# 1. What Happens When a Linux Machine Boots?

The Linux boot process is:

```text
Power On
   ↓
Firmware (BIOS / UEFI)
   ↓
Bootloader
   ↓
Linux Kernel
   ↓
Kernel Initialization
   ↓
init / systemd
   ↓
Services
   ↓
Login / User Space
```

For senior Linux interviews, you should understand what happens at each stage.

---

# 2. Complete Boot Flow

```text
                         POWER ON
                            |
                            v
                    +---------------+
                    | BIOS / UEFI   |
                    +---------------+
                            |
                            v
                    Hardware Init
                            |
                            v
                    Find Boot Device
                            |
                            v
                    +---------------+
                    | Bootloader    |
                    | GRUB          |
                    +---------------+
                            |
                            v
                    Load Kernel
                            |
                            v
                    Load initramfs
                            |
                            v
                    Kernel Entry
                            |
                            v
                Early Kernel Initialization
                            |
                            v
                 CPU / Memory / Drivers
                            |
                            v
                   Mount Root FS
                            |
                            v
                       init/systemd
                            |
                            v
                      User Space
                            |
                            v
                    Login / Services
```

---

# 3. Stage 1 – Power On

When the system is powered on:

```text
CPU
 ↓
Reset state
 ↓
Firmware execution
```

The CPU begins execution from a predefined reset location according to the platform architecture.

Firmware then performs early platform initialization.

---

# 4. BIOS vs UEFI

Two major firmware environments:

```text
BIOS
UEFI
```

Modern systems predominantly use UEFI.

---

# 5. BIOS

Traditional BIOS performs tasks such as:

```text
CPU initialization
Memory initialization
Device detection
Basic hardware setup
Boot device selection
```

It eventually loads bootloader code from the boot device.

Conceptually:

```text
BIOS
  |
  v
Boot Device
  |
  v
Bootloader
```

---

# 6. UEFI

UEFI is the modern firmware interface.

Conceptually:

```text
UEFI
 |
 +-- Hardware initialization
 |
 +-- Boot manager
 |
 +-- EFI System Partition
 |
 +-- Bootloader
```

UEFI can directly understand filesystem structures required for booting and can load an EFI executable.

---

# 7. EFI System Partition

UEFI systems commonly contain an:

```text
EFI System Partition
```

Usually:

```text
ESP
 |
 +-- EFI/
      |
      +-- Boot/
      +-- vendor/
```

The partition contains boot-related EFI programs.

For Linux systems, a bootloader such as GRUB may reside there.

---

# 8. Boot Device Selection

Firmware determines which boot entry/device to use.

Conceptually:

```text
UEFI
 |
 +-- SSD
 +-- HDD
 +-- USB
 +-- Network
 |
 v
Selected Boot Entry
```

The firmware then loads the configured bootloader.

---

# 9. Bootloader

A common Linux bootloader is:

```text
GRUB
```

GRUB stands for:

```text
GRand Unified Bootloader
```

Its major responsibilities include:

```text
Display boot menu
Select kernel
Load kernel
Load initramfs
Pass kernel command line
Transfer control to kernel
```

---

# 10. GRUB Flow

Conceptually:

```text
UEFI
  |
  v
GRUB
  |
  +-- Select kernel
  |
  +-- Load kernel image
  |
  +-- Load initramfs
  |
  +-- Build kernel command line
  |
  v
Jump to kernel
```

---

# 11. Linux Kernel Image

The bootloader loads the Linux kernel image into memory.

Common kernel image:

```text
/boot/vmlinuz-<version>
```

The exact filename varies by distribution.

Conceptually:

```text
Disk
 |
 +-- Kernel image
 |
 +-- initramfs
 |
 +-- boot configuration
```

---

# 12. Kernel Command Line

The bootloader passes parameters to the kernel.

Example:

```text
root=/dev/sda2
ro
quiet
```

Other examples:

```text
init=/bin/sh
loglevel=3
console=ttyS0
```

Kernel parameters can affect:

```text
Root filesystem
Console
Logging
Debugging
Drivers
Memory behavior
CPU behavior
```

---

# 13. Why Is the Kernel Command Line Important?

Consider:

```text
root=/dev/sda2
```

The kernel needs to know which filesystem/device should eventually become the root filesystem.

Another example:

```text
console=ttyS0
```

can direct kernel console output to a serial console.

This is especially useful for embedded systems.

---

# 14. Embedded Linux Boot

Embedded systems often follow:

```text
Boot ROM
   ↓
Bootloader
   ↓
U-Boot
   ↓
Linux Kernel
   ↓
Device Tree
   ↓
initramfs / Root FS
   ↓
init
```

Instead of GRUB, embedded systems commonly use:

```text
U-Boot
```

---

# 15. U-Boot

U-Boot is widely used as a bootloader in embedded systems.

Typical responsibilities:

```text
Hardware initialization
Load kernel
Load device tree
Load initramfs
Set kernel command line
Boot Linux
```

Typical flow:

```text
SoC
 ↓
Boot ROM
 ↓
U-Boot
 ↓
Kernel
```

---

# 16. Boot ROM

Many embedded SoCs contain immutable Boot ROM code.

Conceptually:

```text
Power On
   |
   v
Boot ROM
   |
   v
Load bootloader
   |
   v
U-Boot
```

The Boot ROM may determine where to load the next boot stage from:

```text
eMMC
SD
SPI flash
USB
Network
```

depending on the hardware.

---

# 17. Device Tree

Embedded Linux systems commonly use a:

```text
Device Tree
```

The device tree describes hardware to the kernel.

Example concepts:

```text
CPU
Memory
UART
I2C
SPI
GPIO
Interrupt controller
Ethernet
PCIe
```

Conceptually:

```text
Device Tree
     |
     v
Kernel
     |
     v
Device Drivers
```

---

# 18. Why Device Tree?

The kernel should not need a hard-coded description of every board.

Instead:

```text
Same Kernel
     |
     +---- Board A Device Tree
     |
     +---- Board B Device Tree
```

The device tree provides board-specific hardware information.

---

# 19. Kernel Entry

After the bootloader has loaded the kernel and required boot data:

```text
Bootloader
    |
    v
Kernel entry point
```

Control transfers from the bootloader to the Linux kernel.

This is a major transition:

```text
Firmware / Bootloader
        ↓
Linux Kernel
```

---

# 20. Early Kernel Initialization

The kernel starts executing architecture-specific startup code.

Conceptually:

```text
Kernel Entry
    |
    v
Architecture Initialization
    |
    v
CPU Setup
    |
    v
Memory Setup
    |
    v
Interrupt Setup
    |
    v
Kernel Initialization
```

Exact details vary by architecture.

---

# 21. Kernel Decompression

On many systems, the kernel image is compressed.

Conceptually:

```text
Compressed Kernel
       |
       v
Decompression
       |
       v
Linux Kernel
```

The boot process then continues with the decompressed kernel.

The exact image format and decompression path depend on architecture/configuration.

---

# 22. CPU Initialization

The kernel initializes CPU-related structures.

Conceptually:

```text
CPU
 |
 +-- CPU identification
 +-- CPU features
 +-- Per-CPU data
 +-- Scheduler structures
 +-- Exception handling
```

On SMP systems:

```text
CPU0
CPU1
CPU2
CPU3
```

must be brought into a usable state.

---

# 23. SMP Initialization

SMP means:

```text
Symmetric MultiProcessing
```

The kernel initializes multiple CPUs.

Conceptually:

```text
Boot CPU
   |
   +---- CPU1
   +---- CPU2
   +---- CPU3
```

The boot CPU starts the process and secondary CPUs are brought online.

---

# 24. Memory Initialization

The kernel initializes memory management.

Conceptually:

```text
Physical Memory
      |
      v
Memory Management
      |
      +-- Page allocator
      +-- Virtual memory
      +-- Page tables
      +-- Zones
      +-- Slab/SLUB
```

This establishes the foundation needed for normal kernel operation.

---

# 25. Page Tables

Virtual memory requires page tables.

Conceptually:

```text
Virtual Address
      |
      v
Page Table
      |
      v
Physical Address
```

During early boot, the kernel establishes the mappings required to safely transition into normal virtual-memory operation.

---

# 26. Interrupt Initialization

The kernel initializes interrupt infrastructure.

Conceptually:

```text
Hardware
   |
   v
Interrupt Controller
   |
   v
Linux IRQ subsystem
   |
   v
Device Driver
```

Interrupt handling is essential for:

```text
Timers
NIC
Storage
USB
PCIe
Input devices
```

---

# 27. Timer Initialization

The kernel needs timers for:

```text
Scheduling
Timeouts
Timekeeping
Kernel timers
Periodic activities
```

Conceptually:

```text
Hardware Timer
      |
      v
Kernel Timekeeping
      |
      +-- Scheduler
      +-- Timers
      +-- Timeouts
```

---

# 28. Scheduler Initialization

The scheduler structures are initialized early enough for the kernel to manage tasks.

Conceptually:

```text
Scheduler
 |
 +-- Run queues
 +-- Scheduling classes
 +-- Per-CPU state
 +-- Task state
```

Eventually the system can switch between runnable tasks.

---

# 29. Kernel Subsystems Initialization

The kernel initializes many subsystems.

Examples:

```text
Memory Management
Scheduler
VFS
Networking
Block Layer
Device Model
Security
IPC
Drivers
```

A useful mental model:

```text
Kernel
 |
 +-- MM
 +-- Scheduler
 +-- VFS
 +-- Network
 +-- Block
 +-- Drivers
 +-- Security
```

---

# 30. `start_kernel()`

One important function in the Linux boot path is:

```c
start_kernel()
```

It is a central point in generic kernel initialization.

Conceptually:

```text
Architecture startup
       |
       v
start_kernel()
       |
       +-- Memory initialization
       +-- Scheduler initialization
       +-- IRQ initialization
       +-- VFS initialization
       +-- Networking initialization
       +-- Other subsystems
```

Do not assume every initialization happens literally as one simple linear list inside this function; the real kernel uses architecture-specific and subsystem-specific initialization mechanisms.

---

# 31. `rest_init()`

Near the end of the main kernel initialization sequence, Linux creates important kernel execution contexts and transitions toward normal operation.

Conceptually:

```text
start_kernel()
      |
      v
rest_init()
      |
      +-- kernel threads
      |
      +-- init process
      |
      +-- idle context
```

This is a useful function to know for kernel interviews.

---

# 32. PID 0

The initial idle task is traditionally associated with:

```text
PID 0
```

It is the idle task for CPUs when there is no other runnable work.

Conceptually:

```text
CPU
 |
 +-- Runnable tasks
 |
 +-- If none
       |
       v
    Idle task
```

Do not confuse PID 0 with the first user-space process.

---

# 33. PID 1

The first user-space process is:

```text
PID 1
```

On many modern Linux distributions it is:

```text
systemd
```

But Linux does not fundamentally require the PID 1 executable to be systemd; other init implementations can be used.

Conceptually:

```text
Kernel
  |
  v
PID 1
  |
  v
User Space
```

---

# 34. `init` Process

The kernel starts the initial user-space process.

Historically:

```text
/sbin/init
```

Modern systems often resolve this to:

```text
systemd
```

The kernel command line can also specify an alternative through:

```text
init=
```

---

# 35. PID 1 Responsibilities

PID 1 is responsible for bringing up user space.

Typical responsibilities include:

```text
Start services
Manage service dependencies
Handle shutdown/reboot
Adopt orphaned processes
Coordinate system initialization
```

For systemd systems:

```text
systemd
```

performs these responsibilities.

---

# 36. initramfs

Before mounting the real root filesystem, Linux may use:

```text
initramfs
```

Initial RAM filesystem.

Conceptually:

```text
Bootloader
   |
   +-- Kernel
   |
   +-- initramfs
          |
          v
       Early user space
```

---

# 37. Why Is initramfs Needed?

Suppose the real root filesystem is located on:

```text
RAID
LVM
encrypted disk
NVMe
network storage
```

The kernel may need drivers and utilities before it can mount the real root filesystem.

initramfs provides this early environment.

---

# 38. initramfs Flow

```text
Kernel
   |
   v
initramfs
   |
   +-- Load required modules
   +-- Discover storage
   +-- Assemble RAID
   +-- Activate LVM
   +-- Unlock encrypted volume
   |
   v
Mount real root filesystem
```

Then:

```text
switch_root / equivalent transition
        |
        v
Real root filesystem
```

---

# 39. initramfs vs Root Filesystem

Important distinction:

```text
initramfs
    ↓
Temporary early user space

Root filesystem
    ↓
Actual filesystem used by normal user space
```

Conceptually:

```text
Boot
 |
 v
initramfs
 |
 v
Real Root FS
 |
 v
PID 1
 |
 v
Services
```

---

# 40. Root Filesystem

The kernel eventually needs a root filesystem:

```text
/
```

It contains things such as:

```text
/bin
/etc
/dev
/proc
/sys
/usr
/var
```

The exact layout varies by distribution.

---

# 41. Mounting Root Filesystem

Conceptually:

```text
Kernel
  |
  v
Find root device
  |
  v
Initialize storage/filesystem
  |
  v
Mount root filesystem
  |
  v
Root = /
```

This connects boot internals with:

```text
VFS
Filesystem drivers
Block layer
Storage drivers
```

---

# 42. VFS During Boot

The VFS subsystem is initialized during kernel startup.

Later:

```text
Root filesystem
      |
      v
VFS
      |
      v
Mount
      |
      v
/
```

The root filesystem becomes the starting point for normal pathname resolution.

---

# 43. `/proc`

After boot, Linux exposes process/kernel information through:

```text
/proc
```

Examples:

```text
/proc/cpuinfo
/proc/meminfo
/proc/interrupts
/proc/net/
```

This is a virtual filesystem backed by kernel-generated information.

---

# 44. `/sys`

Linux also exposes the device model and kernel object information through:

```text
/sys
```

This is primarily associated with:

```text
sysfs
```

It is heavily used by:

```text
udev
device management
drivers
hardware discovery
```

---

# 45. Device Initialization

Linux's device model brings devices and drivers together.

Conceptually:

```text
Hardware
   |
   v
Bus
   |
   v
Device
   |
   v
Driver
```

Examples:

```text
PCI
USB
I2C
SPI
Platform devices
```

---

# 46. Driver Initialization

Drivers can be initialized during kernel boot.

Conceptually:

```text
Kernel
  |
  v
Driver initialization
  |
  v
Device discovery
  |
  v
Driver/device binding
```

Some drivers are built into the kernel.

Others are loaded as modules later.

---

# 47. Built-in Driver vs Module

### Built-in

```text
Kernel
  |
  +-- Driver compiled into kernel
```

Available during kernel startup.

### Module

```text
Kernel
  |
  +-- Load .ko later
```

Can be loaded dynamically.

This distinction becomes important when boot depends on a particular driver.

---

# 48. Boot Failure Example

Suppose root filesystem is on NVMe.

If the required NVMe support is unavailable:

```text
Kernel
   |
   v
Cannot access root device
   |
   v
Cannot mount /
```

The system may fail to boot.

This is why required storage drivers may need to be:

```text
Built into kernel
```

or:

```text
Included in initramfs
```

---

# 49. Boot Failure – Kernel Panic

A severe kernel boot failure can result in:

```text
Kernel panic
```

Example:

```text
Unable to mount root filesystem
```

Possible causes:

```text
Wrong root= parameter
Missing storage driver
Missing filesystem driver
Corrupt filesystem
Incorrect initramfs
Incorrect device configuration
```

---

# 50. Boot Failure Debugging

A senior engineer should inspect:

```text
Kernel command line
initramfs
root device
storage driver
filesystem driver
dmesg
system logs
firmware messages
```

Useful commands after boot:

```bash
cat /proc/cmdline
dmesg
lsblk
mount
findmnt
journalctl -b
```

---

# 51. `dmesg`

`dmesg` displays kernel messages from the kernel ring buffer.

Example:

```bash
dmesg | less
```

Useful for debugging:

```text
Boot
Drivers
Storage
Networking
USB
PCIe
Memory
Kernel errors
```

---

# 52. systemd Boot

On a system using systemd:

```text
Kernel
  |
  v
PID 1 = systemd
  |
  +-- mount filesystems
  +-- start services
  +-- start networking
  +-- start logging
  +-- start login services
  |
  v
System Ready
```

systemd manages services and dependencies rather than simply executing one fixed shell script.

---

# 53. systemd Targets

systemd organizes boot using targets.

Examples:

```text
multi-user.target
graphical.target
rescue.target
emergency.target
```

Conceptually:

```text
systemd
   |
   v
Target
   |
   +-- dependencies
   +-- services
   +-- mounts
```

---

# 54. Boot Dependency Example

Suppose:

```text
Application Service
        |
        v
Network
        |
        v
Network Device
```

systemd can represent these relationships through service dependencies.

Conceptually:

```text
network.target
      |
      v
application.service
```

The actual dependency graph can be more complex.

---

# 55. Parallel Initialization

Older initialization models often looked like:

```text
Service A
   ↓
Service B
   ↓
Service C
```

Modern service managers can start independent units concurrently:

```text
             systemd
                |
        +-------+-------+
        |       |       |
        v       v       v
      A       B       C
```

This can reduce boot time.

---

# 56. Boot Completion

Eventually:

```text
Firmware
   ↓
Bootloader
   ↓
Kernel
   ↓
initramfs
   ↓
Root filesystem
   ↓
PID 1
   ↓
Services
   ↓
Login
```

The system is now operating normally.

---

# 57. Shutdown Flow

Boot is not the only important lifecycle.

Shutdown is approximately:

```text
User
 |
 v
systemd
 |
 v
Stop services
 |
 v
Unmount filesystems
 |
 v
Sync storage
 |
 v
Kernel shutdown
 |
 v
Firmware / hardware reset or poweroff
```

---

# 58. Reboot vs Shutdown

Reboot:

```text
Stop user space
    ↓
Shutdown kernel
    ↓
Reset hardware
    ↓
Firmware
    ↓
Boot again
```

Poweroff:

```text
Stop user space
    ↓
Shutdown kernel
    ↓
Power off
```

---

# 59. Embedded Linux Boot Flow

For embedded interviews, memorize:

```text
Power On
   |
   v
Boot ROM
   |
   v
First-stage bootloader
   |
   v
U-Boot
   |
   +-- Load Kernel
   +-- Load Device Tree
   +-- Load initramfs
   +-- Set bootargs
   |
   v
Linux Kernel
   |
   v
Kernel Initialization
   |
   v
Root Filesystem
   |
   v
init / BusyBox / systemd
   |
   v
Application
```

---

# 60. Qualcomm / AMD / NVIDIA / Intel Relevance

For SoC, GPU, CPU, embedded, and driver interviews, boot questions often connect:

```text
Boot ROM
   ↓
Firmware
   ↓
Bootloader
   ↓
Kernel
   ↓
Device Tree / ACPI
   ↓
Driver
   ↓
Hardware
```

You should understand where hardware information comes from and when drivers become active.

---

# 61. ACPI vs Device Tree

A useful distinction:

### Device Tree

Commonly used in:

```text
Embedded ARM systems
```

Hardware description is supplied through a device tree.

### ACPI

Commonly used in:

```text
PC/server platforms
```

Firmware provides hardware configuration information through ACPI tables.

Conceptually:

```text
Embedded:
Bootloader → Device Tree → Kernel

PC/Server:
Firmware → ACPI → Kernel
```

---

# 62. Boot Debugging on Embedded Systems

Serial console is extremely important.

Typical flow:

```text
Boot ROM
   ↓
Bootloader
   ↓
Kernel
   ↓
init
```

Serial output can reveal exactly where boot stops.

Example:

```text
U-Boot...
Loading kernel...
Starting kernel...
```

If output stops after:

```text
Starting kernel...
```

the problem may be in early kernel startup, hardware configuration, device tree, console configuration, or another early boot issue.

---

# 63. Early Console

For embedded debugging, kernel command-line parameters can configure a serial console.

Example:

```text
console=ttyS0,115200
```

The exact console device depends on the hardware.

This allows early kernel messages to be observed over UART.

---

# 64. Secure Boot

Modern systems may implement:

```text
Secure Boot
```

Conceptually:

```text
Firmware
   |
   v
Verify Bootloader
   |
   v
Verify Kernel / next stage
   |
   v
Boot
```

The goal is to prevent unauthorized software from being executed during the boot chain.

---

# 65. Measured Boot vs Secure Boot

### Secure Boot

```text
Verify authenticity
before execution
```

### Measured Boot

```text
Measure components
and record measurements
```

Measured boot can support later attestation.

---

# 66. Boot Chain of Trust

A secure boot chain can look like:

```text
Hardware Root of Trust
        |
        v
Firmware
        |
        v
Bootloader
        |
        v
Kernel
        |
        v
User Space
```

Each stage verifies or trusts the next stage according to the platform's security design.

---

# 67. Kernel Command Line Debugging

Useful parameters can include:

```text
earlyprintk=
earlycon
console=
init=
root=
loglevel=
```

These are particularly useful when normal user-space logging is not yet available.

---

# 68. Important Boot Files

On many Linux systems:

```text
/boot/
```

may contain:

```text
vmlinuz
initramfs
System.map
config
```

Exact files depend on the distribution and boot configuration.

---

# 69. Boot Performance

To investigate slow boot:

```text
Firmware time
+
Bootloader time
+
Kernel initialization
+
initramfs
+
systemd/service startup
```

For systemd systems:

```bash
systemd-analyze
```

can provide boot timing information.

---

# 70. Boot Profiling Mental Model

```text
Total Boot Time
       |
       +-- Firmware
       |
       +-- Bootloader
       |
       +-- Kernel
       |
       +-- initramfs
       |
       +-- PID 1
       |
       +-- Services
```

Optimize the actual bottleneck rather than blindly disabling services.

---

# 71. Senior Interview Question

## Explain the Linux boot process.

Strong answer:

> Firmware initializes the platform and selects a boot entry. The bootloader loads the Linux kernel and usually an initramfs, then passes the kernel command line and transfers control to the kernel. The kernel performs architecture, CPU, memory, interrupt, scheduler, driver, filesystem and other subsystem initialization. Early user space in initramfs prepares access to the real root filesystem. The kernel then starts PID 1, commonly systemd on modern distributions, which initializes the remaining user-space services.

---

# 72. Senior Interview Question

## Why do we need initramfs?

Because the kernel may need additional drivers or early user-space tools to discover and prepare the real root filesystem.

Examples:

```text
LVM
RAID
Encrypted storage
NVMe
Network root
Special storage drivers
```

---

# 73. Senior Interview Question

## What happens if the root filesystem cannot be mounted?

The kernel cannot transition to normal user space.

Possible causes:

```text
Wrong root= parameter
Missing driver
Missing filesystem support
Corrupt filesystem
Bad initramfs
Storage failure
```

Depending on the failure, the system may enter a kernel panic or an emergency/initramfs shell.

---

# 74. Senior Interview Question

## What is PID 1?

PID 1 is the first user-space process created by the kernel.

On many modern Linux distributions:

```text
PID 1 → systemd
```

It manages system initialization and services and has special process-management responsibilities.

---

# 75. Senior Interview Question

## What is the difference between PID 0 and PID 1?

```text
PID 0
    Kernel idle task/context

PID 1
    First user-space process
    init/systemd
```

Do not confuse them.

---

# 76. Senior Interview Question

## Why is a driver sometimes built into the kernel instead of being a module?

If the driver is required before the real root filesystem can be mounted, it must be available early.

It can be:

```text
Built into kernel
```

or:

```text
Included in initramfs as a module
```

---

# 77. Senior Interview Question

## What is the role of the bootloader?

The bootloader:

```text
Selects boot configuration
Loads kernel
Loads initramfs
Loads device tree when applicable
Passes kernel parameters
Transfers control to kernel
```

---

# 78. Senior Interview Question

## GRUB vs U-Boot?

```text
GRUB
    Common on PC/server Linux systems

U-Boot
    Very common in embedded Linux
```

Both can:

```text
Load kernel
Load supporting boot data
Pass boot parameters
Transfer control to Linux
```

---

# 79. Senior Interview Question

## What is Device Tree?

Device Tree is a hardware description passed to the kernel, especially common on embedded platforms.

It describes things such as:

```text
CPU
Memory
Devices
Addresses
Interrupts
GPIOs
Bus relationships
```

Drivers use this information to configure/bind to hardware.

---

# 80. Senior Interview Question

## What is the difference between kernel initialization and user-space initialization?

### Kernel initialization

```text
CPU
Memory
Interrupts
Scheduler
VFS
Networking
Drivers
Devices
```

### User-space initialization

```text
Mount remaining filesystems
Start services
Networking services
Logging
Login
Applications
```

---

# 81. Final Boot Mental Model

Memorize this:

```text
                 POWER ON
                     |
                     v
              BIOS / UEFI
                     |
                     v
               Bootloader
              GRUB / U-Boot
                     |
          +----------+----------+
          |                     |
       Kernel                initramfs
          |                     |
          +----------+----------+
                     |
                     v
              Kernel Entry
                     |
                     v
             start_kernel()
                     |
       +-------------+-------------+
       |             |             |
       v             v             v
     CPU           Memory        IRQs
       |             |             |
       +-------------+-------------+
                     |
                     v
              Kernel Subsystems
                     |
                     v
             Root Filesystem
                     |
                     v
                  PID 1
                     |
                     v
               systemd/init
                     |
                     v
                 Services
                     |
                     v
                  Login
```

---

# 82. What You Must Master

For senior Linux / embedded / Qualcomm / AMD / NVIDIA / Intel interviews:

```text
★★★★★ Complete boot sequence
★★★★★ BIOS vs UEFI
★★★★★ GRUB
★★★★★ U-Boot
★★★★★ Kernel entry
★★★★★ start_kernel()
★★★★★ initramfs
★★★★★ Root filesystem
★★★★★ PID 0 vs PID 1
★★★★★ systemd/init
★★★★★ Device Tree
★★★★★ ACPI
★★★★★ Driver initialization
★★★★★ Boot debugging
★★★★★ Kernel command line
★★★★★ Kernel panic during boot

★★★★☆ Secure Boot
★★★★☆ Measured Boot
★★★★☆ Boot performance
★★★★☆ Early console
★★★★☆ SMP initialization
★★★★☆ Memory initialization
```

---

# 83. One Diagram to Remember

```text
PC / SERVER

Power
  ↓
UEFI
  ↓
GRUB
  ↓
Kernel + initramfs
  ↓
Kernel initialization
  ↓
Root FS
  ↓
systemd (PID 1)
  ↓
Services
  ↓
Applications
```

```text
EMBEDDED

Power
  ↓
Boot ROM
  ↓
U-Boot
  ↓
Kernel + Device Tree + initramfs
  ↓
Kernel initialization
  ↓
Root FS
  ↓
init / BusyBox / systemd
  ↓
Embedded Application
```

The most important senior-level connection is:

```text
Bootloader
    ↓
Kernel
    ↓
Memory + CPU + IRQ + Scheduler
    ↓
Drivers
    ↓
Storage / Filesystem
    ↓
Root FS
    ↓
PID 1
    ↓
User Space
```

Understanding this chain lets you reason about **boot failures, missing drivers, initramfs problems, embedded bring-up, kernel panics, device-tree issues, and Linux startup performance** rather than merely memorizing the boot sequence.
