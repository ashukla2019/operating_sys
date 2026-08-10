# Linux Storage → VFS → File I/O Flow

## 1. Big Picture

The complete flow from a block device to an application `read()` looks like this:

```text
                         STORAGE CREATION
                         ────────────────

/dev/sdb1
    │
    │ mkfs.ext4
    ↓
EXT4 filesystem
    │
    │ On-disk filesystem structures
    ↓
Superblock / inode tables / data blocks


                         MOUNT
                         ─────

/dev/sdb1
    │
    ↓
   ext4
    │
    ↓
struct super_block
    │
    │ filesystem representation
    │
    └──────────────┐
                   ↓
             struct vfsmount
                   │
                   │ mounted at
                   ↓
                /mnt/data


                         PATH LOOKUP
                         ───────────

open("/mnt/data/a.txt")
           │
           ↓
          VFS
           │
           ↓
      Path traversal
           │
           ↓
         dentry
           │
           ↓
         inode
           │
           ↓
      struct file
           │
           ↓
       FD table
           │
           ↓
         fd = 3


                         READ
                         ────

read(3, ...)
      │
      ↓
  FD table
      │
      ↓
 struct file
      │
      ↓
    inode
      │
      ↓
  page cache
      │
      ├──── Cache hit ─────→ application
      │
      │ Cache miss
      ↓
     ext4
      │
      ↓
 block layer
      │
      ↓
 device driver
      │
      ↓
 /dev/sdb
      │
      ↓
    disk
      │
      ↓
    data
      │
      ↓
 page cache
      │
      ↓
 application buffer
```

---

# 2. `/dev/sdb1` — The Block Device

Suppose the system has:

```text
/dev/sdb
```

with partitions:

```text
/dev/sdb
 ├── /dev/sdb1
 ├── /dev/sdb2
 └── ...
```

`/dev/sdb1` represents a **partition of a block device**.

At this point, it is simply a region of storage.

It does not necessarily contain a filesystem yet.

---

# 3. Creating the Filesystem

We run:

```bash
mkfs.ext4 /dev/sdb1
```

Conceptually:

```text
/dev/sdb1
    │
    │ mkfs.ext4
    ↓
EXT4 filesystem
```

`mkfs.ext4` creates the **on-disk filesystem structures**.

A simplified representation is:

```text
/dev/sdb1

+-------------------------+
| Superblock              |
+-------------------------+
| Filesystem metadata     |
+-------------------------+
| Block group metadata    |
+-------------------------+
| Inode bitmaps           |
+-------------------------+
| Block bitmaps           |
+-------------------------+
| Inode tables            |
+-------------------------+
| Data blocks             |
+-------------------------+
```

Important:

> `mkfs.ext4` creates filesystem structures **on disk**.

This is different from the Linux kernel's in-memory:

```c
struct super_block
```

The kernel creates/populates its in-memory representation when the filesystem is mounted.

---

# 4. Mounting the Filesystem

Now run:

```bash
mount /dev/sdb1 /mnt/data
```

The kernel recognizes:

```text
/dev/sdb1
    │
    ↓
  ext4
```

The ext4 filesystem driver reads the filesystem metadata and creates/uses kernel-side filesystem structures.

One important structure is:

```c
struct super_block
```

Think of it as:

```text
                 RAM
                  │
                  ↓
          struct super_block
                  │
                  │ represents
                  ↓
           ext4 filesystem
                  │
                  ↓
             /dev/sdb1
```

---

# 5. What Is `struct super_block`?

`struct super_block` represents a filesystem instance inside the kernel.

It contains information related to the filesystem, including things such as:

* filesystem type
* block size
* filesystem state
* filesystem operations
* root inode
* filesystem-specific information

Conceptually:

```text
struct super_block
 ├── filesystem type
 ├── block size
 ├── filesystem state
 ├── filesystem operations
 ├── root inode
 └── filesystem-specific data
```

For example:

```text
/dev/sdb1
    │
    ↓
   ext4
    │
    ↓
struct super_block
```

---

# 6. What Is `vfsmount`?

This is an important distinction.

It is better **not** to think of:

```text
super_block
    ↓
vfsmount
    ↓
directory
```

as a simple sequential chain.

Instead:

```text
                 struct super_block
                         │
                         │ represents
                         ↓
                  EXT4 filesystem
                         │
              ┌──────────┴──────────┐
              │                     │
              ↓                     ↓
        root inode             other inodes


                 struct vfsmount
                         │
                         │ mount relationship
                         ↓
                     /mnt/data
```

`struct vfsmount` represents a **mount of a filesystem into the VFS namespace**.

A useful mental model is:

```text
super_block
    │
    │ "What filesystem is this?"
    │
    └──── ext4 filesystem


vfsmount
    │
    │ "Where is this filesystem mounted?"
    │
    └──── /mnt/data
```

### Important distinction

| Structure            | Meaning                                                                      |
| -------------------- | ---------------------------------------------------------------------------- |
| `struct super_block` | Represents the filesystem                                                    |
| `struct vfsmount`    | Represents the filesystem's mount relationship/location in the VFS namespace |

---

# 7. What Is `/mnt/data`?

Suppose:

```bash
mount /dev/sdb1 /mnt/data
```

The existing root filesystem may look like:

```text
/
├── etc
├── home
├── var
└── mnt
    └── data
```

Before mounting, `/mnt/data` is simply a directory in the root filesystem.

After mounting:

```text
                    Root filesystem
                          │
                          │
                       /mnt/data
                          │
                          │ mount point
                          ↓
                    EXT4 filesystem
                    on /dev/sdb1
```

The VFS effectively says:

> When path traversal reaches `/mnt/data`, cross into the filesystem mounted there.

This is where the mount infrastructure becomes important.

---

# 8. Creating `a.txt`

Suppose we execute:

```bash
touch /mnt/data/a.txt
```

or:

```c
open("/mnt/data/a.txt", O_CREAT | O_WRONLY);
```

The kernel has to resolve the pathname.

The path is:

```text
/mnt/data/a.txt
```

Conceptually:

```text
"/mnt/data/a.txt"
        │
        ↓
       /
        │
        ↓
      mnt
        │
        ↓
      data
        │
        │ mount crossing
        ↓
    EXT4 root
        │
        ↓
      a.txt
```

During path lookup, the VFS uses **dentries and inodes**.

---

# 9. Dentry vs Inode

This is one of the most important VFS concepts.

## Dentry

A dentry represents a **directory entry/name relationship**.

For:

```text
a.txt
```

the dentry represents the name `a.txt` and its relationship to the corresponding filesystem object.

Think:

```text
"a.txt"
   │
   ↓
dentry
   │
   ↓
inode
```

A dentry answers roughly:

> "What object does this pathname component refer to?"

---

# 10. Inode

An inode represents the **file or directory object**.

It contains metadata such as:

```text
inode
 ├── permissions
 ├── owner
 ├── timestamps
 ├── file size
 ├── link count
 └── filesystem-specific information
```

The inode also participates in mapping the file's logical contents to filesystem storage.

Conceptually:

```text
directory
    │
    │ "a.txt"
    ↓
  dentry
    │
    │ points/references
    ↓
  inode
    │
    │ file data mapping
    ↓
data blocks
```

Do not think of dentry and inode as simply two sequential layers of storage.

A better mental model is:

```text
                VFS
                 │
          path resolution
                 │
                 ↓
              dentry
                 │
                 ↓
               inode
                 │
                 ↓
             filesystem
                 │
                 ↓
            disk blocks
```

---

# 11. `open()` and `struct file`

Now consider:

```c
int fd = open("/mnt/data/a.txt", O_RDONLY);
```

The kernel first resolves the pathname:

```text
/mnt/data/a.txt
        │
        ↓
       VFS
        │
        ↓
     dentry
        │
        ↓
      inode
```

But `open()` needs to create an **open file instance**.

The kernel creates a:

```c
struct file
```

Conceptually:

```text
pathname
   │
   ↓
dentry
   │
   ↓
inode
   │
   ↓
struct file
```

---

# 12. What Is `struct file`?

`struct file` represents an **open instance of a file**.

It contains information such as:

```text
struct file
 ├── current file position
 ├── open flags
 ├── path
 │    ├── dentry
 │    └── mount
 └── file operations
```

One important point:

> The inode represents the underlying file object, while `struct file` represents a particular open instance of that file.

For example:

```text
Process A
   │
   └── open("a.txt")
          │
          ↓
     struct file A
          │
          ↓
        inode


Process B
   │
   └── open("a.txt")
          │
          ↓
     struct file B
          │
          ↓
        same inode
```

The two processes can have different file offsets and open flags even though they refer to the same underlying inode.

---

# 13. File Descriptor — `FD = 3`

The application does not receive the kernel's `struct file *`.

Instead, it receives an integer:

```text
FD = 3
```

Each process has a file descriptor table.

Conceptually:

```text
Process
   │
   ↓
FD table
   │
   ├── 0 → struct file
   ├── 1 → struct file
   ├── 2 → struct file
   └── 3 → struct file
```

Therefore:

```text
application
     │
     │ fd = 3
     ↓
process FD table
     │
     │ lookup index 3
     ↓
struct file
```

### Very important

> **FD 3 is not the file itself.**

It is an integer/index used by the process to identify an open file description.

---

# 14. `read(3, ...)`

Now the application executes:

```c
read(3, buffer, 100);
```

The simplified kernel flow is:

```text
read(3, ...)
      │
      ↓
  FD table
      │
      │ fd = 3
      ↓
 struct file
      │
      ↓
    path
      │
      ├── vfsmount
      │
      └── dentry
             │
             ↓
           inode
             │
             ↓
       filesystem
```

The kernel now needs to retrieve the file data.

---

# 15. The Page Cache

There is one major component missing from a simplistic:

```text
inode → ext4 → disk
```

flow.

For normal buffered I/O, the **page cache** is extremely important.

The more accurate flow is:

```text
read(3)
   │
   ↓
FD table
   │
   ↓
struct file
   │
   ↓
inode
   │
   ↓
page cache
   │
   ├── cache hit ─────────→ application
   │
   │
   └── cache miss
          │
          ↓
        ext4
          │
          ↓
      block layer
          │
          ↓
      device driver
          │
          ↓
       block device
          │
          ↓
          disk
          │
          ↓
         data
          │
          ↓
      page cache
          │
          ↓
 application buffer
```

This is a critical interview concept.

> `read()` does **not necessarily mean "go to disk."`

If the required data is already in the page cache, the kernel can satisfy the read from RAM.

---

# 16. Cache Hit

Suppose the file data is already cached:

```text
read(3)
   │
   ↓
struct file
   │
   ↓
inode
   │
   ↓
page cache
   │
   │ data exists
   ↓
application buffer
```

No disk I/O is required.

So:

```text
Application
     ↑
     │
Page Cache
     ↑
     │
   inode
```

---

# 17. Cache Miss

If the required data isn't in the page cache:

```text
read(3)
   │
   ↓
page cache
   │
   │ cache miss
   ↓
  ext4
   │
   ↓
block layer
   │
   ↓
device driver
   │
   ↓
block device
   │
   ↓
disk
```

The disk returns the data:

```text
disk
  │
  ↓
device driver
  │
  ↓
block layer
  │
  ↓
ext4
  │
  ↓
page cache
  │
  ↓
application
```

---

# 18. Where Does ext4 Fit?

`ext4` is a filesystem implementation.

The VFS provides the common filesystem interface.

Conceptually:

```text
                    Application
                         │
                         ↓
                       VFS
                         │
              ┌──────────┴──────────┐
              │                     │
              ↓                     ↓
            ext4                  XFS
              │                     │
              ↓                     ↓
         block layer           block layer
              │                     │
              └──────────┬──────────┘
                         ↓
                    block device
```

This is why applications don't need to know whether:

```text
a.txt
```

is stored on:

```text
ext4
```

or:

```text
XFS
```

or another filesystem.

The VFS provides the common interface.

---

# 19. Complete File Creation + Read Flow

Putting everything together:

```text
                         PHYSICAL STORAGE
                         ─────────────────

                         /dev/sdb1
                             │
                             │ mkfs.ext4
                             ↓
                      EXT4 filesystem
                             │
                             ↓
                  On-disk filesystem metadata
                             │
                  ┌──────────┼──────────┐
                  ↓          ↓          ↓
             superblock   inode      data blocks
                         tables


                         MOUNT
                         ─────

/dev/sdb1
    │
    ↓
   ext4
    │
    ↓
struct super_block
    │
    └────────────────────┐
                         ↓
                   struct vfsmount
                         │
                         │ mounted at
                         ↓
                      /mnt/data


                         FILE CREATION
                         ──────────────

open("/mnt/data/a.txt", O_CREAT)
              │
              ↓
             VFS
              │
              ↓
        pathname lookup
              │
              ↓
           dentry
              │
              ↓
            inode
              │
              ↓
       struct file
              │
              ↓
          FD table
              │
              ↓
            fd = 3


                         FILE READ
                         ─────────

read(3, buffer, size)
              │
              ↓
          FD table
              │
              ↓
         struct file
              │
              ↓
            inode
              │
              ↓
         page cache
              │
          ┌───┴───┐
          │       │
       HIT       MISS
          │       │
          │       ↓
          │     ext4
          │       │
          │       ↓
          │   block layer
          │       │
          │       ↓
          │  device driver
          │       │
          │       ↓
          │      disk
          │       │
          │       ↓
          │   page cache
          │       │
          └───┬───┘
              ↓
       application buffer
```

---

# 20. The Most Important Relationships

For interviews, remember these relationships:

```text
/dev/sdb1
    │
    ↓
   ext4
    │
    ↓
super_block
```

`super_block` represents the filesystem inside the kernel.

```text
super_block
    +
vfsmount
    │
    ↓
mounted filesystem
    │
    ↓
 /mnt/data
```

`vfsmount` represents the mount relationship in the VFS namespace.

```text
pathname
    │
    ↓
dentry
    │
    ↓
inode
```

Dentry is primarily about **names/path components**.

Inode represents the **file object and metadata**.

```text
inode
    │
    ↓
struct file
```

`struct file` represents a particular **open instance**.

```text
FD = 3
    │
    ↓
FD table
    │
    ↓
struct file
```

The FD is an integer index/handle into the process's descriptor table.

---

# 21. Six Objects to Memorize

For senior Linux/VFS/storage interviews, know these very well:

| Object               | What to remember                                              |
| -------------------- | ------------------------------------------------------------- |
| `struct super_block` | Represents a filesystem instance                              |
| `struct vfsmount`    | Represents a filesystem mount in the VFS namespace            |
| `struct dentry`      | Represents a directory entry/name relationship                |
| `struct inode`       | Represents a filesystem object and its metadata               |
| `struct file`        | Represents one open instance of a file                        |
| File descriptor      | Integer handle/index used by a process to access an open file |

Also remember:

```text
Page cache
    ↓
Cached file data in RAM
```

---

# 22. Interview Answer: "What Happens During read(fd)?"

A strong senior-level answer is:

> When `read(fd)` is called, the kernel uses the file descriptor to index the process's file descriptor table and obtain the corresponding `struct file`. From `struct file`, the kernel can reach the file's path, dentry, and inode. For normal buffered I/O, the kernel checks the page cache first. If the data is cached, it can be returned without accessing the disk. On a cache miss, the filesystem such as ext4 translates the file's logical data into filesystem/storage blocks and submits I/O through the block layer and device driver to the block device. Once the data is read into memory, it is made available through the page cache and ultimately copied into the application's buffer.

---

# 23. One-Line Mental Model

The simplest mental model is:

```text
FD
 ↓
FD table
 ↓
struct file
 ↓
dentry
 ↓
inode
 ↓
page cache
 ↓
ext4
 ↓
block layer
 ↓
device driver
 ↓
disk
```

And for mounting:

```text
/dev/sdb1
    ↓
   ext4
    ↓
super_block
    +
vfsmount
    ↓
/mnt/data
```

The key idea is:

> **VFS provides the common abstraction, dentries handle pathname/name lookup, inodes represent filesystem objects, `struct file` represents an open instance, file descriptors give applications an integer handle, and the page cache sits between file access and actual storage I/O.**
