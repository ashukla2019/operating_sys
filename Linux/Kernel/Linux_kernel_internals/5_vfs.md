# Chapter 5 – Virtual File System (VFS)

## 1. What is VFS?
VFS stands for **Virtual File System** — the abstraction layer in the Linux kernel that provides a common interface to different filesystems. Linux supports many filesystems: ext4, XFS, Btrfs, tmpfs, procfs, sysfs, NFS, FAT, NTFS...

Applications should not need to know which filesystem stores a file. For example, `int fd = open("/home/user/data.txt", O_RDONLY);` uses the same `open()` interface regardless of whether the file is stored on ext4, XFS, NFS, a USB filesystem, SSD, or a RAM-based filesystem. VFS hides these filesystem-specific details.

## 2. Why Do We Need VFS?
Without VFS, every application would need to understand every filesystem's API (ext4 API, XFS API, NFS API, FAT API — extremely complicated). With VFS:
```
Application --> open/read/write --> VFS --> ext4 / XFS / NFS
```
The application uses a generic interface.

## 3. Linux File I/O Architecture
```
Application
   |
System Calls (open/read/write/close)
   |
   VFS
   |
+--+--+
ext4  XFS  NFS
   |
Block / Network
   |
Hardware
```

---

## 4. Important VFS Structures
The most important structures: `struct super_block`, `struct inode`, `struct dentry`, `struct file`, `struct path`. A useful mental model:
- **super_block** → Filesystem instance
- **inode** → Filesystem object
- **dentry** → Directory/path component
- **file** → Open file instance
- **path** → dentry + mount
- **fd** → Userspace handle to an open file

## 5. super_block
`struct super_block` represents an instance of a mounted filesystem:
```
super_block
   +-- Filesystem type
   +-- Block size
   +-- Filesystem information
   +-- Root dentry
   +-- Mount information
   +-- Filesystem operations
```
E.g. `/dev/sda1 --> ext4 --> super_block`.

## 6. Important Distinction: super_block
A common interview mistake: "super_block represents a disk." Not exactly — it represents a **filesystem instance**: `/dev/sda1 --> ext4 --> Mounted filesystem --> super_block`. A single physical disk can contain multiple partitions/filesystems, and each mounted filesystem has its own state represented by kernel structures.

---

## 7. inode
An inode represents a filesystem object — a regular file, directory, symbolic link, or device. It contains metadata: file type, permissions, owner, group, size, timestamps, link count, and filesystem-specific information:
```
inode
 +-- File type
 +-- Permissions
 +-- UID/GID
 +-- Size
 +-- Timestamps
 +-- Link count
 +-- Data mapping information
```

## 8. inode Does Not Store the Filename
Important interview point — the filename is associated with directory entries: `filename --> dentry --> inode --> file data`. E.g. `notes.txt --> inode 125`. The inode represents the underlying filesystem object.

## 9. dentry
`dentry` means **Directory Entry** — it represents a component of a pathname and participates in mapping names to filesystem objects. For `/home/user/notes.txt`, the components are `/`, `home`, `user`, `notes.txt`. During path lookup, Linux uses dentries to represent these components:
```
/ --- home --- user --- notes.txt
```

## 10. Why Does Linux Need Dentries?
Path lookup happens extremely frequently. For `open("/home/user/file.txt", O_RDONLY);`, the kernel needs to resolve `/ → home → user → file.txt`. The dentry cache helps Linux avoid repeatedly performing expensive filesystem lookups.

## 11. Dentry Cache
Linux maintains a cache of dentries:
```
Path Lookup --> Dentry Cache --Hit--> Fast Lookup
                              --Miss--> Filesystem Lookup
```
A dentry cache hit can significantly reduce path lookup overhead.

---

## 12. file
`struct file` represents an **open file instance** — different from an inode. For `int fd = open("data.txt", O_RDONLY);`, the kernel creates/uses an open-file object represented by `struct file`:
```
fd --> struct file --> path
                          +-- dentry
                          +-- mount
                            --> inode
```

## 13. inode vs struct file
Very important distinction.
- **inode** — represents the filesystem object: "What file/object is this?"
- **struct file** — represents a particular open instance: "How is this particular open instance being used?"

Example: Process A's `fd 3 → file A → inode 100` and Process B's `fd 4 → file B → inode 100` — both processes can reference the same underlying inode through different open-file objects.

## 14. File Position
The open-file object maintains state associated with the open instance, including the current file position. For `read(fd, buffer, 100);` — initially `offset = 0`, after reading 100 bytes `offset = 100`:
```
struct file --> f_pos
```
This is one reason `struct file` is different from `inode`.

## 15. File Descriptor
Applications don't directly manipulate `struct file` — they use a file descriptor. For `int fd = open("data.txt", O_RDONLY);` suppose `fd = 3`:
```
Process --> File Descriptor Table
              +-- 0 → stdin
              +-- 1 → stdout
              +-- 2 → stderr
              +-- 3 → struct file --> inode
```
The file descriptor is an integer used by userspace.

## 16. Standard File Descriptors
Normally: `0 → stdin`, `1 → stdout`, `2 → stderr`. E.g. `write(1, "Hello\n", 6);` — the value `1` refers to standard output for that process.

## 17. path
Linux also uses `struct path` to represent a location in the mounted filesystem hierarchy:
```
struct path --> mount + dentry
```
This is important because the same filesystem object can be accessed through different mount contexts.

## 18. Important VFS Relationship
A useful diagram to remember:
```
Process --> File Descriptor --> struct file --> struct path
                                                   +--> dentry
                                                   +--> mount --> super_block
dentry --> inode
```

---

## 19. Mount
A filesystem becomes accessible through the directory hierarchy when mounted, e.g. `mount /dev/sdb1 /mnt`:
```
/dev/sdb1 --> Filesystem --> super_block --> mount --> /mnt
```
Applications can then access files under `/mnt`.

## 20. Root Filesystem
Linux eventually needs a root filesystem, represented as `/`:
```
/ --- bin, etc, home, usr, var, tmp
```
The root filesystem provides the initial directory hierarchy.

## 21. Multiple Filesystems
Linux can combine multiple filesystems into one directory tree, e.g.:
```
/  --- home, var
   +-- proc --- procfs
   +-- sys  --- sysfs
```
Another example: `/dev/sda1 → /`, `/dev/sdb1 → /home`. The user still sees `/` with `home` under it — VFS hides the underlying filesystem boundaries.

---

## 22. Path Resolution
For `/home/user/notes.txt`, the kernel must resolve the path:
```
"/" --> "home" --> "user" --> "notes.txt" --> inode
```
This process is called **Pathname Resolution** or **Path Lookup**.

## 23. Path Lookup
High-level flow:
```
open("/home/user/notes.txt") --> VFS --> Resolve "/" --> Lookup "home"
--> Lookup "user" --> Lookup "notes.txt" --> inode
```
Dentries and the dentry cache play a major role in this process.

## 24. open() Flow
For `int fd = open("/home/user/notes.txt", O_RDONLY);`:
```
Application --> open() --> System Call --> VFS --> Path Resolution
--> dentry --> inode --> struct file --> File Descriptor Table --> fd
```
Finally, `fd = 3` may be returned to userspace.

## 25. What Does open() Actually Do?
Conceptually, `open()`:
1. Resolves the pathname
2. Finds the filesystem object
3. Performs permission/security checks
4. Creates/initializes an open-file object
5. Installs a file descriptor in the process
6. Returns the descriptor to userspace

It does **not** normally read the entire file into memory.

---

## 26. read() Flow
For `read(fd, buffer, 4096);`:
```
Application --> read() --> System Call --> File Descriptor --> struct file
--> VFS --> Page Cache/Filesystem --> Data --> User Buffer
```

## 27. Page Cache
Linux uses RAM to cache filesystem data — the **Page Cache**:
```
Application --> read() --> Page Cache --Hit--> Data
                                       --Miss--> Filesystem --> Storage
```

## 28. Page Cache Hit
If the required data already exists in memory: `read() --> Page Cache --> Data Found --> Return Data`. The storage device does not need to be accessed for that data.

## 29. Page Cache Miss
If the data is not cached: `read() --> Page Cache --> Miss --> Filesystem --> Block Layer --> Storage Driver --> SSD/HDD`. The required data is brought into memory.

## 30. Page Cache vs Buffer Cache
Historically Linux used the term **Buffer Cache** for caching filesystem/block-device metadata and blocks. Modern Linux has unified much of this through the **Page Cache**. For interview purposes: `File data → Page Cache` — don't treat the old "buffer cache" terminology as a completely separate modern file-data cache.

---

## 31. write() Flow
For `write(fd, buffer, 4096);`, simplified buffered-I/O flow:
```
Application --> write() --> VFS --> Page Cache --> Dirty Pages --> Writeback
--> Filesystem --> Block Layer --> Storage Driver --> SSD/HDD
```

## 32. Dirty Pages
When modified file data exists in memory but hasn't yet been written back — a **Dirty Page**:
```
Application --> Modify Data --> Page Cache --> Dirty --> Writeback --> Storage
```

## 33. Does write() Mean Data Is on Disk?
Not necessarily. A successful `write()` usually means the data has been accepted by the kernel/filesystem path — it may still be in memory as dirty cached data:
```
write() --> Page Cache --> Dirty --> Later Writeback --> Storage
```

## 34. fsync()
Applications can request stronger synchronization with storage using `fsync(fd);`:
```
Dirty Data --> fsync() --> Writeback/Flush --> Storage
```
Exact durability guarantees depend on the filesystem and storage stack.

## 35. close()
When `close(fd);` is called:
```
Application --> close(fd) --> File Descriptor Released --> Reference to struct file Released
```
When the relevant references are gone, the open-file object can be released. The inode and dentry may remain cached for later use.

---

## 36. File Descriptor vs struct file vs inode
One of the most important VFS interview questions. Remember the chain:
```
fd → struct file → path → dentry → inode
```
- **fd** → integer used by the process
- **struct file** → open-file instance
- **path** → mount + dentry
- **dentry** → pathname component/directory entry
- **inode** → underlying filesystem object

## 37. Directory Internals
A directory is itself a filesystem object:
```
Directory inode --> Directory Data
                      +-- file1 → inode X
                      +-- file2 → inode Y
                      +-- dir1  → inode Z
```
A directory associates names with filesystem objects.

## 38. Hard Link
Suppose `file1` and `file2` are hard links to the same inode:
```
file1 --+
        v
     inode 100
        ^
file2 --+
```
Both directory entries refer to the same inode; the inode's link count tracks the number of hard links.

## 39. Symbolic Link
A symbolic link is different, e.g. `link.txt → original.txt`:
```
link.txt --> symlink object --> "original.txt" --> target
```
Path resolution follows the symbolic link to its target.

---

## 40. VFS Operations
VFS uses operation tables to allow generic code to call filesystem-specific implementations. Important structures: `file_operations`, `inode_operations`, `super_operations`:
```
struct file --> file_operations
                  +-- read
                  +-- write
                  +-- ioctl
                  +-- mmap
```
The exact operations available depend on the object and implementation.

## 41. Why Function Pointers?
Linux is written largely in C, which does not provide C++-style classes and virtual functions. The kernel therefore uses structures containing function pointers to achieve a form of polymorphism:
```
VFS --> Operation Table --- ext4 implementation
                          --- XFS implementation
                          --- NFS implementation
```
This allows generic VFS code to operate with many different filesystem implementations.

## 42. VFS to ext4
Suppose the file resides on ext4 — a simplified read path:
```
Application --> read() --> System Call --> VFS --> struct file --> inode
--> ext4 --> Page Cache --> Block Layer --> Storage Driver --> SSD
```
VFS provides the generic layer; ext4 provides filesystem-specific behavior.

---

## 43. Disk vs VFS
Extremely important distinction.
**On Disk:** the filesystem stores persistent structures — superblock, filesystem metadata, directory information, inode information, file data.
**In Kernel Memory:** Linux maintains in-memory structures — `super_block`, dentry cache, inode cache, page cache, mount structures, `struct file`.

The kernel uses these structures while the filesystem is mounted and files are accessed.

## 44. Example: notes.txt on Disk
```
DISK
Filesystem --> Root Directory --> notes.txt --> inode 125 --> Data Blocks
```
The directory maps `notes.txt → inode 125`.

## 45. After Mount
The kernel builds/maintains in-memory representations:
```
RAM
super_block --> root dentry --> root inode
```
When the kernel performs `lookup("notes.txt")`, it can reach `notes.txt --> dentry --> inode 125`.

## 46. After open()
For `fd = open("/notes.txt", O_RDONLY);`:
```
Process --> fd = 3 --> struct file --> path --> dentry --> inode 125
```

---

## 47. Complete VFS Mental Model
Memorize this:
```
                     PROCESS
                        |
                File Descriptor
                        |
                   struct file
                        |
                      path
                    /      \
                dentry      mount
                  |            |
                inode     super_block
                  |
              Filesystem
                  |
              Block Layer
                  |
             Device Driver
                  |
               Storage
```

## 48. Complete read() Mental Model
```
Application --> read(fd) --> System Call --> VFS --> struct file --> Page Cache
                                                          +-- Hit --> Data
                                                          +-- Miss --> Filesystem
                                                                          --> Block Layer
                                                                          --> Device Driver
                                                                          --> Storage
                                                                          --> Page Cache
                                                                          --> User Buffer
```

## 49. Complete write() Mental Model
```
Application --> write(fd) --> System Call --> VFS --> Page Cache --> Dirty Pages
--> Writeback --> Filesystem --> Block Layer --> Device Driver --> Storage
```

---

## 50. Deleted File Still Open
A classic Linux interview question. Suppose Process A has `fd 3 --> struct file --> inode`. Another process runs `rm file.txt` — the directory entry for `file.txt` is removed. But the open file can remain accessible because the process still holds a reference to the open object/inode:
```
Directory name --X--> removed
Process fd --> struct file --> inode (still exists)
```
The underlying storage is reclaimed only after the relevant references are gone and filesystem rules allow reclamation.

## 51. Why Can a Deleted File Consume Disk Space?
A process opens `large.log` (`fd 3`). Another process runs `rm large.log` — the directory entry disappears, but the process still has the file open. So: filename → gone, open file → still exists. The file's blocks may continue consuming disk space until the final relevant reference is released. This is a common production debugging problem.

---

## 52. Important Interview Questions

**Q1. Why do we need VFS?**
Because Linux supports many filesystems while applications need a common filesystem API.

**Q2. What is an inode?**
An inode represents a filesystem object and contains metadata and information used to locate/access its data.

**Q3. Does an inode contain the filename?**
No — the name is associated with directory entries/dentries: `name → dentry → inode`.

**Q4. What is a dentry?**
A dentry represents a pathname component/directory entry and participates in mapping names to filesystem objects.

**Q5. What is `struct file`?**
It represents an open file instance, containing state associated with that open instance, such as file position and operation information.

**Q6. What is a file descriptor?**
A small integer used by a process to refer to an open file object.

**Q7. Explain `fd → file → dentry → inode`.**
`fd` indexes the process FD table; `struct file` represents the open instance; `dentry` represents the pathname component; `inode` represents the filesystem object.

**Q8. Does `open()` read the file?**
Normally no — it establishes the open-file context and returns a file descriptor. Actual data access happens through operations such as `read()`/`mmap()`.

**Q9. Does `read()` always access disk?**
No — the data may already be present in the page cache: `read() --> Page Cache --Hit--> Data`, `--Miss--> Storage`.

**Q10. Does `write()` immediately write to disk?**
Not necessarily — buffered writes commonly modify cached pages first; writeback later sends dirty data toward storage.

**Q11. What does `fsync()` do?**
It requests synchronization of modified file data and associated filesystem state according to the filesystem/storage semantics.

**Q12. What happens when a file is deleted while open?**
The directory name is removed, but the open file can remain accessible through existing references. The underlying storage is reclaimed only after the relevant references are released.

**Q13. What is the difference between hard link and symbolic link?**
Hard link: `name1 --> inode <-- name2` (both names point directly to the same inode). Symbolic link: `link --> target pathname --> target object` (the link points to a pathname, which is then resolved separately).

---

## 53. Senior Interview Questions
You should be able to explain these on a whiteboard:
1. What happens when `open("/home/user/file.txt", O_RDONLY);` is called?
2. Explain `fd → struct file → path → dentry → inode`.
3. What happens during `read(fd, buffer, 4096);`?
4. What happens on a page-cache miss?
5. What happens during `write(fd, buffer, 4096);`?
6. Why can `rm file` succeed while a process still reads the file?
7. How does VFS support ext4, XFS, NFS, etc.?
8. What is the difference between `super_block`, `inode`, `dentry`, and `file`?
9. Why does Linux cache dentries?
10. Why doesn't `write()` necessarily mean data is physically persistent?

---

## 54. Must-Remember Diagram
For senior Linux interviews, memorize this relationship:
```
Process --> FD Table --> fd --> struct file --> struct path
                                                    +--> dentry
                                                    +--> mount --> super_block
dentry --> inode --> Filesystem --> Block Layer --> Device Driver --> Storage
```

---

## 55. Final Summary
VFS is the Linux kernel's generic filesystem abstraction layer. The most important structures: `super_block`, `inode`, `dentry`, `file`, `path`. Remember their roles:
- **super_block** → filesystem instance
- **inode** → filesystem object
- **dentry** → pathname component / name lookup
- **struct file** → open file instance
- **path** → mount + dentry
- **fd** → userspace integer referring to an open file

The most important chain:
```
fd --> struct file --> path --> dentry --> inode --> filesystem --> block layer --> device driver --> storage
```

And the most important data-access path:
```
Application --> read() --> VFS --> Page Cache --Hit--> Data
                                              --Miss--> Filesystem --> Block Layer --> Driver --> Storage
```

If you understand these flows deeply, you have the foundation needed to answer a large class of senior Linux Systems, Embedded, Storage, and Infrastructure interview questions.
