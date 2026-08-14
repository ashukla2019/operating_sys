# Chapter 11 — Files + I/O

> **Three-layer approach**
>
> This chapter covers:
> 1. **[OS] File and I/O fundamentals**
> 2. **[LSP] Linux System Programming + C code**
> 3. **[KERNEL] Linux Kernel Internals**
>
> The key flow:
>
> ```text
> Application
>     ↓
> open()/read()/write()
>     ↓
> System call
>     ↓
> VFS
>     ↓
> filesystem
>     ↓
> page cache / block layer
>     ↓
> storage device
> ```

---

# 1. What Is a File?

From an OS perspective, a file is a persistent named object used to store data.

Linux follows the Unix philosophy:

> **Many resources are accessed through file descriptors.**

Examples:

```text
regular file
directory
pipe
FIFO
socket
device
terminal
```

This is why Linux system programming frequently revolves around:

```text
open
read
write
close
```

---

# 2. File Types in Linux

Common file types:

```text
-  regular file
d  directory
l  symbolic link
c  character device
b  block device
p  FIFO
s  socket
```

Check:

```bash
ls -l
```

Example:

```text
-rw-r--r--  file.txt
drwxr-xr-x  directory/
lrwxrwxrwx  link -> target
```

---

# 3. File Descriptor

A file descriptor (FD) is a small integer used by a process to refer to an open resource.

Standard descriptors:

```text
0 → stdin
1 → stdout
2 → stderr
```

Example:

```c
int fd = open("data.txt", O_RDONLY);
```

Suppose:

```text
fd = 3
```

The process can then use:

```c
read(3, ...);
close(3);
```

---

# 4. File Descriptor Table

Conceptually:

```text
Process
   |
   v
File Descriptor Table
   |
   +-- 0 → stdin
   +-- 1 → stdout
   +-- 2 → stderr
   +-- 3 → file
   +-- 4 → socket
   +-- 5 → pipe
```

The descriptor itself is only an integer.

The kernel maintains the actual object/state behind it.

---

# 5. Important Kernel Relationship

A useful simplified model is:

```text
process
   |
   v
fd table
   |
   v
struct file
   |
   +---- f_op
   +---- f_inode
   +---- f_pos
   +---- f_flags
   +---- f_mode
   +---- f_path
   +---- private_data
   |
   v
underlying object
```

For a regular filesystem file, the path eventually relates to:

```text
dentry
inode
superblock
filesystem
```

Exact internal structures can vary across Linux versions.

---

# 6. `open()`

Basic API:

```c
int fd = open("data.txt", O_RDONLY);
```

Header:

```c
#include <fcntl.h>
```

On success:

```text
return → file descriptor
```

On failure:

```text
return → -1
errno → reason
```

---

# 7. Basic `open()` Example

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd = open("data.txt", O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    printf("fd = %d\n", fd);

    close(fd);

    return 0;
}
```

Compile:

```bash
gcc open.c -o open
```

---

# 8. Common `open()` Flags

Read only:

```c
O_RDONLY
```

Write only:

```c
O_WRONLY
```

Read + write:

```c
O_RDWR
```

Create:

```c
O_CREAT
```

Truncate:

```c
O_TRUNC
```

Append:

```c
O_APPEND
```

Non-blocking:

```c
O_NONBLOCK
```

Close-on-exec:

```c
O_CLOEXEC
```

Example:

```c
open("data.txt",
     O_WRONLY | O_CREAT | O_TRUNC,
     0644);
```

The third argument is needed when `O_CREAT` is used.

---

# 9. File Permissions

Example:

```text
0644
```

means:

```text
owner  → rw-
group  → r--
others → r--
```

Conceptually:

```text
6 = rw-
4 = r--
4 = r--
```

Common values:

```text
0600 → owner read/write
0644 → owner read/write, others read
0666 → read/write for all, subject to umask
0755 → executable/readable directory or program
```

Actual creation permissions are affected by the process `umask`.

---

# 10. `read()`

Basic form:

```c
ssize_t n = read(fd, buffer, size);
```

Returns:

```text
> 0 → bytes read
  0 → EOF
< 0 → error
```

Important:

> `read()` is allowed to return fewer bytes than requested.

---

# 11. Basic `read()` Example

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd = open("data.txt", O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    char buffer[128];

    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);

    if (n == -1)
    {
        perror("read");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';

    printf("%s\n", buffer);

    close(fd);

    return 0;
}
```

---

# 12. Why `read()` May Return Less

Suppose:

```c
read(fd, buffer, 1000);
```

It does not guarantee:

```text
1000 bytes
```

Possible result:

```text
500 bytes
```

Reasons depend on the underlying object and conditions.

For streams:

```text
read()
```

usually returns whatever data is currently available according to the object's semantics.

Therefore robust code must handle partial reads.

---

# 13. `write()`

Basic form:

```c
ssize_t n = write(fd, buffer, size);
```

Returns:

```text
> 0 → bytes written
< 0 → error
```

It can also write fewer bytes than requested.

Therefore robust code may need a loop.

---

# 14. Reliable Write Helper

```c
#include <unistd.h>
#include <errno.h>

ssize_t write_all(int fd,
                  const void *buf,
                  size_t count)
{
    const char *p = buf;
    size_t total = 0;

    while (total < count)
    {
        ssize_t n = write(fd,
                          p + total,
                          count - total);

        if (n > 0)
        {
            total += n;
            continue;
        }

        if (n == -1 && errno == EINTR)
            continue;

        return -1;
    }

    return (ssize_t)total;
}
```

This is a useful interview pattern.

---

# 15. `close()`

Close an FD:

```c
close(fd);
```

After successful close:

```text
fd is no longer valid for that process
```

Always close descriptors that are no longer needed.

Descriptor leaks can cause:

```text
resource exhaustion
unexpected EOF behavior
too many open files
```

---

# 16. `lseek()`

`lseek()` changes the current file offset.

```c
off_t pos = lseek(fd, offset, SEEK_SET);
```

Common modes:

```text
SEEK_SET
SEEK_CUR
SEEK_END
```

Example:

```c
lseek(fd, 0, SEEK_SET);
```

moves to the beginning.

---

# 17. File Offset

Conceptually:

```text
struct file
     |
     +-- f_pos
```

Example:

```text
Initial:
f_pos = 0

read(100):
f_pos = 100

read(50):
f_pos = 150
```

The exact behavior for special files differs.

---

# 18. `lseek()` Example

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd = open("data.txt", O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    off_t pos = lseek(fd, 0, SEEK_END);

    if (pos == (off_t)-1)
    {
        perror("lseek");
        close(fd);
        return 1;
    }

    printf("File size = %lld bytes\n",
           (long long)pos);

    close(fd);

    return 0;
}
```

For regular files, this is commonly used to determine the current end offset, but metadata APIs such as `stat()` are often more appropriate for file size.

---

# 19. `pread()` and `pwrite()`

`pread()` reads from a specified offset without changing the file offset.

```c
pread(fd, buffer, size, offset);
```

`pwrite()` writes at a specified offset without changing the current file offset.

Useful for:

```text
concurrent access
random access
thread-safe positioning patterns
```

---

# 20. `read()` vs `pread()`

```text
read()
  ↓
uses current f_pos
  ↓
updates f_pos
```

```text
pread()
  ↓
uses explicit offset
  ↓
does not update f_pos
```

This distinction is important in multithreaded/file-I/O designs.

---

# 21. `stat()`

`stat()` obtains file metadata.

```c
struct stat st;

stat("data.txt", &st);
```

Useful fields:

```text
st_size
st_mode
st_uid
st_gid
st_nlink
st_mtime
```

---

# 22. `stat()` Example

```c
#include <stdio.h>
#include <sys/stat.h>

int main(void)
{
    struct stat st;

    if (stat("data.txt", &st) == -1)
    {
        perror("stat");
        return 1;
    }

    printf("size = %lld\n",
           (long long)st.st_size);

    printf("mode = %o\n",
           st.st_mode);

    printf("links = %lu\n",
           (unsigned long)st.st_nlink);

    return 0;
}
```

---

# 23. `fstat()`

If you already have an FD:

```c
fstat(fd, &st);
```

This avoids doing a separate pathname lookup for the object.

Typical variants:

```text
stat()
lstat()
fstat()
```

`lstat()` reports information about the symbolic link itself rather than following it.

---

# 24. Directory Operations

Directories are special filesystem objects.

Common APIs:

```text
opendir()
readdir()
closedir()
```

Example:

```c
DIR *dir = opendir(".");
```

Then:

```c
struct dirent *entry;

while ((entry = readdir(dir)) != NULL)
{
    printf("%s\n", entry->d_name);
}
```

---

# 25. Directory Example

```c
#include <stdio.h>
#include <dirent.h>

int main(void)
{
    DIR *dir = opendir(".");

    if (dir == NULL)
    {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        printf("%s\n", entry->d_name);
    }

    closedir(dir);

    return 0;
}
```

---

# 26. Hard Link

A hard link is another directory entry referring to the same inode.

Conceptually:

```text
file1
  |
  v
inode 100

file2
  |
  v
inode 100
```

Both names refer to the same underlying file object.

Create:

```bash
ln file1 file2
```

---

# 27. Symbolic Link

A symbolic link stores a pathname/reference to another path.

```text
link
 |
 v
target
 |
 v
inode
```

Create:

```bash
ln -s target link
```

Important distinction:

```text
hard link → same inode
symlink   → separate inode containing link information
```

---

# 28. `unlink()`

Remove a directory entry:

```c
unlink("file.txt");
```

Important:

> `unlink()` removes a name, not necessarily the underlying storage immediately.

If an open file still has references:

```text
directory entry removed
        |
        v
file remains accessible through open FD
        |
        v
storage reclaimed after references are gone
```

This is a very important Unix/Linux behavior.

---

# 29. Rename

```c
rename("old.txt", "new.txt");
```

Within the same filesystem, rename is generally an atomic namespace operation under normal filesystem semantics.

This is widely used for safe update patterns:

```text
write temporary file
      ↓
fsync if required
      ↓
rename temporary → final
```

Durability has additional filesystem/storage considerations.

---

# 30. `fsync()` / `fdatasync()`

Buffered I/O does not necessarily mean data is immediately persistent on physical storage.

```c
fsync(fd);
```

requests synchronization of file data and relevant metadata.

```c
fdatasync(fd);
```

focuses primarily on data and metadata required for subsequent data retrieval.

Use these when application durability requirements demand them.

---

# 31. `sync()`

`sync()` requests writeback of filesystem data.

It operates at a broader scope than `fsync(fd)`.

Do not confuse:

```text
write()
```

with:

```text
durably stored on stable media
```

The exact durability guarantees depend on filesystem, storage hardware, cache behavior, and synchronization operations.

---

# 32. Buffered I/O vs Direct I/O

Normal file I/O commonly interacts with the Linux page cache:

```text
Application
    |
 read/write
    |
    v
Page Cache
    |
    v
Filesystem
    |
    v
Block Layer
```

With:

```text
O_DIRECT
```

applications request direct I/O semantics intended to minimize normal page-cache involvement.

`O_DIRECT` has alignment and I/O constraints and should not be assumed to be universally faster.

---

# 33. Page Cache

The page cache caches file-backed data in memory.

Conceptually:

```text
Application
    |
    v
page cache
    |
    v
filesystem
    |
    v
block layer
    |
    v
disk
```

Read:

```text
read()
  ↓
page cache hit?
  |
  +-- yes → copy/access cached data
  |
  +-- no  → filesystem/storage I/O
```

The exact path varies by I/O mode and filesystem.

---

# 34. Write Path

For normal buffered writes:

```text
write()
   ↓
page cache / filesystem
   ↓
dirty pages
   ↓
writeback
   ↓
filesystem/block layer
   ↓
storage
```

`write()` returning successfully does not by itself mean the data has reached stable storage.

---

# 35. Read Path

Simplified:

```text
read()
  ↓
VFS
  ↓
filesystem
  ↓
page cache
  ↓
cache hit?
 /       \
yes       no
 |         |
data      storage I/O
 |         |
 +----+----+
      |
      v
application
```

---

# 36. VFS

VFS = **Virtual File System**.

It provides a common interface between system calls and different filesystems.

Conceptually:

```text
Application
     |
     v
System calls
     |
     v
VFS
     |
 +---+---+---+
 |   |   |   |
ext4 xfs tmpfs ...
```

The application does not need a different `read()` system call for each filesystem.

---

# 37. VFS Main Objects

Important concepts:

```text
superblock
inode
dentry
file
```

Think:

```text
superblock → filesystem instance
inode      → object metadata
dentry     → pathname/name component
file       → open-file state
```

---

# 38. Path Lookup

For:

```text
open("/home/user/data.txt")
```

conceptually:

```text
/
 ↓
home
 ↓
user
 ↓
data.txt
```

VFS performs pathname lookup using directory entries and inode information.

The lookup uses:

```text
dentry cache
inode information
mount information
filesystem operations
```

---

# 39. Dentry

A dentry represents a directory entry/name relationship in the VFS.

Conceptually:

```text
"name"
  |
  v
dentry
  |
  v
inode
```

Dentries are important for pathname lookup and caching.

---

# 40. Inode

The inode represents filesystem object metadata.

Typical information includes:

```text
file type
permissions
owner
timestamps
size
link count
filesystem-specific metadata
```

A simplified mental model:

```text
directory entry
      |
      v
    inode
      |
      +-- metadata
      +-- file data mapping
```

The exact data mapping mechanism depends on filesystem and kernel implementation.

---

# 41. Superblock

The superblock represents information about a mounted filesystem instance.

Conceptually:

```text
mounted filesystem
       |
       v
   superblock
       |
       +-- filesystem information
       +-- filesystem operations
       +-- mount-related state
```

Examples:

```text
ext4
xfs
tmpfs
```

---

# 42. `struct file`

An open file is represented in the kernel by a `struct file`.

Important conceptual fields:

```text
f_op
f_inode
f_pos
f_flags
f_mode
f_path
private_data
```

Meaning:

```text
f_op         → file operations
f_inode      → associated inode
f_pos        → current file position
f_flags      → open flags
f_mode       → access mode
f_path       → path information
private_data → driver/filesystem-specific state
```

---

# 43. `file_operations`

A filesystem or driver exposes operations through function pointers.

Conceptually:

```text
struct file_operations
{
    read
    write
    open
    release
    poll
    unlocked_ioctl
    mmap
    ...
}
```

Modern kernels use specific operation signatures and names that can differ from simplified textbook examples.

The important concept is:

```text
VFS
 ↓
file_operations
 ↓
filesystem/driver implementation
```

---

# 44. Open Flow

For:

```c
fd = open("data.txt", O_RDONLY);
```

simplified:

```text
user application
      |
      v
open()
      |
      v
system call entry
      |
      v
VFS/path lookup
      |
      v
dentry + inode
      |
      v
filesystem
      |
      v
struct file
      |
      v
fd allocated in process fd table
      |
      v
return fd
```

---

# 45. Read Flow

For:

```c
read(fd, buf, size);
```

simplified:

```text
application
    |
    v
read()
    |
    v
system call
    |
    v
fd table
    |
    v
struct file
    |
    v
file_operations
    |
    v
filesystem
    |
    v
page cache / storage
    |
    v
data returned to user
```

---

# 46. Write Flow

For:

```c
write(fd, buf, size);
```

simplified:

```text
application buffer
       |
       v
write()
       |
       v
VFS
       |
       v
filesystem
       |
       v
page cache / filesystem state
       |
       v
writeback
       |
       v
block layer
       |
       v
storage
```

---

# 47. Block Layer

The Linux block layer sits between filesystems and block devices.

Conceptually:

```text
Filesystem
    ↓
Block layer
    ↓
Block device driver
    ↓
Storage device
```

The block layer handles I/O request management and interfaces with block-device drivers.

Modern Linux block I/O has evolved significantly; `blk-mq` is a key modern architecture for scalable block I/O.

---

# 48. Character vs Block Devices

Character device:

```text
stream-oriented
```

Examples:

```text
terminal
serial device
```

Block device:

```text
block-oriented storage
```

Examples:

```text
SSD
HDD
NVMe block device
```

Device drivers expose operations through kernel interfaces that can integrate with VFS.

---

# 49. `ioctl()`

`ioctl()` provides device/filesystem-specific control operations.

Example:

```c
ioctl(fd, request, arg);
```

It is common for:

```text
device drivers
terminal configuration
special filesystem/device operations
```

Unlike `read()` and `write()`, the operation is request-specific.

---

# 50. `fcntl()`

`fcntl()` performs various descriptor operations.

Examples:

```text
duplicate FD
get/set descriptor flags
get/set file status flags
record locks
```

Example:

```c
int flags = fcntl(fd, F_GETFL);

fcntl(fd, F_SETFL, flags | O_NONBLOCK);
```

---

# 51. Blocking I/O

Blocking call:

```text
read()
  |
  v
data unavailable
  |
  v
process sleeps
  |
  v
data becomes available
  |
  v
process wakes
```

This is efficient when a process can afford to wait.

---

# 52. Non-Blocking I/O

Open with:

```c
O_NONBLOCK
```

or use `fcntl()`.

Conceptually:

```text
read()
  ↓
data unavailable
  ↓
return immediately
```

Often:

```text
-1
errno = EAGAIN/EWOULDBLOCK
```

depending on the object and operation.

---

# 53. Blocking vs Non-Blocking

```text
Blocking
    |
    +-- simple
    +-- process can sleep
    +-- natural for synchronous programs

Non-blocking
    |
    +-- returns without waiting
    +-- useful in event-driven designs
    +-- usually combined with poll/epoll
```

---

# 54. I/O Multiplexing

Problem:

```text
10,000 connections
```

We do not want:

```text
one blocking read per connection
```

or necessarily:

```text
one thread per connection
```

I/O multiplexing allows one thread/process to monitor many descriptors.

Main APIs:

```text
select()
poll()
epoll()
```

---

# 55. `select()`

Conceptually:

```c
select(maxfd + 1,
       &readfds,
       &writefds,
       NULL,
       &timeout);
```

It waits until descriptors are ready.

Limitations include:

```text
fd_set size limits
repeatedly rebuilding/copying descriptor sets
O(n)-style scanning
```

It remains important for understanding classic Unix I/O multiplexing.

---

# 56. `poll()`

Example:

```c
struct pollfd pfd;

pfd.fd = fd;
pfd.events = POLLIN;
pfd.revents = 0;

int ret = poll(&pfd, 1, 5000);
```

Check:

```c
if (pfd.revents & POLLIN)
{
    // readable
}
```

`poll()` avoids the fixed `fd_set` limitation of traditional `select()`, but still involves scanning the supplied descriptor array.

---

# 57. `epoll()`

Linux-specific scalable event notification API.

Basic flow:

```text
epoll_create1()
       ↓
epoll_ctl()
       ↓
epoll_wait()
```

Example:

```c
int epfd = epoll_create1(0);

struct epoll_event ev = {0};

ev.events = EPOLLIN;
ev.data.fd = fd;

epoll_ctl(epfd,
          EPOLL_CTL_ADD,
          fd,
          &ev);

struct epoll_event events[10];

int n = epoll_wait(epfd,
                   events,
                   10,
                   -1);
```

---

# 58. `epoll` Mental Model

```text
             +-- fd 3
             |
epoll set <--+-- fd 4
             |
             +-- fd 5
             |
             +-- fd 6

             ↓

        epoll_wait()

             ↓

only ready events returned
```

This avoids repeatedly asking about every descriptor in the same way as a simple O(n) scan.

---

# 59. Level-Triggered vs Edge-Triggered

`epoll` supports:

```text
Level-triggered
Edge-triggered
```

### Level-triggered

If the FD remains ready:

```text
event can continue to be reported
```

### Edge-triggered

Reports transitions/changes in readiness.

With edge-triggered mode:

```text
read/write until EAGAIN
```

is a common pattern.

---

# 60. Edge-Triggered Example Concept

```text
data arrives
   ↓
EPOLLIN
   ↓
read()
   ↓
read()
   ↓
read()
   ↓
EAGAIN
```

Do not stop after reading only one chunk if the design requires draining the descriptor before waiting again.

---

# 61. `O_NONBLOCK` + `epoll`

A common event-driven combination:

```text
open/socket
      |
      v
O_NONBLOCK
      |
      v
epoll
      |
      v
read/write until EAGAIN
```

This avoids blocking the entire event loop on one descriptor.

---

# 62. `mmap()`

`mmap()` maps files or anonymous memory into a process virtual address space.

For a file:

```c
void *addr = mmap(NULL,
                  length,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED,
                  fd,
                  0);
```

Conceptually:

```text
file
  |
  v
page cache / filesystem
  |
  v
virtual memory mapping
  |
  v
process address space
```

---

# 63. `mmap()` vs `read()`

`read()`:

```text
file
 ↓
kernel memory/page cache
 ↓
copy/access into user buffer
```

`mmap()`:

```text
file-backed pages
 ↓
mapped into process virtual address space
```

`mmap()` can be useful for:

```text
random access
large files
shared mappings
memory-oriented access
```

But it has its own page-fault and synchronization behavior.

---

# 64. Memory-Mapped File

```text
Process A
+----------------------+
| virtual address      |
|      mapping         |
+----------+-----------+
           |
           v
      file-backed
        pages
           ^
           |
+----------+-----------+
| filesystem/storage   |
+----------------------+
```

With `MAP_SHARED`, modifications can be shared through the mapped file according to the mapping and filesystem semantics.

---

# 65. `MAP_PRIVATE`

`MAP_PRIVATE` creates a private copy-on-write mapping.

Conceptually:

```text
initial:
Process → shared file-backed page

write:
       ↓
copy-on-write
       ↓
private modified page
```

Changes are not written back to the underlying file as normal shared modifications.

---

# 66. `MAP_SHARED`

```text
Process A ──┐
            |
            v
      shared mapping
            ^
            |
Process B ──┘
```

Useful for:

```text
shared memory
memory-mapped files
inter-process communication
```

Synchronization is still required when multiple processes modify shared state concurrently.

---

# 67. Page Fault and File I/O

With `mmap()`:

```text
application accesses address
        |
        v
page present?
      /   \
    yes    no
    |       |
 continue   page fault
              |
              v
      filesystem/page cache
              |
              v
          page mapped
              |
              v
          execution resumes
```

This connects:

```text
Files
+
Virtual Memory
+
Page Cache
```

---

# 68. Direct I/O

`O_DIRECT` attempts to minimize the normal page-cache path.

Conceptually:

```text
Application
    |
    v
O_DIRECT I/O
    |
    v
filesystem/block layer
    |
    v
storage
```

Important:

```text
alignment constraints
buffer constraints
filesystem/device behavior
```

must be considered.

Do not use `O_DIRECT` simply because it "sounds faster."

---

# 69. `sendfile()`

`sendfile()` can transfer data between file descriptors efficiently for supported cases.

Conceptually:

```text
file
  |
  v
kernel
  |
  v
socket
```

This can reduce unnecessary user-space copying.

It is useful in high-performance file-serving paths.

---

# 70. `splice()`

`splice()` can move data between file descriptors and pipes without requiring a normal user-space data copy for supported paths.

Conceptually:

```text
FD
 |
 v
pipe
 |
 v
FD
```

It is part of Linux's zero-copy-oriented I/O facilities.

"Zero-copy" here should be understood as avoiding particular copies, not as guaranteeing that no memory movement occurs anywhere in the hardware/software stack.

---

# 71. `copy_file_range()`

Linux provides:

```c
copy_file_range()
```

for kernel-assisted copying between file descriptors in supported cases.

It can allow the filesystem to optimize the copy.

Actual behavior depends on filesystem and kernel support.

---

# 72. File Locks

Linux provides several locking mechanisms.

Common concepts:

```text
advisory locking
record locking
flock()
fcntl()
```

Advisory locks require participating processes to honor the locking protocol.

They do not automatically prevent all access by every program.

---

# 73. `flock()`

Example:

```c
#include <sys/file.h>

if (flock(fd, LOCK_EX) == -1)
{
    perror("flock");
}
```

Release:

```c
flock(fd, LOCK_UN);
```

Useful for simple whole-file advisory locking patterns.

---

# 74. `fcntl()` Record Locks

`fcntl()` can implement byte-range advisory locks.

Conceptually:

```text
file
+--------------------------------+
| locked |       free            |
+--------------------------------+
   ^
   |
 byte range
```

This is useful when independent regions of a file need separate locking.

---

# 75. Atomic Append

Opening with:

```c
O_APPEND
```

causes the file offset to be positioned at the end for each write operation according to the kernel's append semantics.

Useful for:

```text
log files
multiple writers
```

But application-level record integrity still needs careful design.

---

# 76. `O_CLOEXEC`

Prefer close-on-exec semantics when a descriptor should not leak into an executed child.

Example:

```c
int fd = open("data.txt",
              O_RDONLY | O_CLOEXEC);
```

This is preferable to relying on a separate `fcntl()` call when an atomic creation flag is available because it avoids a race in multithreaded programs.

---

# 77. File Descriptor Leaks

Suppose a server repeatedly opens files:

```text
open()
open()
open()
...
```

without:

```text
close()
```

Eventually:

```text
EMFILE
```

may occur when the process reaches its descriptor limit.

Check:

```bash
ulimit -n
```

Inspect:

```bash
ls -l /proc/<pid>/fd
```

---

# 78. `ulimit` and File Descriptors

Check current limit:

```bash
ulimit -n
```

System-wide limits also exist.

Useful for diagnosing:

```text
too many open files
high-connection servers
descriptor leaks
```

---

# 79. `/proc/<pid>/fd`

Example:

```bash
ls -l /proc/1234/fd
```

You may see:

```text
0 -> /dev/pts/0
1 -> /dev/pts/0
2 -> /dev/pts/0
3 -> /home/user/data.txt
4 -> socket:[12345]
5 -> pipe:[67890]
```

This is one of the best ways to understand what a process currently has open.

---

# 80. `lsof`

Useful command:

```bash
lsof -p <pid>
```

It can show:

```text
files
sockets
pipes
devices
cwd
executables
```

For system debugging, combine:

```text
lsof
/proc/<pid>/fd
strace
```

---

# 81. `strace` File-I/O Debugging

Run:

```bash
strace -f ./program
```

You may see:

```text
openat(...)
read(...)
write(...)
lseek(...)
close(...)
mmap(...)
```

For a specific program:

```bash
strace -e trace=file ./program
```

This is very useful for diagnosing pathname and file-descriptor problems.

---

# 82. `strace` I/O Example

Conceptually:

```text
openat(...)
   ↓
fd = 3

read(3,...)
   ↓
data

close(3)
```

This lets you compare:

```text
application expectation
```

with:

```text
actual system calls
```

---

# 83. VFS Mount Concept

Before accessing a filesystem:

```text
mkfs.ext4 /dev/sdb1
```

creates filesystem structures on the block device.

Then:

```bash
mount /dev/sdb1 /mnt/data
```

conceptually creates:

```text
/dev/sdb1
    |
    v
ext4 filesystem
    |
    v
mounted at /mnt/data
```

Path lookup can then traverse into the mounted filesystem.

---

# 84. Filesystem Mount Mental Model

```text
/dev/sdb1
    |
    v
filesystem
    |
    v
superblock
    |
    v
mount
    |
    v
/mnt/data
```

VFS provides the common mount/path abstraction.

---

# 85. Full File Open Mental Model

For:

```c
open("/mnt/data/file.txt", O_RDONLY);
```

think:

```text
Application
    |
    v
open()
    |
    v
system call
    |
    v
VFS
    |
    v
path lookup
    |
    +--> /
    |
    +--> mnt
    |
    +--> data
    |
    +--> file.txt
    |
    v
dentry/inode
    |
    v
filesystem
    |
    v
struct file
    |
    v
FD table
    |
    v
fd = 3
```

---

# 86. Full Read Mental Model

```text
read(fd, buf, size)
        |
        v
process FD table
        |
        v
struct file
        |
        v
f_op
        |
        v
filesystem operation
        |
        v
page cache
        |
        +-- hit → data available
        |
        +-- miss
              |
              v
         block I/O
              |
              v
          storage
              |
              v
        page cache
              |
              v
          userspace
```

This is a key interview flow.

---

# 87. Full Write Mental Model

```text
write(fd, buf, size)
        |
        v
system call
        |
        v
VFS
        |
        v
filesystem
        |
        v
page cache
        |
        v
dirty page
        |
        v
writeback
        |
        v
block layer
        |
        v
device driver
        |
        v
storage
```

For direct I/O, the path differs.

---

# 88. File I/O and Kernel Scheduling

Blocking file I/O can cause a task to sleep while waiting for I/O completion.

Conceptually:

```text
task running
    |
    v
I/O request
    |
    v
wait
    |
    v
task sleeps
    |
    v
scheduler runs another task
    |
    v
I/O completes
    |
    v
task becomes runnable
    |
    v
scheduler runs task
```

This connects file I/O with:

```text
processes
scheduler
wait queues
block layer
interrupts
```

---

# 89. I/O Completion

A simplified storage path:

```text
application
    |
    v
filesystem
    |
    v
block layer
    |
    v
device driver
    |
    v
storage device
    |
    v
completion
    |
    v
kernel wakes waiting task
```

Modern Linux storage paths may involve:

```text
blk-mq
interrupts
polling
DMA
NVMe queues
```

depending on hardware and configuration.

---

# 90. DMA

DMA = Direct Memory Access.

A device can transfer data to/from memory without the CPU copying every byte itself.

Conceptually:

```text
Storage device
      |
      | DMA
      v
RAM
```

The CPU still sets up and manages the operation, and the IOMMU may participate in address translation/protection.

---

# 91. I/O Buffering

There can be multiple layers:

```text
Application buffer
       ↓
Kernel/page cache
       ↓
Filesystem
       ↓
Block layer
       ↓
Device/controller cache
       ↓
Storage media
```

Therefore:

```text
write() returned
```

does not necessarily imply:

```text
data is physically persistent
```

Durability requirements determine which synchronization mechanisms are needed.

---

# 92. Important Distinction — `write()` vs `fsync()`

```text
write()
   ↓
accept data into kernel/filesystem path
```

versus:

```text
fsync()
   ↓
request synchronization of file state
```

For a database or transactional application, this distinction is critical.

---

# 93. I/O Models

Common Linux I/O approaches:

```text
blocking I/O
non-blocking I/O
I/O multiplexing
signal-driven I/O
memory-mapped I/O
asynchronous I/O
```

Modern Linux also has:

```text
io_uring
```

which provides an advanced asynchronous I/O interface.

---

# 94. `io_uring` — High-Level View

Conceptually:

```text
Application
     |
     v
submission queue
     |
     v
kernel
     |
     v
I/O
     |
     v
completion queue
     |
     v
Application
```

It is designed to reduce overhead and support efficient asynchronous I/O.

For senior Linux interviews, understand the concept even if you do not use it daily.

---

# 95. `select` vs `poll` vs `epoll`

| Feature | select | poll | epoll |
|---|---|---|---|
| Linux | Yes | Yes | Yes |
| FD set limit | Traditional `FD_SETSIZE` limitation | No fixed bitset limit | No equivalent `fd_set` limit |
| Typical monitoring | O(n) scan | O(n) scan | Ready-event oriented |
| Registration | Each call | Each call | Persistent interest list |
| Scalable high FD count | Poorer | Better interface, still scan-oriented | Usually preferred |
| Linux-specific | No | No | Yes |

Do not reduce this to:

```text
epoll = always faster
```

Performance depends on workload and implementation.

---

# 96. Readiness vs Completion

Important interview distinction.

### Readiness

```text
epoll
poll
select
```

tell you:

```text
operation should be able to proceed without blocking
```

### Completion

An asynchronous API may instead tell you:

```text
operation has completed
```

This distinction is useful when comparing:

```text
epoll
io_uring
AIO-style designs
```

---

# 97. File I/O Interview Questions

### Q1. What is a file descriptor?

An integer handle used by a process to refer to an open kernel resource.

---

### Q2. What happens during `open()`?

Simplified:

```text
pathname
 ↓
VFS/path lookup
 ↓
dentry/inode
 ↓
filesystem
 ↓
struct file
 ↓
FD table
 ↓
fd returned
```

---

### Q3. What is the difference between inode and dentry?

```text
dentry → pathname/name relationship
inode  → filesystem object metadata/state
```

---

### Q4. What is `struct file`?

Kernel representation of an open file instance containing state such as:

```text
f_pos
f_flags
f_op
f_path
```

---

# 98. Interview Questions — Page Cache

### Q5. Does Linux have a page cache?

Yes.

File-backed data can be cached in memory, allowing repeated reads to avoid storage access when the relevant pages are resident.

---

### Q6. What happens on a page-cache miss?

Simplified:

```text
read
 ↓
page not cached
 ↓
filesystem/block I/O
 ↓
page loaded
 ↓
cache populated
 ↓
data returned
```

---

### Q7. What is a dirty page?

A page whose cached contents have been modified relative to the underlying persistent representation.

It eventually needs writeback.

---

# 99. Interview Questions — File Offset

### Q8. Where is the current file offset maintained?

Conceptually in:

```text
struct file
    |
    +-- f_pos
```

---

### Q9. `read()` vs `pread()`?

```text
read()
 → current file offset
 → updates file offset

pread()
 → explicit offset
 → does not change current file offset
```

---

# 100. Interview Questions — `mmap`

### Q10. Why use `mmap()`?

Potential reasons:

```text
random access
large files
shared memory
memory-oriented programming model
avoid some explicit read/copy operations
```

---

### Q11. Does `mmap()` eliminate page faults?

No.

Accessing a not-yet-present mapped page can generate a page fault.

---

# 101. Interview Questions — `epoll`

### Q12. Why use `epoll()`?

For scalable event-driven monitoring of many file descriptors on Linux.

---

### Q13. Level vs edge triggered?

```text
LT:
readiness remains observable while condition remains true

ET:
notification is associated with changes/readiness transitions
```

In ET mode, non-blocking I/O and draining until `EAGAIN` are common.

---

# 102. Interview Questions — `fsync`

### Q14. Does `write()` guarantee persistence?

No.

`write()` success generally means the kernel accepted the data according to the I/O semantics.

Persistence requires appropriate synchronization and depends on filesystem/storage behavior.

---

### Q15. Why is `fsync()` important?

It is used when the application requires stronger durability guarantees.

---

# 103. Interview Questions — `unlink`

### Q16. What happens if you unlink an open file?

The directory entry is removed, but the file's storage can remain accessible through existing open references until the relevant references are released.

Classic example:

```bash
rm large.log
```

while a process still has it open.

The pathname disappears, but disk space may not be reclaimed until the open reference is closed.

---

# 104. Interview Questions — File Descriptor

### Q17. Why can two FDs refer to the same open file state?

Because duplicated/inherited descriptors can reference the same underlying open-file description.

Example:

```text
dup()
fork()
```

can produce descriptors sharing state such as the file offset.

---

# 105. Interview Questions — `dup2()`

### Q18. Why is `dup2()` important?

It allows an existing descriptor to be duplicated onto a specific descriptor number.

Classic uses:

```text
stdin redirection
stdout redirection
stderr redirection
shell pipelines
```

---

# 106. Interview Questions — `O_APPEND`

### Q19. Why use `O_APPEND`?

It requests append semantics so each write is positioned at the end according to the kernel's append semantics.

Useful for log writers.

It does not automatically solve every multi-process record-formatting or durability problem.

---

# 107. Interview Questions — `O_DIRECT`

### Q20. Is `O_DIRECT` always faster?

No.

It changes normal buffering/cache behavior and has alignment/implementation requirements.

It may help specific workloads but can hurt others.

Benchmark the actual workload.

---

# 108. Senior System-Programming Exercise 1

Implement:

```text
copy_file(src, dst)
```

Requirements:

```text
open source
open/create destination
read in chunks
handle partial writes
handle EINTR
close both descriptors
handle errors
```

Skeleton:

```c
char buffer[8192];

while ((n = read(src, buffer, sizeof(buffer))) > 0)
{
    // write all n bytes
}
```

This is an excellent interview exercise.

---

# 109. Senior System-Programming Exercise 2

Implement:

```text
tail -f style program
```

Requirements:

```text
open file
seek to end
read new data
handle EOF
sleep/poll
continue
```

Concepts tested:

```text
open
read
lseek
EOF
polling
file growth
```

---

# 110. Senior System-Programming Exercise 3

Implement a mini pipeline:

```text
producer | consumer
```

Use:

```text
pipe()
fork()
dup2()
exec()
waitpid()
```

This tests core Linux process + file descriptor + IPC knowledge.

---

# 111. Senior System-Programming Exercise 4

Implement an event-driven server using:

```text
socket
bind
listen
accept
fcntl(O_NONBLOCK)
epoll_create1
epoll_ctl
epoll_wait
```

This combines:

```text
files
file descriptors
networking
non-blocking I/O
I/O multiplexing
```

---

# 112. Senior System-Programming Exercise 5

Create a shared-memory producer/consumer:

```text
shm_open
ftruncate
mmap
semaphore
```

Implement:

```text
producer → shared ring buffer → consumer
```

This combines:

```text
virtual memory
IPC
synchronization
file descriptors
```

---

# 113. Important Commands

Check file:

```bash
file data.txt
```

Permissions:

```bash
ls -l data.txt
```

Metadata:

```bash
stat data.txt
```

Open descriptors:

```bash
lsof -p <pid>
```

Process descriptors:

```bash
ls -l /proc/<pid>/fd
```

System-call tracing:

```bash
strace -f ./program
```

File-related tracing:

```bash
strace -e trace=file ./program
```

FD limit:

```bash
ulimit -n
```

Mounts:

```bash
mount
findmnt
```

Block devices:

```bash
lsblk
```

Disk usage:

```bash
df -h
du -sh <directory>
```

---

# 114. Debugging Checklist

If a file operation fails:

```text
1. Check return value
2. Check errno
3. Check permissions
4. Check pathname
5. Check mount/filesystem
6. Check FD validity
7. Check descriptor limits
8. Use strace
9. Inspect /proc/<pid>/fd
10. Check blocking/non-blocking state
```

Example:

```c
if (fd == -1)
{
    perror("open");
}
```

---

# 115. Common Mistakes

### Mistake 1

Assuming:

```text
read(fd, buf, 4096)
```

always returns 4096.

Wrong.

---

### Mistake 2

Assuming:

```text
write()
```

means data is durable.

Wrong.

---

### Mistake 3

Forgetting:

```text
close()
```

causes FD leaks.

---

### Mistake 4

Using:

```text
read/write
```

on a stream and assuming message boundaries.

Wrong.

---

### Mistake 5

Using blocking I/O inside an event loop.

Can stall the entire event loop.

---

# 116. Key Linux File-I/O Architecture

```text
                 USER SPACE
+-----------------------------------------+
| Application                             |
| open/read/write/mmap/ioctl              |
+--------------------+--------------------+
                     |
                     | system calls
                     v
                 KERNEL SPACE
+-----------------------------------------+
| VFS                                     |
|  ↓                                      |
| dentry / inode / struct file            |
|  ↓                                      |
| Filesystem (ext4/xfs/tmpfs/...)         |
|  ↓                                      |
| Page Cache / MM                         |
|  ↓                                      |
| Block Layer / blk-mq                    |
|  ↓                                      |
| Device Driver                           |
|  ↓                                      |
| Storage Device                          |
+-----------------------------------------+
```

---

# 117. The Most Important Mental Model

For senior Linux interviews, remember:

```text
PATH
 ↓
dentry
 ↓
inode
 ↓
struct file
 ↓
file descriptor
```

And:

```text
read/write
 ↓
VFS
 ↓
filesystem
 ↓
page cache / direct I/O
 ↓
block layer
 ↓
driver
 ↓
device
```

And for memory-mapped files:

```text
file
 ↓
mapping
 ↓
virtual address
 ↓
page fault if page absent
 ↓
page cache/filesystem
 ↓
physical page
```

---

# 118. Chapter 11 One-Minute Revision

```text
FILE
  → named persistent object

FD
  → integer handle in process FD table

struct file
  → kernel state for an open file

inode
  → filesystem object metadata

dentry
  → pathname/name relationship

superblock
  → filesystem-instance information

VFS
  → common interface to filesystems

open()
  → path lookup + open-file creation + FD

read()
  → read bytes; may return partial result

write()
  → write bytes; may return partial result

lseek()
  → change current offset

pread/pwrite
  → explicit offset without changing f_pos

stat/fstat
  → metadata

mmap()
  → map file/memory into virtual address space

page cache
  → cache file-backed data

fsync()
  → request synchronization/durability-related writeback

O_DIRECT
  → direct-I/O semantics; not automatically faster

select/poll
  → classic readiness notification

epoll
  → scalable Linux event notification

io_uring
  → modern asynchronous I/O interface

strace
  → inspect system calls

/proc/<pid>/fd
  → inspect process FDs
```

---

# 119. Final Interview Flow

If asked:

> "Explain what happens when a process reads a file."

Answer:

```text
1. Application calls read(fd, buffer, size).
2. Kernel validates the FD and obtains the open-file state.
3. VFS dispatches the operation.
4. Filesystem handles the file operation.
5. If data is in the page cache, it can be served from memory.
6. Otherwise filesystem/storage I/O may be initiated.
7. The task may block while waiting for I/O.
8. Storage completion wakes the waiting task.
9. Data is made available to the process.
10. read() returns the number of bytes read.
```

For:

> "What happens when opening a file?"

```text
open()
 ↓
path lookup
 ↓
VFS
 ↓
dentry/inode
 ↓
filesystem
 ↓
struct file
 ↓
FD table
 ↓
fd returned
```

For:

> "Where does the data go on write?"

```text
user buffer
 ↓
write()
 ↓
VFS/filesystem
 ↓
page cache for normal buffered I/O
 ↓
dirty data
 ↓
writeback
 ↓
block layer
 ↓
driver
 ↓
storage
```

---

# 120. Chapter 12 Preview — Networking

Next chapter:

```text
Chapter 12 — Networking

OS
 ├── Network fundamentals
 ├── TCP/IP
 ├── sockets
 ├── blocking/non-blocking sockets
 ├── client/server model
 └── I/O multiplexing

Linux System Programming
 ├── socket()
 ├── bind()
 ├── listen()
 ├── accept()
 ├── connect()
 ├── send()/recv()
 ├── sendmsg()/recvmsg()
 ├── setsockopt()
 ├── getaddrinfo()
 ├── TCP client/server C code
 ├── UDP C code
 ├── Unix domain sockets
 └── epoll server

Linux Kernel Internals
 ├── socket layer
 ├── sk_buff
 ├── network namespaces
 ├── TCP state machine
 ├── receive path
 ├── transmit path
 ├── NIC driver
 ├── NAPI
 ├── interrupts
 ├── routing
 └── packet flow
```

Key flow:

```text
Application
    ↓
socket API
    ↓
Socket layer
    ↓
TCP/UDP
    ↓
IP
    ↓
qdisc / networking stack
    ↓
NIC driver
    ↓
NIC
    ↓
Network
```
