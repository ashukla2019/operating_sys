1. CREATE FILESYSTEM

/dev/sdb1
    │
   mkfs
    ↓
 ext4 filesystem
    │
    └── superblock / inode structures / data


2. MOUNT

ext4 filesystem
      │
    mount
      ↓
  super_block
      ↓
   vfsmount
      ↓
 /mnt/data


3. CREATE FILE

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


4. OPEN

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


5. READ

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
