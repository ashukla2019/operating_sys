

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
