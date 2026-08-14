# Linux Kernel + System Programming

# Chapters 14–24

---

# Chapter 14 — Kernel Modules + Device Drivers

## Objectives

After completing this chapter, you should understand:

* What a kernel module is
* Why kernel modules are required
* Kernel module lifecycle
* Built-in kernel code vs loadable modules
* Module loading/unloading
* Device drivers
* Character vs block vs network drivers
* Major/minor device numbers
* `/dev`
* `file_operations`
* `ioctl`
* `sysfs`
* `udev`
* Kernel module architecture

---

## 1. What Is a Kernel Module?

A **kernel module** is code that can be dynamically loaded into or removed from the Linux kernel.

Instead of compiling every feature directly into the kernel:

```text
Linux Kernel
    |
    +-- Built-in functionality
    |
    +-- Loadable Kernel Modules
            |
            +-- Device Drivers
            +-- Filesystems
            +-- Networking
            +-- Other functionality
```

Typical module file:

```text
*.ko
```

`.ko` = Kernel Object.

---

## 2. Built-in vs Module

### Built-in

Compiled directly into the kernel:

```text
vmlinux
```

Advantages:

* Always available
* Required during early boot
* No module loading required

Disadvantages:

* Larger kernel
* Cannot be unloaded independently

### Loadable Module

Loaded at runtime:

```text
driver.ko
```

Advantages:

* Dynamic
* Smaller base kernel
* Can add/remove functionality
* Useful for device drivers

---

## 3. Module Lifecycle

```text
Compile
   |
   v
driver.ko
   |
   v
insmod / modprobe
   |
   v
Kernel
   |
   v
module_init()
   |
   v
Module Running
   |
   v
rmmod
   |
   v
module_exit()
   |
   v
Removed
```

---

## 4. Important Commands

List loaded modules:

```bash
lsmod
```

Load module:

```bash
sudo insmod driver.ko
```

Load module with dependency handling:

```bash
sudo modprobe driver
```

Remove module:

```bash
sudo rmmod driver
```

Module information:

```bash
modinfo driver
```

Kernel messages:

```bash
dmesg
```

---

## 5. `insmod` vs `modprobe`

### insmod

Directly inserts the specified `.ko` file.

```bash
insmod driver.ko
```

Does not automatically resolve dependencies.

### modprobe

Understands module dependencies.

```bash
modprobe driver
```

Preferred for normal module management.

---

## 6. Basic Kernel Module

Conceptually:

```c
#include <linux/module.h>
#include <linux/kernel.h>

static int __init my_init(void)
{
    printk(KERN_INFO "Module loaded\n");
    return 0;
}

static void __exit my_exit(void)
{
    printk(KERN_INFO "Module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
```

Important:

```text
module_init()
    -> initialization

module_exit()
    -> cleanup
```

---

# 7. Device Driver

A device driver allows the kernel to communicate with hardware.

```text
Application
     |
     | system call
     v
Linux Kernel
     |
     v
Device Driver
     |
     v
Hardware
```

Example:

```text
Application
    |
    v
read()
    |
    v
VFS
    |
    v
Device Driver
    |
    v
Disk / USB / Network Device
```

---

# 8. Types of Device Drivers

## Character Driver

Transfers data as a stream.

Examples:

```text
Keyboard
Serial Port
Terminal
```

Typical operations:

```text
open
read
write
ioctl
release
```

---

## Block Driver

Works with block-oriented storage.

Examples:

```text
HDD
SSD
NVMe
USB Storage
```

Block devices support random access.

---

## Network Driver

Handles network hardware.

```text
Application
    |
Socket
    |
Networking Stack
    |
Network Driver
    |
NIC
```

---

# 9. Major and Minor Numbers

Linux identifies devices using:

```text
Major Number
Minor Number
```

Major number:

```text
Which driver?
```

Minor number:

```text
Which device handled by that driver?
```

Example:

```text
major = 8
minor = 0
```

---

# 10. `/dev`

Device nodes are exposed under:

```text
/dev
```

Examples:

```text
/dev/sda
/dev/nvme0n1
/dev/tty
/dev/null
/dev/random
```

Application opens a device:

```c
open("/dev/mydevice", ...);
```

The kernel routes the operation to the corresponding driver.

---

# 11. `file_operations`

Character drivers commonly provide callbacks through:

```c
struct file_operations
```

Conceptually:

```text
file_operations
 |
 +-- open()
 +-- read()
 +-- write()
 +-- ioctl()
 +-- mmap()
 +-- poll()
 +-- release()
```

Flow:

```text
Application
   |
   | read()
   v
sys_read()
   |
   v
VFS
   |
   v
file_operations.read
   |
   v
Driver
```

---

# 12. ioctl

`ioctl()` provides device-specific control operations.

Normal operations:

```text
read()
write()
```

Special control:

```text
ioctl()
```

Examples:

```text
Configure device
Set mode
Query status
Reset device
```

---

# 13. sysfs

Linux exposes kernel/device information through:

```text
/sys
```

Example:

```text
/sys/class/
/sys/devices/
/sys/block/
```

`sysfs` provides a structured view of kernel objects and devices.

---

# 14. udev

`udev` manages device nodes in user space.

When hardware appears:

```text
Hardware
   |
   v
Kernel detects device
   |
   v
uevent
   |
   v
udev
   |
   v
/dev/device
```

---

# Interview Points

* Kernel module = dynamically loadable kernel code.
* `.ko` = kernel object.
* `insmod` directly loads a module.
* `modprobe` handles dependencies.
* Character devices provide stream-oriented access.
* Block devices provide block-oriented storage access.
* Major number identifies the driver.
* Minor number identifies a device handled by that driver.
* `/dev` contains device nodes.
* `/sys` exposes kernel/device information.
* `udev` manages device nodes from user space.
* `file_operations` connects VFS operations to driver callbacks.

---

# Chapter 15 — Linux VFS Internals

## Objectives

Understand:

* VFS
* inode
* dentry
* superblock
* file
* file descriptor
* filesystem implementation
* pathname lookup
* open/read/write flow
* page cache
* VFS architecture

---

# 1. What Is VFS?

VFS = **Virtual File System**.

It provides a common interface to different filesystems.

```text
             Applications
                   |
              system calls
                   |
                   v
                  VFS
        ___________|____________
       |           |            |
      ext4        XFS         NFS
       |           |            |
       +-----------+------------+
                   |
                Storage
```

Application does not need to know whether a file is:

```text
ext4
xfs
tmpfs
NFS
```

---

# 2. Important VFS Objects

Four major objects:

```text
super_block
inode
dentry
file
```

---

# 3. Superblock

Represents a mounted filesystem.

Contains information such as:

```text
Filesystem type
Block size
Filesystem state
Root inode
Filesystem operations
```

Conceptually:

```text
Disk
 |
Filesystem
 |
super_block
```

---

# 4. Inode

An inode represents a filesystem object.

Contains metadata:

```text
File type
Permissions
Owner
Group
Size
Timestamps
Block information
```

Important:

**Filename is not stored in the inode.**

Filename mapping is handled through dentries.

---

# 5. Dentry

Dentry = Directory Entry.

It connects:

```text
Filename
    |
    v
inode
```

Example:

```text
/home/user/test.txt
```

Path lookup:

```text
/
 |
home
 |
user
 |
test.txt
 |
inode
```

Dentries are heavily cached.

---

# 6. File Object

A `struct file` represents an **open file instance**.

Important distinction:

```text
inode
    = filesystem object

file
    = open instance of that object
```

Two processes can open the same file:

```text
Process A ---> file object A
                    |
                    v
                  inode

Process B ---> file object B
                    |
                    v
                  inode
```

---

# 7. File Descriptor

A file descriptor is a process-local integer.

Example:

```text
0 -> stdin
1 -> stdout
2 -> stderr
```

Conceptually:

```text
Process
 |
fd table
 |
fd = 3
 |
v
struct file
 |
v
inode
```

---

# 8. VFS Open Flow

When:

```c
fd = open("/home/user/a.txt", O_RDONLY);
```

Conceptually:

```text
Application
   |
   v
open()
   |
   v
System Call
   |
   v
VFS
   |
   v
Path Lookup
   |
   +--> dentry
   |
   +--> inode
   |
   v
Filesystem
   |
   v
struct file
   |
   v
File Descriptor
```

---

# 9. Read Flow

```text
read(fd, buffer, size)
        |
        v
      VFS
        |
        v
   struct file
        |
        v
 Page Cache
    /       \
 HIT         MISS
 |             |
 v             v
copy        filesystem
              |
              v
            disk
```

---

# 10. Page Cache

The page cache stores filesystem data in RAM.

```text
Application
     |
     v
   VFS
     |
     v
 Page Cache
   /    \
 HIT    MISS
 |        |
RAM      Disk
```

Purpose:

* Reduce disk I/O
* Improve performance
* Cache frequently accessed file data

---

# 11. Write Flow

For buffered writes:

```text
Application
    |
   write()
    |
    v
Page Cache
    |
    v
Dirty Pages
    |
    v
Writeback
    |
    v
Filesystem
    |
    v
Block Layer
    |
    v
Storage
```

---

# 12. VFS Operations

Filesystems implement operation structures such as:

```text
super_operations
inode_operations
file_operations
address_space_operations
```

These allow VFS to interact with filesystem-specific implementations.

---

# Interview Points

* VFS provides a common interface for filesystems.
* inode represents the filesystem object and metadata.
* dentry maps pathname component to inode.
* `struct file` represents an open file instance.
* file descriptor is a process-local integer referring to an open file.
* superblock represents a mounted filesystem.
* Page cache caches file data in RAM.
* Filename is associated through dentry, not inode.
* Multiple file objects can reference the same inode.

---

# Chapter 16 — Linux Process Internals

## Objectives

Understand:

* `task_struct`
* Process creation
* `fork`
* `exec`
* `exit`
* Process states
* PID
* PPID
* Process hierarchy
* Context switching
* Kernel stack
* `mm_struct`
* `files_struct`
* `fs_struct`
* Process credentials

---

# 1. Linux Process Representation

Linux represents a process using:

```c
struct task_struct
```

It contains information about the process/thread.

Conceptually:

```text
task_struct
 |
 +-- PID
 +-- State
 +-- Scheduling information
 +-- Memory information
 +-- Files
 +-- Credentials
 +-- Signals
 +-- Parent/child relationships
 +-- Kernel stack
```

---

# 2. Process Architecture

```text
Process
 |
 +-- task_struct
 |
 +-- mm_struct
 |      |
 |      +-- Virtual Memory
 |
 +-- files_struct
 |      |
 |      +-- File Descriptor Table
 |
 +-- fs_struct
 |
 +-- signal information
 |
 +-- credentials
 |
 +-- kernel stack
```

---

# 3. PID

Every process has a PID.

Example:

```bash
ps
```

Linux also has:

```text
PID
PPID
PGID
SID
```

---

# 4. Process Creation

Typical flow:

```text
Parent
  |
  | fork()
  v
Child
  |
  | exec()
  v
New Program
```

`fork()` creates a new process.

`exec()` replaces the current process image with another program.

---

# 5. fork()

Conceptually:

```text
Parent
   |
 fork()
   |
   +------> Child
```

After `fork()`:

```text
Parent continues
Child continues
```

Both return from the same point, but with different return values.

---

# 6. Copy-on-Write

Linux does not immediately copy all memory during `fork()`.

Instead:

```text
Parent page
     |
     +---- Parent mapping
     |
     +---- Child mapping
```

Pages are shared as read-only.

When one process writes:

```text
Write
  |
  v
Page Fault
  |
  v
Copy Page
  |
  v
Modify private copy
```

This is **Copy-on-Write (COW)**.

---

# 7. exec()

`exec()` replaces the process address space with a new program.

```text
Old Program
    |
   exec()
    |
    v
New Program
```

PID usually remains the same.

---

# 8. exit()

When a process exits:

```text
Running
   |
 exit()
   |
   v
Zombie
   |
 parent wait()
   |
   v
Removed
```

---

# 9. Zombie

A zombie has:

```text
Execution finished
Resources mostly released
Exit status retained
```

Parent retrieves status using:

```text
wait()
waitpid()
```

---

# 10. Orphan

If parent exits first:

```text
Parent exits
     |
     v
Child becomes orphan
     |
     v
Adopted/reparented
```

Modern Linux uses a suitable subreaper/init process depending on the hierarchy.

---

# 11. Process States

Common states conceptually:

```text
RUNNING
INTERRUPTIBLE SLEEP
UNINTERRUPTIBLE SLEEP
STOPPED
ZOMBIE
```

A task in `TASK_RUNNING` may be:

```text
Currently executing
```

or:

```text
Runnable and waiting for CPU
```

---

# 12. Kernel Stack

Each Linux thread has a kernel stack used while executing kernel code.

```text
User Stack
     |
     v
User Space
----------------
Kernel Space
     |
Kernel Stack
```

System call:

```text
User
 |
syscall
 |
 v
Kernel Stack
 |
Kernel
```

---

# 13. Process Context

Kernel executes in:

```text
Process Context
```

when associated with a process/thread.

It can generally:

* Sleep
* Access process-specific state
* Be interrupted by scheduler

Interrupt context is different.

---

# Interview Points

* `task_struct` represents a Linux task.
* Linux treats threads as tasks.
* `fork()` creates a child.
* `exec()` replaces the current process image.
* PID generally remains unchanged across exec.
* `fork()` uses Copy-on-Write.
* Zombie has exited but parent has not collected its status.
* File descriptors are represented through process file structures.
* `mm_struct` represents process memory-management information.
* Every thread has a kernel stack.

---

# Chapter 17 — Linux Memory Management Internals

## Objectives

Understand:

* `mm_struct`
* Virtual memory areas
* Page tables
* Physical pages
* Page allocator
* Buddy allocator
* SLAB/SLUB
* Page cache
* Reclaim
* Swap
* Memory zones
* NUMA
* OOM killer

---

# 1. Linux Memory Architecture

```text
Process
   |
Virtual Address
   |
Page Tables
   |
Physical Address
   |
RAM
```

---

# 2. `mm_struct`

Linux maintains memory-management information using:

```c
struct mm_struct
```

It describes a process address space.

Conceptually:

```text
mm_struct
 |
 +-- Page tables
 +-- VMAs
 +-- Memory statistics
 +-- Address-space information
```

Threads of the same process generally share the same `mm_struct`.

---

# 3. VMA

VMA = Virtual Memory Area.

A VMA represents a contiguous virtual address range with common properties.

Examples:

```text
Code
Read-only data
Data
Heap
Shared libraries
Stack
mmap regions
```

Conceptual layout:

```text
High Address
+------------------+
| Stack            |
+------------------+
| mmap             |
+------------------+
| Shared Libraries |
+------------------+
| Heap             |
+------------------+
| Data             |
+------------------+
| Code             |
+------------------+
Low Address
```

---

# 4. Physical Page

Linux manages physical memory in units called:

```text
pages
```

Typical page size:

```text
4 KB
```

but architecture/configuration can support other sizes.

---

# 5. Page Allocator

The kernel needs pages for:

```text
Processes
Kernel structures
Page cache
Buffers
Drivers
```

The physical page allocator manages these pages.

---

# 6. Buddy Allocator

The buddy allocator manages contiguous physical pages in powers of two.

Example:

```text
Order 0 -> 1 page
Order 1 -> 2 pages
Order 2 -> 4 pages
Order 3 -> 8 pages
```

Conceptually:

```text
Large Block
    |
    +---- Buddy A
    |
    +---- Buddy B
```

Blocks can be split and merged.

---

# 7. SLAB / SLUB Allocator

Allocating an entire page for every small kernel object is inefficient.

Kernel uses object allocators:

```text
SLAB
SLUB
SLOB (historical/specialized)
```

Modern Linux commonly uses:

```text
SLUB
```

Example objects:

```text
task_struct
inode
dentry
```

---

# 8. Page Cache

The page cache stores filesystem data in RAM.

```text
Disk
  |
  v
Page Cache
  |
  v
Applications
```

It is a major consumer of available memory.

---

# 9. Memory Reclaim

When memory pressure increases:

```text
Memory Pressure
      |
      v
Reclaim
   /     \
Clean     Dirty
Pages     Pages
 |          |
Drop      Writeback
```

Linux can reclaim:

* Page cache
* Reclaimable kernel memory
* Anonymous memory through swap, when configured

---

# 10. Swap

Swap allows memory pages to move from RAM to storage.

```text
RAM
 |
 | pressure
 v
Swap
 |
Disk
```

Swap is much slower than RAM.

---

# 11. Memory Zones

Physical memory is organized into zones.

Historically/common examples:

```text
ZONE_DMA
ZONE_DMA32
ZONE_NORMAL
ZONE_HIGHMEM
```

Exact availability depends on architecture and configuration.

---

# 12. NUMA

NUMA = Non-Uniform Memory Access.

```text
CPU 0 ---- RAM 0
   \
    \---- RAM 1

CPU 1 ---- RAM 1
   \
    \---- RAM 0
```

Local memory is generally faster than remote-node memory.

Linux tries to keep CPU and memory locality efficient.

---

# 13. OOM Killer

OOM = Out Of Memory.

When the kernel cannot satisfy memory requirements and reclaim is insufficient:

```text
Memory Exhausted
      |
      v
OOM handling
      |
      v
Select process
      |
      v
Terminate process
```

Goal:

```text
Recover memory
```

---

# Interview Points

* `mm_struct` represents an address space.
* VMA represents a virtual address range.
* Buddy allocator manages physical page blocks.
* SLUB manages frequently allocated kernel objects.
* Page cache caches filesystem data.
* Swap moves eligible memory pages to storage.
* NUMA introduces memory locality.
* OOM killer terminates selected processes under severe memory exhaustion.

---

# Chapter 18 — Linux Scheduler Internals

## Objectives

Understand:

* Scheduler
* Runnable tasks
* Context switching
* Scheduling classes
* CFS
* `vruntime`
* Real-time scheduling
* Runqueues
* CPU affinity
* Load balancing
* Preemption

---

# 1. Scheduler

Linux scheduler decides:

```text
Which runnable task
should execute on which CPU?
```

```text
Runnable Tasks
      |
      v
Scheduler
      |
      v
CPU
```

---

# 2. Runqueue

Each CPU has scheduler state including a runqueue.

Conceptually:

```text
CPU 0
 |
runqueue
 |
 +-- Task A
 +-- Task B
 +-- Task C
```

---

# 3. Scheduling Classes

Linux scheduler is organized into scheduling classes.

Examples include:

```text
stop
deadline
real-time
CFS / fair scheduling
idle
```

Scheduling classes determine how tasks are selected.

---

# 4. CFS

CFS = Completely Fair Scheduler.

Its goal is to provide fair CPU allocation among normal tasks.

Important concept:

```text
vruntime
```

A task that has received less CPU time generally has lower virtual runtime and becomes more eligible to run.

---

# 5. Nice Value

Normal tasks have a nice value.

```text
Lower nice
    ->
Higher priority
```

Example:

```bash
nice -n 10 command
```

Nice affects CPU scheduling weight for normal tasks.

---

# 6. Real-Time Scheduling

Real-time policies include:

```text
SCHED_FIFO
SCHED_RR
```

These have different semantics from normal fair scheduling.

---

# 7. Context Switch

When CPU changes from one task to another:

```text
Task A
  |
save context
  |
  v
Task B
  |
restore context
  |
  v
CPU executes B
```

Context includes processor state necessary to resume execution.

---

# 8. Preemption

A running task may be preempted so another task can execute.

```text
Task A
   |
interrupt/scheduler
   |
   v
Task B
```

Preemption improves responsiveness and scheduling fairness.

---

# 9. CPU Affinity

CPU affinity controls which CPUs a task may run on.

Example:

```bash
taskset -c 2 ./program
```

Conceptually:

```text
Process
 |
Affinity mask
 |
 +-- CPU 2 allowed
```

---

# 10. Load Balancing

Linux attempts to balance runnable tasks across CPUs.

```text
CPU 0
Task A
Task B
Task C

CPU 1
Task D
```

Scheduler may move:

```text
Task C
   |
   v
CPU 1
```

to improve balance.

---

# 11. Scheduler Tick

The kernel periodically receives timer events.

These can cause scheduler activity and accounting.

Modern Linux also supports tickless operation in suitable configurations.

---

# Interview Points

* Scheduler chooses runnable tasks for CPUs.
* Each CPU has scheduler/runqueue state.
* CFS is used for normal fair scheduling in traditional Linux scheduler terminology.
* `vruntime` is central to fair scheduling.
* Nice value affects normal scheduling weight.
* `SCHED_FIFO` and `SCHED_RR` are real-time policies.
* Context switch changes CPU execution from one task to another.
* CPU affinity restricts where a task may execute.
* Load balancing moves tasks between CPUs.

---

# Chapter 19 — Interrupts + Deferred Work

## Objectives

Understand:

* Hardware interrupts
* Interrupt handlers
* IRQ
* Interrupt context
* Top half
* Bottom half
* Softirq
* Tasklet
* Workqueue
* Threaded interrupts
* Interrupt affinity

---

# 1. What Is an Interrupt?

An interrupt is a mechanism through which hardware requests CPU attention.

Example:

```text
NIC receives packet
       |
       v
Hardware Interrupt
       |
       v
CPU
       |
       v
Kernel Interrupt Handler
```

---

# 2. Why Interrupts?

Without interrupts, CPU would need to continuously poll hardware:

```text
CPU
 |
 +-- Is device ready?
 +-- Is device ready?
 +-- Is device ready?
```

Interrupts allow:

```text
CPU executes useful work
        |
        v
Device becomes ready
        |
        v
Interrupt
```

---

# 3. IRQ

IRQ = Interrupt Request.

Linux maintains interrupt information and associates interrupt numbers with handlers.

---

# 4. Interrupt Handler

High-level flow:

```text
Hardware
   |
Interrupt
   |
CPU
   |
Generic IRQ Layer
   |
Driver Handler
```

The handler should do minimal urgent work.

---

# 5. Interrupt Context

Interrupt handlers execute in interrupt context.

Important:

**Interrupt context cannot perform operations that may sleep.**

Therefore:

```text
Cannot block
Cannot sleep
```

in normal hard IRQ context.

---

# 6. Top Half and Bottom Half

Historically:

```text
Top Half
   |
Immediate work
   |
Bottom Half
   |
Deferred work
```

Goal:

```text
Keep interrupt handling short.
```

---

# 7. Softirq

Softirqs provide deferred kernel work.

Examples include networking-related processing.

Softirqs execute in a context different from ordinary process context and cannot simply sleep like a normal process.

---

# 8. Tasklets

Tasklets historically provide deferred execution built on softirq infrastructure.

Important:

* Run in softirq context
* Cannot sleep

Tasklets are less central in newer kernel development, and newer code often prefers other mechanisms where appropriate.

---

# 9. Workqueues

Workqueues execute work in kernel worker threads.

```text
Interrupt
    |
    v
Schedule Work
    |
    v
Kernel Worker Thread
    |
    v
Deferred Function
```

Because work runs in process context, workqueue functions can generally sleep.

---

# 10. Threaded Interrupts

Linux supports threaded IRQ handlers.

Conceptually:

```text
Hard IRQ
   |
Minimal handler
   |
Wake IRQ thread
   |
Kernel Thread
   |
Actual processing
```

This reduces work performed in hard interrupt context.

---

# 11. Interrupt Affinity

Interrupts can be directed toward selected CPUs.

```text
NIC IRQ
   |
   +---- CPU 2
```

This can help with:

* Cache locality
* Load distribution
* Network performance

---

# Interview Points

* Interrupts allow hardware to notify the CPU.
* Hard IRQ handlers execute in interrupt context.
* Interrupt context cannot sleep.
* Keep hard interrupt handlers short.
* Deferred work can be handled through softirqs, workqueues, threaded IRQs, etc.
* Workqueues run work in kernel worker threads and can generally sleep.
* Tasklets are softirq-based deferred mechanisms and are legacy/de-emphasized in modern kernel development.
* IRQ affinity controls CPU placement of interrupts.

---

# Chapter 20 — Kernel Synchronization

## Objectives

Understand:

* Why kernel synchronization is required
* Race conditions
* Spinlocks
* Mutexes
* Semaphores
* RW locks
* Atomic operations
* Memory barriers
* RCU
* Per-CPU data
* Locking in interrupt context

---

# 1. Why Kernel Synchronization?

Kernel code executes concurrently because of:

```text
Multiple CPUs
Multiple threads
Interrupts
Preemption
```

Example:

```text
CPU 0                 CPU 1

counter++             counter++
```

Without synchronization:

```text
Lost Update
```

---

# 2. Race Condition

```text
Thread A              Thread B

read counter
                      read counter
counter = counter + 1
                      counter = counter + 1
```

Expected:

```text
2
```

Possible result:

```text
1
```

---

# 3. Spinlock

Spinlock makes another CPU wait by spinning.

```text
CPU 0
 |
lock
 |
critical section

CPU 1
 |
tries lock
 |
spins
```

Useful when:

```text
Critical section is short
```

and sleeping is not allowed.

---

# 4. Mutex

Mutex provides mutual exclusion and may block/sleep.

```text
Thread A
 |
mutex_lock
 |
critical section
 |
mutex_unlock
```

Another thread waits rather than continuously spinning.

Use mutex in process context when sleeping is allowed.

---

# 5. Spinlock vs Mutex

| Spinlock                                             | Mutex                    |
| ---------------------------------------------------- | ------------------------ |
| Spins                                                | Sleeps/blocks            |
| Short critical sections                              | Longer critical sections |
| Can be used in appropriate atomic/interrupt contexts | Process context          |
| No sleeping while held                               | May sleep                |

---

# 6. Semaphore

Semaphore controls access using a count.

Binary semaphore:

```text
0 / 1
```

Counting semaphore:

```text
0 ... N
```

Modern Linux code generally prefers mutexes for mutual exclusion and other specialized primitives where appropriate.

---

# 7. Read-Write Lock

Allows:

```text
Multiple readers
One writer
```

Conceptually:

```text
Reader ----\
Reader ----- > Shared data
Reader ----/

Writer ----> exclusive access
```

Useful when reads dominate writes.

---

# 8. Atomic Operations

Atomic operations perform indivisible operations.

Example:

```text
atomic_inc()
atomic_dec()
atomic_add()
```

Useful for simple counters/state.

---

# 9. Memory Barriers

CPU and compiler reordering can make concurrency difficult.

Memory barriers enforce required ordering.

Conceptually:

```text
Store A
 |
Memory Barrier
 |
Store B
```

Important for lock-free and low-level concurrent algorithms.

---

# 10. RCU

RCU = Read-Copy-Update.

Designed for read-heavy workloads.

Conceptually:

```text
Readers
  |
read old data
```

Writer:

```text
Create new version
       |
Update pointer
       |
Wait for readers
       |
Free old version
```

Advantages:

```text
Very cheap reads
Good for read-heavy data
```

---

# 11. Per-CPU Data

Instead of sharing one global variable:

```text
Global Counter
```

use:

```text
CPU 0 -> Counter 0
CPU 1 -> Counter 1
CPU 2 -> Counter 2
```

This reduces contention and improves cache locality.

---

# 12. Locking and Interrupts

If shared data can be accessed from:

```text
Process context
+
Interrupt context
```

the locking strategy must account for interrupt/preemption behavior.

A normal mutex cannot be used from hard IRQ context because it may sleep.

---

# Interview Points

* Kernel synchronization protects shared data.
* Spinlocks spin and are useful for short critical sections where sleeping is not allowed.
* Mutexes may sleep.
* Mutexes cannot be used in hard interrupt context.
* Atomic operations are useful for simple shared state.
* Memory barriers provide ordering guarantees.
* RCU is optimized for read-heavy workloads.
* Per-CPU data reduces cross-CPU contention.
* Synchronization design must consider process context, interrupt context, preemption, and CPU concurrency.

---

# Chapter 21 — Kernel Networking Internals

## Objectives

Understand:

* Linux networking stack
* Socket layer
* TCP/IP stack
* `sk_buff`
* NIC driver
* Receive path
* Transmit path
* NAPI
* Softirq
* Netfilter
* Routing
* TCP internals

---

# 1. Linux Network Architecture

```text
Application
     |
   Socket
     |
Socket Layer
     |
TCP / UDP
     |
IP
     |
Network Device Layer
     |
NIC Driver
     |
Hardware
```

---

# 2. Socket

Applications communicate through sockets.

Example:

```c
socket()
bind()
listen()
accept()
connect()
send()
recv()
```

The socket API hides much of the networking implementation.

---

# 3. Receive Path

Conceptually:

```text
NIC
 |
DMA
 |
Driver
 |
NAPI
 |
Network Stack
 |
IP
 |
TCP/UDP
 |
Socket
 |
Application
```

---

# 4. `sk_buff`

Linux represents network packets using:

```c
struct sk_buff
```

Often called:

```text
skb
```

It contains metadata and references to packet data.

Conceptually:

```text
sk_buff
 |
 +-- Packet metadata
 +-- Protocol information
 +-- Data buffers
 +-- Device information
```

---

# 5. NAPI

NAPI = New API.

Used to improve network packet processing efficiency.

Instead of handling every packet entirely through hard interrupts:

```text
Interrupt
   |
Schedule polling
   |
NAPI poll
   |
Process packets
```

This reduces interrupt overhead during high traffic.

---

# 6. Receive Flow

```text
Packet arrives
     |
     v
NIC
     |
     v
DMA buffer
     |
     v
Interrupt
     |
     v
NAPI scheduled
     |
     v
Driver poll
     |
     v
skb
     |
     v
IP layer
     |
     v
TCP/UDP
     |
     v
Socket receive queue
     |
     v
recv()
```

---

# 7. Transmit Flow

```text
Application
     |
send()
     |
Socket
     |
TCP/UDP
     |
IP
     |
qdisc / network device
     |
Driver
     |
DMA
     |
NIC
     |
Network
```

---

# 8. Routing

Linux maintains routing information.

Conceptually:

```text
Packet
 |
Destination IP
 |
Routing lookup
 |
Next hop / interface
```

Useful commands:

```bash
ip route
ip addr
ip link
```

---

# 9. Netfilter

Netfilter provides hooks into the networking stack.

Used by:

```text
Firewall
NAT
Packet filtering
Connection tracking
```

Tools such as nftables use this infrastructure.

---

# 10. TCP Internals

TCP maintains state such as:

```text
Connection state
Sequence numbers
Acknowledgments
Receive window
Congestion control
Retransmission timers
```

Typical state progression:

```text
CLOSED
  |
LISTEN / SYN-SENT
  |
SYN-RECEIVED
  |
ESTABLISHED
```

---

# 11. Socket Queues

Sockets may contain queues for:

```text
Receive
Transmit
Pending connections
```

Applications consume data through:

```text
recv()
read()
```

---

# Interview Points

* Linux networking is layered.
* `sk_buff` is the central packet buffer structure.
* NAPI reduces interrupt overhead under high packet rates.
* Receive path begins at NIC and eventually reaches a socket.
* Transmit path begins at socket and eventually reaches NIC.
* Netfilter provides packet-processing hooks.
* Routing determines packet forwarding/interface decisions.
* TCP maintains connection state, sequencing, retransmission, flow control, and congestion control.

---

# Chapter 22 — Block Layer + Storage

## Objectives

Understand:

* Block devices
* Block layer
* BIO
* Request
* I/O scheduler
* Device mapper
* Filesystem to storage flow
* Page cache
* Direct I/O
* NVMe
* Storage stack

---

# 1. Linux Storage Architecture

```text
Application
     |
Filesystem
     |
VFS
     |
Page Cache
     |
Filesystem
     |
Block Layer
     |
Device Driver
     |
Controller
     |
SSD/HDD/NVMe
```

---

# 2. Block Device

Block devices provide block-oriented storage access.

Examples:

```text
HDD
SSD
NVMe
USB Storage
```

---

# 3. BIO

Linux uses structures such as:

```c
struct bio
```

to represent block I/O operations.

Conceptually:

```text
BIO
 |
 +-- Read / Write
 +-- Sector information
 +-- Memory segments
```

---

# 4. Request

Block I/O requests may be represented and managed through request structures and queues.

Conceptually:

```text
Application
   |
Filesystem
   |
BIO
   |
Block Layer
   |
Request
   |
Driver
```

---

# 5. I/O Scheduler

The block layer may use an I/O scheduler to manage requests.

Goals can include:

```text
Reduce latency
Improve throughput
Merge requests
Optimize ordering
```

Different storage devices benefit from different scheduling strategies.

---

# 6. Page Cache

Buffered file I/O:

```text
Application
     |
Filesystem
     |
Page Cache
     |
Dirty pages
     |
Writeback
     |
Block Layer
```

---

# 7. Direct I/O

Direct I/O attempts to bypass normal page-cache buffering for file data.

Conceptually:

```text
Application
     |
Direct I/O
     |
Filesystem
     |
Block Layer
     |
Storage
```

It is often used by applications that implement their own caching or need specialized I/O behavior.

---

# 8. HDD vs SSD

### HDD

Mechanical:

```text
Platter
Head
Seek
Rotation
```

Latency is strongly influenced by mechanical movement.

### SSD

Flash-based:

```text
No mechanical seek
Lower latency
High parallelism
```

---

# 9. NVMe

NVMe is designed for non-volatile memory such as SSDs and uses PCIe.

Conceptually:

```text
CPU
 |
PCIe
 |
NVMe Controller
 |
SSD NAND
```

NVMe supports multiple queues and high parallelism.

---

# 10. Device Mapper

Device Mapper provides virtual block devices.

Used for technologies such as:

```text
LVM
dm-crypt
Snapshots
```

Conceptually:

```text
Filesystem
   |
Virtual Block Device
   |
Device Mapper
   |
Physical Devices
```

---

# 11. Storage Read Flow

```text
Application
    |
read()
    |
VFS
    |
Page Cache
   / \
 HIT MISS
 |     |
RAM   Filesystem
        |
        v
     Block Layer
        |
        v
     Driver
        |
        v
      Disk
```

---

# 12. Storage Write Flow

```text
Application
    |
write()
    |
Page Cache
    |
Dirty Page
    |
Writeback
    |
Filesystem
    |
Block Layer
    |
Driver
    |
Storage
```

---

# Interview Points

* Block layer abstracts block storage I/O.
* `bio` represents block I/O.
* Filesystems eventually submit I/O to the block layer.
* Page cache accelerates buffered file access.
* Direct I/O can bypass normal page-cache buffering.
* NVMe uses PCIe and supports high parallelism.
* Device Mapper provides virtual block devices.
* I/O schedulers manage block I/O requests.

---

# Chapter 23 — Linux Boot Process

## Objectives

Understand:

* Firmware
* BIOS/UEFI
* Bootloader
* Kernel
* initramfs
* Kernel initialization
* PID 1
* systemd
* Services
* User space startup

---

# 1. Complete Boot Flow

```text
Power On
   |
   v
Firmware
BIOS / UEFI
   |
   v
Bootloader
GRUB
   |
   v
Linux Kernel
   |
   v
initramfs
   |
   v
Kernel Initialization
   |
   v
PID 1
systemd
   |
   v
Services
   |
   v
Login / User Space
```

---

# 2. Firmware

Firmware initializes basic hardware.

Examples:

```text
BIOS
UEFI
```

It performs early hardware initialization and identifies a boot target.

---

# 3. Bootloader

Common Linux bootloader:

```text
GRUB
```

Responsibilities:

```text
Select kernel
Load kernel
Load initramfs
Pass kernel parameters
Transfer control to kernel
```

---

# 4. Kernel Image

Bootloader loads the Linux kernel into memory.

Kernel then begins execution.

Conceptually:

```text
Bootloader
    |
    v
Kernel Entry Point
    |
    v
Early Kernel Initialization
```

---

# 5. initramfs

initramfs = Initial RAM filesystem.

It provides an early user-space environment before the real root filesystem is mounted.

Why needed?

The kernel may need:

```text
Storage drivers
Filesystem drivers
LVM
RAID
Encryption setup
Other initialization
```

before accessing the real root filesystem.

---

# 6. Root Filesystem

Eventually:

```text
Real Root Filesystem
```

is mounted.

Then boot transitions toward normal user space.

---

# 7. PID 1

The first normal user-space process is:

```text
PID 1
```

On many Linux systems:

```text
systemd
```

is PID 1.

PID 1 is responsible for bringing up user-space services and managing the system lifecycle.

---

# 8. Service Startup

Conceptually:

```text
systemd
   |
   +-- networking
   +-- logging
   +-- SSH
   +-- storage
   +-- other services
```

---

# 9. Kernel Initialization

Kernel initialization includes:

```text
Memory initialization
Scheduler initialization
Interrupt setup
Driver initialization
Filesystem initialization
Networking initialization
Kernel subsystem initialization
```

---

# 10. Boot Parameters

Kernel parameters can influence boot behavior.

Example:

```text
root=
ro
quiet
```

View current command line:

```bash
cat /proc/cmdline
```

---

# 11. Boot Troubleshooting

Useful commands:

```bash
dmesg
journalctl -b
systemctl status
systemctl list-units
```

---

# Interview Points

* Firmware performs early hardware initialization.
* Bootloader loads the kernel and initramfs.
* GRUB is a common Linux bootloader.
* initramfs provides early user space.
* Real root filesystem is mounted after necessary initialization.
* PID 1 is the first normal user-space process.
* `systemd` commonly acts as PID 1.
* Kernel initializes core subsystems before handing control to normal user space.

---

# Chapter 24 — Kernel Debugging + Performance

## Objectives

Understand:

* Kernel logs
* `/proc`
* `/sys`
* `strace`
* `perf`
* ftrace
* eBPF
* kprobes
* printk
* crash analysis
* lock debugging
* CPU profiling
* memory debugging
* I/O debugging
* performance methodology

---

# 1. Why Kernel Debugging Is Different

Kernel bugs can cause:

```text
System crash
Kernel panic
Deadlock
Data corruption
Memory corruption
Performance degradation
```

Debugging requires observing:

```text
CPU
Memory
Processes
Locks
Interrupts
I/O
Networking
Kernel execution
```

---

# 2. printk

Kernel code can log messages using:

```c
printk()
```

Modern kernel code commonly uses logging helpers such as:

```c
pr_info()
pr_err()
pr_warn()
```

Messages can be viewed using:

```bash
dmesg
```

---

# 3. `/proc`

`/proc` exposes process and kernel runtime information.

Examples:

```text
/proc/cpuinfo
/proc/meminfo
/proc/stat
/proc/loadavg
/proc/<pid>/
/proc/interrupts
```

Examples:

```bash
cat /proc/cpuinfo
cat /proc/meminfo
cat /proc/interrupts
```

---

# 4. `/sys`

`sysfs` exposes kernel objects and device information.

Examples:

```text
/sys/class/
/sys/devices/
/sys/block/
/sys/kernel/
```

---

# 5. strace

`strace` traces system calls.

Example:

```bash
strace ./program
```

Flow:

```text
Application
    |
system calls
    |
strace
```

Useful for:

```text
File access
Process creation
Networking
Signals
System-call failures
```

Example:

```bash
strace -f ./program
```

`-f` follows child processes/threads as applicable.

---

# 6. perf

`perf` is a Linux performance analysis tool.

Examples:

```bash
perf stat ./program
perf record ./program
perf report
```

Can measure/profile:

```text
CPU cycles
Instructions
Cache misses
Branches
Context switches
CPU sampling
```

---

# 7. ftrace

ftrace is a kernel tracing framework.

Useful for:

```text
Function tracing
Kernel events
Scheduling
Interrupts
Latency
```

Conceptually:

```text
Kernel
 |
ftrace
 |
Execution trace
```

---

# 8. eBPF

eBPF allows programmable instrumentation and networking functionality inside the kernel execution environment.

Common uses:

```text
Tracing
Observability
Networking
Security
Performance analysis
```

Conceptually:

```text
Kernel Events
     |
     v
    eBPF
     |
     v
Observability Data
```

Tools/ecosystem include:

```text
bpftool
bpftrace
BCC
```

---

# 9. kprobes

Kprobes allow instrumentation at kernel function locations.

Conceptually:

```text
Kernel Function
      |
    kprobe
      |
Tracing / Debugging
```

Useful when you want to observe kernel execution without permanently modifying the function implementation.

---

# 10. Kernel Panic

Kernel panic occurs when the kernel reaches an unrecoverable condition.

Conceptually:

```text
Fatal Kernel Error
       |
       v
Kernel Panic
       |
       v
System stops/restarts
```

Useful information:

```text
Call trace
Registers
CPU
Process
Kernel message
```

---

# 11. Deadlock Debugging

Potential symptoms:

```text
Process stuck
CPU idle
Lock never released
System latency
```

Important concepts:

```text
Lock ordering
Lock dependency
Circular wait
```

Linux provides debugging infrastructure such as:

```text
lockdep
```

to detect problematic lock dependencies.

---

# 12. Memory Debugging

Common problems:

```text
Use-after-free
Double-free
Buffer overflow
Memory leak
Invalid pointer
Race condition
```

Useful kernel facilities/tools can include:

```text
KASAN
kmemleak
KFENCE
SLUB debugging
```

---

# 13. CPU Performance Analysis

Important metrics:

```text
CPU utilization
IPC
Context switches
Cache misses
Interrupts
Scheduler latency
```

Useful tools:

```bash
top
htop
vmstat
mpstat
perf
```

---

# 14. Memory Performance

Useful commands:

```bash
free
vmstat
cat /proc/meminfo
sar
```

Look for:

```text
Page faults
Swap activity
Reclaim
Memory pressure
Cache usage
```

---

# 15. I/O Performance

Useful tools:

```bash
iostat
iotop
vmstat
pidstat
```

Look for:

```text
IOPS
Throughput
Latency
Queue depth
CPU wait
```

---

# 16. Network Performance

Useful tools:

```bash
ss
ip
ethtool
tcpdump
nstat
```

Look for:

```text
Packet drops
Retransmissions
Socket queues
Bandwidth
Latency
NIC errors
```

---

# 17. Kernel Debugging Methodology

Do not randomly inspect everything.

Use:

```text
1. Reproduce
      |
2. Define symptom
      |
3. Collect evidence
      |
4. Identify subsystem
      |
5. Trace execution
      |
6. Find root cause
      |
7. Fix
      |
8. Reproduce again
      |
9. Measure improvement
```

---

# 18. Performance Methodology

Always distinguish:

```text
Latency
Throughput
CPU utilization
Memory usage
I/O
Contention
```

Example:

```text
Application slow
      |
      +-- CPU?
      |
      +-- Memory?
      |
      +-- Disk?
      |
      +-- Network?
      |
      +-- Lock contention?
      |
      +-- Scheduler?
```

---

# 19. Golden Rule of Performance

Do not optimize based only on assumptions.

Use:

```text
Measure
  |
Profile
  |
Identify bottleneck
  |
Optimize
  |
Measure again
```

---

# 20. Important Debugging Tool Map

| Problem            | Useful Tools  |
| ------------------ | ------------- |
| System calls       | strace        |
| CPU profiling      | perf          |
| Kernel tracing     | ftrace        |
| Dynamic tracing    | eBPF          |
| Kernel logs        | dmesg         |
| Process info       | /proc         |
| Device/kernel info | /sys          |
| CPU usage          | top, mpstat   |
| Memory             | free, vmstat  |
| Disk I/O           | iostat, iotop |
| Network            | ss, tcpdump   |
| Lock issues        | lockdep       |
| Memory bugs        | KASAN, KFENCE |
| Kernel leaks       | kmemleak      |

---

# Interview Points

* `dmesg` shows kernel messages.
* `/proc` exposes runtime process/kernel information.
* `/sys` exposes kernel objects and devices.
* `strace` traces system calls.
* `perf` is used for performance profiling and hardware/software event analysis.
* ftrace provides kernel tracing.
* eBPF provides programmable tracing/observability and networking capabilities.
* Kprobes dynamically instrument kernel functions.
* lockdep helps detect locking problems.
* KASAN detects many classes of memory safety bugs.
* Performance optimization should always be measurement-driven.

---

# PART 2 — COMPLETE KERNEL EXECUTION PICTURE

After Chapters 14–24, the major Linux kernel components can be connected together:

```text
                    USER SPACE
+------------------------------------------------------+
| Applications                                         |
|                                                      |
| Chrome | Python | GCC | Shell | Services            |
+------------------------------------------------------+
                       |
                       | System Calls
                       v
+------------------------------------------------------+
|                    VFS / SOCKET                     |
|                                                      |
| Files | Sockets | IPC                               |
+------------------------------------------------------+
                       |
                       v
+------------------------------------------------------+
|                 LINUX KERNEL                        |
|                                                      |
| +----------------+  +-----------------------------+ |
| | Process        |  | Scheduler                   | |
| | Management     |  |                             | |
| +----------------+  +-----------------------------+ |
|                                                      |
| +----------------+  +-----------------------------+ |
| | Memory         |  | Networking                  | |
| | Management     |  | Stack                       | |
| +----------------+  +-----------------------------+ |
|                                                      |
| +----------------+  +-----------------------------+ |
| | VFS            |  | Block Layer                 | |
| +----------------+  +-----------------------------+ |
|                                                      |
| +----------------+  +-----------------------------+ |
| | Interrupts     |  | Device Drivers              | |
| +----------------+  +-----------------------------+ |
|                                                      |
| +--------------------------------------------------+ |
| | Synchronization / RCU / Workqueues / Modules     | |
| +--------------------------------------------------+ |
+------------------------------------------------------+
                       |
                       v
+------------------------------------------------------+
|                    HARDWARE                         |
|                                                      |
| CPU | RAM | NIC | SSD | HDD | USB | GPU | Devices  |
+------------------------------------------------------+
```

---

# COMPLETE APPLICATION → KERNEL → HARDWARE FLOWS

## 1. Process Execution

```text
Application
    |
    v
fork()
    |
    v
task_struct
    |
    v
Scheduler
    |
    v
CPU
```

---

## 2. Memory Access

```text
Application
    |
Virtual Address
    |
Page Tables
    |
TLB
    |
Physical Address
    |
RAM
```

On a missing mapping:

```text
Page Access
    |
TLB Miss / Page Fault
    |
Kernel
    |
Page Fault Handler
    |
Memory Manager
    |
Page Allocation / Disk
    |
Page Table Update
    |
Resume Process
```

---

## 3. File Read

```text
Application
    |
read()
    |
System Call
    |
VFS
    |
File Object
    |
Page Cache
   /     \
 HIT     MISS
 |         |
RAM      Filesystem
            |
            v
        Block Layer
            |
            v
        Driver
            |
            v
          Disk
```

---

## 4. Network Receive

```text
Network
   |
   v
NIC
   |
   v
DMA
   |
   v
Interrupt
   |
   v
NAPI
   |
   v
Network Driver
   |
   v
sk_buff
   |
   v
IP
   |
   v
TCP / UDP
   |
   v
Socket
   |
   v
Application
```

---

## 5. Network Send

```text
Application
    |
send()
    |
Socket
    |
TCP / UDP
    |
IP
    |
Network Device Layer
    |
Driver
    |
DMA
    |
NIC
    |
Network
```

---

## 6. Storage Write

```text
Application
    |
write()
    |
VFS
    |
Page Cache
    |
Dirty Page
    |
Writeback
    |
Filesystem
    |
BIO
    |
Block Layer
    |
Driver
    |
Storage Controller
    |
SSD / HDD
```

---

## 7. Hardware Interrupt

```text
Hardware
    |
    v
IRQ
    |
    v
CPU
    |
    v
Interrupt Handler
    |
    +---- Immediate Work
    |
    +---- Deferred Work
              |
              +-- Softirq
              +-- Workqueue
              +-- Threaded IRQ
```

---

# LINUX KERNEL OBJECT MAP

```text
Process
   |
   +--> task_struct
   |
   +--> mm_struct
   |       |
   |       +--> VMAs
   |       +--> Page Tables
   |
   +--> files_struct
   |       |
   |       +--> FD Table
   |
   +--> fs_struct
   |
   +--> credentials
   |
   +--> kernel stack
```

Filesystem:

```text
Mount
 |
super_block
 |
 +-- inode
 |     |
 |     +-- metadata
 |
 +-- dentry
       |
       +-- filename
       |
       +-- inode

Open File
 |
struct file
 |
file_operations
```

Networking:

```text
Socket
 |
TCP/UDP
 |
IP
 |
sk_buff
 |
Network Device
 |
Driver
 |
NIC
```

Storage:

```text
Filesystem
 |
BIO
 |
Block Layer
 |
Request
 |
Driver
 |
Controller
 |
Storage
```

---

# KERNEL CONTEXTS

One of the most important interview concepts:

```text
                KERNEL EXECUTION
                       |
          +------------+-------------+
          |                          |
   Process Context             Interrupt Context
          |                          |
    Associated task             No sleeping
          |                          |
    Can generally sleep        Very short work
          |                          |
    Can access process         Deferred work
    context                     |
          |                  +--------+--------+
          |                  |        |        |
          |                Softirq Workqueue Threaded IRQ
```

Remember:

```text
Process context
    -> may sleep when allowed

Hard interrupt context
    -> must not sleep
```

---

# MOST IMPORTANT LINUX KERNEL DATA STRUCTURES

```text
task_struct
    -> Process/Thread

mm_struct
    -> Address space

vm_area_struct
    -> Virtual memory region

page
    -> Physical memory page

inode
    -> Filesystem object metadata

dentry
    -> Filename/path lookup object

file
    -> Open file instance

super_block
    -> Mounted filesystem

sk_buff
    -> Network packet

bio
    -> Block I/O

file_operations
    -> File/device operation callbacks
```

---

# MOST IMPORTANT KERNEL SUBSYSTEMS

```text
                    Linux Kernel
                         |
       +-----------------+-----------------+
       |                 |                 |
 Process Management   Memory            Scheduler
       |             Management             |
       |                 |                  CPU
       |                 |
       +-----------------+-----------------+
                         |
              +----------+----------+
              |                     |
             VFS               Networking
              |                     |
         Filesystems            Sockets
              |                     |
         Block Layer             NIC
              |
          Storage
```

---

# IMPORTANT INTERVIEW DIFFERENCES

## Process vs Thread

```text
Process
    -> Own address space

Thread
    -> Execution unit
    -> Threads of same process share address space
```

---

## fork vs exec

```text
fork()
    -> Creates child

exec()
    -> Replaces current program image
```

---

## inode vs dentry

```text
inode
    -> Metadata/object

dentry
    -> Filename/path lookup
```

---

## file descriptor vs file object

```text
FD
    -> Integer in process

struct file
    -> Kernel open-file object
```

---

## VMA vs Page

```text
VMA
    -> Virtual address range

Page
    -> Physical memory unit
```

---

## Mutex vs Spinlock

```text
Mutex
    -> May sleep

Spinlock
    -> Spins
    -> No sleeping while held
```

---

## Softirq vs Workqueue

```text
Softirq
    -> Deferred atomic-context work
    -> Cannot sleep

Workqueue
    -> Runs in worker thread/process context
    -> Can generally sleep
```

---

## Kernel Space vs User Space

```text
User Space
    -> Applications
    -> Restricted access

Kernel Space
    -> Kernel
    -> Drivers
    -> Memory management
    -> Scheduler
    -> Direct hardware control
```

---

# FINAL LINUX EXECUTION MODEL

The complete mental model should be:

```text
                         APPLICATION
                              |
                              |
                       System Call / API
                              |
                              v
+------------------------------------------------------+
|                    KERNEL SPACE                      |
|                                                      |
|  Process        Scheduler       Memory               |
|  Management     Management      Management           |
|                                                      |
|  VFS            Networking      IPC                  |
|                                                      |
|  Filesystems    Block Layer     Security             |
|                                                      |
|  Interrupts     Drivers         Synchronization      |
|                                                      |
+------------------------------------------------------+
          |              |              |
          v              v              v
        CPU             RAM          DEVICES
                                       |
                           +-----------+-----------+
                           |           |           |
                          NIC         SSD         USB
```

The most important idea is:

```text
Application
    |
    v
System Call
    |
    v
Kernel Subsystem
    |
    v
Driver
    |
    v
Hardware
```

And in the opposite direction:

```text
Hardware Event
    |
    v
Interrupt / DMA
    |
    v
Driver
    |
    v
Kernel Subsystem
    |
    v
User Space
```

---

# FINAL INTERVIEW CHECKLIST

## Operating System Fundamentals

* [ ] Process vs thread
* [ ] CPU scheduling
* [ ] Synchronization
* [ ] Deadlock
* [ ] Virtual memory
* [ ] Page tables
* [ ] TLB
* [ ] Page faults
* [ ] Memory allocation
* [ ] IPC
* [ ] Filesystems
* [ ] I/O
* [ ] Networking

## Linux Internals

* [ ] Kernel modules
* [ ] Device drivers
* [ ] VFS
* [ ] inode
* [ ] dentry
* [ ] superblock
* [ ] file object
* [ ] task_struct
* [ ] mm_struct
* [ ] VMAs
* [ ] Buddy allocator
* [ ] SLUB
* [ ] Page cache
* [ ] Scheduler
* [ ] CFS
* [ ] Context switching
* [ ] Interrupts
* [ ] Softirq
* [ ] Workqueue
* [ ] Spinlocks
* [ ] Mutex
* [ ] RCU
* [ ] Linux networking
* [ ] sk_buff
* [ ] NAPI
* [ ] Block layer
* [ ] BIO
* [ ] Device Mapper
* [ ] Boot process
* [ ] initramfs
* [ ] PID 1
* [ ] Kernel debugging
* [ ] perf
* [ ] ftrace
* [ ] eBPF
* [ ] strace
* [ ] KASAN
* [ ] lockdep

---

# FINAL MENTAL MODEL

If you remember only one architecture, remember this:

```text
                         USER SPACE
+------------------------------------------------------+
| Applications                                         |
|                                                      |
| Chrome | Shell | Python | GCC | Services             |
+------------------------------------------------------+
                         |
                         | System Calls
                         v
+------------------------------------------------------+
|                       VFS                            |
|                       IPC                            |
|                     SOCKETS                          |
+------------------------------------------------------+
                         |
                         v
+------------------------------------------------------+
|                    LINUX KERNEL                      |
|                                                      |
| Process Management       Scheduler                   |
| Memory Management        Networking                  |
| Filesystems              Block Layer                 |
| Interrupts               Synchronization             |
| Security                 Device Drivers              |
+------------------------------------------------------+
                         |
                         v
+------------------------------------------------------+
|                     HARDWARE                         |
|                                                      |
| CPU | RAM | NIC | SSD | HDD | USB | GPU | Devices  |
+------------------------------------------------------+
```

**Core rule:**

```text
User program
     ↓
System call
     ↓
Kernel subsystem
     ↓
Driver
     ↓
Hardware
```

**Hardware event:**

```text
Hardware
     ↓
Interrupt / DMA
     ↓
Driver
     ↓
Kernel subsystem
     ↓
Application
```

This is the foundation for understanding Linux internals, containers, Docker, Kubernetes, performance engineering, and kernel-level debugging.
