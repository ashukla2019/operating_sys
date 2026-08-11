```text
1. CREATE FILESYSTEM
   mkfs.ext4 /dev/sdb1
          │
          ▼
   Creates the filesystem structure ON DISK
          │
          ▼

                     RAM
    ┌────────────────────────────────────────┐
    │                                        │
    │       FILESYSTEM / VFS METADATA        │
    │                                        │
    │  VFS                                   │
    │  Mount information                     │
    │  Dentry cache                          │
    │  Inode cache                           │
    │                                        │
    │  struct file                           │
    │  ┌────────────────────────────────┐    │
    │  │ f_op    → filesystem operations│    │
    │  │ f_inode → associated inode      │    │
    │  │ f_pos   → current file offset   │    │
    │  │ f_flags → O_RDONLY/O_WRONLY...  │    │
    │  │ f_mode  → read/write mode       │    │
    │  │ f_path  → file's path           │    │
    │  │ private_data → FS/device data   │    │
    │  └────────────────────────────────┘    │
    │                                        │
    ├────────────────────────────────────────┤
    │                                        │
    │              FILE DATA                 │
    │                                        │
    │  Page Cache                            │
    │     ├── a.txt contents                 │
    │     ├── b.txt contents                 │
    │     └── ...                            │
    │                                        │
    └──────────────────┬─────────────────────┘
                       │
                       │ accesses / reads / writes
                       ▼
                     DISK
    ┌────────────────────────────────────────┐
    │ ext4 filesystem                        │
    │                                        │
    │ Superblock                             │
    │ Bitmaps                                │
    │ Inodes                                 │
    │ Directories                            │
    │ File data                              │
    └────────────────────────────────────────┘


2. MOUNT FILESYSTEM

   mount /dev/sdb1 /mnt/data
          │
          ▼
   Makes the ext4 filesystem available
   through VFS at /mnt/data.

   IMPORTANT:
   Mount does NOT copy the filesystem
   from disk into RAM.

   Linux creates/uses in-memory structures
   needed to manage the mounted filesystem.


3. OPEN FILE

   fd = open("/mnt/data/a.txt", O_RDONLY)
          │
          ▼
   VFS pathname lookup
          │
          ▼
   dentry → inode
          │
          ▼
   Create/populate struct file
          │
          ├── f_op
          │     └── points to operations for this file
          │         (read/write/ioctl/mmap/etc.)
          │
          ├── f_inode
          │     └── points to the file's inode
          │
          ├── f_pos
          │     └── current file offset
          │         initially usually 0
          │
          ├── f_flags
          │     └── O_RDONLY, O_WRONLY, O_RDWR,
          │         O_APPEND, O_NONBLOCK, etc.
          │
          ├── f_mode
          │     └── kernel read/write mode information
          │
          ├── f_path
          │     └── path/dentry + mount information
          │
          └── private_data
                └── optional filesystem/device-specific data
          │
          ▼
   Process FD table
          │
          ▼
        fd = 3


   Relationship:

   Process
      │
      │ fd = 3
      ▼
   FD table
      │
      ▼
   struct file
      │
      ├── f_op
      ├── f_inode ──────► inode
      ├── f_pos
      ├── f_flags
      ├── f_mode
      ├── f_path
      └── private_data


4. READ FILE

   read(fd, buffer, size)
          │
          ▼
   fd
          │
          ▼
   struct file
          │
          ├── f_pos
          │
          ├── f_inode
          │
          └── f_op
                 │
                 ▼
          filesystem read operation
                 │
                 ▼
             Page Cache
                 │
        ┌────────┴────────┐
        │                 │
     CACHE HIT        CACHE MISS
        │                 │
        ▼                 ▼
   copy data            Disk
   to user               │
                         ▼
                    Page Cache
                         │
                         ▼
                   copy data
                   to user


5. WRITE FILE

   write(fd, buffer, size)
          │
          ▼
   struct file
          │
          ├── f_pos
          ├── f_inode
          └── f_op
                 │
                 ▼
          filesystem write path
                 │
                 ▼
             Page Cache
                 │
                 ▼
          Dirty Page
                 │
                 │ later writeback
                 ▼
               ext4
                 │
                 ▼
            Block Layer
                 │
                 ▼
                Disk


CORE FLOW:

   mkfs
     │
     ▼
   ON-DISK EXT4 STRUCTURES
     │
     ▼
   mount
     │
     ▼
   VFS + in-memory filesystem state
     │
     ▼
   open()
     │
     ▼
   pathname → dentry → inode
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
   fd
     │
     ├─────────────── read() ───────────────► page cache
     │                                          │
     │                                      cache miss
     │                                          │
     │                                          ▼
     │                                         disk
     │
     └─────────────── write() ──────────────► page cache
                                                │
                                                ▼
                                           writeback
                                                │
                                                ▼
                                               disk
```
