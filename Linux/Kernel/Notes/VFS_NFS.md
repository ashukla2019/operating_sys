# Linux VFS Deep Dive
# Chapter 01 - VFS Architecture

> **Goal**
>
> After reading this chapter, you should be able to answer:
>
> - What is VFS?
> - Why was VFS created?
> - How does VFS hide filesystem differences?
> - What happens when an application calls `open()`?
> - Where do ext4, NFS, FAT and XFS fit into the Linux architecture?
> - What kernel layer talks to the filesystem?
> - Why can the same application work on different filesystems without modification?

---

# Contents

1. What is VFS?
2. Why VFS Exists
3. Linux Storage Stack
4. VFS Architecture
5. Responsibilities of VFS
6. Local vs Network Filesystems
7. Complete open() Request Flow
8. VFS Operations
9. Why VFS Makes Linux Powerful
10. Interview Questions

---

# 1. What is VFS?

VFS stands for **Virtual File System**.

It is a **kernel layer** that provides a common interface to every filesystem.

Without VFS, every application would need to know how each filesystem works.

For example,

```c
open("notes.txt", O_RDONLY);
```

The application doesn't know whether the file is stored on:

- ext4
- XFS
- FAT32
- NTFS
- NFS
- tmpfs

The application simply calls `open()`.

The VFS handles everything else.

---

# The Big Idea

Think of VFS as a **translator**.

```text
Application

       open()

          │
          ▼

      Linux VFS

          │
 ┌────────┼──────────┐
 │        │          │
 ▼        ▼          ▼

ext4     NFS       FAT32
```

Applications speak one language.

Each filesystem speaks another.

VFS translates between them.

---

# Real Life Analogy

Imagine travelling to different countries.

You speak English.

The local people speak:

- Japanese
- German
- French
- Hindi

Instead of learning every language, you use a translator.

```
You

↓

Translator

↓

Local Person
```

Linux VFS is exactly that translator.

---

# 2. Why VFS Exists

Suppose VFS didn't exist.

Application would need code like

```text
if(ext4)
    ext4_open();

else if(nfs)
    nfs_open();

else if(fat)
    fat_open();

else if(xfs)
    xfs_open();
```

Every application would need to understand every filesystem.

Impossible.

Instead,

Application always does

```c
open();
read();
write();
close();
```

The kernel decides which filesystem should actually execute the operation.

---

# Without VFS

```text
Application

↓

ext4 Code

↓

Disk
```

Another application

```text
Application

↓

NFS Code

↓

Network
```

Every application becomes filesystem dependent.

---

# With VFS

```text
Application

↓

open()

↓

Linux VFS

↓

Filesystem Driver

↓

Storage Device
```

Applications become completely filesystem independent.

---

# 3. Linux Storage Stack

The complete Linux storage stack looks like this.

```text
+--------------------------------------+
|           User Application           |
+--------------------------------------+

                |

                ▼

+--------------------------------------+
|          glibc (open/read)           |
+--------------------------------------+

                |

                ▼

+--------------------------------------+
|          System Call Layer           |
+--------------------------------------+

                |

                ▼

+--------------------------------------+
|        Virtual File System           |
+--------------------------------------+

                |

     ----------------------------

     |            |            |

     ▼            ▼            ▼

   ext4         XFS          NFS

     |            |            |

     ----------------------------

                |

                ▼

+--------------------------------------+
|          Block Layer                 |
+--------------------------------------+

                |

                ▼

+--------------------------------------+
|         Device Driver                |
+--------------------------------------+

                |

                ▼

+--------------------------------------+
|              Disk                    |
+--------------------------------------+
```

This architecture is one of Linux's biggest strengths.

---

# Where is VFS?

```
Userspace

Application

↓

glibc

↓

============================

Kernel

↓

System Calls

↓

VFS

↓

Filesystem

↓

Driver

↓

Hardware
```

VFS is completely inside the kernel.

---

# 4. Responsibilities of VFS

VFS does **not** store files.

VFS does **not** know ext4 disk layout.

VFS does **not** know FAT tables.

Instead, VFS provides common services.

Its responsibilities include:

- Path lookup
- Mount management
- File descriptor management
- Permission checking
- File object creation
- Inode abstraction
- Dentry cache
- Dispatching operations to the correct filesystem

Think of VFS as the **traffic controller** for all file operations.

---

# 5. Local vs Network Filesystems

A local filesystem stores data on storage attached to the machine.

Examples:

- ext4
- XFS
- FAT32

```text
Application

↓

VFS

↓

ext4

↓

SSD
```

---

A network filesystem stores data on another machine.

Example:

NFS

```text
Application

↓

VFS

↓

NFS Client

↓

Ethernet

↓

NFS Server

↓

Disk
```

Notice something important.

Application still uses

```c
read();
write();
```

Nothing changes.

That's the beauty of VFS.

---

# 6. Complete open() Request Flow

Suppose

```c
fd = open("/home/user/test.txt", O_RDONLY);
```

Internally,

```text
Application

↓

glibc

↓

System Call

↓

Kernel

↓

VFS

↓

Path Lookup

↓

Dentry Cache

↓

Filesystem

↓

Create File Object

↓

Allocate File Descriptor

↓

Return fd
```

Notice that the application never communicates directly with ext4.

Everything goes through VFS.

---

# 7. VFS Operations

Every filesystem implements a common set of operations.

For example,

```text
open()

↓

VFS

↓

filesystem->open()
```

Similarly,

```text
read()

↓

VFS

↓

filesystem->read()
```

Likewise,

```text
write()

↓

VFS

↓

filesystem->write()
```

This is called **polymorphism** inside the kernel.

Different filesystems implement the same operations differently, but VFS always invokes them through a common interface.

---

# Example

Application

```c
read(fd, buf, 100);
```

If file belongs to ext4

```text
VFS

↓

ext4_read()
```

If file belongs to NFS

```text
VFS

↓

nfs_read()
```

Application never changes.

---

# 8. Why VFS Makes Linux Powerful

Suppose tomorrow someone develops a new filesystem.

Without VFS

Every application would require modification.

With VFS

Only the new filesystem driver needs to implement the required VFS operations.

Everything else continues working.

This design makes Linux highly extensible.

---

# Complete Architecture

```text
                    Userspace
+------------------------------------------------+
|                  Application                   |
+------------------------------------------------+
                    |
                    ▼
              glibc Library
                    |
                    ▼
================ Kernel Boundary =================
                    |
                    ▼
              System Call Layer
                    |
                    ▼
          Virtual File System (VFS)
                    |
      +-------------+--------------+
      |             |              |
      ▼             ▼              ▼
    ext4          XFS            NFS
      |             |              |
      +-------------+--------------+
                    |
                    ▼
              Block Layer
                    |
                    ▼
             Device Driver
                    |
                    ▼
               SSD / HDD / NVMe
```

---

# Summary

VFS provides:

- Filesystem independence
- Common API
- Common kernel interface
- Path lookup
- File descriptor management
- Mount management
- Permission handling
- Dispatching operations to the correct filesystem

Applications never need to know which filesystem stores the file.

---

# Interview Questions

### What is VFS?

A kernel abstraction layer that provides a common interface to different filesystems.

---

### Does VFS store data?

No.

The actual filesystem (ext4, NFS, XFS, etc.) stores the data.

---

### Does VFS know ext4 disk layout?

No.

That knowledge belongs to the ext4 filesystem driver.

---

### Can the same application work on ext4 and NFS without recompilation?

Yes.

Because the application interacts only with the VFS interface (`open()`, `read()`, `write()`, etc.), while VFS dispatches the request to the appropriate filesystem implementation.

---

### Why is VFS considered an abstraction layer?

Because it hides filesystem-specific implementations behind a common interface, allowing applications and much of the kernel to work uniformly with different filesystems.

---

# What's Next?

In the next chapter (**02_VFS_Core_Objects.md**), we'll study the five kernel objects that form the foundation of VFS:

```text
Superblock
      │
      ▼
    Inode
      │
      ▼
    Dentry
      │
      ▼
   File Object
      │
      ▼
File Descriptor
```

Understanding how these objects connect is the key to mastering VFS.
---------------------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 02 - Core VFS Objects

> **Goal**
>
> After this chapter, you should be able to explain:
>
> - Why Linux needs Superblock, Inode, Dentry, File Object and File Descriptor
> - How they are connected
> - Which objects are persistent and which are created at runtime
> - Which objects belong to the filesystem and which belong to a process
> - What happens internally when `open()` is called
>
> **This is the single most important chapter of the VFS handbook.**

---

# Contents

1. The Five Core Objects
2. Relationship Between Objects
3. Superblock
4. Inode
5. Dentry
6. File Object
7. File Descriptor
8. Object Lifetime
9. Complete Object Creation Flow
10. Interview Questions

---

# 1. The Five Core Objects

Whenever Linux opens a file, five important kernel objects are involved.

```text
Filesystem

      │

      ▼

Superblock

      │

      ▼

Inode

      │

      ▼

Dentry

      │

      ▼

File Object

      │

      ▼

File Descriptor

      │

      ▼

Application
```

These objects work together to translate a filename into readable or writable data.

---

# Think of Them Like a Library

Suppose you visit a library to borrow a book.

| Library Concept | Linux VFS Object |
|-----------------|------------------|
| Library Building | Superblock |
| Book | Inode |
| Book Name | Dentry |
| Borrow Slip | File Object |
| Token Number | File Descriptor |

This analogy helps remember each object's purpose.

---

# 2. Relationship Between Objects

Let's understand them one by one.

```
Disk
│
├── Filesystem
│
├── Superblock
│
├── Inodes
│
├── Data Blocks
│
└── Directories

Memory (Kernel)

├── Dentry Cache
├── File Objects
├── File Descriptor Table
└── Page Cache
```

Notice:

Some objects exist **on disk**.

Some exist **only in memory**.

---

# Overview

| Object | Exists On Disk | Exists In Memory |
|----------|---------------|-----------------|
| Superblock | Yes | Yes (mounted copy) |
| Inode | Yes | Yes (inode cache) |
| Dentry | No | Yes |
| File Object | No | Yes |
| File Descriptor | No | Yes |

---

# 3. Superblock

## What is it?

A **Superblock** represents an entire mounted filesystem.

Think of it as the filesystem's identity card.

It contains metadata about the filesystem itself.

---

## Stores

- Filesystem type
- Block size
- Total blocks
- Free blocks
- Root inode
- Mount information

---

## Relationship

```text
Filesystem

↓

Superblock

↓

Everything else
```

Every mounted filesystem has one active superblock in memory.

---

## Example

Suppose

```
/dev/sda1
```

contains ext4.

After mounting,

```
mount /dev/sda1 /home
```

Linux creates

```text
struct super_block
```

for that filesystem.

---

## Important Kernel Structure

```c
struct super_block
```

Contains

- block size
- filesystem operations
- root inode
- mount information

---

## Interview Question

### How many Superblocks exist?

One active `super_block` structure per mounted filesystem.

---

# 4. Inode

## What is an Inode?

An inode represents **a file**.

Not its name.

The actual file.

---

Suppose

```
report.pdf
```

Linux stores

```
inode #4521
```

The filename is stored elsewhere.

---

## Stores

- File size
- Owner
- Group
- Permissions
- Timestamps
- Block addresses
- File type

---

## Does NOT Store

- Filename

---

## Relationship

```text
Filename

↓

Dentry

↓

Inode

↓

Disk Blocks
```

---

## Important Kernel Structure

```c
struct inode
```

Contains

- metadata
- inode operations
- reference count
- filesystem information

---

## Interview Question

### Does inode contain the filename?

No.

The filename is stored in the directory entry (dentry).

---

# 5. Dentry

## What is a Dentry?

A **Dentry** (Directory Entry) connects a filename to an inode.

Without a dentry, Linux would have no way to map:

```
notes.txt
```

to

```
inode 245
```

---

## Relationship

```text
notes.txt

↓

Dentry

↓

inode
```

---

## Why is it Needed?

Applications use names.

Kernel uses inode numbers.

Dentry bridges the gap.

---

## Dentry Cache

Linux caches dentries.

```
Path Lookup

↓

Dentry Cache

↓

Found?

↓

Yes

↓

Done
```

Without the dentry cache, every lookup would require disk access.

---

## Important Kernel Structure

```c
struct dentry
```

Contains

- filename
- parent dentry
- inode pointer
- reference count

---

## Interview Question

### Why is the dentry cache important?

It speeds up pathname resolution by avoiding repeated directory lookups.

---

# 6. File Object

The file object exists **only after a file is opened**.

Suppose

```c
open("test.txt");
```

Linux creates

```text
struct file
```

---

## Stores

- Current file offset
- Open flags
- File operations
- Pointer to inode
- Reference count

---

## Relationship

```text
File Descriptor

↓

File Object

↓

Inode
```

---

## Why?

Two processes may open the same file.

Each process needs its own file position.

Example

Process A

```
offset = 100
```

Process B

```
offset = 500
```

Same inode.

Different file objects.

---

## Important Kernel Structure

```c
struct file
```

Contains

- file position
- file operations
- inode pointer
- address_space pointer

---

## Interview Question

### Why is a file object needed?

It stores information specific to one open instance of a file.

---

# 7. File Descriptor

A File Descriptor is what applications receive.

```c
fd = open(...);
```

Suppose

```
fd = 3
```

---

## What is FD?

Simply an integer.

Internally

```
3
```

indexes the process's file descriptor table.

---

## Relationship

```text
fd = 3

↓

files_struct

↓

fdtable

↓

struct file
```

---

## Important Structures

```c
files_struct

↓

fdtable

↓

struct file
```

Each process has its own descriptor table.

---

## Interview Question

### Is a File Descriptor the file itself?

No.

It is only an index that points to a `struct file`.

---

# 8. Object Lifetime

Different objects live for different durations.

| Object | Lifetime |
|----------|----------|
| Superblock | While filesystem is mounted |
| Inode | While cached or referenced |
| Dentry | While cached or referenced |
| File Object | While file is open |
| File Descriptor | Until `close()` or process exits |

---

# 9. Complete Object Creation Flow

Suppose the application executes:

```c
fd = open("/home/user/data.txt", O_RDONLY);
```

Internally:

```text
Application

↓

open()

↓

System Call

↓

VFS

↓

Locate Mounted Filesystem

↓

Superblock

↓

Path Lookup

↓

Dentry

↓

Inode

↓

Allocate File Object

↓

Install File Descriptor

↓

Return fd
```

After `open()` completes, the relationships are:

```text
Application
      │
      ▼
File Descriptor (3)
      │
      ▼
File Object
      │
      ▼
Dentry
      │
      ▼
Inode
      │
      ▼
Superblock
      │
      ▼
Filesystem
```

This chain is the foundation of every file operation in Linux.

---

# Summary

| Object | Represents |
|----------|------------|
| Superblock | Entire filesystem |
| Inode | File metadata |
| Dentry | Filename → inode mapping |
| File Object | One open instance of a file |
| File Descriptor | Process handle to an open file |

Remember this dependency chain:

```text
Filesystem
      │
Superblock
      │
Inode
      │
Dentry
      │
File Object
      │
File Descriptor
      │
Application
```

---

# Interview Questions

### Which VFS object stores the filename?

**Dentry**

---

### Which object stores the file size?

**Inode**

---

### Which object stores the current file offset?

**File Object (`struct file`)**

---

### Which object is returned to the application?

**File Descriptor**

---

### Can two processes share the same inode?

Yes. They can reference the same inode while having different file objects and different file descriptors.

---

### Which object is created only after `open()`?

**File Object (`struct file`)** and a **File Descriptor**.

---

# What's Next?

The next chapter (**03_Path_Lookup.md**) explains how Linux resolves a pathname like:

```text
/home/user/project/src/main.c
```

You'll learn:

- How pathname resolution works
- Directory traversal
- Root vs current working directory
- Dentry cache (dcache)
- Negative dentries
- Mount points
- Symbolic links
- Hard links
- The complete kernel path lookup algorithm
  -------------------------------------------------------------------------------------------
  # Linux VFS Deep Dive
# Chapter 03 - Path Lookup (Pathname Resolution)

> **Goal**
>
> After this chapter, you should be able to explain:
>
> - How Linux finds a file from a pathname
> - How the VFS walks each directory
> - Why the dentry cache exists
> - Absolute vs Relative paths
> - Hard Links vs Symbolic Links
> - Mount Points
> - Negative Dentries
> - Complete kernel path lookup flow
>
> **This is one of the most frequently asked VFS interview topics.**

---

# Contents

1. What is Path Lookup?
2. Absolute vs Relative Paths
3. Path Components
4. Root Dentry
5. Walking the Path
6. Dentry Cache (dcache)
7. Negative Dentries
8. Mount Points
9. Symbolic Links
10. Hard Links
11. Complete Path Lookup Flow
12. Summary
13. Interview Questions

---

# 1. What is Path Lookup?

Whenever an application opens a file,

```c
open("/home/user/docs/file.txt", O_RDONLY);
```

Linux must answer one question:

> **Where is this file?**

The kernel cannot jump directly to the file.

It must resolve the path **one directory at a time**.

---

## Example

```
/home/user/docs/file.txt
```

Linux resolves it like this:

```text
/

↓

home

↓

user

↓

docs

↓

file.txt
```

Each component is looked up separately.

---

# Why Can't Linux Jump Directly?

The filename itself is **not** stored in an inode.

The kernel only knows:

- directories
- dentries
- inode numbers

So every directory must be searched.

---

# 2. Absolute vs Relative Paths

## Absolute Path

Starts from the filesystem root.

Example

```text
/etc/passwd
```

```text
Root

↓

etc

↓

passwd
```

---

## Relative Path

Starts from the process's current working directory.

Example

```text
docs/report.txt
```

If Current Working Directory is

```
/home/user
```

Linux searches

```text
/home/user

↓

docs

↓

report.txt
```

---

## Current Working Directory

Every process stores

```text
Current Working Directory (CWD)
```

inside

```c
struct fs_struct
```

The VFS uses this as the starting point for relative paths.

---

# 3. Path Components

Linux splits every path into components.

Example

```
/var/log/nginx/access.log
```

becomes

```text
/

↓

var

↓

log

↓

nginx

↓

access.log
```

Each component is resolved independently.

---

# 4. Root Dentry

Every mounted filesystem has a **Root Dentry**.

```
/
```

represents the starting point.

```text
Root Dentry

↓

home

↓

user

↓

project
```

The kernel begins traversal from the root dentry (for absolute paths).

---

# 5. Walking the Path

Suppose

```c
open("/home/user/file.txt");
```

Linux performs:

Step 1

```text
Find "/"
```

↓

Step 2

```text
Find "home"
```

↓

Step 3

```text
Find "user"
```

↓

Step 4

```text
Find "file.txt"
```

↓

Return inode

---

## Visual Flow

```text
Root

↓

home

↓

user

↓

file.txt

↓

inode
```

Notice:

Linux never searches the complete path at once.

It searches **one directory level at a time**.

---

# 6. Dentry Cache (dcache)

Searching directories repeatedly would be slow.

Linux caches directory entries.

This cache is called the **Dentry Cache**.

---

## First Access

```text
Application

↓

Path Lookup

↓

Disk

↓

Create Dentry

↓

Cache It
```

---

## Second Access

```text
Application

↓

Path Lookup

↓

Dentry Cache

↓

Found

↓

Done
```

No disk access is needed.

---

## Example

First call

```c
open("/home/user/file.txt");
```

Kernel searches

```
/

↓

home

↓

user

↓

file
```

Second call

```c
open("/home/user/file.txt");
```

Kernel often finds the path directly in the dentry cache.

---

## Why dcache Matters

Without dcache

Every lookup would involve reading directories from disk.

With dcache

Most pathname resolutions happen entirely in RAM.

---

# 7. Negative Dentries

Suppose

```c
open("unknown.txt");
```

The file does not exist.

Without caching,

Linux would search the filesystem every time.

Instead,

Linux creates a **Negative Dentry**.

```text
unknown.txt

↓

Not Found

↓

Cache Result
```

Next lookup

```
unknown.txt
```

Immediately returns

```
ENOENT
```

without another filesystem search.

---

## Benefit

Negative dentries significantly improve performance for repeated failed lookups.

---

# 8. Mount Points

Linux supports multiple mounted filesystems.

Example

```
/

↓

home

↓

mnt

↓

usb
```

Suppose

```
/mnt/usb
```

is another filesystem.

During path lookup,

```text
Root FS

↓

mnt

↓

usb

↓

New Superblock

↓

Continue Lookup
```

The VFS transparently switches to the mounted filesystem.

Applications are unaware of this transition.

---

# 9. Symbolic Links

A symbolic link stores a pathname.

Example

```
shortcut

↓

"/home/user/docs/report.txt"
```

During lookup

```text
shortcut

↓

Read Link Target

↓

Restart Lookup

↓

report.txt
```

The kernel follows the referenced path.

---

## Example

```
report

↓

symlink

↓

/home/user/docs/report.pdf
```

Opening `report` actually opens `report.pdf`.

---

# 10. Hard Links

A hard link is another filename pointing to the **same inode**.

```text
report.txt

↓

inode 120
```

Another filename

```text
backup.txt

↓

inode 120
```

Same inode.

Different filenames.

---

## Difference

### Hard Link

```text
report

↓

inode 55

↑

backup
```

Both names reference the same inode.

---

### Symbolic Link

```text
shortcut

↓

"/home/report"
```

Stores a pathname, not an inode.

---

# 11. Complete Path Lookup Flow

Suppose

```c
open("/home/user/project/main.cpp");
```

Complete kernel flow:

```text
Application

↓

glibc

↓

System Call

↓

VFS

↓

Start at Root Dentry

↓

Lookup "home"

↓

Lookup "user"

↓

Lookup "project"

↓

Lookup "main.cpp"

↓

Dentry

↓

inode

↓

Create File Object

↓

Allocate File Descriptor

↓

Return fd
```

---

## Where is dcache Used?

```text
Lookup Component

↓

dcache

↓

Hit ?

│

├── Yes

│      ↓

│ Continue

│

└── No

↓

Filesystem Lookup

↓

Create Dentry

↓

Insert into dcache

↓

Continue
```

---

# Complete Picture

```text
Path

↓

Split Components

↓

Root Dentry

↓

home

↓

user

↓

project

↓

main.cpp

↓

inode

↓

struct file

↓

fd
```

---

# Summary

Linux resolves a pathname:

- One component at a time
- Starting from root or current working directory
- Using the dentry cache whenever possible
- Following mount points transparently
- Resolving symbolic links when encountered
- Returning the target inode before creating a file object

---

# Interview Questions

### Does Linux search the entire pathname at once?

No.

It resolves one directory component at a time.

---

### Why is the dentry cache important?

It caches directory lookups, reducing filesystem and disk accesses.

---

### What is a negative dentry?

A cached record indicating that a pathname component does not exist, avoiding repeated failed lookups.

---

### What is the difference between a hard link and a symbolic link?

| Hard Link | Symbolic Link |
|------------|---------------|
| Points directly to an inode | Stores another pathname |
| Cannot span filesystems | Can span filesystems |
| Survives if another filename is removed (until link count reaches zero) | Breaks if the target path no longer exists |

---

### Where does path lookup start?

- **Absolute path:** Root dentry (`/`)
- **Relative path:** Current Working Directory (`struct fs_struct`)

---

### What happens when a mount point is encountered?

The VFS switches to the mounted filesystem's superblock and root dentry, then continues pathname resolution transparently.

---

# What's Next?

In **Chapter 04 – Mounting a Filesystem**, we'll answer:

- What actually happens when `mount()` is called?
- How is a `super_block` created?
- How does the VFS know which filesystem to use?
- What is the mount tree?
- How are mount namespaces implemented?
- How does Linux switch between different mounted filesystems during path lookup?
- ----------------------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 04 - Mounting a Filesystem

> **Goal**
>
> After reading this chapter, you should understand:
>
> - What `mount()` actually does
> - What happens inside the Linux kernel during mounting
> - How the VFS creates a `super_block`
> - How Linux supports multiple mounted filesystems
> - What a mount point is
> - How path lookup crosses mount points
>
> **Mounting is where a filesystem becomes visible to the VFS.**

---

# Contents

1. What is Mounting?
2. Why Mounting is Required
3. Mount Point
4. What Happens During mount()
5. Superblock Creation
6. Mount Tree
7. Path Lookup Across Mount Points
8. Unmounting
9. Complete Mount Flow
10. Interview Questions

---

# 1. What is Mounting?

A filesystem stored on disk cannot be accessed directly.

Before Linux can access files on it, the filesystem must be **mounted**.

For example,

```bash
mount /dev/sda1 /home
```

After mounting,

```
/home
```

becomes the root of that filesystem.

---

## Before Mount

```text
Disk

↓

ext4 Filesystem

(Not Accessible)
```

---

## After Mount

```text
Disk

↓

ext4

↓

VFS

↓

/home

↓

Application
```

The filesystem is now visible through the directory tree.

---

# 2. Why Mounting is Required?

Linux treats **everything as one directory tree**.

Instead of assigning drive letters like Windows,

```
C:
D:
E:
```

Linux has only one root.

```
/
```

Every filesystem is attached somewhere under this root.

Example

```text
/

├── home
├── boot
├── media
└── mnt
```

Each directory may belong to a different filesystem.

---

## Example

```text
/

↓

ext4

↓

home

↓

Another ext4

↓

mnt

↓

NFS
```

Applications cannot tell the difference.

---

# 3. Mount Point

A **mount point** is simply an existing directory where another filesystem is attached.

Example

```
mkdir /mnt/usb

mount /dev/sdb1 /mnt/usb
```

Now,

```text
/mnt/usb
```

represents the root of `/dev/sdb1`.

---

## Before Mount

```text
/

↓

mnt

↓

usb

(empty directory)
```

---

## After Mount

```text
/

↓

mnt

↓

usb

↓

Filesystem Root
```

The original empty directory is hidden while the filesystem is mounted.

---

# 4. What Happens During mount()

Suppose

```bash
mount -t ext4 /dev/sda1 /home
```

Internally:

```text
User Space

↓

mount()

↓

System Call

↓

Kernel

↓

VFS

↓

Find Filesystem Driver

↓

Read Superblock

↓

Create struct super_block

↓

Create Root Dentry

↓

Attach to Mount Tree

↓

Mount Complete
```

---

## Important Steps

1. Locate filesystem driver.
2. Read filesystem metadata.
3. Create kernel objects.
4. Attach filesystem into VFS.

---

# 5. Superblock Creation

Every mounted filesystem has one active

```c
struct super_block
```

---

## Why?

The VFS needs information about the filesystem.

For example:

- Block size
- Root inode
- Filesystem operations
- Maximum filename length
- Free blocks

---

## Flow

```text
Mount

↓

Read Superblock

↓

Create struct super_block

↓

Initialize Operations

↓

Filesystem Ready
```

---

## Relationship

```text
Filesystem

↓

Superblock

↓

Root Inode

↓

Directories

↓

Files
```

---

# 6. Mount Tree

Linux internally maintains a **mount tree**.

Example

```text
/

├── boot

├── home

├── proc

├── sys

├── media

└── mnt
```

Each entry may be a completely different filesystem.

Example

```text
/

↓

ext4

↓

home

↓

ext4

↓

mnt

↓

NFS

↓

media

↓

FAT32
```

Applications always see one unified directory hierarchy.

---

# 7. Path Lookup Across Mount Points

Suppose

```text
/home/project/file.txt
```

belongs to one filesystem.

But

```text
/mnt/nfs/project/file.txt
```

belongs to another.

During path lookup,

```text
Root Filesystem

↓

mnt

↓

nfs

↓

Mount Point Found

↓

Switch Superblock

↓

Continue Lookup
```

The VFS transparently switches to the mounted filesystem.

Applications never notice this transition.

---

## Example

```
/

↓

mnt

↓

usb

↓

report.pdf
```

The lookup process:

```text
Find /

↓

Find mnt

↓

Find usb

↓

Switch Filesystem

↓

Find report.pdf

↓

inode
```

---

# 8. Unmounting

When

```bash
umount /mnt/usb
```

is executed,

Linux performs:

```text
Stop New Access

↓

Flush Dirty Pages

↓

Release References

↓

Destroy Mount

↓

Release Superblock
```

If files are still open,

```
umount
```

usually fails with

```
Device Busy
```

because active references still exist.

---

# 9. Complete Mount Flow

```text
Application

↓

mount()

↓

glibc

↓

System Call

↓

Kernel

↓

VFS

↓

Filesystem Driver

↓

Read On-Disk Superblock

↓

Create struct super_block

↓

Create Root Dentry

↓

Create Mount Object

↓

Insert into Mount Tree

↓

Filesystem Available
```

---

# Complete View

```text
                Application
                     │
                     ▼
                 mount()
                     │
                     ▼
               System Call
                     │
                     ▼
                    VFS
                     │
        +------------+-------------+
        |                          |
        ▼                          ▼
Filesystem Driver          Existing Mount Tree
        │                          │
        ▼                          │
Read On-Disk Superblock            │
        │                          │
        ▼                          │
Create struct super_block          │
        │                          │
        ▼                          ▼
     Root Dentry  ───────────► Attach to Mount Tree
                     │
                     ▼
            Filesystem Ready
```

---

# Important Kernel Objects

During mounting, Linux creates or initializes:

| Object | Purpose |
|----------|----------|
| `struct super_block` | Represents the mounted filesystem |
| Root inode | Metadata for the filesystem root |
| Root dentry | Entry point for pathname lookup |
| Mount object (`struct mount`/`vfsmount`) | Connects the filesystem to the directory tree |

---

# Summary

Mounting does **not** copy files into memory.

Instead, it:

- Reads filesystem metadata
- Creates a `super_block`
- Creates the root dentry
- Links the filesystem into the global directory tree
- Makes the filesystem accessible through the VFS

Once mounted, applications use normal operations like:

```c
open();
read();
write();
close();
```

without knowing which filesystem they are using.

---

# Interview Questions

### What is a mount point?

A directory where another filesystem is attached.

---

### Does mounting copy the entire filesystem into RAM?

No.

Only essential metadata (such as the superblock and root structures) is initialized. File data is loaded on demand.

---

### Can multiple filesystems be mounted simultaneously?

Yes.

Linux maintains a mount tree containing all mounted filesystems.

---

### What happens when path lookup reaches a mount point?

The VFS switches to the mounted filesystem's `super_block` and root dentry, then continues pathname resolution.

---

### Why does `umount` sometimes fail with "Device Busy"?

Because there are still active references, such as open files or a process whose current working directory is inside the mounted filesystem.

---

# What's Next?

The next chapter, **05_Open.md**, begins the complete execution flow of file operations.

We'll trace a single call:

```c
int fd = open("/home/user/file.txt", O_RDONLY);
```

from the application all the way through:

- glibc
- System Call
- VFS
- Path Lookup
- Dentry Cache
- Inode Lookup
- File Object Creation
- File Descriptor Allocation
- Return to User Space

This is one of the most important execution paths in the Linux kernel.
-----------------------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 05 - How open() Works Internally

> **Goal**
>
> After reading this chapter, you should be able to explain the complete execution of `open()` from userspace to the kernel and understand every VFS object involved.
>
> **This is one of the highest-frequency Linux VFS interview questions.**

---

# Contents

1. Why open() is Needed
2. What Does open() Return?
3. Complete open() Flow
4. Step 1 - Application Calls open()
5. Step 2 - System Call
6. Step 3 - VFS Receives the Request
7. Step 4 - Path Lookup
8. Step 5 - Inode Lookup
9. Step 6 - Create File Object
10. Step 7 - Allocate File Descriptor
11. Final Data Structure Relationship
12. Common Interview Questions

---

# 1. Why open() is Needed

Applications cannot directly access files.

Instead they request the Linux kernel.

Example

```c
int fd = open("/home/user/report.txt", O_RDONLY);
```

The kernel must:

- Find the file
- Verify permissions
- Create kernel objects
- Return a handle

---

# What Does open() Actually Do?

Many beginners think

```
open()

↓

Reads File
```

This is incorrect.

The correct flow is

```
open()

↓

Locate File

↓

Create Kernel Objects

↓

Return File Descriptor
```

No file data is read.

Reading begins only when

```c
read()
```

is called.

---

# 2. What Does open() Return?

Example

```c
int fd = open("notes.txt", O_RDONLY);
```

Suppose

```
fd = 3
```

This integer is **not** the file.

It is simply an index.

```
fd = 3

↓

Process FD Table

↓

struct file
```

---

# 3. Complete open() Flow

This is the complete execution path.

```text
Application

↓

glibc

↓

System Call

↓

Kernel

↓

VFS

↓

Path Lookup

↓

Dentry Cache

↓

inode

↓

Filesystem

↓

Create struct file

↓

Allocate File Descriptor

↓

Return fd
```

Remember this diagram.

It is frequently asked in interviews.

---

# 4. Step 1 - Application Calls open()

Application executes

```c
int fd = open("/home/user/report.txt", O_RDONLY);
```

This is only a library function.

The application is still running in **User Space**.

---

## User Space

```
Application

↓

glibc open()
```

glibc prepares the arguments.

Then performs a system call.

---

# 5. Step 2 - System Call

The CPU switches

```
User Mode

↓

Kernel Mode
```

The kernel now starts executing.

```text
Application

↓

glibc

↓

syscall

↓

Kernel
```

This is called a **mode switch**.

---

# 6. Step 3 - VFS Receives the Request

The kernel forwards the request to VFS.

```text
Kernel

↓

VFS

↓

Open Request
```

The VFS now performs

- pathname lookup
- permission checking
- filesystem selection

---

# 7. Step 4 - Path Lookup

Suppose

```
/home/user/report.txt
```

Linux resolves every component.

```
/

↓

home

↓

user

↓

report.txt
```

Every directory lookup checks

```
dcache

↓

Hit?

↓

Yes

↓

Continue
```

Otherwise

```
Filesystem Lookup
```

---

# Result

After path lookup

Linux now knows

```
inode
```

representing

```
report.txt
```

---

# 8. Step 5 - Inode Lookup

The inode stores

- file size
- permissions
- owner
- timestamps
- block locations

Example

```
report.txt

↓

inode #4521
```

The VFS verifies

- Does file exist?
- Permission allowed?
- Correct filesystem?

If successful,

the kernel continues.

---

# 9. Step 6 - Create File Object

Now Linux creates

```c
struct file
```

This represents **this specific open instance**.

---

## Why?

Suppose

Two processes open the same file.

```
Process A

↓

Offset = 0
```

```
Process B

↓

Offset = 100
```

Same inode.

Different file objects.

---

## struct file stores

- current offset
- open flags
- inode pointer
- file operations
- reference count

---

# 10. Step 7 - Allocate File Descriptor

Now Linux allocates

```
fd = 3
```

Internally

```
Process

↓

files_struct

↓

fdtable

↓

fd = 3

↓

struct file
```

The application receives only

```
3
```

---

# 11. Final Data Structure Relationship

After open() completes

```
Application

↓

fd = 3

↓

File Descriptor Table

↓

struct file

↓

dentry

↓

inode

↓

superblock

↓

Filesystem
```

Notice

Nothing has been read from disk yet.

Only kernel objects have been prepared.

---

# What Happens If File Doesn't Exist?

Suppose

```c
open("unknown.txt");
```

Flow

```
Path Lookup

↓

File Not Found

↓

Return ENOENT
```

No file object is created.

No file descriptor is allocated.

---

# What Happens If Permission Fails?

Example

```
No Read Permission
```

Flow

```
Path Lookup

↓

inode

↓

Permission Check

↓

Denied

↓

Return EACCES
```

Again,

no file object is created.

---

# Complete open() Execution

```text
Application

↓

open()

↓

glibc

↓

System Call

↓

Kernel Mode

↓

VFS

↓

Path Lookup

↓

Dentry Cache

↓

inode

↓

Permission Check

↓

Create struct file

↓

Allocate File Descriptor

↓

Return fd
```

---

# Important Objects Used

| Object | Used For |
|----------|----------|
| Superblock | Mounted filesystem |
| Dentry | Pathname lookup |
| Inode | File metadata |
| File Object | Open instance |
| File Descriptor | Handle returned to application |

---

# Common Misconceptions

### Does open() read the file?

❌ No.

It only prepares kernel data structures.

---

### Does open() load the whole file into RAM?

❌ No.

Pages are loaded later when `read()` or `mmap()` accesses the file.

---

### Is File Descriptor stored on disk?

❌ No.

It exists only inside the process.

---

### Does every open() create a new inode?

❌ No.

The inode already exists.

Only a new `struct file` and file descriptor are created.

---

# Summary

```
Application

↓

fd

↓

File Descriptor Table

↓

struct file

↓

dentry

↓

inode

↓

superblock

↓

Filesystem
```

Every successful `open()` creates:

- a new **File Object (`struct file`)**
- a new **File Descriptor**

It **does not** create a new inode or read the file contents.

---

# Interview Questions

### What is the purpose of open()?

To locate the file, validate access, create kernel bookkeeping structures, and return a file descriptor.

---

### Does open() perform disk I/O?

It may perform metadata reads (for pathname resolution or inode lookup) if the required information is not already cached, but it does **not** read the file's data.

---

### What kernel object stores the current file position?

`struct file`

---

### Can two processes have different file offsets for the same file?

Yes.

Each process (or each successful `open()`) has its own `struct file`, which maintains its own current file offset.

---

### What does the File Descriptor actually point to?

It indexes the process's file descriptor table, which contains a pointer to a `struct file`.

---

# What's Next?

In **Chapter 06 - How read() Works Internally**, we'll follow the complete execution of:

```c
read(fd, buffer, size);
```

You'll learn:

- How the File Descriptor is resolved
- How the Page Cache is checked
- What happens on a cache hit vs. cache miss
- How the filesystem reads blocks
- How the block layer and device driver interact
- DMA, interrupts, and `copy_to_user()`

This is the chapter where VFS, Memory Management, and Storage all come together.
---------------------------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 06 - How read() Works Internally

> **Goal**
>
> After reading this chapter, you should be able to explain exactly what happens when an application executes:
>
> ```c
> read(fd, buffer, size);
> ```
>
> This chapter connects:
>
> - VFS
> - Page Cache
> - Memory Management
> - Block Layer
> - Device Driver
> - Disk
>
> **This is one of the most important Linux interview topics.**

---

# Contents

1. What read() Actually Does
2. Complete read() Flow
3. Step 1 - File Descriptor Lookup
4. Step 2 - Locate struct file
5. Step 3 - Locate inode
6. Step 4 - Check Page Cache
7. Cache Hit
8. Cache Miss
9. Block Layer
10. Device Driver
11. DMA
12. Interrupt
13. copy_to_user()
14. Complete Execution Flow
15. Interview Questions

---

# 1. What read() Actually Does

Suppose

```c
char buf[100];

read(fd, buf, 100);
```

Linux must answer one question:

> **Where is the requested data?**

There are two possibilities:

```
Already in RAM

or

Stored on Disk
```

Everything depends on this.

---

# 2. Complete read() Flow

```
Application

↓

read()

↓

glibc

↓

System Call

↓

Kernel

↓

File Descriptor

↓

struct file

↓

inode

↓

Page Cache

↓

Cache Hit ?

│
├── Yes
│
│   copy_to_user()
│
│   Return
│
└── No
    ↓
Filesystem

↓

Block Layer

↓

Device Driver

↓

Disk

↓

DMA

↓

RAM (Page Cache)

↓

Interrupt

↓

copy_to_user()

↓

Return
```

This is the complete execution path.

---

# 3. Step 1 - File Descriptor Lookup

Suppose

```c
read(fd, buf, 100);
```

The first step is

```
fd

↓

files_struct

↓

fdtable

↓

struct file
```

The File Descriptor itself contains no information.

It simply indexes the process's File Descriptor Table.

---

# Process View

```
Process

↓

files_struct

↓

fdtable

↓

fd = 3

↓

struct file
```

---

# 4. Step 2 - Locate struct file

Linux now has

```c
struct file
```

This contains

- current offset
- open flags
- inode pointer
- file operations

Example

```
struct file

↓

Offset = 4096

↓

Read Next Bytes
```

---

# Why Offset Matters

Suppose

```
read(fd,100)

↓

Offset = 0

↓

Offset =100
```

Next

```
read(fd,100)

↓

Offset =100

↓

Offset =200
```

The offset is automatically updated.

---

# 5. Step 3 - Locate inode

The File Object points to

```
inode
```

The inode tells Linux

- which filesystem
- file size
- block mapping
- permissions

Notice

The inode still does **not** contain file data.

It only tells Linux where the data is located.

---

# 6. Step 4 - Check Page Cache

This is the most important step.

Linux first checks

```
Page Cache
```

instead of reading from disk.

```
inode

↓

Page Cache

↓

Hit?
```

---

# Why?

Disk is slow.

RAM is fast.

Approximate access times:

| Device | Typical Latency |
|----------|----------------|
| CPU Cache | ~1 ns |
| RAM | ~100 ns |
| SSD | ~100 µs |
| HDD | ~5-10 ms |

Reading from RAM is thousands to millions of times faster than accessing storage.

---

# 7. Cache Hit

Suppose the page already exists.

```
Page Cache

↓

Found

↓

copy_to_user()

↓

Return
```

No filesystem access.

No block layer.

No disk.

No driver.

Everything happens in RAM.

---

# Cache Hit Flow

```
Application

↓

read()

↓

fd

↓

struct file

↓

inode

↓

Page Cache

↓

copy_to_user()

↓

Application Buffer
```

This is the fastest possible path.

---

# 8. Cache Miss

Suppose the requested page is not cached.

Now Linux must fetch it.

```
Page Cache

↓

Miss

↓

Filesystem

↓

Block Layer

↓

Driver

↓

Disk
```

---

# What Happens?

The filesystem converts

```
File Offset

↓

Physical Block Number
```

For example

```
Offset 8192

↓

Block 240
```

---

# 9. Block Layer

The filesystem sends a request to the Block Layer.

```
Filesystem

↓

bio

↓

Request Queue

↓

Driver
```

The Block Layer

- merges requests
- schedules I/O
- sends them to the correct device

Think of it as a traffic manager.

---

# 10. Device Driver

Now the request reaches the storage driver.

Example

```
NVMe Driver

or

SATA Driver
```

The driver communicates directly with the hardware.

```
Driver

↓

SSD

↓

Read Blocks
```

---

# 11. DMA (Direct Memory Access)

The SSD transfers data directly into RAM.

```
Disk

↓

DMA Controller

↓

RAM
```

The CPU does **not** copy each byte.

DMA performs the transfer efficiently.

---

# Page Cache Update

The data is placed into

```
Page Cache
```

Now future reads will be much faster.

---

# 12. Interrupt

After the disk finishes,

the hardware sends an interrupt.

```
Disk

↓

Interrupt

↓

Kernel

↓

Wake Waiting Process
```

Linux now knows the requested data is available.

---

# 13. copy_to_user()

The data is still inside kernel memory.

Applications cannot directly access kernel memory.

Linux copies the data.

```
Page Cache

↓

copy_to_user()

↓

Application Buffer
```

Now

```c
buf
```

contains the requested bytes.

---

# Why copy_to_user()?

Linux separates memory into

```
User Space

Kernel Space
```

Direct access is forbidden.

This provides

- security
- stability
- process isolation

---

# 14. Complete Execution Flow

```
Application

↓

read()

↓

glibc

↓

System Call

↓

Kernel

↓

File Descriptor

↓

struct file

↓

inode

↓

Page Cache

↓

Cache Hit ?

│

├── Yes

│

│ copy_to_user()

│

└── No

↓

Filesystem

↓

Block Layer

↓

Device Driver

↓

Disk

↓

DMA

↓

RAM

↓

Interrupt

↓

Page Cache

↓

copy_to_user()

↓

Application Buffer
```

---

# Complete Relationship

```
Application

↓

Buffer

↑

copy_to_user()

↑

Page Cache

↑

inode

↑

struct file

↑

File Descriptor
```

Everything starts with the File Descriptor.

Everything ends with the application buffer.

---

# Important Kernel Structures

```
files_struct
```

↓

```
fdtable
```

↓

```
struct file
```

↓

```
struct inode
```

↓

```
address_space
```

↓

```
Page Cache
```

Understanding this chain is critical for VFS interviews.

---

# Summary

A successful `read()` follows these steps:

1. Resolve the File Descriptor to a `struct file`.
2. Use the file object to locate the inode.
3. Check the Page Cache.
4. If the page is cached, copy it directly to user space.
5. If not, fetch the data through the filesystem, block layer, and device driver.
6. DMA transfers the data into RAM.
7. The page is stored in the Page Cache.
8. `copy_to_user()` copies the requested bytes into the application's buffer.

---

# Interview Questions

### Does read() always access the disk?

No.

If the requested page is already present in the Page Cache, no disk I/O occurs.

---

### What is the first object resolved during read()?

The File Descriptor.

---

### Which kernel object stores the current file offset?

`struct file`

---

### Which object maps file offsets to disk blocks?

The filesystem using information associated with the inode.

---

### Why is the Page Cache important?

It avoids repeated disk accesses by keeping recently accessed file data in RAM.

---

### What is DMA?

Direct Memory Access allows storage devices to transfer data directly into RAM without the CPU copying every byte.

---

### Why is copy_to_user() required?

Because user space cannot directly access kernel memory.

---

# What's Next?

In **Chapter 07 - How write() Works Internally**, we'll study:

```c
write(fd, buffer, size);
```

You'll learn:

- `copy_from_user()`
- Dirty Pages
- Page Cache writes
- Writeback
- Flusher Threads
- `fsync()`
- Journaling (ext4)
- How data finally reaches the disk

This chapter explains why `write()` often returns **before** the data is physically written to storage.
-----------------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 07 - How write() Works Internally

> **Goal**
>
> After this chapter, you should understand:
>
> - Why `write()` usually does NOT write directly to disk
> - How Linux uses the Page Cache for writes
> - What Dirty Pages are
> - What Writeback is
> - How `fsync()` differs from `write()`
> - How ext4 eventually stores data on disk
>
> This chapter connects:
>
> - VFS
> - Page Cache
> - Memory Management
> - Block Layer
> - Filesystem
> - Device Driver

---

# Contents

1. What write() Actually Does
2. Complete write() Flow
3. Kernel Call Flow
4. File Descriptor Lookup
5. copy_from_user()
6. Page Cache
7. Dirty Pages
8. Delayed Write
9. Writeback
10. fsync()
11. Complete Kernel Flow
12. Summary
13. Interview Questions

---

# 1. What write() Actually Does

Suppose

```c
char msg[] = "Hello";

write(fd, msg, 5);
```

Most beginners think

```
Application

↓

Disk
```

This is **wrong**.

Linux almost never writes directly to disk.

Instead

```
Application

↓

Kernel

↓

Page Cache

↓

Return

↓

Background Writeback

↓

Disk
```

This is one of the biggest optimizations in Linux.

---

# Why?

Disk is slow.

RAM is fast.

Instead of waiting for the disk,

Linux stores data in RAM first.

---

# 2. Complete write() Flow

```
Application

↓

write()

↓

glibc

↓

System Call

↓

Kernel

↓

File Descriptor

↓

struct file

↓

inode

↓

Page Cache

↓

copy_from_user()

↓

Dirty Page

↓

Return

↓

Background Writeback

↓

Filesystem

↓

Block Layer

↓

Driver

↓

Disk
```

Notice

The application returns **before** the disk write completes.

---

# 3. Kernel Call Flow

Modern kernels roughly execute

```
Application

↓

write()

↓

glibc

↓

ksys_write()

↓

vfs_write()

↓

new_sync_write()

↓

file->f_op->write_iter()

↓

generic_file_write_iter()
```

Eventually

```
Page Cache
```

is updated.

Different filesystems implement different write operations.

For example

```
ext4_file_write_iter()

xfs_file_write_iter()

nfs_file_write()
```

---

# 4. File Descriptor Lookup

Exactly like read()

```
fd

↓

files_struct

↓

fdtable

↓

struct file
```

The kernel now knows

- current offset
- inode
- filesystem

---

# 5. copy_from_user()

The application's buffer is in **User Space**.

Kernel cannot access it directly.

```
Application Buffer

↓

copy_from_user()

↓

Kernel Memory
```

This copies the data safely.

---

# Why?

Linux protects

```
Kernel Memory

User Memory
```

from each other.

This prevents

- crashes
- invalid pointers
- security issues

---

# 6. Page Cache

Now Linux writes into the Page Cache.

```
Application

↓

copy_from_user()

↓

Page Cache
```

Nothing has reached the disk yet.

The page now contains the modified data.

---

# 7. Dirty Pages

Once modified,

the page becomes

```
Dirty
```

Meaning

```
RAM

≠

Disk
```

Disk still contains

```
Old Data
```

RAM contains

```
New Data
```

Example

```
Disk

Hello

↓

Application writes

Hello Linux
```

Now

```
RAM

Hello Linux

Disk

Hello
```

The page is marked **Dirty**.

---

# Why Dirty Pages?

Writing every byte immediately to disk would be slow.

Linux batches writes together.

Benefits

- Faster
- Less disk activity
- Better SSD/HDD performance

---

# 8. Delayed Write

Suppose

```
write()

↓

Dirty Page

↓

Return
```

The application continues running.

Meanwhile

Linux waits.

Later

```
Writeback Thread

↓

Flush Dirty Pages
```

This technique is called

```
Delayed Write
```

---

# 9. Writeback

Eventually Linux flushes Dirty Pages.

```
Dirty Page

↓

Filesystem

↓

bio

↓

Block Layer

↓

Driver

↓

SSD
```

After successful write

```
Dirty

↓

Clean
```

---

## Who Performs Writeback?

Kernel background threads.

Examples

```
flush-8:0

kworker

writeback threads
```

Their job is to write Dirty Pages to storage.

---

# 10. fsync()

Suppose

```c
write(fd,data,size);
```

Application crashes immediately.

Question

Was data stored?

Maybe.

Maybe not.

Because write()

only updates RAM.

---

Now

```c
fsync(fd);
```

Flow

```
Dirty Pages

↓

Filesystem

↓

Block Layer

↓

Driver

↓

Disk

↓

Return
```

Now Linux guarantees

the data reached stable storage (subject to hardware guarantees).

---

# write() vs fsync()

| write() | fsync() |
|----------|----------|
| Updates Page Cache | Flushes Dirty Pages |
| Returns quickly | Waits for disk completion |
| Fast | Slower |
| Data may still be only in RAM | Data is committed to storage |

---

# ext4 Example

```
write()

↓

Page Cache

↓

Dirty Page

↓

Journal

↓

Writeback

↓

Disk
```

Journaling helps recover from crashes.

(We'll study journaling later.)

---

# Complete Kernel Flow

```
Application

↓

write()

↓

glibc

↓

ksys_write()

↓

vfs_write()

↓

new_sync_write()

↓

file->f_op->write_iter()

↓

generic_file_write_iter()

↓

Page Cache

↓

Dirty Page

↓

Return

↓

Writeback Thread

↓

Filesystem

↓

Block Layer

↓

Driver

↓

Disk
```

---

# Complete Data Flow

```
User Buffer

↓

copy_from_user()

↓

Page Cache

↓

Dirty Page

↓

Writeback

↓

Filesystem

↓

bio

↓

Block Layer

↓

Driver

↓

SSD
```

---

# Summary

A successful write follows these steps:

1. Resolve the File Descriptor.
2. Locate the `struct file`.
3. Locate the inode.
4. Copy user data into the Page Cache.
5. Mark the page as Dirty.
6. Return to the application.
7. Background writeback eventually writes the page to storage.
8. `fsync()` can force immediate persistence.

---

# Interview Questions

### Does write() immediately write to disk?

No.

Normally it writes only to the Page Cache and marks the page Dirty.

---

### What is a Dirty Page?

A page that has been modified in RAM but whose contents have not yet been written to storage.

---

### Why is write() faster than fsync()?

Because `write()` usually returns after updating the Page Cache, while `fsync()` waits until Dirty Pages are flushed to disk.

---

### Why does Linux delay writes?

To improve performance by combining multiple writes into fewer disk operations.

---

### Which function copies data from the application into kernel memory?

`copy_from_user()`

---

### Which kernel component eventually writes Dirty Pages to disk?

The filesystem's writeback mechanism, assisted by kernel background writeback threads.

---

# What's Next?

In **Chapter 08 - Page Cache & address_space**, we'll study one of the most important and least understood parts of Linux.

You'll learn:

- Why Page Cache exists
- `struct address_space`
- How pages are indexed
- Cache hit vs cache miss
- Read-ahead
- Write-back
- Memory management interaction
- Why VFS, MM, and filesystems all meet at the Page Cache

Understanding this chapter is essential before moving on to ext4 or NFS internals.
-----------------------------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 08 - Page Cache & address_space

> **Goal**
>
> After this chapter, you should understand:
>
> - What the Page Cache is
> - Why Linux uses it
> - What `struct address_space` does
> - How pages are stored and found
> - How read(), write(), mmap(), ext4, and NFS all use the same Page Cache
> - Why the Page Cache is the bridge between VFS and Memory Management
>
> **If you fully understand this chapter, you've understood one of the most important concepts in the Linux kernel.**

---

# Contents

1. Why Page Cache Exists
2. The Linux I/O Problem
3. What is the Page Cache?
4. struct address_space
5. Relationship Between VFS Objects
6. Reading from the Page Cache
7. Writing to the Page Cache
8. Cache Hit vs Cache Miss
9. Readahead
10. Dirty Pages
11. mmap() and the Page Cache
12. Page Cache vs Buffer Cache
13. Complete Data Flow
14. Interview Questions

---

# 1. Why Page Cache Exists

Suppose

```c
read(fd, buf, 4096);
```

Without a cache

```
Application

↓

Disk

↓

Application

↓

Disk

↓

Application

↓

Disk
```

Every read would require disk access.

Even if the same file is read 1000 times.

That would be extremely slow.

---

# Linux Solution

Linux stores recently accessed file data in RAM.

```
Disk

↓

RAM (Page Cache)

↓

Application
```

The next read comes from RAM instead of the disk.

---

# 2. The Linux I/O Problem

Storage devices are much slower than RAM.

Approximate latency

| Device | Latency |
|----------|----------|
| CPU Cache | ~1 ns |
| RAM | ~100 ns |
| NVMe SSD | ~100 µs |
| HDD | ~5-10 ms |

Disk access is thousands to millions of times slower than memory.

The Page Cache hides most of this latency.

---

# 3. What is the Page Cache?

The Page Cache is a region of RAM used by the kernel to cache **file data**.

```
Disk

↓

Page Cache

↓

Application
```

Think of it as a copy of frequently used file pages.

---

## Important

The Page Cache stores

✅ File contents

It does **not** store

- File Descriptor
- Dentry
- Inode metadata
- Permissions

Those are different kernel objects.

---

# Example

Suppose

```
report.pdf
```

contains

```
64 KB
```

Linux divides it into pages.

```
Page 0

Page 1

Page 2

Page 3
```

Usually

```
4 KB

+

4 KB

+

4 KB

...
```

Each page may be cached independently.

---

# 4. struct address_space

This is one of the most important kernel structures.

Every inode owns one.

```
inode

↓

address_space

↓

Page Cache
```

---

## Why?

The inode describes

```
What file?
```

The address_space describes

```
Where are this file's cached pages?
```

---

Think of it like this

```
inode

↓

Metadata

↓

address_space

↓

Cached Data
```

---

## Simplified Structure

```text
struct inode
        │
        ▼
struct address_space
        │
        ▼
Cached Pages
```

---

# 5. Relationship Between VFS Objects

Everything now connects.

```
Application

↓

File Descriptor

↓

struct file

↓

inode

↓

address_space

↓

Page Cache

↓

Disk
```

Notice

The Page Cache belongs to the inode through the address_space.

---

# 6. Reading from the Page Cache

Suppose

```c
read(fd, buf, 4096);
```

Flow

```
fd

↓

struct file

↓

inode

↓

address_space

↓

Page Cache
```

Question

```
Page Present?
```

---

## Cache Hit

```
Page Cache

↓

Found

↓

copy_to_user()

↓

Return
```

No disk access.

---

## Cache Miss

```
Page Cache

↓

Not Found

↓

Filesystem

↓

Block Layer

↓

Driver

↓

Disk

↓

DMA

↓

Page Cache

↓

copy_to_user()
```

Now the page is cached.

---

# 7. Writing to the Page Cache

Suppose

```c
write(fd, buf, 4096);
```

Linux does

```
copy_from_user()

↓

Page Cache

↓

Dirty Page
```

The disk is not updated immediately.

---

# 8. Cache Hit vs Cache Miss

## Cache Hit

```
Application

↓

Page Cache

↓

Return
```

Fast.

---

## Cache Miss

```
Application

↓

Filesystem

↓

Disk

↓

Page Cache

↓

Return
```

Slower.

Future reads become cache hits.

---

# 9. Readahead

Linux assumes

"If you read one page,
you'll probably read the next."

Example

```
Application reads

Page 10
```

Linux loads

```
Page 10

Page 11

Page 12

Page 13
```

This is called

```
Readahead
```

Sequential reads become much faster.

---

# 10. Dirty Pages

Suppose

```
write()

↓

Page Cache
```

The page changes.

Now

```
RAM

≠

Disk
```

Linux marks the page

```
Dirty
```

Later

```
Writeback

↓

Disk
```

After writeback

```
Dirty

↓

Clean
```

---

# 11. mmap() and the Page Cache

One of Linux's greatest optimizations.

Instead of

```
Disk

↓

Page Cache

↓

copy_to_user()

↓

Application
```

Linux maps the cached pages directly into the process address space.

```
Disk

↓

Page Cache

↓

Virtual Memory

↓

Application
```

No explicit `read()` call is required.

The application accesses memory directly.

---

# 12. Page Cache vs Buffer Cache

Older Linux kernels had

```
Page Cache

Buffer Cache
```

Modern Linux

```
Unified Page Cache
```

File data is cached in the Page Cache.

Filesystem metadata and block-related information are managed through the page cache and associated block I/O mechanisms rather than a separate buffer cache.

---

# 13. Complete Data Flow

```
Application

↓

File Descriptor

↓

struct file

↓

inode

↓

address_space

↓

Page Cache

↓

Cache Hit ?

│

├── Yes

│

│ copy_to_user()

│

└── No

↓

Filesystem

↓

Block Layer

↓

Driver

↓

Disk

↓

DMA

↓

Page Cache

↓

copy_to_user()

↓

Application
```

---

# Complete Relationship Diagram

```
Application
      │
      ▼
File Descriptor
      │
      ▼
struct file
      │
      ▼
inode
      │
      ▼
address_space
      │
      ▼
Page Cache
      │
      ▼
Filesystem
      │
      ▼
Block Layer
      │
      ▼
Device Driver
      │
      ▼
Disk
```

**This is one of the most important diagrams in Linux.**

---

# Summary

The Page Cache:

- Stores file **data** in RAM.
- Is owned by an inode through `struct address_space`.
- Speeds up reads by avoiding disk I/O.
- Buffers writes as Dirty Pages before writeback.
- Is shared by `read()`, `write()`, and `mmap()`.
- Connects VFS, Memory Management, and the filesystem.

Remember this dependency chain:

```
inode

↓

address_space

↓

Page Cache

↓

Disk
```

---

# Interview Questions

### Does every inode have an `address_space`?

Yes. Every regular file inode has an associated `address_space` that manages its cached pages.

---

### Does the Page Cache store metadata?

No.

It stores **file contents**. Metadata such as permissions and timestamps belongs to the inode.

---

### Who owns the Page Cache?

Conceptually, each file's cached pages are managed by the file's `address_space`, which is associated with its inode.

---

### Why is `address_space` needed?

It maps a file's logical page offsets to cached memory pages and provides the interface between the VFS, Memory Management, and the filesystem.

---

### Does `mmap()` use the Page Cache?

Yes.

For file-backed mappings, `mmap()` uses the same Page Cache that `read()` and `write()` use.

---

### Why is the Page Cache considered the bridge between VFS and Memory Management?

Because the VFS identifies *which file* is being accessed, while Memory Management manages the physical pages that cache that file's contents. `struct address_space` links these two subsystems.

---

# What's Next?

The next chapter, **Chapter 09 - Block Layer & BIO**, moves below the filesystem.

You'll learn:

- How ext4/NFS send I/O requests
- What a `bio` is
- How requests are merged
- The request queue
- `blk-mq` (Multi-Queue Block Layer)
- NVMe vs SATA request flow
- How an I/O request finally reaches the storage device

This chapter completes the journey from the VFS down to the hardware.
-----------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 09 - Linux Block Layer & BIO

> **Goal**
>
> After reading this chapter, you should understand:
>
> - Why Linux has a Block Layer
> - What a BIO (Block I/O) is
> - What Request Queues are
> - What blk-mq is
> - How ext4 communicates with storage devices
> - Complete Disk I/O flow
>
> **This chapter connects the Filesystem with the Device Driver.**

---

# Contents

1. Why Block Layer Exists
2. Linux Storage Stack
3. What is a Block Device?
4. Block Layer Overview
5. What is BIO?
6. BIO Lifecycle
7. Request Queue
8. blk-mq (Multi Queue Block Layer)
9. Device Driver
10. Complete Read Flow
11. Complete Write Flow
12. NVMe vs SATA
13. Interview Questions

---

# 1. Why Block Layer Exists

Suppose ext4 wants to read a block.

Should ext4 know how to communicate with

- SATA
- NVMe
- USB
- RAID
- SAN

No.

That would make every filesystem hardware dependent.

Instead Linux inserts another abstraction layer.

```
Filesystem

↓

Block Layer

↓

Driver

↓

Disk
```

Exactly like VFS hides filesystem differences,

the Block Layer hides storage hardware differences.

---

# 2. Linux Storage Stack

```
Application

↓

VFS

↓

Filesystem (ext4)

↓

Block Layer

↓

Device Driver

↓

Storage Device
```

Each layer has one responsibility.

| Layer | Responsibility |
|--------|----------------|
| VFS | Common filesystem interface |
| Filesystem | Convert file offsets into disk blocks |
| Block Layer | Manage block I/O |
| Driver | Talk to hardware |
| Device | Store data |

---

# 3. What is a Block Device?

A block device stores data in fixed-size blocks.

Examples

```
SSD

HDD

NVMe

USB Flash Drive

eMMC

SD Card
```

Typical block size

```
4096 Bytes
```

or

```
512 Bytes
```

Unlike character devices,

block devices support

- Random access
- Block addressing
- Request scheduling

---

# 4. Block Layer Overview

Suppose

```c
read(fd, buffer, 4096);
```

The filesystem knows

```
File Offset

↓

Physical Block Number
```

Example

```
Offset 8192

↓

Block 32500
```

The Block Layer receives

```
Read Block 32500
```

instead of

```
Read report.pdf
```

Notice

The Block Layer doesn't know filenames.

It only knows blocks.

---

# 5. What is BIO?

BIO stands for

```
Block I/O
```

Kernel structure

```
struct bio
```

A BIO represents

```
One Block I/O Operation
```

Example

```
Read Block

Write Block

Flush Cache

Discard Block
```

---

## Simplified BIO

```
BIO

↓

Operation

↓

Start Block

↓

Length

↓

Memory Pages

↓

Target Device
```

---

Example

```
Read

↓

Block 400

↓

Length 8 KB
```

---

# BIO Lifecycle

```
Filesystem

↓

Create BIO

↓

Submit BIO

↓

Block Layer

↓

Driver

↓

Disk
```

---

# 6. Request Queue

Suppose

Applications generate

```
Read Block 10

Read Block 11

Read Block 12

Read Block 13
```

Sending every request separately is inefficient.

Linux combines them.

```
BIO

↓

Request Queue

↓

Merged Request

↓

Driver
```

---

# Why Merge?

Instead of

```
Read Block 10

Read Block 11

Read Block 12
```

Linux creates

```
Read Blocks

10-12
```

One larger request.

Benefits

- Fewer commands
- Better throughput
- Less disk seek
- Better SSD utilization

---

# 7. blk-mq

Older Linux

```
Single Request Queue
```

Modern Linux

```
blk-mq

↓

Multiple Hardware Queues
```

---

## Why?

Modern NVMe SSDs

can process

```
Thousands

of requests

simultaneously.
```

One queue becomes a bottleneck.

---

## blk-mq Architecture

```
Filesystem

↓

BIO

↓

Software Queue

↓

Hardware Queue

↓

NVMe Driver

↓

SSD
```

Multiple CPUs

↓

Multiple Queues

↓

Higher Performance

---

# 8. Device Driver

The driver converts generic requests into hardware commands.

Example

```
BIO

↓

NVMe Driver

↓

NVMe Command

↓

SSD
```

or

```
BIO

↓

SATA Driver

↓

ATA Command

↓

HDD
```

Filesystem doesn't know either command.

Driver handles everything.

---

# 9. Complete Read Flow

Suppose

```c
read(fd, buffer, 4096);
```

Kernel Flow

```
Application

↓

read()

↓

VFS

↓

ext4

↓

Physical Block

↓

BIO

↓

Request Queue

↓

NVMe Driver

↓

SSD

↓

DMA

↓

RAM

↓

Page Cache

↓

copy_to_user()

↓

Application
```

---

# 10. Complete Write Flow

```
Application

↓

write()

↓

Page Cache

↓

Dirty Page

↓

Writeback

↓

ext4

↓

BIO

↓

Request Queue

↓

Driver

↓

SSD
```

---

# 11. DMA

Driver tells storage device

```
Transfer

↓

RAM
```

CPU doesn't copy data.

Storage controller performs

```
DMA

↓

Memory
```

Benefits

- Lower CPU usage
- Faster transfers

---

# 12. NVMe vs SATA

### SATA

```
Filesystem

↓

BIO

↓

Request Queue

↓

SATA Driver

↓

HDD / SSD
```

Usually

```
One Command Queue
```

---

### NVMe

```
Filesystem

↓

BIO

↓

blk-mq

↓

Many Hardware Queues

↓

NVMe Driver

↓

SSD
```

Thousands of concurrent requests.

Much higher performance.

---

# Complete Storage Stack

```
Application

↓

glibc

↓

System Call

↓

VFS

↓

Filesystem

↓

BIO

↓

Block Layer

↓

Request Queue

↓

Driver

↓

DMA

↓

SSD

↓

Interrupt

↓

Kernel

↓

Application
```

---

# Relationship with Previous Chapters

```
read()

↓

File Descriptor

↓

struct file

↓

inode

↓

address_space

↓

Page Cache

↓

Cache Miss

↓

Filesystem

↓

BIO

↓

Block Layer

↓

Driver

↓

Disk
```

Notice

The Block Layer is used

**only when the Page Cache misses**

or during writeback.

---

# Simplified Kernel Structures

## struct bio

```
struct bio

↓

Operation

↓

Device

↓

Memory Pages

↓

Sector

↓

Length
```

---

## Request

```
Request

↓

Multiple BIOs

↓

Driver
```

One Request

may contain

multiple BIOs.

---

# Summary

The Block Layer

- Accepts BIOs from filesystems
- Merges requests
- Schedules I/O
- Sends requests to device drivers
- Supports multiple hardware queues
- Hides storage hardware differences

Remember

```
Filesystem

↓

BIO

↓

Request Queue

↓

Driver

↓

Disk
```

---

# Interview Questions

### What is BIO?

A kernel object representing a block I/O operation.

---

### Who creates the BIO?

Usually the filesystem.

---

### Does VFS know about BIO?

No.

VFS works with files.

BIO exists below the filesystem layer.

---

### Why is the Block Layer needed?

To isolate filesystems from storage hardware and optimize block I/O.

---

### What is blk-mq?

The Linux Multi-Queue Block Layer that allows multiple CPUs and modern storage devices (especially NVMe SSDs) to process I/O requests concurrently.

---

### Why merge BIOs?

To reduce the number of I/O operations and improve storage throughput.

---

### Does every read() reach the Block Layer?

No.

Only cache misses (or operations requiring storage access) are sent to the Block Layer.

---

# What's Next?

The next chapter is **Chapter 10 – ext4 Filesystem Internals**.

This is where we'll dive into:

- ext4 disk layout
- Superblock
- Block Groups
- Inode Table
- Directory structure
- Journaling (JBD2)
- Delayed Allocation
- Extents
- Complete ext4 read/write flow

You'll finally understand how Linux maps:

```
/home/user/report.pdf

↓

inode

↓

Extent

↓

Physical Block

↓

SSD
```

This is the foundation needed before moving on to **NFS**, where you'll see how the same VFS interface works across the network instead of a local disk.
----------------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 10 - ext4 Filesystem Internals

> **Goal**
>
> After reading this chapter, you should understand:
>
> - How ext4 stores files on disk
> - Superblock, Block Groups, Inodes, Data Blocks
> - What Extents are
> - Delayed Allocation
> - Journaling (JBD2)
> - Complete ext4 read/write flow
>
> **This chapter explains what happens after VFS hands control to ext4.**

---

# Contents

1. Why ext4?
2. ext4 Disk Layout
3. Superblock
4. Block Groups
5. Block Bitmap
6. Inode Bitmap
7. Inode Table
8. Data Blocks
9. Extents
10. Delayed Allocation
11. Journaling (JBD2)
12. Complete Read Flow
13. Complete Write Flow
14. Summary
15. Interview Questions

---

# 1. Why ext4?

Until now we've learned

```
Application

↓

VFS

↓

Filesystem
```

Now the question becomes

> **How does ext4 know where a file is stored?**

Unlike VFS,

ext4 understands

- physical blocks
- inode tables
- free space
- block allocation
- journaling

---

# 2. ext4 Disk Layout

A disk formatted with ext4 looks like this

```text
+--------------------------------------------------------------+
| Superblock                                                   |
+--------------------------------------------------------------+
| Group Descriptor Table                                       |
+--------------------------------------------------------------+
| Block Group 0                                                |
|   ├── Block Bitmap                                           |
|   ├── Inode Bitmap                                           |
|   ├── Inode Table                                            |
|   └── Data Blocks                                            |
+--------------------------------------------------------------+
| Block Group 1                                                |
|   ├── Block Bitmap                                           |
|   ├── Inode Bitmap                                           |
|   ├── Inode Table                                            |
|   └── Data Blocks                                            |
+--------------------------------------------------------------+
| Block Group 2                                                |
|        ...                                                   |
+--------------------------------------------------------------+
```

Instead of managing one huge disk,

ext4 divides it into **Block Groups**.

---

# Why Block Groups?

Imagine a 2 TB disk.

Searching the entire disk for a free block would be slow.

Instead,

ext4 searches within a nearby Block Group.

Benefits

- Faster allocation
- Less fragmentation
- Better locality

---

# 3. Superblock

Every ext4 filesystem has one primary Superblock.

It stores filesystem-wide metadata.

```
Superblock

↓

Block Size

↓

Total Blocks

↓

Total Inodes

↓

Filesystem UUID

↓

Root Inode

↓

Features
```

Without the Superblock,

the filesystem cannot be mounted.

---

## Important

VFS creates

```
struct super_block
```

by reading the on-disk ext4 Superblock during `mount()`.

---

# 4. Block Groups

Each Block Group is almost like a small filesystem.

```
Block Group

↓

Block Bitmap

↓

Inode Bitmap

↓

Inode Table

↓

Data Blocks
```

This design keeps related data physically close.

---

# 5. Block Bitmap

The Block Bitmap tracks which data blocks are free.

Example

```
0 = Free

1 = Used
```

```
111001010001...
```

When ext4 needs a new block,

it scans this bitmap.

---

# 6. Inode Bitmap

Exactly the same idea,

but for inodes.

```
0 = Free inode

1 = Allocated inode
```

When a new file is created,

ext4 finds a free inode here.

---

# 7. Inode Table

Every inode lives inside the Inode Table.

Example

```
Inode 2

↓

Root Directory
```

```
Inode 150

↓

notes.txt
```

Each inode stores

- permissions
- owner
- timestamps
- file size
- extent information

Notice

Still **no filename**.

Filename belongs to the directory entry.

---

# Relationship

```
Filename

↓

Directory Entry

↓

Inode

↓

Extent

↓

Physical Blocks
```

---

# 8. Data Blocks

Finally,

the file contents.

```
Inode

↓

Extent

↓

Data Blocks
```

Example

```
notes.txt

↓

Hello Linux
```

The characters are stored only inside Data Blocks.

---

# 9. Extents

Older filesystems stored

```
Block 100

↓

Block 101

↓

Block 102

↓

Block 103
```

using one pointer per block.

For large files,

this required many pointers.

---

## ext4 Solution

An **Extent** stores a continuous range.

Instead of

```
100

101

102

103
```

Store

```
Start Block = 100

Length = 4
```

One extent replaces many block pointers.

---

## Example

```
Extent

↓

Start = 5000

Length = 256 Blocks
```

Benefits

- Smaller metadata
- Faster lookup
- Less fragmentation
- Better sequential performance

---

# 10. Delayed Allocation

Suppose

```c
write(fd, buf, 4096);
```

Does ext4 immediately allocate blocks?

Usually **No**.

Instead

```
Page Cache

↓

Dirty Page

↓

Wait
```

Actual block allocation happens later during writeback.

---

## Why?

If the application writes

```
4 KB

↓

4 KB

↓

4 KB

↓

4 KB
```

ext4 can allocate one larger contiguous extent.

Benefits

- Better performance
- Reduced fragmentation

---

# 11. Journaling (JBD2)

Imagine

```
Create File

↓

Power Failure
```

Without journaling,

filesystem metadata could become inconsistent.

---

## ext4 Solution

Before modifying filesystem metadata,

ext4 records the operation in the journal.

```
Operation

↓

Journal

↓

Filesystem Metadata

↓

Journal Complete
```

If the system crashes,

the journal is replayed during mount.

---

## What is Journaled?

Mainly **metadata**.

Examples

- inode updates
- directory updates
- block allocation

Depending on mount options,

file data may also be journaled, but the default mode (`ordered`) journals metadata while ensuring data blocks are written before the related metadata is committed.

---

# 12. Complete Read Flow

Suppose

```c
read(fd, buf, 4096);
```

Flow

```
Application

↓

VFS

↓

inode

↓

address_space

↓

Page Cache

↓

Cache Miss

↓

ext4

↓

Extent

↓

Physical Block

↓

BIO

↓

Block Layer

↓

Driver

↓

SSD

↓

DMA

↓

Page Cache

↓

copy_to_user()

↓

Application
```

---

# 13. Complete Write Flow

```
Application

↓

write()

↓

Page Cache

↓

Dirty Page

↓

Writeback

↓

ext4

↓

Allocate Extent

↓

Update Journal

↓

BIO

↓

Block Layer

↓

Driver

↓

SSD
```

---

# Complete ext4 Picture

```
Filename

↓

Directory Entry

↓

Inode

↓

Extent

↓

Physical Block

↓

BIO

↓

Block Layer

↓

Driver

↓

SSD
```

This is the complete mapping from a filename to the storage device.

---

# Summary

ext4 organizes the disk into:

```
Superblock

↓

Block Groups

↓

Inode Table

↓

Inodes

↓

Extents

↓

Physical Blocks
```

Key features:

- Block Groups improve locality.
- Extents efficiently describe large contiguous files.
- Delayed Allocation reduces fragmentation.
- Journaling protects filesystem metadata from crashes.

---

# Interview Questions

### Does an inode store the filename?

No.

The filename is stored in a directory entry (dentry on the VFS side, directory records on disk).

---

### What is an Extent?

A descriptor that maps a contiguous range of logical file blocks to a contiguous range of physical disk blocks.

---

### Why are Extents better than individual block pointers?

They require less metadata and improve performance for large sequential files.

---

### Why does ext4 use Delayed Allocation?

To choose better block layouts during writeback, improving locality and reducing fragmentation.

---

### What is Journaling?

Recording metadata updates in a journal before applying them, allowing recovery after crashes.

---

### Does ext4 journal file data?

By default (`data=ordered`), ext4 journals **metadata**, while ensuring modified file data reaches disk before the associated metadata commit. Other journaling modes are also available.

---

# What's Next?

The next chapter is **Chapter 11 - NFS Internals**.

You'll learn:

- Why NFS does not access local disks
- How VFS works with remote filesystems
- NFS Client and Server architecture
- RPC (Remote Procedure Call)
- File Handles
- Attribute Cache
- Page Cache with NFS
- Complete network read/write flow

After Chapter 11, you'll understand how the **same VFS interface** supports both:

- Local filesystems (ext4, XFS)
- Remote filesystems (NFS)

without applications needing to know the difference.
-------------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 11 - NFS (Network File System) Internals

> **Goal**
>
> After reading this chapter, you should understand:
>
> - How NFS works internally
> - How VFS supports remote filesystems
> - NFS Client and Server architecture
> - RPC (Remote Procedure Call)
> - File Handles
> - Attribute Cache
> - Page Cache with NFS
> - Complete NFS read/write flow
>
> **NFS is simply another filesystem from the VFS perspective. The difference is that data comes from another machine instead of a local disk.**

---

# Contents

1. What is NFS?
2. Why NFS?
3. NFS Architecture
4. Mounting an NFS Share
5. File Handles
6. How open() Works
7. How read() Works
8. How write() Works
9. NFS Caching
10. Attribute Cache
11. Close-to-Open Consistency
12. Complete Read & Write Flow
13. Common Interview Questions

---

# 1. What is NFS?

NFS (Network File System) allows one Linux machine to access files stored on another Linux machine.

Example

```
Application

↓

VFS

↓

NFS Client

↓

Network

↓

NFS Server

↓

ext4

↓

SSD
```

To the application,

there is **no difference** between

```
/home/file.txt
```

and

```
/mnt/nfs/file.txt
```

The application still calls

```c
open();
read();
write();
close();
```

---

# 2. Why NFS?

Without NFS

```
Machine A

Own Disk
```

```
Machine B

Own Disk
```

Files cannot be shared easily.

---

With NFS

```
Machine A

↓

Network

↓

Machine B

↓

Shared Filesystem
```

Multiple machines can access the same files.

---

# 3. NFS Architecture

```
+----------------------------+
| Client                     |
|----------------------------|
| Application                |
| VFS                        |
| NFS Client                 |
+-------------|--------------+
              |
        TCP/IP Network
              |
+-------------|--------------+
| Server                     |
|----------------------------|
| NFS Server                 |
| VFS                        |
| ext4/XFS                   |
| Block Layer                |
| SSD/HDD                    |
+----------------------------+
```

The client never accesses the disk directly.

Only the server does.

---

# 4. Mounting an NFS Share

Example

```bash
mount -t nfs server:/export/data /mnt/nfs
```

After mounting

```
Application

↓

/mnt/nfs/file.txt
```

looks like a normal local file.

Internally

```
VFS

↓

NFS Client

↓

RPC

↓

Server
```

---

# 5. File Handles

Local filesystems use

```
inode
```

NFS cannot send kernel pointers or inode addresses across the network.

Instead,

the server returns a

```
File Handle
```

Think of it as a unique identifier for a file.

```
Client

↓

File Handle

↓

Server

↓

inode
```

The client stores the file handle.

Future requests use it.

---

# 6. How open() Works

Application

```c
fd = open("/mnt/nfs/report.txt", O_RDONLY);
```

Flow

```
Application

↓

glibc

↓

System Call

↓

VFS

↓

NFS Client

↓

RPC LOOKUP

↓

NFS Server

↓

VFS

↓

ext4

↓

inode

↓

File Handle

↓

Client

↓

struct file

↓

fd
```

Notice

The client never receives the server's inode.

It receives a file handle.

---

# 7. How read() Works

Suppose

```c
read(fd, buf, 4096);
```

First,

the client checks its own Page Cache.

```
fd

↓

struct file

↓

Page Cache
```

---

## Cache Hit

```
Page Cache

↓

copy_to_user()

↓

Application
```

No network request.

---

## Cache Miss

```
Application

↓

VFS

↓

NFS Client

↓

RPC READ

↓

Network

↓

NFS Server

↓

ext4

↓

SSD

↓

Server Page Cache

↓

Network

↓

Client Page Cache

↓

copy_to_user()

↓

Application
```

Notice

Both the client **and** the server may cache file data.

---

# 8. How write() Works

```
Application

↓

write()

↓

copy_from_user()

↓

Client Page Cache

↓

Dirty Page

↓

RPC WRITE

↓

Server

↓

Server Page Cache

↓

ext4

↓

Writeback

↓

SSD
```

The client and server both participate.

---

# 9. NFS Caching

NFS uses several caches.

### Client

```
Page Cache

Attribute Cache

Directory Cache
```

---

### Server

```
Page Cache

inode Cache

Dentry Cache
```

Caching reduces network traffic.

---

# 10. Attribute Cache

File metadata

```
Size

Permissions

Owner

Timestamp
```

is cached on the client.

Without it

Every

```
stat()

open()

ls
```

would require a network request.

---

# 11. Close-to-Open Consistency

Suppose

Machine A writes

```
notes.txt
```

Machine B opens

```
notes.txt
```

NFS checks whether the file changed since it was last cached.

If necessary,

the client refreshes its cache.

This behavior is called

```
Close-to-Open Consistency
```

It provides a practical consistency model for many workloads, though it is not the same as strict real-time consistency.

---

# 12. Complete Read Flow

```
Application

↓

read()

↓

VFS

↓

NFS Client

↓

Client Page Cache

↓

Cache Hit?

│

├── Yes

│

│ copy_to_user()

│

└── No

↓

RPC READ

↓

TCP

↓

NFS Server

↓

VFS

↓

ext4

↓

Server Page Cache

↓

Cache Hit?

│

├── Yes

│

│ Return Data

│

└── No

↓

SSD

↓

Server Page Cache

↓

Network

↓

Client Page Cache

↓

copy_to_user()

↓

Application
```

---

# Complete Write Flow

```
Application

↓

write()

↓

copy_from_user()

↓

Client Page Cache

↓

RPC WRITE

↓

Server

↓

Server Page Cache

↓

Dirty Page

↓

Writeback

↓

ext4

↓

SSD
```

---

# Local File vs NFS File

## Local File

```
Application

↓

VFS

↓

ext4

↓

SSD
```

---

## NFS File

```
Application

↓

VFS

↓

NFS Client

↓

Network

↓

NFS Server

↓

VFS

↓

ext4

↓

SSD
```

The VFS hides this complexity.

Applications use the same APIs.

---

# Relationship with Previous Chapters

Everything learned so far still applies.

```
Application

↓

File Descriptor

↓

struct file

↓

VFS

↓

Filesystem

↓

Page Cache
```

The only difference is:

- For **ext4**, the filesystem accesses the local block layer.
- For **NFS**, the filesystem communicates with a remote server using RPC.

---

# Summary

NFS extends the VFS over a network.

Key points:

- Applications use the same POSIX APIs.
- VFS treats NFS like any other filesystem.
- The client communicates with the server using RPC.
- The server performs actual disk I/O.
- Both client and server use Page Cache.
- File Handles identify files across the network.

Remember this flow:

```
Application

↓

VFS

↓

NFS Client

↓

RPC

↓

NFS Server

↓

VFS

↓

ext4

↓

Disk
```

---

# Interview Questions

### Does VFS know whether a file is local or remote?

No.

VFS dispatches operations through filesystem-specific methods. The application uses the same APIs regardless of the underlying filesystem.

---

### Why does NFS use File Handles instead of inode pointers?

Because inode pointers are only valid inside the server's kernel. A file handle is a portable identifier that can be sent across the network.

---

### Does NFS use the Page Cache?

Yes.

Both the client and the server maintain their own Page Caches.

---

### Does every read() generate a network request?

No.

If the requested data is already present and valid in the client's Page Cache, no network access is needed.

---

### Who performs the actual disk I/O?

The NFS server.

The client never accesses the server's storage device directly.

---

### What is Close-to-Open Consistency?

A consistency model where a client validates cached data when a file is opened after another client has closed it following modifications.

---

# What's Next?

The final chapter of the VFS handbook should be:

# Chapter 12 - Complete Linux File I/O Journey

In that chapter we'll connect **everything** you've learned into one end-to-end story:

```
Application

↓

glibc

↓

System Call

↓

VFS

↓

Path Lookup

↓

Dentry Cache

↓

inode

↓

address_space

↓

Page Cache

↓

Filesystem (ext4 / NFS)

↓

BIO

↓

Block Layer

↓

Device Driver

↓

DMA

↓

SSD

↓

Interrupt

↓

Page Cache

↓

copy_to_user()

↓

Application
```

That chapter becomes the **master revision sheet** for Linux Embedded, Kernel, Filesystem, and Storage interviews, tying together all previous chapters into a single coherent execution flow.
-------------------------------------------------------------------------------
# Linux VFS Deep Dive
# Chapter 12 - Complete Linux File I/O Journey (Master Revision)

> **Goal**
>
> This chapter connects everything you've learned into one complete execution flow.
>
> After finishing this chapter, you should be able to explain the entire journey of a file operation—from the application to the storage device and back.
>
> **If you can explain this chapter without looking at the notes, you're ready for most Linux Embedded, Kernel, Filesystem, and Storage interviews.**

---

# Complete Handbook Dependency

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
VFS
      │
      ▼
Path Lookup
      │
      ▼
Dentry Cache
      │
      ▼
inode
      │
      ▼
address_space
      │
      ▼
Page Cache
      │
      ▼
Filesystem
(ext4 / XFS / NFS)
      │
      ▼
BIO
      │
      ▼
Block Layer
      │
      ▼
Device Driver
      │
      ▼
DMA
      │
      ▼
SSD / HDD
```

Every chapter fits into this dependency chain.

---

# Example Program

```c
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd = open("hello.txt", O_RDONLY);

    char buffer[64];

    read(fd, buffer, sizeof(buffer));

    close(fd);

    return 0;
}
```

This tiny program executes thousands of lines of Linux kernel code.

---

# Step 1 — Application

```
Application

↓

open()

↓

read()

↓

close()
```

Application runs entirely in **User Space**.

No hardware access is allowed.

---

# Step 2 — glibc

Application calls

```c
open()

read()

write()

close()
```

glibc converts them into

```
System Calls
```

---

# Step 3 — System Call

CPU switches

```
User Mode

↓

Kernel Mode
```

Kernel execution begins.

---

# Step 4 — VFS

Kernel receives

```
open("hello.txt")
```

VFS becomes responsible.

Questions answered by VFS

```
Which filesystem?

Where is the inode?

Permission?

Mounted filesystem?

Operations?
```

---

# Step 5 — Path Lookup

Suppose

```
/home/user/hello.txt
```

Kernel walks

```
/

↓

home

↓

user

↓

hello.txt
```

Each component checks

```
Dentry Cache
```

---

# Dentry Cache

If found

```
RAM

↓

Continue
```

Otherwise

```
Filesystem Lookup

↓

Create Dentry

↓

Cache It
```

---

# Step 6 — inode

After pathname resolution

Linux finds

```
inode
```

The inode stores

- file size
- owner
- permissions
- timestamps
- extent mapping

Not

- filename

---

# Step 7 — File Object

Linux creates

```
struct file
```

Stores

- offset
- flags
- inode
- file operations

Every

```
open()
```

creates

```
struct file
```

---

# Step 8 — File Descriptor

Kernel inserts

```
struct file
```

into

```
Process FD Table
```

Returns

```
fd = 3
```

Application only sees

```
3
```

---

# Step 9 — read()

Application calls

```c
read(fd,buffer,100);
```

Kernel resolves

```
fd

↓

struct file

↓

inode

↓

address_space
```

---

# Step 10 — Page Cache

Question

```
Page Present?
```

---

## Cache Hit

```
Page Cache

↓

copy_to_user()

↓

Application
```

Done.

No disk access.

---

## Cache Miss

```
Filesystem

↓

Extent

↓

Physical Block

↓

BIO

↓

Block Layer

↓

Driver

↓

SSD
```

---

# Step 11 — BIO

Filesystem creates

```
struct bio
```

BIO contains

```
Operation

Sector

Length

Pages
```

---

# Step 12 — Block Layer

BIO enters

```
Request Queue
```

Linux may merge requests

```
Read Block 10

Read Block 11

↓

Read Blocks 10-11
```

Better performance.

---

# Step 13 — Device Driver

Driver converts

```
BIO

↓

NVMe Command

or

ATA Command
```

Hardware now understands the request.

---

# Step 14 — DMA

Storage device transfers data

```
SSD

↓

DMA

↓

RAM
```

CPU doesn't copy every byte.

---

# Step 15 — Interrupt

SSD finishes

↓

Interrupt

↓

Kernel

↓

Waiting Process Wakes Up

---

# Step 16 — Page Cache Update

Kernel stores

```
Disk Data

↓

Page Cache
```

Future reads

↓

Cache Hit

---

# Step 17 — copy_to_user()

Kernel copies

```
Page Cache

↓

User Buffer
```

Application finally receives data.

---

# Complete Read Flow

```
Application

↓

glibc

↓

System Call

↓

VFS

↓

Path Lookup

↓

Dentry Cache

↓

inode

↓

address_space

↓

Page Cache

↓

Cache Hit ?

│

├── Yes

│

│ copy_to_user()

│

└── No

↓

Filesystem

↓

Extent

↓

BIO

↓

Block Layer

↓

Driver

↓

SSD

↓

DMA

↓

Interrupt

↓

Page Cache

↓

copy_to_user()

↓

Application
```

---

# Complete Write Flow

```
Application

↓

write()

↓

copy_from_user()

↓

Page Cache

↓

Dirty Page

↓

Return

↓

Writeback Thread

↓

Filesystem

↓

Journal

↓

Extent Allocation

↓

BIO

↓

Block Layer

↓

Driver

↓

SSD
```

Notice

```
write()
```

returns

BEFORE

disk write completes.

---

# Local File vs NFS

## Local

```
Application

↓

VFS

↓

ext4

↓

BIO

↓

Driver

↓

SSD
```

---

## NFS

```
Application

↓

VFS

↓

NFS Client

↓

RPC

↓

Network

↓

NFS Server

↓

ext4

↓

BIO

↓

SSD
```

Application never knows the difference.

---

# Relationship Between All Important Structures

```
Process
   │
   ▼
files_struct
   │
   ▼
fdtable
   │
   ▼
struct file
   │
   ▼
dentry
   │
   ▼
inode
   │
   ▼
address_space
   │
   ▼
Page Cache
   │
   ▼
Filesystem
   │
   ▼
BIO
   │
   ▼
Request Queue
   │
   ▼
Driver
   │
   ▼
Storage Device
```

This is the single most important diagram in the entire handbook.

---

# Which Structure Stores What?

| Structure | Stores |
|------------|--------|
| super_block | Mounted filesystem information |
| dentry | Pathname component |
| inode | File metadata |
| struct file | Open instance of a file |
| File Descriptor | Index into process FD table |
| address_space | Cached pages for an inode |
| Page Cache | File contents in RAM |
| BIO | Block I/O request |

---

# Interview Story (5-Minute Answer)

If an interviewer asks:

> **"Explain what happens when read() is called."**

A complete answer is:

1. Application calls `read()`.
2. glibc performs a system call.
3. CPU switches to kernel mode.
4. Kernel resolves the File Descriptor to a `struct file`.
5. `struct file` points to the inode.
6. The inode's `address_space` is checked for cached pages.
7. If the page is present, `copy_to_user()` returns the data.
8. If not, the filesystem maps the file offset to physical blocks.
9. The filesystem creates BIOs.
10. The Block Layer schedules the requests.
11. The device driver issues hardware commands.
12. DMA transfers data into RAM.
13. An interrupt notifies the kernel.
14. The Page Cache is updated.
15. `copy_to_user()` copies the data to the application's buffer.
16. `read()` returns the requested bytes.

---

# Complete Linux Storage Stack

```
Application
      │
glibc
      │
System Call
      │
VFS
      │
Filesystem
      │
Page Cache
      │
BIO
      │
Block Layer
      │
Device Driver
      │
Storage Controller
      │
SSD / HDD
```

---

# Things Interviewers Frequently Ask

### Difference between inode and dentry?

```
dentry

↓

Name
```

```
inode

↓

Metadata
```

---

### Difference between inode and struct file?

```
inode

↓

Represents the file itself
```

```
struct file

↓

Represents one open instance
```

---

### Why Page Cache?

To avoid repeated disk access.

---

### Why BIO?

To represent block I/O independent of hardware.

---

### Why Block Layer?

To isolate filesystems from storage hardware.

---

### Why address_space?

To connect an inode with its cached pages.

---

### Why VFS?

To provide one common interface for every filesystem.

---

# Master Memory Diagram

```
                    User Space
+----------------------------------------------+
|                                              |
|  Application                                 |
|      │                                       |
|      ▼                                       |
|   File Descriptor                            |
+----------------------------------------------+

                System Call

                    │

                    ▼

                  Kernel

+-------------------------------------------------------------+

files_struct
      │
      ▼
fdtable
      │
      ▼
struct file
      │
      ▼
dentry
      │
      ▼
inode
      │
      ▼
address_space
      │
      ▼
Page Cache
      │
      ▼
Filesystem
      │
      ▼
BIO
      │
      ▼
Block Layer
      │
      ▼
Driver
      │
      ▼
SSD/HDD

+-------------------------------------------------------------+
```

---

# Final Takeaway

If you remember only one dependency chain from the entire handbook, make it this one:

```
Application
      │
      ▼
File Descriptor
      │
      ▼
struct file
      │
      ▼
dentry
      │
      ▼
inode
      │
      ▼
address_space
      │
      ▼
Page Cache
      │
      ▼
Filesystem
      │
      ▼
BIO
      │
      ▼
Block Layer
      │
      ▼
Device Driver
      │
      ▼
Storage Device
```

Understanding this chain means you understand how Linux performs file I/O from user space down to the hardware.

---

# End of VFS Handbook

After completing these chapters, you should be comfortable with interview questions on:

- ✅ Linux VFS Architecture
- ✅ Path Lookup
- ✅ Dentry Cache
- ✅ Inodes
- ✅ Superblock
- ✅ File Object (`struct file`)
- ✅ File Descriptors
- ✅ Page Cache
- ✅ `address_space`
- ✅ `open()`, `read()`, `write()`, `close()`
- ✅ Block Layer
- ✅ BIO
- ✅ blk-mq
- ✅ ext4 Internals
- ✅ Journaling
- ✅ Delayed Allocation
- ✅ NFS Architecture
- ✅ Complete Linux File I/O Flow

This forms a strong foundation for Linux Embedded, Kernel Development, Filesystem, Storage, and Systems interviews.
