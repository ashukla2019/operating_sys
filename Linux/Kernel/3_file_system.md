# Linux Filesystem, VFS & NFS (Interview Notes)

---

# 1. What is a Filesystem?

A **filesystem** defines how data is stored, organized, and retrieved from a storage device.

### Responsibilities

- Store files and directories
- Maintain metadata
- Manage free space
- Enforce permissions
- Locate file data on disk

### Examples

| Local Filesystem | Network Filesystem |
|------------------|--------------------|
| ext4             | NFS                |
| XFS              | SMB/CIFS           |
| Btrfs            |                    |

---

# 2. What is VFS (Virtual File System)?

The **Virtual File System (VFS)** is a kernel abstraction layer that provides a **common interface** for all filesystems.

Applications always use the same system calls:

```c
open()
read()
write()
close()
```

VFS hides whether the underlying filesystem is:

- ext4
- XFS
- Btrfs
- NFS
- tmpfs

Applications never know which filesystem is actually used.

---

# 3. VFS Core Data Structures

| Object | Represents | Created |
|---------|------------|---------|
| `super_block` | Mounted filesystem | mount |
| `inode` | File metadata | Loaded during lookup |
| `dentry` | Filename → inode mapping | Path lookup |
| `file` | Open file instance | open() |
| `fd` | Process handle | open() |

---

## Relationship

```
fd
 │
 ▼
struct file
 │
 ├── f_inode ──────────────► inode
 │                            │
 ├── f_path ─► dentry ────────┘
 │               │
 │               ▼
 │          filename
 │
 ├── f_op
 └── f_pos

inode
  │
  ▼
super_block
```

---

# 4. Filesystem Lifecycle

Objects are created at different stages.

| Stage | Objects Created |
|--------|-----------------|
| mkfs | Disk superblock, inode table, data blocks |
| mount | `struct super_block`, root dentry |
| path lookup | dentries, inodes |
| open | `struct file`, file descriptor |
| read/write | Uses `file → inode` |

---

# 5. Stage 1 : Filesystem Creation (mkfs)

Example:

```bash
mkfs.ext4 /dev/sda1
```

Creates **on disk**:

```
Disk

├── Superblock
├── Inode Table
├── Data Blocks
└── Root Directory
```

Nothing is loaded into RAM yet.

No kernel objects exist.

---

# 6. Stage 2 : Mount

Example

```bash
mount /dev/sda1 /mnt
```

Kernel performs:

```
Read Disk Superblock
        │
        ▼
Create struct super_block
        │
        ▼
Load Root Inode
        │
        ▼
Create Root Dentry
```

Result

```
super_block
      │
      ▼
   s_root
      │
      ▼
   root dentry
      │
      ▼
   root inode
```

Now the filesystem is accessible through VFS.

---

# 7. Stage 3 : Path Lookup

Suppose

```c
open("/home/user/file.txt");
```

VFS walks the pathname:

```
/

↓

home

↓

user

↓

file.txt
```

For each component:

1. Check dentry cache.
2. If missing, ask filesystem.
3. Create dentry.
4. Load inode.

Result

```
dentry("file.txt")
        │
        ▼
      inode
```

---

# 8. Stage 4 : open()

After pathname lookup,

VFS creates:

```
struct file
```

and initializes it.

Conceptually

```c
file->f_inode = inode;
file->f_path  = path;
file->f_op    = inode->i_fop;
file->f_pos   = 0;
```

Finally

```
fd

↓

struct file
```

gets stored in the process file descriptor table.

Result

```
fd
 │
 ▼
struct file
 ├── f_inode
 ├── f_path
 ├── f_op
 └── f_pos
```

---

# 9. Stage 5 : read()

Application

```c
read(fd, buf, size);
```

Flow

```
fd
 │
 ▼
struct file
 │
 ▼
file->f_op->read_iter()
 │
 ▼
filesystem implementation
 │
 ▼
Page Cache
 │
 ▼
Disk (if cache miss)
```

After reading,

```
file->f_pos
```

is updated.

---

# 10. Stage 6 : write()

```
fd
 │
 ▼
struct file
 │
 ▼
file->f_op->write_iter()
 │
 ▼
Page Cache (Dirty)
 │
 ▼
Disk (later)
```

---

# 11. How VFS Chooses the Correct Filesystem

This is the most important concept.

Suppose we mount two filesystems.

```bash
mount /dev/sda1 /local
mount -t nfs server:/share /nfs
```

Internally,

```
Mount Table

/local  ─────► super_block (ext4)
/nfs    ─────► super_block (nfs)
```

Each mounted filesystem has its own

```
struct super_block
```

---

## ext4 super_block

```
super_block

s_root
s_op  → ext4_super_operations
```

---

## NFS super_block

```
super_block

s_root
s_op → nfs_super_operations
```

---

# 12. Path Lookup Across Mount Points

Suppose

```c
open("/local/file.txt");
```

VFS starts at

```
/

├── local
└── nfs
```

When it reaches

```
/local
```

it recognizes a **mount point**.

Instead of continuing inside the root filesystem,

it switches into the mounted ext4 filesystem.

```
Root FS
     │
     └──── local (mount)
               │
               ▼
       ext4 super_block
```

Similarly,

```
open("/nfs/file.txt");
```

becomes

```
Root FS
     │
     └──── nfs (mount)
               │
               ▼
        nfs super_block
```

**Important**

The pathname determines which mounted filesystem VFS enters.

---

# 13. Where is Mount Information Stored?

Conceptually,

Linux maintains mount objects.

```
struct mount

mount_point
      │
      ▼
"/local"

root
      │
      ▼
ext4 super_block->s_root
```

Another mount

```
struct mount

mount_point
      │
      ▼
"/nfs"

root
      │
      ▼
nfs super_block->s_root
```

During pathname lookup,

VFS follows these mount objects to switch filesystems.

---

# 14. How read() Knows Whether It Is ext4 or NFS

After lookup,

the inode already belongs to its filesystem.

Example:

```
inode
 │
 ▼
super_block(ext4)
```

or

```
inode
 │
 ▼
super_block(nfs)
```

Every inode stores filesystem-specific operations.

For ext4

```
inode

i_fop

↓

ext4_file_operations
```

For NFS

```
inode

i_fop

↓

nfs_file_operations
```

During open(),

VFS performs

```c
file->f_op = inode->i_fop;
```

Therefore

For ext4

```
struct file

f_op

↓

ext4_file_operations
```

For NFS

```
struct file

f_op

↓

nfs_file_operations
```

Later,

```c
read(fd,...)
```

simply executes

```c
file->f_op->read_iter();
```

For ext4

```
read()

↓

ext4_read_iter()
```

For NFS

```
read()

↓

nfs_file_read()

↓

RPC

↓

NFS Server

↓

Server VFS

↓

ext4/xfs

↓

Disk
```

**VFS never checks**

```
if(ext4)

if(nfs)
```

The correct implementation is already stored in

```
file->f_op
```

---

# 15. Complete End-to-End Flow

```
mkfs
 │
 ▼
Disk Structures Created

──────────────────────────────────

mount
 │
 ▼
struct super_block
 │
 ▼
root dentry
 │
 ▼
root inode

──────────────────────────────────

open()
 │
 ▼
Path Lookup
 │
 ▼
dentry
 │
 ▼
inode
 │
 ▼
struct file
 │
 ▼
fd

──────────────────────────────────

read()
 │
 ▼
file->f_op
 │
 ▼
Filesystem Implementation

      ext4
         │
         ▼
      Page Cache
         │
         ▼
        Disk

      OR

      NFS
         │
         ▼
        RPC
         │
         ▼
     NFS Server
         │
         ▼
      Server VFS
         │
         ▼
     ext4/xfs
         │
         ▼
        Disk
```

---

# 16. Interview Questions

### Difference between inode and dentry

| inode | dentry |
|--------|---------|
| Metadata | Filename mapping |

---

### Difference between inode and file

| inode | file |
|--------|------|
| Represents the file | Represents an open instance |

---

### When is `struct file` created?

During

```
open()
```

---

### When is inode created?

It already exists on disk.

The kernel loads (or caches) it during pathname lookup.

---

### Why does `struct file` contain `f_inode`?

To directly access the inode without dereferencing the dentry every time.

---

### Why does `struct file` contain `f_op`?

To call the correct filesystem implementation (ext4, NFS, XFS, etc.).

---

### Does VFS check whether a file belongs to ext4 or NFS during every read?

**No.**

During `open()`,

```
file->f_op = inode->i_fop;
```

From then on,

```
read()

↓

file->f_op->read_iter()
```

directly invokes the correct filesystem implementation.

---

# Memory Trick

```
FD

↓

FILE

↓

DENTRY

↓

INODE

↓

SUPERBLOCK

↓

FILESYSTEM

↓

PAGE CACHE

↓

DISK
```

---

# One-Line Summary

**`mkfs` creates filesystem metadata on disk, `mount` creates the VFS objects, pathname lookup resolves names to inodes, `open()` creates a `struct file` and binds filesystem operations, and `read()/write()` invoke those operations without needing to identify the filesystem again.**

----------------------------------------------------------------------
# Linux Filesystem + VFS Flow (Short)

When we want to perform file operations (`read/write`) on disk, we need a filesystem layer.

---

## a) Create filesystem

Without a filesystem, disk contains only raw blocks.

No:

- File paths
- Directories
- Inodes
- Metadata

Example:

```
Disk
 |
 +-- Raw Blocks
```

---

## b) `mkfs.ext4`

Creates filesystem structures **on disk**:

```
Superblock
Inode Table
Data Blocks
Root Directory
```

At this stage:

- Disk filesystem exists.
- No kernel VFS objects are created yet.

---

## c) Mount filesystem

Example:

```bash
mount /dev/sda1 /mnt
```

During mount:

- Filesystem driver (ext4/NFS/etc.) is already registered with VFS.
- VFS selects the filesystem driver.
- Kernel creates the in-memory representation of the mounted filesystem.

---

## d) After mount

Kernel creates:

```
struct super_block
        |
        v
      s_root
        |
        v
   root dentry
        |
        v
   root inode
```

Note:

- Disk superblock → created by `mkfs`
- `struct super_block` → created during mount in RAM

---

## e) `open()` flow

Example:

```c
open("/home/user/file.txt");
```

Flow:

```
sys_open()
    |
    v
path_lookup()
```

Path lookup provides:

```
struct path

    |
    +-- dentry
          |
          v
        inode
```

The inode belongs to:

```
inode
  |
  v
super_block
  |
  v
filesystem (ext4/NFS)
```

---

## f) Create open file instance

VFS creates:

```
struct file
```

and fills:

```c
file->f_path  = path;
file->f_inode = inode;
file->f_pos   = 0;
file->f_op    = inode->i_fop;
```

Meaning:

```
f_path
 |
 +-- mount
 |
 +-- dentry


f_inode
 |
 v
inode


f_op
 |
 v
filesystem operations


f_pos
 |
 v
current file offset
```

---

## g) `read()` / `write()` flow

Application uses:

```
fd
 |
 v
struct file
 |
 v
file->f_op
 |
 v
filesystem operation
```

Examples:

```
ext4
 |
 v
ext4_file_operations
```

```
NFS
 |
 v
nfs_file_operations
```

VFS does not check ext4 or NFS during every read/write.

The correct filesystem operation is already stored in:

```c
file->f_op
```
