# Chapter 5 – Virtual File System (VFS)

---

## 1. What is VFS?

VFS stands for **Virtual File System**.

VFS is the abstraction layer in the Linux kernel that provides a common interface to different filesystems.

Linux supports many filesystems:

```text
ext4
XFS
Btrfs
tmpfs
procfs
sysfs
NFS
FAT
NTFS
...
```

Applications should not need to know which filesystem stores a file.

For example:

```c
int fd = open("/home/user/data.txt", O_RDONLY);
```

The application uses the same `open()` interface regardless of whether the file is stored on:

```text
ext4
XFS
NFS
USB filesystem
SSD
RAM-based filesystem
```

VFS hides these filesystem-specific details.

---

# 2. Why Do We Need VFS?

Without VFS, every application would need to understand every filesystem.

For example:

```text
Application
    |
    +---- ext4 API
    |
    +---- XFS API
    |
    +---- NFS API
    |
    +---- FAT API
```

This would be extremely complicated.

With VFS:

```text
             Application
                  |
                  v
          open/read/write
                  |
                  v
                 VFS
            /     |     \
           /      |      \
        ext4     XFS     NFS
```

The application uses a generic interface.

---

# 3. Linux File I/O Architecture

High-level architecture:

```text
+----------------------------+
|       Application          |
+----------------------------+
             |
             v
+----------------------------+
|       System Calls         |
| open/read/write/close      |
+----------------------------+
             |
             v
+----------------------------+
|            VFS             |
+----------------------------+
             |
       +-----+-----+
       |     |     |
       v     v     v
     ext4   XFS   NFS
       |     |     |
       +-----+-----+
             |
             v
       Block / Network
             |
             v
          Hardware
```

---

# 4. Important VFS Structures

The most important structures are:

```text
struct super_block
struct inode
struct dentry
struct file
struct path
```

A useful mental model:

```text
super_block → Filesystem instance

inode       → Filesystem object

dentry      → Directory/path component

file        → Open file instance

path        → dentry + mount

fd          → Userspace handle to an open file
```

---

# 5. super_block

`struct super_block` represents an instance of a mounted filesystem.

Conceptually:

```text
super_block
    |
    +-- Filesystem type
    +-- Block size
    +-- Filesystem information
    +-- Root dentry
    +-- Mount information
    +-- Filesystem operations
```

Example:

```text
/dev/sda1
    |
    v
   ext4
    |
    v
super_block
```

---

# 6. Important Distinction: super_block

A common interview mistake is:

> super_block represents a disk.

Not exactly.

It represents a **filesystem instance**.

For example:

```text
/dev/sda1
    |
    v
  ext4
    |
    v
Mounted filesystem
    |
    v
super_block
```

A single physical disk can contain multiple partitions/filesystems, and each mounted filesystem has its own filesystem state represented by kernel structures.

---

# 7. inode

An inode represents a filesystem object.

Examples:

```text
Regular file
Directory
Symbolic link
Device
```

An inode contains metadata such as:

```text
File type
Permissions
Owner
Group
Size
Timestamps
Link count
Filesystem-specific information
```

Conceptually:

```text
inode
 |
 +-- File type
 +-- Permissions
 +-- UID/GID
 +-- Size
 +-- Timestamps
 +-- Link count
 +-- Data mapping information
```

---

# 8. inode Does Not Store the Filename

This is an important interview point.

The filename is associated with directory entries.

Conceptually:

```text
filename
    |
    v
 dentry
    |
    v
 inode
    |
    v
file data
```

For example:

```text
notes.txt
    |
    v
inode 125
```

The inode represents the underlying filesystem object.

---

# 9. dentry

`dentry` means:

```text
Directory Entry
```

It represents a component of a pathname and participates in mapping names to filesystem objects.

Consider:

```text
/home/user/notes.txt
```

The pathname consists of components:

```text
/
home
user
notes.txt
```

During path lookup, Linux uses dentries to represent these pathname components.

Conceptually:

```text
/
|
+-- home
     |
     +-- user
          |
          +-- notes.txt
```

---

# 10. Why Does Linux Need Dentries?

Path lookup happens extremely frequently.

For:

```c
open("/home/user/file.txt", O_RDONLY);
```

the kernel needs to resolve:

```text
/
 →
home
 →
user
 →
file.txt
```

The dentry cache helps Linux avoid repeatedly performing expensive filesystem lookups.

---

# 11. Dentry Cache

Linux maintains a cache of dentries.

Conceptually:

```text
Path Lookup
     |
     v
Dentry Cache
     |
   +---+---+
   |       |
  Hit     Miss
   |       |
   v       v
 Fast    Filesystem
 Lookup    Lookup
```

A dentry cache hit can significantly reduce path lookup overhead.

---

# 12. file

`struct file` represents an **open file instance**.

This is different from an inode.

Example:

```c
int fd = open("data.txt", O_RDONLY);
```

The kernel creates/uses an open-file object represented by `struct file`.

Conceptually:

```text
fd
 |
 v
struct file
 |
 v
path
 |
 +-- dentry
 |
 +-- mount
 |
 v
inode
```

---

# 13. inode vs struct file

This distinction is very important.

### inode

Represents the filesystem object.

```text
"What file/object is this?"
```

### struct file

Represents a particular open instance.

```text
"How is this particular open instance being used?"
```

Example:

```text
Process A
    |
    +-- fd 3
          |
          v
       file A
          |
          v
       inode 100


Process B
    |
    +-- fd 4
          |
          v
       file B
          |
          v
       inode 100
```

Both processes can reference the same underlying inode through different open-file objects.

---

# 14. File Position

The open-file object maintains state associated with the open instance, including the current file position.

Example:

```c
read(fd, buffer, 100);
```

Initially:

```text
offset = 0
```

After reading 100 bytes:

```text
offset = 100
```

Conceptually:

```text
struct file
    |
    +-- f_pos
```

This is one reason `struct file` is different from `inode`.

---

# 15. File Descriptor

Applications do not directly manipulate `struct file`.

They use a file descriptor.

Example:

```c
int fd = open("data.txt", O_RDONLY);
```

Suppose:

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
   +-- 0 → stdin
   +-- 1 → stdout
   +-- 2 → stderr
   +-- 3 → struct file
                 |
                 v
               inode
```

The file descriptor is an integer used by userspace.

---

# 16. Standard File Descriptors

Normally:

```text
0 → stdin
1 → stdout
2 → stderr
```

Example:

```c
write(1, "Hello\n", 6);
```

The value `1` refers to standard output for that process.

---

# 17. path

Linux also uses a `struct path` to represent a location in the mounted filesystem hierarchy.

Conceptually:

```text
struct path
    |
    +-- mount
    |
    +-- dentry
```

The path therefore combines:

```text
Mount context
+
Dentry
```

This is important because the same filesystem object can be accessed through different mount contexts.

---

# 18. Important VFS Relationship

A useful diagram to remember:

```text
Process
   |
   v
File Descriptor
   |
   v
struct file
   |
   v
struct path
   |
   +--------> dentry
   |
   +--------> mount
                 |
                 v
            super_block

dentry
   |
   v
inode
```

---

# 19. Mount

A filesystem becomes accessible through the directory hierarchy when it is mounted.

Example:

```bash
mount /dev/sdb1 /mnt
```

Conceptually:

```text
/dev/sdb1
    |
    v
Filesystem
    |
    v
super_block
    |
    v
mount
    |
    v
/mnt
```

Applications can then access files under:

```text
/mnt
```

---

# 20. Root Filesystem

Linux eventually needs a root filesystem.

It is represented as:

```text
/
```

Example:

```text
/
|
+-- bin
+-- etc
+-- home
+-- usr
+-- var
+-- tmp
```

The root filesystem provides the initial directory hierarchy.

---

# 21. Multiple Filesystems

Linux can combine multiple filesystems into one directory tree.

Example:

```text
/
|
+-- home
|
+-- var
|
+-- proc
|    |
|    +-- procfs
|
+-- sys
     |
     +-- sysfs
```

Another example:

```text
/dev/sda1 → /

/dev/sdb1 → /home
```

The user still sees:

```text
/
|
+-- home
```

VFS hides the underlying filesystem boundaries.

---

# 22. Path Resolution

Consider:

```text
/home/user/notes.txt
```

The kernel must resolve the path.

Conceptually:

```text
"/"
 |
 v
"home"
 |
 v
"user"
 |
 v
"notes.txt"
 |
 v
inode
```

This process is called:

```text
Pathname Resolution
```

or:

```text
Path Lookup
```

---

# 23. Path Lookup

High-level flow:

```text
open("/home/user/notes.txt")
             |
             v
            VFS
             |
             v
       Resolve "/"
             |
             v
       Lookup "home"
             |
             v
       Lookup "user"
             |
             v
       Lookup "notes.txt"
             |
             v
           inode
```

Dentries and the dentry cache play a major role in this process.

---

# 24. open() Flow

Consider:

```c
int fd = open("/home/user/notes.txt", O_RDONLY);
```

High-level flow:

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
Path Resolution
     |
     v
dentry
     |
     v
inode
     |
     v
struct file
     |
     v
File Descriptor Table
     |
     v
fd
```

Finally:

```text
fd = 3
```

may be returned to userspace.

---

# 25. What Does open() Actually Do?

Conceptually, `open()`:

1. Resolves the pathname.
2. Finds the filesystem object.
3. Performs permission/security checks.
4. Creates/initializes an open-file object.
5. Installs a file descriptor in the process.
6. Returns the descriptor to userspace.

It does **not** normally read the entire file into memory.

---

# 26. read() Flow

Consider:

```c
read(fd, buffer, 4096);
```

High-level flow:

```text
Application
     |
     v
read()
     |
     v
System Call
     |
     v
File Descriptor
     |
     v
struct file
     |
     v
VFS
     |
     v
Page Cache / Filesystem
     |
     v
Data
     |
     v
User Buffer
```

---

# 27. Page Cache

Linux uses RAM to cache filesystem data.

This is called the:

```text
Page Cache
```

Conceptually:

```text
Application
     |
     v
   read()
     |
     v
Page Cache
   /     \
 Hit     Miss
  |        |
  v        v
Data    Filesystem
           |
           v
         Storage
```

---

# 28. Page Cache Hit

If the required data already exists in memory:

```text
read()
  |
  v
Page Cache
  |
  v
Data Found
  |
  v
Return Data
```

The storage device does not need to be accessed for that data.

---

# 29. Page Cache Miss

If the data is not cached:

```text
read()
  |
  v
Page Cache
  |
  v
Miss
  |
  v
Filesystem
  |
  v
Block Layer
  |
  v
Storage Driver
  |
  v
SSD/HDD
```

The required data is brought into memory.

---

# 30. Page Cache vs Buffer Cache

Historically Linux used the term:

```text
Buffer Cache
```

for caching filesystem/block-device metadata and blocks.

Modern Linux has unified much of this through the:

```text
Page Cache
```

For interview purposes, remember:

```text
File data
    ↓
Page Cache
```

and don't treat the old "buffer cache" terminology as a completely separate modern file-data cache.

---

# 31. write() Flow

Consider:

```c
write(fd, buffer, 4096);
```

Simplified buffered-I/O flow:

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
Storage Driver
     |
     v
SSD/HDD
```

---

# 32. Dirty Pages

When modified file data exists in memory but has not yet been written back:

```text
Dirty Page
```

Conceptually:

```text
Application
     |
     v
Modify Data
     |
     v
Page Cache
     |
     v
Dirty
     |
     v
Writeback
     |
     v
Storage
```

---

# 33. Does write() Mean Data Is on Disk?

Not necessarily.

A successful:

```c
write()
```

usually means the data has been accepted by the kernel/filesystem path.

The data may still be in memory as dirty cached data.

Conceptually:

```text
write()
  |
  v
Page Cache
  |
  v
Dirty
  |
  v
Later Writeback
  |
  v
Storage
```

---

# 34. fsync()

Applications can request that modified data be synchronized more strongly with storage using:

```c
fsync(fd);
```

Conceptually:

```text
Dirty Data
    |
    v
fsync()
    |
    v
Writeback / Flush
    |
    v
Storage
```

Exact durability guarantees depend on the filesystem and storage stack.

---

# 35. close()

When:

```c
close(fd);
```

is called:

```text
Application
    |
    v
close(fd)
    |
    v
File Descriptor Released
    |
    v
Reference to struct file Released
```

When the relevant references are gone, the open-file object can be released.

The inode and dentry may remain cached for later use.

---

# 36. File Descriptor vs struct file vs inode

This is one of the most important VFS interview questions.

Remember:

```text
fd
↓
struct file
↓
path
↓
dentry
↓
inode
```

Meaning:

```text
fd
→ integer used by the process

struct file
→ open-file instance

path
→ mount + dentry

dentry
→ pathname component / directory entry

inode
→ underlying filesystem object
```

---

# 37. Directory Internals

A directory is itself a filesystem object.

Conceptually:

```text
Directory inode
      |
      v
Directory Data
      |
      +-- file1 → inode X
      +-- file2 → inode Y
      +-- dir1  → inode Z
```

A directory associates names with filesystem objects.

---

# 38. Hard Link

Suppose:

```text
file1
file2
```

are hard links to the same inode.

Conceptually:

```text
file1 --------+
              |
              v
           inode 100
              ^
              |
file2 --------+
```

Both directory entries refer to the same inode.

The inode's link count tracks the number of hard links.

---

# 39. Symbolic Link

A symbolic link is different.

Example:

```text
link.txt → original.txt
```

Conceptually:

```text
link.txt
   |
   v
symlink object
   |
   v
"original.txt"
   |
   v
target
```

Path resolution follows the symbolic link to its target.

---

# 40. VFS Operations

VFS uses operation tables to allow generic code to call filesystem-specific implementations.

Important structures include:

```text
file_operations
inode_operations
super_operations
```

For example:

```text
struct file
     |
     v
file_operations
     |
     +-- read
     +-- write
     +-- ioctl
     +-- mmap
```

The exact operations available depend on the object and implementation.

---

# 41. Why Function Pointers?

Linux is written largely in C.

C does not provide C++-style classes and virtual functions.

The kernel therefore uses structures containing function pointers to achieve a form of polymorphism.

Conceptually:

```text
VFS
 |
 v
Operation Table
 |
 +---- ext4 implementation
 |
 +---- XFS implementation
 |
 +---- NFS implementation
```

This allows generic VFS code to operate with many different filesystem implementations.

---

# 42. VFS to ext4

Suppose the file resides on ext4.

A simplified read path is:

```text
Application
     |
     v
read()
     |
     v
System Call
     |
     v
VFS
     |
     v
struct file
     |
     v
inode
     |
     v
ext4
     |
     v
Page Cache
     |
     v
Block Layer
     |
     v
Storage Driver
     |
     v
SSD
```

VFS provides the generic layer.

ext4 provides filesystem-specific behavior.

---

# 43. Disk vs VFS

This distinction is extremely important.

## On Disk

The filesystem stores persistent structures such as:

```text
Superblock
Filesystem metadata
Directory information
Inode information
File data
```

## In Kernel Memory

Linux maintains in-memory structures such as:

```text
super_block
dentry cache
inode cache
page cache
mount structures
struct file
```

The kernel uses these structures while the filesystem is mounted and files are accessed.

---

# 44. Example: notes.txt on Disk

Suppose:

```text
DISK
====

Filesystem
   |
   v
Root Directory
   |
   +-- notes.txt
          |
          v
       inode 125
          |
          v
       Data Blocks
```

The directory maps:

```text
notes.txt → inode 125
```

---

# 45. After Mount

The kernel builds/maintains in-memory representations:

```text
RAM
===

super_block
     |
     v
root dentry
     |
     v
root inode
```

When the kernel performs:

```text
lookup("notes.txt")
```

it can reach:

```text
notes.txt
    |
    v
dentry
    |
    v
inode 125
```

---

# 46. After open()

Suppose:

```c
fd = open("/notes.txt", O_RDONLY);
```

Conceptually:

```text
Process
   |
   v
fd = 3
   |
   v
struct file
   |
   v
path
   |
   v
dentry
   |
   v
inode 125
```

---

# 47. Complete VFS Mental Model

Memorize this:

```text
                     PROCESS
                        |
                        v
                File Descriptor
                        |
                        v
                   struct file
                        |
                        v
                      path
                    /      \
                   /        \
                  v          v
               dentry      mount
                 |            |
                 v            v
               inode     super_block
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

---

# 48. Complete read() Mental Model

```text
Application
     |
     v
read(fd)
     |
     v
System Call
     |
     v
VFS
     |
     v
struct file
     |
     v
Page Cache
    /    \
  Hit     Miss
  |         |
  v         v
Data     Filesystem
           |
           v
       Block Layer
           |
           v
       Device Driver
           |
           v
         Storage
           |
           v
      Page Cache
           |
           v
      User Buffer
```

---

# 49. Complete write() Mental Model

```text
Application
     |
     v
write(fd)
     |
     v
System Call
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

---

# 50. Deleted File Still Open

This is a classic Linux interview question.

Suppose:

```text
Process A
    |
    +-- fd 3
          |
          v
       struct file
          |
          v
         inode
```

Another process executes:

```bash
rm file.txt
```

The directory entry for:

```text
file.txt
```

is removed.

But the open file can remain accessible because the process still holds a reference to the open object/inode.

Conceptually:

```text
Directory name
      |
      X
    removed

Process fd
      |
      v
struct file
      |
      v
inode
```

The underlying storage is reclaimed only after the relevant references are gone and filesystem rules allow reclamation.

---

# 51. Why Can a Deleted File Consume Disk Space?

Example:

```text
Process opens large.log
        |
        v
      fd 3
```

Another process:

```bash
rm large.log
```

The directory entry disappears.

But the process still has the file open.

Therefore:

```text
Filename → gone

Open file → still exists
```

The file's blocks may continue consuming disk space until the final relevant reference is released.

This is a common production debugging problem.

---

# 52. Important Interview Questions

## Q1. Why do we need VFS?

Because Linux supports many filesystems while applications need a common filesystem API.

---

## Q2. What is an inode?

An inode represents a filesystem object and contains metadata and information used to locate/access its data.

---

## Q3. Does an inode contain the filename?

No.

The name is associated with directory entries/dentries.

Conceptually:

```text
name → dentry → inode
```

---

## Q4. What is a dentry?

A dentry represents a pathname component/directory entry and participates in mapping names to filesystem objects.

---

## Q5. What is `struct file`?

It represents an open file instance.

It contains state associated with that open instance, such as file position and operation information.

---

## Q6. What is a file descriptor?

A file descriptor is a small integer used by a process to refer to an open file object.

---

## Q7. Explain:

```text
fd → file → dentry → inode
```

Answer:

```text
fd
 ↓
indexes the process FD table

struct file
 ↓
represents the open instance

dentry
 ↓
represents the pathname component

inode
 ↓
represents the filesystem object
```

---

## Q8. Does `open()` read the file?

Normally no.

It establishes the open-file context and returns a file descriptor.

Actual data access happens through operations such as:

```text
read()
mmap()
```

---

## Q9. Does `read()` always access disk?

No.

The data may already be present in the page cache.

```text
read()
   |
   v
Page Cache
 /       \
Hit       Miss
 |          |
Data      Storage
```

---

## Q10. Does `write()` immediately write to disk?

Not necessarily.

Buffered writes commonly modify cached pages first. Writeback later sends dirty data toward storage.

---

## Q11. What does `fsync()` do?

It requests synchronization of modified file data and associated filesystem state according to the filesystem/storage semantics.

---

## Q12. What happens when a file is deleted while open?

The directory name is removed, but the open file can remain accessible through existing references.

The underlying storage is reclaimed only after the relevant references are released.

---

## Q13. What is the difference between hard link and symbolic link?

Hard link:

```text
name1 ──┐
        v
      inode
        ^
name2 ──┘
```

Symbolic link:

```text
link
 |
 v
target pathname
 |
 v
target object
```

---

# 53. Senior Interview Questions

You should be able to explain these on a whiteboard:

### 1. What happens when:

```c
open("/home/user/file.txt", O_RDONLY);
```

### 2. Explain:

```text
fd → struct file → path → dentry → inode
```

### 3. What happens during:

```c
read(fd, buffer, 4096);
```

### 4. What happens on a page-cache miss?

### 5. What happens during:

```c
write(fd, buffer, 4096);
```

### 6. Why can `rm file` succeed while a process still reads the file?

### 7. How does VFS support ext4, XFS, NFS, etc.?

### 8. What is the difference between:

```text
super_block
inode
dentry
file
```

### 9. Why does Linux cache dentries?

### 10. Why doesn't `write()` necessarily mean data is physically persistent?

---

# 54. Must-Remember Diagram

For senior Linux interviews, memorize this relationship:

```text
Process
   |
   v
FD Table
   |
   v
fd
   |
   v
struct file
   |
   v
struct path
   |
   +--------> dentry
   |
   +--------> mount
                 |
                 v
            super_block

dentry
   |
   v
inode
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

---

# 55. Final Summary

VFS is the Linux kernel's generic filesystem abstraction layer.

The most important structures are:

```text
super_block
inode
dentry
file
path
```

Remember their roles:

```text
super_block
    ↓
filesystem instance

inode
    ↓
filesystem object

dentry
    ↓
pathname component / name lookup

struct file
    ↓
open file instance

path
    ↓
mount + dentry

fd
    ↓
userspace integer referring to an open file
```

The most important chain is:

```text
fd
 ↓
struct file
 ↓
path
 ↓
dentry
 ↓
inode
 ↓
filesystem
 ↓
block layer
 ↓
device driver
 ↓
storage
```

And the most important data-access path is:

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
   /     \
 Hit      Miss
  |         |
  v         v
Data     Filesystem
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

If you understand these flows deeply, you have the foundation needed to answer a large class of senior Linux Systems, Embedded, Storage, and Infrastructure interview questions.
