

# Filesystem + VFS Complete Notes (Interview Ready)

---

## 1.  What is a Filesystem?

A filesystem is a method to store, organize, and retrieve data from storage devices.

### Responsibilities:
- File storage & retrieval
- Metadata management
- Directory hierarchy
- Permissions & security
- Space allocation

### Examples:
- Linux: ext4, XFS, Btrfs
- Windows: NTFS
- Network: NFS

---

## 2. What is VFS (Virtual File System)?

VFS is a kernel abstraction layer that provides a uniform interface to multiple filesystems.

### It allows:
- Same syscalls (`open`, `read`, `write`) for all filesystems
- Plug-and-play filesystem support

---

## 3. Core VFS Data Structures (VERY IMPORTANT)

---

### 3.1 Superblock (`struct super_block`)

Represents a mounted filesystem.

#### Contains:
- Filesystem type (ext4, xfs)
- Block size
- Root inode
- Filesystem operations

#### Key Pointer:
super_block → root dentry


---

### 3.2 Inode (`struct inode`)

Represents file metadata (**NOT filename**).

#### Contains:
- File size
- Permissions
- Owner (UID/GID)
- Timestamps
- Block pointers (data location)

#### Important:
❗ Inode does NOT store filename

---

### 3.3 Dentry (`struct dentry`)

Represents directory entry (**name ↔ inode mapping**).

#### Contains:
- Filename
- Pointer to inode
- Parent directory

#### Purpose:
- Speeds up path lookup (dentry cache)

---

### 3.4 File Object (`struct file`)

Represents an open file instance.

#### Contains:
- File offset
- Flags (O_RDONLY, etc.)
- Pointer to inode
- File operations

---

### 3.5 File Descriptor (FD)

- Integer returned to user (0,1,2,...)
- Index into process file table

---

# 📁 Filesystem + VFS Complete Lifecycle & Mapping (Interview Ready)

---

# 🧠 0. BIG PICTURE

Different objects are created at different stages:

| Stage       | Objects Created                                        |
| ----------- | ------------------------------------------------------ |
| mkfs        | Disk structures (superblock, inode table, data blocks) |
| mount       | struct super_block, root dentry, root inode            |
| path lookup | dentries + inodes (cached, lazy)                       |
| open        | struct file + fd                                       |
| read/write  | uses file → inode → data                               |

---

# 🧱 1. FILESYSTEM CREATION (mkfs)

```bash
mkfs.ext4 /dev/sda1
```

### ✅ Created ON DISK:

* Superblock (disk)
* Inode table
* Data blocks
* Root directory inode

### ❌ NOT created:

* struct super_block (kernel)
* struct dentry
* struct file

---

# 🧱 2. MOUNT FLOW

```bash
mount /dev/sda1 /mnt
```

### Kernel actions:

```text
read disk superblock
↓
create struct super_block
↓
load root inode
↓
create root dentry
```

### Final mapping:

```text
super_block
   ↓
s_root (dentry "/")
   ↓
inode (root directory)
```

---

# 🧱 3. PATH LOOKUP (LAZY CREATION)

Example:

```c
open("/home/user/file.txt")
```

### Step-by-step:

```text
start from root dentry ("/")
↓
lookup "home"
↓
lookup "user"
↓
lookup "file.txt"
```

### At each step:

* Check dentry cache
* If miss → filesystem lookup
* Create dentry
* Load inode into memory

### Result:

```text
dentry("file.txt") → inode
```

---

# 🧱 4. OPEN() FLOW (CRITICAL)

```c
open("/home/user/file.txt")
```

## Full Flow

```text
sys_open()
↓
path_openat()
↓
path lookup
↓
dentry + inode obtained
```

---

## 🔥 FILE OBJECT CREATION

### 1. Allocate file

```c
struct file *file = alloc_empty_file();
```

---

### 2. Attach path

```c
file->f_path.dentry = dentry;
file->f_path.mnt    = vfsmount;
```

---

### 3. Attach inode

```c
file->f_inode = dentry->d_inode;
```

---

### 4. Set file operations

```c
file->f_op = inode->i_fop;
```

---

### 5. Call filesystem open

```c
if (file->f_op->open)
    file->f_op->open(inode, file);
```

---

### 6. Initialize state

```c
file->f_pos = 0;
file->f_flags = flags;
```

---

### 7. Assign fd

```text
fd → file (stored in process fd table)
```

---

# 🔗 FINAL OPEN MAPPING

```text
fd
↓
struct file
   ├── f_path → dentry → inode
   ├── f_inode → inode
   ├── f_op → file_operations
   └── f_pos
```

---

# 🧱 5. READ() FLOW

```c
read(fd, buf, size)
```

## Steps

```text
fd
↓
struct file
↓
file->f_op->read()
↓
inode
↓
page cache
↓
disk (if miss)
```

## Important:

* Uses f_pos
* Updates f_pos after read

---

# 🧱 6. WRITE() FLOW

```c
write(fd, buf, size)
```

## Steps

```text
fd
↓
struct file
↓
file->f_op->write()
↓
page cache (dirty)
↓
mark inode dirty
↓
flush to disk later
```

---

# 🔥 FULL END-TO-END FLOW

```text
mkfs
↓
disk structures created

----------------------------------

mount
↓
super_block created
↓
root dentry + inode

----------------------------------

open()
↓
path lookup (dentry walk)
↓
inode found
↓
struct file created
↓
fd assigned

----------------------------------

read/write
↓
file → f_op → inode → page cache → disk
```

---

# 🧠 KEY POINTER RELATIONSHIPS

```text
[mkfs]
disk structures created (superblock, inode table, root dir)

[Mount]
super_block → s_root → dentry        (filesystem loaded into RAM)

[Path Resolution]
dentry → inode                       (name → file)
inode → super_block                 (belongs to FS)

[Open]
file → f_path → dentry → inode      (path tracking)
file → f_inode → inode              (direct access)
file → f_op → inode->i_fop          (operations bound)

[Read]
file → f_op → read()                (execute read)
file → f_inode → inode → super_block (locate data)
file → f_pos                        (offset updated)


mkfs creates the filesystem on disk, mount brings it into memory, open binds a file handle to an inode, and read operates directly via file → inode without redoing path lookup.

```

---

# ⚠️ IMPORTANT INTERVIEW POINTS

### 1. file vs inode

* file = open instance
* inode = metadata

### 2. inode vs dentry

* inode = data info
* dentry = name mapping

### 3. Why f_inode exists?

* Shortcut to avoid dentry dereference

### 4. When is struct file created?

* Only during open()

### 5. Is inode created at open?

* No, loaded/cached from disk

---

# 🧠 ONE-LINE SUMMARY

**Disk stores data, superblock anchors filesystem, dentries resolve path, inodes hold metadata, and struct file represents an open instance used by read/write.**

---

# 🚀 MEMORY TRICK

```text
FD → FILE → DENTRY → INODE → DATA
```

# Understanding VFS and NFS Mapping Internals

This document explains how Linux Virtual File System (VFS) knows whether a file belongs to **ext4**, **NFS**, or any other filesystem.

The most important concept is:

> **VFS never checks "Is this ext4 or NFS?" during every read/write operation.**
>
> The mapping is established **once during mount**, and all subsequent operations follow pointers stored inside kernel objects.

---

# 1. Two Mounted Filesystems

Suppose we mount two filesystems:

```bash
mount /dev/sda1 /local          # ext4
mount -t nfs server:/share /nfs # NFS
```

After these commands, Linux has **two mounted filesystems**.

Conceptually:

```
Mount Table

/local  ---------> super_block (ext4)
/nfs    ---------> super_block (nfs)
```

Each mounted filesystem owns its own `struct super_block`.

---

# 2. What is inside a super_block?

## ext4

```
super_block (ext4)

s_op    ---> ext4_super_operations
s_root  ---> root dentry
```

## NFS

```
super_block (nfs)

s_op    ---> nfs_super_operations
s_root  ---> root dentry
```

Notice that **each mounted filesystem has its own super_block**.

This is the first level of mapping.

---

# 3. Path Lookup

Suppose an application executes:

```c
open("/local/a.txt");
```

VFS begins pathname resolution from the root directory.

Conceptually:

```
/
|
+--- local
|
+--- nfs
```

As VFS walks the pathname, it reaches:

```
/local
```

VFS immediately recognizes:

> `/local` is a mount point.

Instead of continuing inside the root filesystem, it switches to the mounted filesystem.

```
Root Filesystem
      |
      +------ local (mount point)
                  |
                  +------ ext4 super_block
```

Now the lookup continues **inside ext4**.

---

# 4. Path Lookup for NFS

Now suppose:

```c
open("/nfs/a.txt");
```

Again VFS begins at:

```
/
|
+---- nfs
```

When it reaches:

```
/nfs
```

it finds another mount point.

```
Root Filesystem
      |
      +------ nfs (mount point)
                  |
                  +------ nfs super_block
```

Now lookup continues **inside the NFS filesystem**.

---

## Important Observation

The pathname determines **which mounted filesystem (super_block)** VFS enters.

There is no filesystem detection during `read()`.

The decision happens during pathname lookup.

---

# 5. Where is this Mapping Stored?

Linux keeps mount information using mount objects.

Conceptually:

```
struct mount

mount_point ---> "/local"

root ---------> ext4 super_block->s_root
```

Another mount:

```
struct mount

mount_point ---> "/nfs"

root ---------> nfs super_block->s_root
```

So during pathname lookup:

```
/nfs
```

VFS follows:

```
mount object
        |
        v
super_block
```

instead of remaining in the root filesystem.

---

# 6. Path Lookup Result

Suppose the lookup finishes for:

```
/local/a.txt
```

The result is:

```
dentry
   |
inode
   |
super_block (ext4)
```

For NFS:

```
dentry
   |
inode
   |
super_block (nfs)
```

Notice something important:

The inode already belongs to the correct filesystem.

---

# 7. open()

After pathname lookup, VFS allocates:

```
struct file
```

Conceptually:

```c
file->f_inode = inode;
file->f_op = inode->i_fop;
```

This is one of the most important assignments in VFS.

---

## Case 1: ext4

The inode contains:

```
inode

i_fop
 |
 +---- ext4_file_operations
```

Therefore:

```
file

f_op
 |
 +---- ext4_file_operations
```

---

## Case 2: NFS

The inode contains:

```
inode

i_fop
 |
 +---- nfs_file_operations
```

Therefore:

```
file

f_op
 |
 +---- nfs_file_operations
```

Notice:

The `struct file` now remembers which filesystem implements file operations.

---

# 8. read()

Later the application executes:

```c
read(fd, buf, 4096);
```

VFS simply executes:

```c
file->f_op->read_iter(...);
```

There is **no switch statement** like:

```c
if (filesystem == ext4)
```

or

```c
if (filesystem == nfs)
```

Instead, function pointers are used.

---

## ext4

```
read()

↓

ext4_read_iter()
```

---

## NFS

```
read()

↓

nfs_file_read()
```

The correct implementation is already stored inside:

```
file->f_op
```

---

# 9. Why Doesn't VFS Need to Check?

Because the mapping has already been established during:

- mount
- pathname lookup
- open()

After `open()`:

```
struct file
        |
        +------ f_op
```

already points to the correct filesystem implementation.

---

# 10. Analogy: TV Remote vs AC Remote

Imagine you have:

```
TV Remote

AC Remote
```

Both contain a button named:

```
Power
```

When you press **Power**:

```
TV Remote

Power

↓

TV turns on
```

```
AC Remote

Power

↓

AC turns on
```

You never write:

```c
if (remote == TV)
```

The remote itself already knows which signal to send.

Similarly:

```
struct file
        |
        +------ f_op
```

acts like the remote.

For ext4:

```
f_op
 |
 +---- ext4 operations
```

For NFS:

```
f_op
 |
 +---- nfs operations
```

Calling:

```c
file->f_op->read_iter();
```

is exactly like pressing the **Power** button on whichever remote you already have.

---

# 11. Complete Mapping

## ext4

```
mount("/dev/sda1", "/local")
                |
                v
      super_block (ext4)
                |
                v
             inode
                |
          i_fop
                |
                v
     ext4_file_operations
                |
                v
           struct file
                |
                v
 file->f_op->read_iter()
                |
                v
        ext4_read_iter()
```

---

## NFS

```
mount("server:/share", "/nfs")
                |
                v
      super_block (nfs)
                |
                v
             inode
                |
          i_fop
                |
                v
      nfs_file_operations
                |
                v
           struct file
                |
                v
 file->f_op->read_iter()
                |
                v
        nfs_file_read()
                |
                v
          RPC to NFS Server
```

---

# 12. Complete VFS Flow

```
Application
      |
      v
open("/local/a.txt")
      |
      v
Path Lookup
      |
      +---- reaches "/local"
      |
      +---- mount object
      |
      +---- ext4 super_block
      |
      +---- dentry
      |
      +---- inode
      |
      +---- inode->i_fop
      |
      +---- create struct file
      |
      +---- file->f_op = inode->i_fop
      |
      v
read()
      |
      v
file->f_op->read_iter()
      |
      v
ext4_read_iter()
```

---

## NFS Flow

```
Application
      |
      v
open("/nfs/a.txt")
      |
      v
Path Lookup
      |
      +---- reaches "/nfs"
      |
      +---- mount object
      |
      +---- nfs super_block
      |
      +---- dentry
      |
      +---- inode
      |
      +---- inode->i_fop
      |
      +---- create struct file
      |
      +---- file->f_op = inode->i_fop
      |
      v
read()
      |
      v
file->f_op->read_iter()
      |
      v
nfs_file_read()
      |
      v
RPC Client
      |
      v
Network
      |
      v
NFS Server
      |
      v
Server VFS
      |
      v
ext4/xfs
      |
      v
Disk
```

---

# Key Takeaways

1. Every mounted filesystem has its own `struct super_block`.
2. Mount points (`/local`, `/nfs`) map to different `super_block` instances.
3. Pathname lookup switches to the correct mounted filesystem using mount objects.
4. Every inode belongs to one `super_block`.
5. Every inode contains filesystem-specific operation tables (`i_fop`, `i_op`).
6. `open()` copies `inode->i_fop` into `file->f_op`.
7. `read()` simply calls `file->f_op->read_iter()`.
8. VFS never checks whether a file belongs to ext4 or NFS during I/O—the correct implementation is already encoded in the pointers established during mount and open.
