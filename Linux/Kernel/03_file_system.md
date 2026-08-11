# Linux VFS — Interview Notes

## 1. VFS in One Picture

VFS provides a common interface for filesystems such as `ext4`, `xfs`, `nfs`, etc.

```text
Application
     │
     │ open/read/write
     ▼
    VFS
     │
     ├── ext4
     ├── xfs
     └── nfs
```

---

# 2. Important VFS Structures

| Structure                   | Purpose                                                |
| --------------------------- | ------------------------------------------------------ |
| `struct super_block`        | Represents a mounted filesystem                        |
| `struct inode`              | Represents file/directory metadata                     |
| `struct dentry`             | Represents a pathname component and links name → inode |
| `struct file`               | Represents one open instance of a file                 |
| `struct path`               | Represents mount + dentry                              |
| `struct mount` / `vfsmount` | Represents mount information                           |
| `struct file_operations`    | Function-pointer table for file operations             |
| `struct address_space`      | Connects inode/file with page-cache mapping            |
| FD table                    | Maps application FD → `struct file`                    |
| Page cache                  | Caches file contents in RAM                            |

---

# 3. `struct super_block`

Represents a **mounted filesystem instance** in memory.

```text
struct super_block
├── s_type       → filesystem type (ext4, xfs...)
├── s_root       → root dentry
├── s_op         → superblock operations
└── s_fs_info    → filesystem-specific information
```

Important distinction:

```text
Disk:
    ext4 on-disk superblock

RAM:
    struct super_block
```

---

# 4. `struct inode`

Represents the **file/directory object and its metadata**.

Important fields:

```text
struct inode
├── i_mode       → type + permissions
├── i_uid/i_gid  → owner/group
├── i_size       → file size
├── i_ino        → inode number
├── i_nlink      → link count
├── i_op         → inode operations
├── i_fop        → default file operations
├── i_mapping    → address_space / page-cache mapping
└── i_sb         → associated super_block
```

Think:

```text
inode = "What is this file?"
```

The inode does **not** primarily represent the filename.

---

# 5. `struct dentry`

Represents a **directory entry / pathname component**.

Conceptually:

```text
"a.txt" → inode #100
```

For:

```text
/mnt/data/a.txt
```

pathname lookup walks through dentries:

```text
/ → mnt → data → a.txt
                    │
                    ▼
                 inode
```

Think:

```text
dentry = "What name/path refers to this inode?"
```

### Dentry Cache

Dentries are cached in RAM to speed up pathname lookup.

```text
dentry cache
"a.txt" → inode #100
```

---

# 6. `struct file`

Represents **one particular `open()` instance**.

Important fields:

```text
struct file
├── f_op          → file_operations
├── f_inode       → associated inode
├── f_pos         → current file offset
├── f_flags       → O_RDONLY, O_WRONLY, O_APPEND...
├── f_mode        → kernel read/write mode
├── f_path        → mount + dentry
├── f_mapping     → address_space
└── private_data  → filesystem/device-specific data
```

### `f_op`

Points to:

```text
struct file_operations
```

which contains operation callbacks such as:

```text
read
write
read_iter
write_iter
llseek
ioctl
mmap
fsync
release
...
```

Think:

```text
inode = file object
struct file = one open instance
```

---

# 7. `struct path`

Represents where an object exists in the mounted filesystem.

```text
struct path
├── mnt      → mount
└── dentry   → dentry
```

Conceptually:

```text
path
 │
 ├── mount  → mounted filesystem
 │
 └── dentry → pathname/object
```

---

# 8. Mount Structures

Example:

```bash
mount /dev/sdb1 /mnt/data
```

Conceptually:

```text
/dev/sdb1
    │
    ▼
   ext4
    │
    ▼
mounted at /mnt/data
```

The kernel maintains mount information connecting the filesystem to the mount point.

Modern kernels use `struct mount` internally, with `vfsmount` providing the VFS mount interface.

---

# 9. `struct address_space` and Page Cache

`address_space` represents the mapping between a file/inode and cached pages.

```text
inode
  │
  │ i_mapping
  ▼
address_space
  │
  ▼
Page Cache
  │
  ▼
file contents
```

Example:

```text
a.txt on disk
      │
      │ read
      ▼
Page Cache in RAM
      │
      ▼
application
```

### Cache hit

```text
read()
  │
  ▼
Page Cache
  │
  └── HIT → return data
```

### Cache miss

```text
read()
  │
  ▼
Page Cache
  │
  └── MISS
       │
       ▼
   filesystem
       │
       ▼
      disk
       │
       ▼
   Page Cache
```

---

# 10. FD Table

The application gets an integer, not a `struct file *`.

Example:

```c
int fd = open("a.txt", O_RDONLY);
```

Conceptually:

```text
Process
   │
   │ fd = 3
   ▼
FD table
   │
   ▼
struct file
```

Therefore:

```text
fd → struct file → inode
```

---

# 11. Dentry vs Inode vs File

Very common interview question:

```text
DENTRY
  ↓
"name/path lookup"

INODE
  ↓
"file/directory metadata"

STRUCT FILE
  ↓
"one particular open instance"

FD
  ↓
"integer handle used by application"
```

---

# 12. Multiple `open()` vs `dup()`

### Two independent `open()` calls

```c
fd1 = open("a.txt", O_RDONLY);
fd2 = open("a.txt", O_RDONLY);
```

```text
              inode #100
               /      \
              /        \
        struct file  struct file
            │             │
         f_pos=0       f_pos=0
            │             │
           fd1           fd2
```

Each `struct file` has its own `f_pos`.

### `dup()`

```c
fd2 = dup(fd1);
```

```text
          struct file
          /         \
        fd1         fd2
```

Both FDs refer to the **same `struct file`**, so they share `f_pos` and open-file state.

---

# 13. Complete Filesystem → VFS → `open()` → `read()` Flow

## Create filesystem

```bash
mkfs.ext4 /dev/sdb1
```

Creates persistent filesystem structures on disk:

```text
DISK
├── Superblock
├── Bitmaps
├── Inode tables
├── Directories
└── File data
```

## Mount

```bash
mount /dev/sdb1 /mnt/data
```

Makes the filesystem accessible through VFS at `/mnt/data`.

**Mount does NOT copy the entire filesystem into RAM.**

Linux creates/uses in-memory VFS/filesystem state and caches.

---

## `open()`

```c
fd = open("/mnt/data/a.txt", O_RDONLY);
```

```text
pathname
   │
   ▼
 VFS
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
   ├── f_op
   ├── f_inode
   ├── f_pos
   ├── f_flags
   ├── f_mode
   └── f_path
   │
   ▼
FD table
   │
   ▼
fd = 3
```

---

## `read()`

```c
read(fd, buffer, size);
```

```text
fd
 │
 ▼
struct file
 │
 ▼
address_space
 │
 ▼
Page Cache
 │
 ├── HIT  → return data
 │
 └── MISS
       │
       ▼
   filesystem
       │
       ▼
      disk
       │
       ▼
   Page Cache
       │
       ▼
   application
```

---

# 14. Final Memory Map

```text
                         PROCESS
                            │
                          fd=3
                            │
                            ▼
                      ┌────────────┐
                      │ struct file│
                      │            │
                      │ f_op       │──────► file_operations
                      │ f_inode    │───┐
                      │ f_pos      │   │
                      │ f_flags    │   │
                      │ f_path     │─┐ │
                      └────────────┘ │ │
                                     │ │
                              ┌──────┘ │
                              ▼        │
                           dentry      │
                              │        │
                              ▼        │
                            inode ◄────┘
                              │
                         i_mapping
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
                            DISK
```

### One-line revision

```text
Path → dentry → inode → struct file → fd
                         │
                         └→ address_space → page cache → filesystem → block layer → disk
```

### Most important distinction

```text
dentry = name/path
inode  = file metadata/object
file   = one open instance
fd     = application handle
page cache = file contents in RAM
```
