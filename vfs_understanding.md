# VFS Flow: Filesystem → Mount → Create → Open → Read

## 1. CREATE FILESYSTEM

/dev/sdb1
    │
   mkfs
    ↓
ext4 filesystem
    │
    └── superblock / inode structures / data


## 2. MOUNT

ext4 filesystem
      │
    mount
      ↓
  super_block
      ↓
   vfsmount
      ↓
 /mnt/data


## 3. CREATE FILE

/mnt/data/a.txt
       │
       ↓
    dentry
       │
       ↓
    inode
       │
       ↓
    ext4 → disk


## 4. OPEN

open("/mnt/data/a.txt")
          │
          ↓
         VFS
          │
          ↓
   vfsmount + dentry
          │
          ↓
        inode
          │
          ↓
     struct file
          │
          ↓
       FD = 3


## 5. READ

read(3)
   │
   ↓
FD table
   │
   ↓
struct file
   │
   ↓
inode / filesystem
   │
   ↓
ext4
   │
   ↓
disk
   │
   ↓
data → application


## COMPLETE FLOW

/dev/sdb1
    │
   mkfs
    ↓
ext4 filesystem
    │
  mount
    ↓
super_block
    │
    ↓
vfsmount
    │
    ↓
/mnt/data
    │
    │ create a.txt
    ↓
dentry
    │
    ↓
inode
    │
    │ open("/mnt/data/a.txt")
    ↓
struct file
    │
    ↓
FD = 3
    │
    │ read(3)
    ↓
FD table
    │
    ↓
struct file
    │
    ↓
inode / filesystem
    │
    ↓
ext4
    │
    ↓
disk
    │
    ↓
data → application


## VFS OBJECTS

super_block
    → Describes the filesystem

vfsmount
    → Represents a mounted filesystem

dentry
    → Represents a name/path component

inode
    → Represents the actual file/directory

struct file
    → Represents one particular open instance

fd
    → Integer handle used by the application


## KEY RELATIONSHIP

Filesystem object:

    dentry → inode

Open instance:

    struct file

Application handle:

    fd


PATH
 │
 ↓
vfsmount + dentry
 │
 ↓
inode
 │
 ↓
struct file
 │
 ↓
fd
 │
 ↓
Application


## IMPORTANT DISTINCTION

             FILE
              │
              ▼
           inode
              ▲
              │
       ┌──────┴──────┐
       │             │
       ▼             ▼
   struct file   struct file
       │             │
       ▼             ▼
      fd=3          fd=4

The inode represents the file itself.

Each struct file represents a particular open instance.

The fd is the number the application uses to access that open instance.
