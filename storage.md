# Linux Storage Internals — 00. Storage Domain Basics

## 1. Why Learn Storage Basics?

For a Senior Linux/C++ Systems, Storage, Backup, Infrastructure, or Distributed Systems interview, you should understand what happens when an application performs:

```cpp
read(fd, buffer, size);
write(fd, buffer, size);
```

The application sees a simple file API.

Underneath, Linux may perform:

```text
Application
    ↓
System Call
    ↓
VFS
    ↓
Filesystem
    ↓
Page Cache
    ↓
Block Layer
    ↓
Device Mapper / RAID
    ↓
SCSI / NVMe
    ↓
Storage Driver
    ↓
PCIe
    ↓
SSD / HDD
```

The purpose of this chapter is to build the vocabulary needed to understand that entire path.

---

# 2. Storage Hierarchy

A useful mental model:

```text
+-----------------------------+
| Application                 |
+-----------------------------+
              |
              v
+-----------------------------+
| POSIX File API              |
| open/read/write/fsync       |
+-----------------------------+
              |
              v
+-----------------------------+
| VFS                         |
+-----------------------------+
              |
              v
+-----------------------------+
| Filesystem                  |
| ext4 / XFS / etc.           |
+-----------------------------+
              |
              v
+-----------------------------+
| Page Cache / Direct I/O     |
+-----------------------------+
              |
              v
+-----------------------------+
| Block Layer                 |
+-----------------------------+
              |
              v
+-----------------------------+
| Device Mapper / RAID / LVM  |
+-----------------------------+
              |
              v
+-----------------------------+
| SCSI / NVMe                 |
+-----------------------------+
              |
              v
+-----------------------------+
| Storage Driver              |
+-----------------------------+
              |
              v
+-----------------------------+
| PCIe                        |
+-----------------------------+
              |
              v
+-----------------------------+
| SSD / HDD                   |
+-----------------------------+
```

Not every I/O passes through every layer exactly as shown. For example, an NVMe device does not use the traditional SCSI command path in the same way a SCSI/SAS disk does.

---

# 3. What Is a Storage Device?

A storage device provides persistent or relatively persistent storage for data.

Common examples:

```text
HDD
SSD
NVMe SSD
SAS disk
SATA disk
USB storage
SAN LUN
virtual disk
cloud block device
```

---

# 4. HDD

HDD = Hard Disk Drive.

Basic physical components:

```text
+-------------------------+
| Platters                |
|      ↓                  |
| Magnetic surface        |
|      ↓                  |
| Read/Write Head         |
|      ↓                  |
| Actuator                |
+-------------------------+
```

Data is stored magnetically.

Important characteristics:

```text
Mechanical movement
Higher latency
Lower random IOPS
Good sequential throughput
```

The major performance cost is mechanical movement.

---

# 5. SSD

SSD = Solid State Drive.

Unlike HDDs, SSDs use flash memory instead of rotating magnetic platters.

Conceptually:

```text
Application
    ↓
SSD controller
    ↓
Flash memory
```

Advantages:

```text
No mechanical movement
Lower latency
Higher IOPS
Better random access
```

But SSDs introduce their own concepts:

```text
NAND flash
Pages
Erase blocks
Flash Translation Layer (FTL)
Garbage collection
Wear leveling
TRIM
Write amplification
```

---

# 6. NVMe

NVMe = Non-Volatile Memory Express.

It is a storage protocol designed primarily for high-speed non-volatile memory such as PCIe SSDs.

Typical architecture:

```text
CPU
 |
PCIe
 |
NVMe Controller
 |
NVMe Queues
 |
NAND Flash
```

Important distinction:

```text
NVMe
    = protocol/interface

SSD
    = storage device
```

An NVMe SSD commonly communicates with the CPU through PCIe.

---

# 7. SATA, SAS and NVMe

At a high level:

| Technology | Typical connection/protocol | Typical use                     |
| ---------- | --------------------------- | ------------------------------- |
| SATA       | SATA                        | consumer/server SATA disks/SSDs |
| SAS        | SAS/SCSI ecosystem          | enterprise storage              |
| NVMe       | PCIe + NVMe                 | high-performance SSDs           |

Do not treat:

```text
SATA = disk
NVMe = SSD
```

as strict definitions.

They describe different aspects of the storage stack.

---

# 8. Block Device

Linux exposes many storage devices as block devices.

Examples:

```text
/dev/sda
/dev/sdb
/dev/nvme0n1
```

A block device provides access to storage in blocks.

Conceptually:

```text
Block 0
Block 1
Block 2
Block 3
...
```

Applications normally do not directly manipulate these blocks.

They usually interact with a filesystem:

```text
Application
    ↓
File
    ↓
Filesystem
    ↓
Block Device
```

---

# 9. Sector vs Block

These terms are often confused.

## Sector

A sector is a unit associated with the storage device/interface.

Historically:

```text
512 bytes
```

Modern devices may use:

```text
4096-byte physical sectors
```

or present 512-byte logical sectors while internally using 4 KiB physical sectors.

## Filesystem Block

A filesystem has its own block/allocation unit.

For example, a filesystem may use:

```text
4 KiB filesystem blocks
```

Do not assume:

```text
sector == filesystem block
```

They are different concepts.

---

# 10. Logical Block Address

Storage devices are commonly addressed using logical block addresses.

```text
LBA 0
LBA 1
LBA 2
LBA 3
...
```

The operating system can issue an I/O such as:

```text
Read:
    starting LBA = X
    number of blocks = N
```

The device translates the logical request into its internal physical representation.

For SSDs, the Flash Translation Layer is involved in mapping logical addresses to NAND locations.

---

# 11. Partition

A physical disk can be divided into partitions.

Example:

```text
/dev/sda
 |
 +---- /dev/sda1
 |
 +---- /dev/sda2
 |
 +---- /dev/sda3
```

A partition represents a range of the disk.

Modern systems commonly use:

```text
GPT
```

Older systems may use:

```text
MBR
```

---

# 12. Filesystem

A filesystem organizes data into files and directories.

Examples:

```text
ext4
XFS
Btrfs
ZFS
NFS
tmpfs
```

The filesystem provides abstractions such as:

```text
File
Directory
Inode
Permissions
Links
Metadata
Allocation
```

Example:

```text
Application
     |
     | open("/data/a.txt")
     v
VFS
     |
     v
ext4
     |
     v
Block device
```

---

# 13. Filesystem vs Block Device

This is a very common interview question.

### Block device

Provides storage blocks.

```text
/dev/nvme0n1
```

### Filesystem

Organizes those blocks into:

```text
directories
files
metadata
allocation structures
```

Therefore:

```text
Block device
    ↓
Filesystem
    ↓
Files
```

is conceptually different from:

```text
Filesystem
    ↓
Block device
```

The filesystem sits on top of a block-oriented storage device in the common case.

---

# 14. Mount

A filesystem becomes accessible through a mount point.

Example:

```bash
mount /dev/nvme0n1p2 /data
```

Conceptually:

```text
/dev/nvme0n1p2
        |
        v
   filesystem
        |
        v
      /data
```

After mounting:

```text
/data/file.txt
```

can be accessed by applications.

---

# 15. VFS

Linux uses VFS to provide a common interface to different filesystems.

```text
                    VFS
                     |
       +-------------+-------------+
       |             |             |
      ext4          XFS           NFS
       |             |             |
       +-------------+-------------+
                     |
                Storage
```

Important VFS objects:

```text
superblock
inode
dentry
file
```

These are fundamental Linux storage concepts.

---

# 16. Inode

An inode represents filesystem metadata for an object.

Conceptually:

```text
inode
 |
 +-- file type
 +-- permissions
 +-- owner
 +-- timestamps
 +-- size
 +-- block mapping
 +-- link count
```

Important:

> The filename itself is not the inode.

A directory entry connects a filename to an inode.

---

# 17. Dentry

Dentry = directory entry.

Conceptually:

```text
"test.txt"
     |
     v
  dentry
     |
     v
  inode
```

For:

```text
/data/test.txt
```

Linux performs pathname lookup through directory components.

Conceptually:

```text
/
 |
 +-- data
      |
      +-- test.txt
```

VFS maintains caches such as the dentry cache to make pathname lookup efficient.

---

# 18. File Descriptor

Applications access opened files using file descriptors.

Example:

```cpp
int fd = open("data.txt", O_RDONLY);
```

The application receives:

```text
fd = 3
```

Conceptually:

```text
Process
   |
   v
File Descriptor Table
   |
   +---- 0 → stdin
   +---- 1 → stdout
   +---- 2 → stderr
   +---- 3 → opened file
```

The file descriptor is an integer handle, not the file itself.

---

# 19. Open File Description

A useful senior-level distinction:

```text
Application
    |
    v
fd
    |
    v
struct file
    |
    v
inode
```

Multiple file descriptors can refer to the same open file description.

For example:

```text
dup()
fork()
```

can result in multiple descriptors referring to the same underlying open file description.

This matters for:

```text
file offset
status flags
sharing
concurrent I/O
```

---

# 20. Page Cache

Linux often caches filesystem data in RAM.

```text
Application
     |
     v
read()
     |
     v
Page Cache
     |
     +---- HIT → return data
     |
     +---- MISS
              |
              v
          Storage I/O
```

### Cache hit

Data is already in memory.

```text
read()
  ↓
page cache
  ↓
data returned
```

### Cache miss

```text
read()
  ↓
page cache miss
  ↓
filesystem
  ↓
block layer
  ↓
device
  ↓
data loaded
  ↓
page cache
  ↓
application
```

This is one of the most important concepts for understanding Linux storage performance.

---

# 21. Buffered I/O

Normal filesystem I/O often uses the page cache.

Conceptually:

```text
Application
    |
    v
Page Cache
    |
    v
Filesystem
    |
    v
Storage
```

Advantages:

```text
Caching
Read-ahead
Write-back
Reduced device accesses
```

---

# 22. Direct I/O

Direct I/O attempts to bypass the normal page-cache path.

A common API is:

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
Filesystem / Block Layer
    |
    v
Storage
```

It does not mean "no kernel involvement."

It means the normal page-cache path is bypassed for the data transfer, subject to filesystem and alignment requirements.

---

# 23. Read Path

A simplified read:

```text
read(fd, buffer, size)
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
       / \
      /   \
   HIT    MISS
    |       |
    |       v
    |   Block Layer
    |       |
    |       v
    |   Storage Driver
    |       |
    |       v
    |     Device
    |       |
    |       v
    |   Data returned
    |       |
    +-------+
            |
            v
       Application
```

---

# 24. Write Path

A simplified buffered write:

```text
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
Block Layer
   |
   v
Storage Driver
   |
   v
Device
```

Important:

> A successful `write()` does not necessarily mean the data has reached stable storage.

This distinction is extremely important.

---

# 25. fsync()

If an application needs stronger durability guarantees, it can use:

```cpp
fsync(fd);
```

Conceptually:

```text
Application
    |
    v
write()
    |
    v
Page Cache
    |
    v
Dirty Data
    |
    v
fsync()
    |
    v
Storage
```

The exact durability semantics depend on the filesystem, storage stack, device behavior, and hardware guarantees.

Senior interviewers often ask:

> What is the difference between `write()` and `fsync()`?

Basic answer:

```text
write()
    = transfer data into the kernel's I/O path

fsync()
    = request that appropriate buffered data/metadata
      be synchronized according to its documented semantics
```

Do not simplify this to "write stores in RAM and fsync writes to disk" because the real stack is more complicated.

---

# 26. Block Layer

The Linux block layer sits between filesystems and block-device drivers.

Conceptually:

```text
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

It handles things such as:

```text
I/O submission
I/O completion
I/O scheduling/merging where applicable
queue management
request handling
```

Modern Linux block I/O uses `bio` and request/queue infrastructure.

For senior interviews, know these names:

```text
bio
request
request queue
blk-mq
```

---

# 27. I/O Request

Suppose the application requests:

```text
Read 16 KB
```

Conceptually:

```text
Application
      |
      v
Filesystem
      |
      v
    bio
      |
      v
Block Layer
      |
      v
 request/queue
      |
      v
Driver
      |
      v
Device
```

The exact internal path depends on kernel version and device type.

---

# 28. I/O Completion

Storage I/O is usually asynchronous internally even when the application uses a blocking API.

Conceptually:

```text
Submit I/O
    |
    v
Device processing
    |
    v
Interrupt / completion mechanism
    |
    v
Driver
    |
    v
Block layer
    |
    v
Filesystem
    |
    v
Waiting task becomes runnable
```

This is important for understanding:

```text
I/O latency
interrupts
DMA
blocking
wakeups
scheduler behavior
```

---

# 29. DMA

DMA = Direct Memory Access.

Instead of the CPU copying every byte between the device and memory, a device can transfer data directly to/from RAM under appropriate configuration.

Conceptually:

```text
             CPU
              |
        configure DMA
              |
              v
        +-----------+
        | DMA/Device|
        +-----------+
              |
              |
              v
             RAM
```

Typical read:

```text
SSD/NIC
   |
   | DMA
   v
RAM
   |
   v
Kernel/Application
```

This greatly reduces CPU overhead compared with CPU-driven byte-by-byte transfers.

---

# 30. Interrupt + DMA

A common I/O flow:

```text
1. CPU submits I/O
2. Driver programs/configures device
3. Device performs DMA
4. Device completes operation
5. Device signals interrupt
6. Driver handles completion
7. Kernel processes completion
8. Waiting task may wake
```

This is a key connection between:

```text
Storage
+
Drivers
+
DMA
+
Interrupts
+
Scheduler
```

---

# 31. RAID

RAID = Redundant Array of Independent Disks.

Common levels:

```text
RAID 0
RAID 1
RAID 5
RAID 6
RAID 10
```

### RAID 0

Striping:

```text
Data:
A B C D

Disk 1: A C
Disk 2: B D
```

Advantages:

```text
High performance
No redundancy
```

Failure of one disk can destroy the array's data.

---

# 32. RAID 1

Mirroring:

```text
Disk 1: A B C D
Disk 2: A B C D
```

Advantages:

```text
Redundancy
Simple recovery
```

Storage efficiency is approximately 50% for a two-disk mirror.

---

# 33. RAID 5

Uses:

```text
Striping + distributed parity
```

Conceptually:

```text
Disk 1: Data
Disk 2: Data
Disk 3: Data
Disk 4: Parity
```

Parity is distributed across disks in actual RAID 5 layouts.

Provides tolerance against one disk failure.

---

# 34. RAID 6

Similar to RAID 5 but provides two independent parity values.

Can tolerate:

```text
2 disk failures
```

at the RAID level.

---

# 35. RAID 10

Combination of:

```text
RAID 1 + RAID 0
```

Conceptually:

```text
       RAID 10
          |
      +---+---+
      |       |
   Mirror   Mirror
    / \       / \
   D1 D2     D3 D4
      \       /
       Stripe
```

Provides good performance and redundancy, at the cost of storage capacity.

---

# 36. LVM

LVM = Logical Volume Manager.

It provides abstraction between physical storage and filesystems.

Conceptually:

```text
Physical Disks
      |
      v
Physical Volumes
      |
      v
Volume Group
      |
      v
Logical Volumes
      |
      v
Filesystem
```

Example:

```text
/dev/sda
/dev/sdb
    |
    v
   LVM
    |
    +---- /dev/vg0/data
    +---- /dev/vg0/logs
```

Benefits include:

```text
Flexible volume management
Resize operations
Snapshots
Pooling storage
```

---

# 37. Device Mapper

Linux Device Mapper provides a framework for creating virtual block devices.

Conceptually:

```text
Filesystem
    |
    v
Logical Block Device
    |
    v
Device Mapper
    |
    +---- underlying device
    +---- another device
```

It is used by technologies such as:

```text
LVM
dm-crypt
multipath
thin provisioning
```

---

# 38. Multipath

In enterprise storage, a server may have multiple physical paths to the same storage.

```text
             Storage
             /     \
            /       \
        Path 1     Path 2
          |          |
        HBA        HBA
          \          /
           \        /
             Server
```

Multipath software can provide:

```text
Path redundancy
Failover
Load balancing
```

This is common in SAN environments.

---

# 39. SAN vs NAS

Very common interview question.

## SAN

Storage is presented as block storage.

```text
Server
   |
   v
SAN
   |
   v
Block device
```

The server typically creates a filesystem on the presented block device.

## NAS

Storage is presented as a filesystem/file service.

```text
Server
   |
   v
Network
   |
   v
NAS
   |
   v
Remote filesystem
```

Examples include:

```text
NFS
SMB
```

Simple distinction:

```text
SAN → block-level storage

NAS → file-level storage
```

---

# 40. Local Storage vs Network Storage

```text
Local:
Application
    ↓
Filesystem
    ↓
Local storage device
```

Network:

```text
Application
    ↓
Filesystem/client
    ↓
Network
    ↓
Remote storage server
    ↓
Storage
```

Network storage introduces additional concerns:

```text
Network latency
Network failures
Timeouts
Retries
Connection state
Distributed consistency
```

---

# 41. Storage Performance Terms

Know these terms very well.

## Latency

Time taken for an operation.

Example:

```text
I/O submitted
    ↓
completion
```

The elapsed time is latency.

---

## Throughput

Amount of data transferred per unit time.

Example:

```text
500 MB/s
```

---

## IOPS

I/O Operations Per Second.

Example:

```text
100,000 IOPS
```

Important distinction:

```text
IOPS
    = number of operations

Throughput
    = amount of data
```

For small I/O:

```text
High IOPS
```

may be more important.

For large sequential I/O:

```text
High throughput
```

may be more important.

---

# 42. Sequential vs Random I/O

### Sequential

```text
Block 1
Block 2
Block 3
Block 4
Block 5
```

### Random

```text
Block 100
Block 7
Block 5000
Block 42
```

HDDs are especially sensitive to random access because of mechanical movement.

SSDs handle random I/O much better, although performance still depends heavily on workload, queue depth, device architecture, and other factors.

---

# 43. Queue Depth

Queue depth is roughly the number of outstanding I/O operations.

```text
Application
   |
   +-- I/O 1
   +-- I/O 2
   +-- I/O 3
   +-- I/O 4
   |
   v
Storage queue
```

Higher queue depth can improve device utilization up to a point.

Too much concurrency can increase:

```text
Latency
CPU overhead
Contention
```

Therefore:

> Maximum IOPS and minimum latency are not necessarily achieved at the same queue depth.

---

# 44. Write Amplification

Write amplification means the storage device performs more physical/internal writes than the host logically requested.

Example:

```text
Host:
    write 4 KB

SSD internally:
    may need to move/rewrite
    considerably more data
```

Reasons can include:

```text
Garbage collection
Flash erase-block behavior
FTL mapping
Data movement
Over-provisioning
```

This matters for:

```text
SSD performance
SSD endurance
Latency
```

---

# 45. TRIM / Discard

When files are deleted, the filesystem may tell the SSD which blocks are no longer needed.

Conceptually:

```text
delete file
    |
    v
filesystem knows blocks are unused
    |
    v
discard/TRIM
    |
    v
SSD knows those logical blocks are no longer required
```

This can help SSD garbage collection and long-term performance.

Linux exposes discard functionality through appropriate filesystem/device interfaces.

---

# 46. Storage Failure Model

Senior engineers should think beyond "disk failed."

Possible failures include:

```text
Device failure
Controller failure
Cable/path failure
PCIe problems
Filesystem corruption
Bad sectors
Media errors
Timeouts
Queue stalls
Driver bugs
Firmware bugs
Power loss
Write-cache issues
Network storage failure
```

A storage system must consider:

```text
Detection
Recovery
Retry
Failover
Redundancy
Consistency
Durability
Data integrity
```

---

# 47. Important Linux Storage Commands

### Identify block devices

```bash
lsblk
```

### Show filesystem information

```bash
lsblk -f
```

### Show mounted filesystems

```bash
mount
findmnt
```

### Disk usage

```bash
df -h
du -sh /path
```

Important distinction:

```text
df
    → filesystem/block usage

du
    → directory/file usage
```

---

# 48. Identify Devices

```bash
lspci
```

Useful for PCI devices such as:

```text
NVMe controllers
Network cards
HBAs
GPUs
```

For NVMe:

```bash
nvme list
```

For SCSI-style devices:

```bash
lsscsi
```

---

# 49. I/O Performance

Useful tools:

```bash
iostat
pidstat -d
vmstat
iotop
```

Example:

```bash
iostat -xz 1
```

Look for:

```text
utilization
latency
IOPS
throughput
queue behavior
```

---

# 50. Kernel Storage Information

Useful locations:

```text
/proc/diskstats
/sys/block
/sys/class/block
```

Example:

```bash
cat /proc/diskstats
```

These interfaces are useful when investigating storage activity.

---

# 51. Storage Troubleshooting Flow

Suppose:

> "Application is slow because disk I/O is slow."

Do not immediately blame the disk.

Use:

```text
Application
    ↓
CPU?
    ↓
Memory?
    ↓
Filesystem?
    ↓
Page cache?
    ↓
Block layer?
    ↓
Device?
    ↓
Hardware?
```

A practical investigation:

```text
1. Check application latency
2. Check CPU
3. Check memory pressure
4. Check filesystem usage
5. Check I/O statistics
6. Check device latency
7. Check kernel logs
8. Check device errors
9. Check filesystem errors
10. Check hardware/path failures
```

---

# 52. Critical Storage Interview Questions

## Fundamentals

1. What is a block device?
2. Sector vs block?
3. What is an LBA?
4. HDD vs SSD?
5. SATA vs SAS vs NVMe?
6. What is NVMe?
7. What is PCIe?
8. What is a filesystem?
9. Filesystem vs block device?
10. What is a partition?
11. What is mounting?

## Linux Storage

12. Explain the Linux storage stack.
13. What is VFS?
14. What is an inode?
15. What is a dentry?
16. What is a file descriptor?
17. What is the page cache?
18. Buffered I/O vs direct I/O?
19. What happens during `read()`?
20. What happens during `write()`?
21. What does `fsync()` do?
22. What is the Linux block layer?
23. What is `bio`?
24. What is blk-mq?
25. What is DMA?

## Enterprise Storage

26. What is RAID?
27. RAID 0 vs RAID 1?
28. RAID 5 vs RAID 6?
29. RAID 10?
30. What is LVM?
31. What is Device Mapper?
32. What is multipath?
33. SAN vs NAS?
34. What is NFS?

## Performance

35. Latency vs throughput?
36. IOPS vs throughput?
37. Sequential vs random I/O?
38. What is queue depth?
39. What is write amplification?
40. What is TRIM?
41. Why can SSD latency increase under heavy load?

## Troubleshooting

42. Disk utilization is 100%. What does that actually mean?
43. Application has high `read()` latency. How do you debug it?
44. `write()` is fast but data disappears after power loss. Why?
45. Filesystem is full but `du` does not show the expected usage.
46. Disk is slow but CPU is idle. How do you investigate?
47. A storage device disappears intermittently. What do you check?
48. One path to SAN storage fails. What happens?
49. How do you identify whether the problem is application, filesystem, block layer, driver, or hardware?
50. How would you debug high storage latency in production?

---

# 53. The Storage Stack You Must Remember

The single most important mental model:

```text
                 APPLICATION
                      |
                      v
                POSIX API
              open/read/write
                      |
                      v
                    VFS
                      |
                      v
                 FILESYSTEM
                      |
             +--------+--------+
             |                 |
             v                 v
         PAGE CACHE        DIRECT I/O
             |                 |
             +--------+--------+
                      |
                      v
                 BLOCK LAYER
                      |
                      v
             DEVICE MAPPER / RAID
                      |
              +-------+-------+
              |               |
              v               v
            SCSI             NVMe
              |               |
              +-------+-------+
                      |
                      v
                STORAGE DRIVER
                      |
                      v
                    PCIe
                      |
                      v
                 SSD / HDD
```

You should be able to explain **every box and every arrow** before moving into advanced Linux storage internals.

---

# 54. Senior Interview Mental Model

When an interviewer asks:

> "What happens when a C++ application writes 4 KB to a file?"

Do not answer only:

```text
write() writes to disk.
```

A senior-level answer starts with:

```text
Application
    ↓
write() syscall
    ↓
VFS
    ↓
Filesystem
    ↓
Page cache / filesystem write path
    ↓
Dirty page
    ↓
Writeback
    ↓
Block layer
    ↓
Device driver
    ↓
DMA/device processing
    ↓
Storage completion
```

Then discuss:

```text
buffered vs O_DIRECT
fsync
writeback
I/O scheduling/queueing
latency
SSD/HDD behavior
failure/durability
```

That is the level of storage understanding expected for a senior Linux systems engineer.
