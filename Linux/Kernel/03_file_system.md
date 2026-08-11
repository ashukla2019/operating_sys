```text
# Linux VFS + Filesystem: mkfs → mount → open → read/write

============================================================
1. CREATE FILESYSTEM
============================================================

   mkfs.ext4 /dev/sdb1
          │
          ▼
   Creates persistent filesystem structures ON DISK
          │
          ▼

                     DISK
    ┌────────────────────────────────────────┐
    │              EXT4 FILESYSTEM           │
    │                                        │
    │  Superblock                            │
    │  Group descriptors                     │
    │  Block bitmaps                         │
    │  Inode bitmaps                         │
    │  Inode tables                          │
    │  Directories                           │
    │  File data blocks                      │
    │                                        │
    └────────────────────────────────────────┘

   These are persistent structures.
   They remain on disk across reboot.


============================================================
2. MOUNT FILESYSTEM
============================================================

   mount /dev/sdb1 /mnt/data
          │
          ▼
   Linux makes the ext4 filesystem available
   through VFS at /mnt/data.

   IMPORTANT:
   Mount does NOT copy the entire filesystem
   from disk into RAM.

   Linux creates/initializes in-memory runtime
   structures needed to manage the mounted filesystem.

                     RAM
    ┌────────────────────────────────────────┐
    │        VFS / FILESYSTEM STATE          │
    │                                        │
    │  struct super_block                    │
    │      └── represents mounted FS        │
    │                                        │
    │  struct mount / vfsmount               │
    │      └── mount information             │
    │                                        │
    │  root dentry                           │
    │      └── root of mounted filesystem    │
    │                                        │
    │  filesystem-specific state             │
    │                                        │
    │  dentry/inode caches are used as       │
    │  pathname/inode information is needed  │
    │                                        │
    └──────────────────┬─────────────────────┘
                       │
                       │ accesses filesystem
                       ▼
                     DISK
    ┌────────────────────────────────────────┐
    │              EXT4 FILESYSTEM           │
    │                                        │
    │  Superblock                            │
    │  Bitmaps                               │
    │  Inode tables                           │
    │  Directories                           │
    │  File data                             │
    └────────────────────────────────────────┘


============================================================
3. OPEN FILE
============================================================

   fd = open("/mnt/data/a.txt", O_RDONLY)
          │
          ▼
   VFS pathname lookup
          │
          ▼
   / → mnt → data → a.txt
          │
          ▼
       dentry
          │
          ▼
       inode
          │
          │
          ▼
   Create/populate struct file
          │
          ├── f_op
          │     └── points to file_operations
          │         (read/write/ioctl/mmap/etc.)
          │
          ├── f_inode
          │     └── points to associated inode
          │
          ├── f_pos
          │     └── current file offset
          │
          ├── f_flags
          │     └── O_RDONLY/O_WRONLY/O_RDWR/
          │         O_APPEND/O_NONBLOCK/etc.
          │
          ├── f_mode
          │     └── kernel read/write mode
          │
          ├── f_path
          │     └── mount + dentry
          │
          └── private_data
                └── optional FS/device-specific data
          │
          ▼
   Process FD table
          │
          ▼
        fd = 3


   IMPORTANT RELATIONSHIP:

   Process
      │
      │ fd = 3
      ▼
   FD table
      │
      ▼
   struct file
      │
      ├── f_op ─────────► file_operations
      │
      ├── f_inode ──────► inode
      │
      ├── f_pos
      ├── f_flags
      ├── f_mode
      ├── f_path
      └── private_data


============================================================
4. READ FILE
============================================================

   read(fd, buffer, size)
          │
          ▼
         fd
          │
          ▼
      FD table
          │
          ▼
      struct file
          │
          ├── f_pos
          ├── f_inode
          └── f_op
                 │
                 ▼
          filesystem read path
                 │
                 ▼
          inode->i_mapping
                 │
                 ▼
          struct address_space
                 │
                 ▼
             Page Cache
                 │
        ┌────────┴────────┐
        │                 │
     CACHE HIT        CACHE MISS
        │                 │
        ▼                 ▼
   Copy data            ext4
   to user                │
                          ▼
                    storage/block I/O
                          │
                          ▼
                         Disk
                          │
                          ▼
                     Page Cache
                          │
                          ▼
                    Copy data
                    to user


============================================================
5. WRITE FILE
============================================================

   write(fd, buffer, size)
          │
          ▼
      FD table
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
          inode->i_mapping
                 │
                 ▼
          struct address_space
                 │
                 ▼
             Page Cache
                 │
                 ▼
             Dirty Pages
                 │
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


============================================================
6. IMPORTANT VFS STRUCTURES
============================================================

   struct super_block
      → represents a mounted filesystem

   struct mount / vfsmount
      → represents mount information

   struct dentry
      → pathname component / name → inode

   struct inode
      → file/directory metadata and object

   struct file
      → one particular open instance

   struct path
      → mount + dentry

   struct file_operations
      → operations available through f_op

   struct address_space
      → file/inode ↔ page-cache mapping

   Page Cache
      → cached file contents in RAM

   FD table
      → fd → struct file


============================================================
7. DENTRY vs INODE vs STRUCT FILE
============================================================

   DENTRY
      → "What name/path is this?"

   INODE
      → "What is this file?"

   STRUCT FILE
      → "How is this particular open() using the file?"

   FD
      → "Integer handle used by the application"


============================================================
8. COMPLETE FLOW
============================================================

   mkfs.ext4 /dev/sdb1
          │
          ▼
   ON-DISK EXT4 STRUCTURES
          │
          │
   mount /dev/sdb1 /mnt/data
          │
          ▼
   VFS / in-memory filesystem state
          │
          ├── super_block
          ├── mount
          └── root dentry
          │
          ▼
   open("/mnt/data/a.txt")
          │
          ▼
   pathname lookup
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
       fd = 3
          │
          ├──────────── read() ────────────► address_space
          │                                      │
          │                                      ▼
          │                                  Page Cache
          │                                      │
          │                                  cache miss
          │                                      │
          │                                      ▼
          │                                     ext4
          │                                      │
          │                                      ▼
          │                                     Disk
          │
          └──────────── write() ───────────► address_space
                                                 │
                                                 ▼
                                             Page Cache
                                                 │
                                                 ▼
                                           Dirty Pages
                                                 │
                                                 ▼
                                            writeback
                                                 │
                                                 ▼
                                                ext4
                                                 │
                                                 ▼
                                                Disk


============================================================
9. INTERVIEW MEMORY MAP
============================================================

                     APPLICATION
                          │
                       fd = 3
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


CORE FORMULA:

   Path
    ↓
   Dentry
    ↓
   Inode
    ↓
   struct file
    ↓
   FD

   struct file
        ↓
   address_space
        ↓
   Page Cache
        ↓
   Filesystem
        ↓
   Block Layer
        ↓
   Disk
```
