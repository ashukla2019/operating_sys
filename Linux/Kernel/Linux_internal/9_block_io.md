# Chapter 6 – Linux Block I/O

---

# Objectives

After completing this chapter, you should understand:

* What block I/O is
* Block devices vs character devices
* Linux block layer
* BIO
* Requests
* Request queues
* I/O schedulers
* Buffered I/O
* Direct I/O
* Page cache interaction
* Read and write paths
* DMA
* Interrupt-driven I/O
* NVMe vs SATA at a high level
* I/O completion
* Important interview questions

---

# 1. What is Block I/O?

Block I/O is the mechanism Linux uses to communicate with storage devices that operate on blocks of data.

Examples:

```text
HDD
SSD
NVMe SSD
USB storage
eMMC
SD card
```

These devices are generally accessed through the Linux block layer.

---

# 2. What is a Block Device?

A block device provides storage that can be accessed in units of blocks/sectors.

Examples:

```text
/dev/sda
/dev/sdb
/dev/nvme0n1
/dev/mmcblk0
```

Conceptually:

```text
Application
    |
    v
Filesystem
    |
    v
Block Layer
    |
    v
Block Device Driver
    |
    v
Storage Device
```

---

# 3. Block Device vs Character Device

This is an important interview question.

## Block Device

Designed for block-oriented storage.

Examples:

```text
HDD
SSD
NVMe
eMMC
```

## Character Device

Provides a stream-oriented interface.

Examples:

```text
Serial port
Terminal
Some sensors
Some device drivers
```

Conceptually:

```text
Block Device

Data
+----+----+----+----+
| B0 | B1 | B2 | B3 |
+----+----+----+----+
```

Character device:

```text
Data stream

A → B → C → D → E → F
```

---

# 4. Why Do We Need the Block Layer?

Different storage devices have different hardware interfaces.

For example:

```text
SATA
NVMe
USB Storage
eMMC
```

Linux applications should not need to know these hardware details.

The block layer provides a common abstraction.

```text
                VFS
                 |
                 v
             Filesystem
                 |
                 v
             Block Layer
             /    |    \
            /     |     \
         SATA   NVMe   USB
           |      |      |
          SSD    SSD    Disk
```

---

# 5. High-Level Storage Stack

A useful mental model:

```text
Application
     |
     v
System Call
     |
     v
VFS
     |
     v
Filesystem
     |
     v
Page Cache
     |
     v
Block Layer
     |
     v
I/O Request
     |
     v
Block Device Driver
     |
     v
Hardware
```

For direct I/O, the page-cache path can be bypassed.

---

# 6. Buffered Read

Consider:

```c
read(fd, buffer, 4096);
```

Simplified flow:

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
Page Cache
     |
   +---+---+
   |       |
  Hit     Miss
   |       |
   |       v
   |   Filesystem
   |       |
   |       v
   |   Block Layer
   |       |
   |       v
   |   Device Driver
   |       |
   |       v
   |     SSD
   |       |
   +-------+
       |
       v
   User Buffer
```

---

# 7. Buffered Write

Consider:

```c
write(fd, buffer, 4096);
```

Simplified:

```text
Application
     |
     v
write()
     |
     v
VFS
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
Device Driver
     |
     v
Storage
```

The write does not necessarily reach the physical device immediately.

---

# 8. Direct I/O

Applications can request direct I/O using mechanisms such as:

```text
O_DIRECT
```

Conceptually:

```text
Application
     |
     v
Direct I/O
     |
     v
Filesystem
     |
     v
Block Layer
     |
     v
Driver
     |
     v
Storage
```

The page cache is generally bypassed for the file data path.

Direct I/O has alignment and filesystem-specific restrictions.

---

# 9. Why Use Direct I/O?

Potential reasons include:

* Database workloads
* Applications with their own caching
* Avoiding double buffering
* Predictable I/O behavior in some workloads

But direct I/O is not automatically faster.

It increases application responsibility for:

* Alignment
* Buffer management
* Caching
* I/O behavior

---

# 10. What is a BIO?

`BIO` is a kernel structure used to represent an I/O operation at the block layer.

Conceptually:

```text
BIO
 |
 +-- Operation
 |    |
 |    +-- READ
 |    +-- WRITE
 |
 +-- Sector information
 |
 +-- Data segments
 |
 +-- Completion information
```

A BIO describes the data involved in an I/O operation.

---

# 11. BIO Mental Model

Suppose the filesystem needs to read several sectors.

Conceptually:

```text
Filesystem
    |
    v
   BIO
    |
    +-- READ
    +-- Starting sector
    +-- Data pages/segments
    |
    v
Block Layer
```

The block layer processes the I/O and eventually sends it toward the device driver.

---

# 12. BIO Is Not the Physical Device Request

This is an important distinction.

A BIO represents an I/O operation at a particular layer.

The block layer may combine, split, transform, or schedule I/O before it reaches the hardware.

Conceptually:

```text
Filesystem
    |
    v
BIO
    |
    v
Block Layer
    |
    v
Request
    |
    v
Driver
    |
    v
Hardware
```

The exact internal path varies by kernel version and block architecture.

---

# 13. Request

A block request represents work that the block layer sends toward a device queue.

Conceptually:

```text
BIO
  |
  v
Block Layer
  |
  v
Request
  |
  v
Device Queue
```

Multiple BIOs may be associated with a request depending on the I/O path and whether they can be merged.

---

# 14. I/O Request Flow

Simplified:

```text
Application
     |
     v
Filesystem
     |
     v
BIO
     |
     v
Block Layer
     |
     v
Request
     |
     v
Device Queue
     |
     v
Driver
     |
     v
Hardware
```

This is the core block-I/O mental model.

---

# 15. Request Queue

The block layer manages I/O through queues associated with block devices.

Conceptually:

```text
              Block Device
                   |
                   v
              Request Queue
             /     |      \
            /      |       \
         READ    WRITE    READ
```

The queue allows the kernel and driver to manage outstanding operations.

---

# 16. Why Queue I/O?

Storage devices can process multiple operations.

Instead of:

```text
READ
wait
WRITE
wait
READ
wait
```

the system can maintain multiple outstanding requests.

```text
READ
WRITE
READ
WRITE
READ
```

This allows better utilization of modern storage devices.

---

# 17. I/O Scheduling

Linux can use I/O scheduling mechanisms to manage block requests.

Goals may include:

* Throughput
* Latency
* Fairness
* Request merging
* Device utilization

Historically Linux used schedulers such as:

```text
CFQ
Deadline
NOOP
```

Modern Linux also uses:

```text
mq-deadline
BFQ
none
```

depending on kernel/device configuration.

---

# 18. Why Multiple I/O Schedulers?

Different workloads have different requirements.

For example:

```text
Desktop
Server
Database
Embedded system
NVMe storage
```

may benefit from different scheduling behavior.

---

# 19. I/O Scheduler Example

Suppose requests arrive:

```text
READ sector 100
READ sector 101
READ sector 5000
READ sector 102
```

A scheduler may reorder or merge operations where appropriate.

Conceptually:

```text
Before:

100
101
5000
102

After:

100
101
102
5000
```

The exact behavior depends on the scheduler and device.

---

# 20. Request Merging

Suppose:

```text
Request A:

READ sectors 100-103


Request B:

READ sectors 104-107
```

These may be merged into:

```text
READ sectors 100-107
```

Conceptually:

```text
Request A + Request B
          |
          v
       Combined
          |
          v
      Device I/O
```

Merging can reduce overhead.

---

# 21. Random vs Sequential I/O

## Sequential

```text
100
101
102
103
104
```

Data is accessed continuously.

## Random

```text
100
5000
72
9000
301
```

Accesses are scattered.

Historically, HDDs benefited significantly from request ordering because of seek time.

Modern SSD/NVMe devices have very different characteristics.

---

# 22. HDD vs SSD

## HDD

Uses:

```text
Mechanical head
Rotating platters
Seek
Rotation
```

Random I/O can be expensive.

## SSD

Uses:

```text
Flash memory
No mechanical seek
```

Much lower random-access latency.

---

# 23. NVMe

NVMe is a protocol designed for high-performance non-volatile storage, especially PCIe-connected SSDs.

Conceptually:

```text
CPU
 |
 v
PCIe
 |
 v
NVMe Controller
 |
 v
Flash
```

NVMe supports many queues and high concurrency.

---

# 24. SATA vs NVMe

Simplified:

```text
SATA:

CPU
 |
 v
SATA Controller
 |
 v
SATA SSD
```

NVMe:

```text
CPU
 |
 v
PCIe
 |
 v
NVMe Controller
 |
 v
NVMe SSD
```

NVMe is designed for much higher parallelism and lower protocol overhead.

---

# 25. DMA

DMA stands for:

```text
Direct Memory Access
```

DMA allows a device to transfer data to/from memory without the CPU copying every byte.

Conceptually:

```text
Without DMA:

Device
  |
  v
CPU
  |
  v
RAM
```

With DMA:

```text
Device
   |
   | DMA
   v
 RAM
```

The CPU configures the operation and handles setup/completion rather than manually copying every byte.

---

# 26. Why DMA Is Important

Suppose a network card receives:

```text
1 MB
```

Without DMA:

```text
NIC
 |
 v
CPU copies data
 |
 v
RAM
```

The CPU spends significant effort moving data.

With DMA:

```text
NIC
 |
 | DMA
 v
RAM

CPU
 |
 +-- Configure DMA
 +-- Handle completion
```

This improves efficiency.

---

# 27. Storage + DMA

For a storage read:

```text
SSD
 |
 v
Controller
 |
 | DMA
 v
RAM
 |
 v
Kernel
 |
 v
Application
```

The device/controller transfers data directly into memory.

---

# 28. Interrupts and I/O Completion

After an I/O operation completes, the device needs to notify the CPU.

One mechanism is an interrupt.

Conceptually:

```text
CPU
 |
 | submits I/O
 v
Storage Device
 |
 | performs operation
 v
Completion
 |
 v
Interrupt
 |
 v
CPU
 |
 v
Kernel handles completion
```

---

# 29. Interrupt + DMA

A common high-level flow:

```text
1. CPU prepares I/O
        |
        v
2. Device is programmed
        |
        v
3. Device performs DMA
        |
        v
4. Data reaches RAM
        |
        v
5. Device signals completion
        |
        v
6. Interrupt
        |
        v
7. Kernel processes completion
```

---

# 30. Polling vs Interrupts

Devices can sometimes be handled using polling rather than interrupts.

## Interrupt

```text
Device
   |
   | interrupt
   v
CPU
```

CPU does not continuously check the device.

## Polling

```text
CPU
 |
 +-- Check?
 |
 +-- Check?
 |
 +-- Check?
```

Polling can be useful for very high event rates because interrupt overhead can become expensive.

---

# 31. High-Level Block Read Path

A useful interview diagram:

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
Filesystem
     |
     v
Page Cache
     |
     | miss
     v
    BIO
     |
     v
Block Layer
     |
     v
Request
     |
     v
Device Queue
     |
     v
Driver
     |
     v
Storage Controller
     |
     v
Storage
```

---

# 32. Read Completion

After the device completes:

```text
Storage
   |
   v
DMA
   |
   v
RAM
   |
   v
Completion
   |
   v
Interrupt / polling
   |
   v
Block Layer
   |
   v
Filesystem
   |
   v
Page Cache
   |
   v
read() completes
   |
   v
Application
```

---

# 33. High-Level Block Write Path

```text
Application
     |
     v
write()
     |
     v
VFS
     |
     v
Filesystem
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
BIO
     |
     v
Block Layer
     |
     v
Request
     |
     v
Driver
     |
     v
Storage
```

---

# 34. Direct I/O Path

With direct I/O:

```text
Application
     |
     v
Direct I/O
     |
     v
Filesystem
     |
     v
Block Layer
     |
     v
BIO
     |
     v
Request
     |
     v
Driver
     |
     v
Storage
```

The normal file-data page-cache path is bypassed.

---

# 35. I/O Completion

A simplified completion model:

```text
I/O submitted
     |
     v
Device processing
     |
     v
DMA
     |
     v
Data in memory
     |
     v
Completion event
     |
     v
Kernel
     |
     v
Complete BIO/request
     |
     v
Wake waiting task
     |
     v
System call returns
```

---

# 36. Blocking I/O

Suppose a process executes:

```c
read(fd, buffer, 4096);
```

and data is unavailable.

The process may sleep:

```text
Process
   |
   v
read()
   |
   v
Waiting for I/O
   |
   v
SLEEPING
```

When the I/O completes:

```text
I/O Completion
      |
      v
Wake Process
      |
      v
RUNNABLE
      |
      v
RUNNING
```

---

# 37. Nonblocking I/O

A file descriptor may be configured for nonblocking operation.

Example:

```text
O_NONBLOCK
```

Instead of waiting indefinitely:

```text
read()
   |
   v
No data
   |
   v
Return immediately
```

The exact return/error behavior depends on the object and operation.

---

# 38. Synchronous vs Asynchronous I/O

## Synchronous

The caller waits for the operation to complete.

```text
Application
    |
    v
I/O
    |
   wait
    |
    v
completion
    |
    v
return
```

## Asynchronous

The application can continue while the I/O progresses.

Conceptually:

```text
Application
    |
    +---- submit I/O
    |
    +---- continue work
    |
    +---- receive completion
```

Linux provides several mechanisms for asynchronous I/O.

---

# 39. Important Distinction

Do not confuse:

```text
Nonblocking I/O
```

with:

```text
Asynchronous I/O
```

Nonblocking means:

```text
Do not wait if the operation cannot proceed immediately.
```

Asynchronous I/O means:

```text
Submit the operation and receive completion separately.
```

They are related but not identical concepts.

---

# 40. Block Layer and Filesystem

The filesystem determines what storage operations are needed.

Example:

```text
Application
     |
     v
read()
     |
     v
ext4
     |
     v
Determine required blocks
     |
     v
BIO
     |
     v
Block Layer
```

The block layer does not understand the full meaning of the file.

It primarily handles block-device I/O.

---

# 41. Storage Stack Mental Model

Memorize:

```text
Application
     |
     v
System Call
     |
     v
VFS
     |
     v
Filesystem
     |
     v
Page Cache
     |
     v
BIO
     |
     v
Block Layer
     |
     v
Request
     |
     v
Driver
     |
     v
Controller
     |
     v
Storage
```

For a page-cache hit, the lower part may not be needed.

---

# 42. Example: Reading a File

Suppose:

```text
file.txt
```

is stored on an NVMe SSD.

Application:

```c
read(fd, buffer, 4096);
```

Flow:

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
Filesystem
     |
     v
Page Cache
     |
     | miss
     v
BIO
     |
     v
Block Layer
     |
     v
NVMe Driver
     |
     v
PCIe
     |
     v
NVMe Controller
     |
     v
Flash
```

Completion:

```text
Flash
  |
  v
NVMe Controller
  |
  | DMA
  v
RAM
  |
  v
Completion
  |
  v
Application
```

---

# 43. Important Interview Question

## What is the Linux block layer?

It is the kernel subsystem that provides generic block-device I/O infrastructure between filesystems and block-device drivers.

---

# 44. Important Interview Question

## What is a BIO?

A BIO represents an I/O operation at the block layer, describing the operation and associated data segments.

---

# 45. Important Interview Question

## What is a request?

A request represents block-layer work being processed toward a block device. Depending on the I/O path, it can contain or be associated with one or more BIOs.

---

# 46. Important Interview Question

## Why do we need an I/O scheduler?

To manage outstanding block I/O and potentially improve:

```text
Throughput
Latency
Fairness
Request merging
Device utilization
```

---

# 47. Important Interview Question

## Why is DMA used?

To allow devices to transfer data directly between the device and memory without requiring the CPU to copy every byte.

---

# 48. Important Interview Question

## What happens when a disk read completes?

High-level:

```text
Device
   |
   v
DMA → RAM
   |
   v
Completion
   |
   v
Interrupt/polling
   |
   v
Kernel
   |
   v
Complete I/O
   |
   v
Wake waiting task
```

---

# 49. Important Interview Question

## Why is NVMe faster than traditional SATA storage?

NVMe is designed for high-performance storage over PCIe and supports substantial parallelism with multiple queues and lower protocol overhead.

---

# 50. Important Interview Question

## What is the difference between buffered I/O and direct I/O?

Buffered I/O:

```text
Application
    |
    v
Page Cache
    |
    v
Storage
```

Direct I/O:

```text
Application
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

Direct I/O generally bypasses the normal page-cache data path.

---

# 51. Important Interview Question

## Why can a process sleep during I/O?

If required data is not immediately available, a blocking operation can put the task to sleep instead of wasting CPU cycles.

When the I/O completes:

```text
SLEEPING
   |
   v
Wakeup
   |
   v
RUNNABLE
   |
   v
RUNNING
```

---

# 52. Important Interview Question

## What is the difference between sequential and random I/O?

Sequential:

```text
100
101
102
103
104
```

Random:

```text
100
9000
32
500
7000
```

Sequential I/O is generally easier for storage devices to process efficiently, especially on rotational media.

---

# 53. Senior Interview Whiteboard Flow

You should be able to draw:

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
Filesystem
     |
     v
Page Cache
     |
     | miss
     v
BIO
     |
     v
Block Layer
     |
     v
Request
     |
     v
Device Queue
     |
     v
Driver
     |
     v
Controller
     |
     v
Storage
```

And explain the return path:

```text
Storage
   |
   v
DMA
   |
   v
RAM
   |
   v
Completion
   |
   v
Kernel
   |
   v
Wake Process
   |
   v
read() returns
```

---

# 54. What You Must Remember

### Block device

```text
Storage device accessed through block I/O.
```

### Block layer

```text
Generic kernel infrastructure between filesystem and block-device driver.
```

### BIO

```text
Represents an I/O operation and its data segments.
```

### Request

```text
Block-layer work sent toward a device queue.
```

### I/O scheduler

```text
Manages/schedules block I/O.
```

### DMA

```text
Device ↔ RAM transfer without CPU copying every byte.
```

### Page cache

```text
Caches filesystem data in RAM.
```

---

# 55. Final Mental Model

The complete storage path to remember is:

```text
                    APPLICATION
                         |
                         v
                    System Call
                         |
                         v
                        VFS
                         |
                         v
                    Filesystem
                         |
                         v
                    Page Cache
                     /       \
                   Hit       Miss
                   |           |
                   |           v
                   |          BIO
                   |           |
                   |           v
                   |       Block Layer
                   |           |
                   |           v
                   |        Request
                   |           |
                   |           v
                   |         Driver
                   |           |
                   |           v
                   |       Controller
                   |           |
                   |           v
                   |        Storage
                   |           |
                   |          DMA
                   |           |
                   +-----------+
                         |
                         v
                       RAM
                         |
                         v
                   I/O Completion
                         |
                         v
                    Application
```

---

# Chapter Summary

Linux uses the block layer to provide a common abstraction for block storage devices.

The important concepts are:

```text
Block Device
     ↓
Block Layer
     ↓
BIO
     ↓
Request
     ↓
I/O Queue
     ↓
Driver
     ↓
Controller
     ↓
Storage
```

The most important end-to-end flow is:

```text
Application
     ↓
VFS
     ↓
Filesystem
     ↓
Page Cache
     ↓
BIO
     ↓
Block Layer
     ↓
Request
     ↓
Driver
     ↓
Storage
```

For senior Linux Systems, Storage, Embedded, and Infrastructure interviews, you should be able to explain this flow and clearly distinguish:

```text
BIO
Request
Block Layer
I/O Scheduler
DMA
Interrupt
Page Cache
Buffered I/O
Direct I/O
```

without memorizing kernel source code.
