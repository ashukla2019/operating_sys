# Chapter 4 – Linux System Calls

---

# 1. What Is a System Call?

A **system call** is the controlled interface through which a user-space program requests a service from the Linux kernel.

Examples:

```c
read();
write();
openat();
close();
mmap();
fork();
clone();
execve();
socket();
ioctl();
```

The basic flow is:

```text
User Application
       |
       v
C Library / Wrapper
       |
       v
System Call
       |
       v
CPU Privilege Transition
       |
       v
Linux Kernel
       |
       v
Kernel Subsystem
       |
       v
Hardware / Memory / Filesystem / Network
```

---

# 2. Why Do We Need System Calls?

User applications cannot directly perform privileged operations.

For example, an application should not be able to directly:

```text
Access arbitrary physical memory
Modify page tables
Program hardware registers arbitrarily
Access disk hardware directly
Change CPU control state
```

Instead:

```text
Application
     |
     v
System Call
     |
     v
Kernel validates request
     |
     v
Kernel performs operation
```

This provides:

* Security
* Isolation
* Hardware abstraction
* Resource management
* Controlled access to kernel services

---

# 3. User Mode and Kernel Mode

Modern CPUs provide privilege levels.

The important distinction for Linux is:

```text
User Mode
    |
    | Restricted
    |
    v
Kernel Mode
    |
    | Privileged
    |
    v
Hardware
```

Applications normally execute in **user mode**.

The Linux kernel executes in **kernel mode**.

---

# 4. Example: read()

Consider:

```c
char buf[100];

read(fd, buf, 100);
```

The application is asking:

> Kernel, please read data from this file descriptor into this buffer.

Conceptually:

```text
Application
    |
    | read(fd, buf, 100)
    v
C Library
    |
    v
System Call Interface
    |
    v
Kernel
    |
    v
VFS
    |
    v
Filesystem
    |
    v
Block Layer
    |
    v
Storage
```

The actual path depends on the type of file descriptor.

---

# 5. System Call vs Library Function

This distinction is important.

Not every function you call is a system call.

For example:

```c
printf("Hello");
```

is a C library function.

It may eventually use:

```text
write()
```

to output data.

Similarly:

```c
malloc()
```

is generally a C library allocator function.

It may obtain more virtual memory using system calls such as:

```text
mmap()
brk()
```

So:

```text
Application
     |
     v
Library Function
     |
     v
System Call
     |
     v
Kernel
```

---

# 6. System Call Interface

Linux exposes a system-call interface to user space.

Examples:

```text
read
write
openat
close
mmap
munmap
clone
execve
wait4
socket
connect
sendto
recvfrom
ioctl
```

The exact system-call ABI depends on the architecture.

---

# 7. System Call Number

The kernel needs to know which system call the application requested.

Therefore, a system-call number is used.

Conceptually:

```text
System Call Number
        |
        v
Kernel System Call Table
        |
        v
System Call Handler
```

For example:

```text
syscall number
      |
      v
system_call_table
      |
      v
appropriate kernel implementation
```

The exact internal implementation and table details vary by architecture and kernel version.

---

# 8. System Call Arguments

Suppose:

```c
read(fd, buffer, size);
```

The kernel needs:

```text
fd
buffer
size
```

The architecture's system-call ABI specifies how the system-call number and arguments are passed, commonly using CPU registers.

Conceptually:

```text
Registers
+----------------+
| syscall number |
| argument 1     |
| argument 2     |
| argument 3     |
+----------------+
        |
        v
Kernel Entry
```

---

# 9. CPU Privilege Transition

A system call requires transitioning from user execution to kernel execution.

Conceptually:

```text
User Mode
    |
    | syscall instruction
    v
Kernel Entry
    |
    v
Kernel Mode
```

On x86-64 Linux, the `syscall` instruction is commonly used for this transition.

Other architectures use their own system-call mechanisms.

---

# 10. x86-64 Example

On x86-64, a simplified system-call sequence looks like:

```text
User Code
   |
   v
Registers prepared
   |
   v
syscall instruction
   |
   v
CPU enters kernel
   |
   v
Kernel syscall entry
   |
   v
System call dispatcher
   |
   v
Requested syscall
```

The exact register usage is defined by the Linux x86-64 syscall ABI.

---

# 11. System Call Return

After the kernel finishes:

```text
Kernel
   |
   v
Return value
   |
   v
Return to user mode
   |
   v
Application continues
```

Example:

```c
ssize_t n = read(fd, buf, 100);
```

If successful:

```text
n > 0
```

If an error occurs:

```text
read() returns -1
errno is set appropriately
```

---

# 12. Complete System Call Flow

The important mental model is:

```text
             USER SPACE
+--------------------------------+
| Application                    |
|                                |
| read(fd, buf, size)            |
+---------------+----------------+
                |
                v
+--------------------------------+
| C Library / Syscall Wrapper    |
+---------------+----------------+
                |
                v
         syscall instruction
                |
================ CPU BOUNDARY =================
                |
                v
+--------------------------------+
| Kernel Entry                   |
+---------------+----------------+
                |
                v
+--------------------------------+
| Syscall Dispatcher             |
+---------------+----------------+
                |
                v
+--------------------------------+
| Specific Kernel Subsystem      |
+---------------+----------------+
                |
                v
+--------------------------------+
| Filesystem / Network / Memory  |
+--------------------------------+
```

---

# 13. What Happens During a System Call?

A simplified sequence:

```text
1. Application calls wrapper
2. Arguments are prepared
3. System-call number is prepared
4. CPU executes syscall instruction
5. CPU transitions to kernel mode
6. Kernel saves required execution state
7. Kernel validates the request
8. Kernel dispatches the requested system call
9. Kernel performs the operation
10. Kernel prepares return value
11. CPU returns to user mode
12. Application continues
```

---

# 14. Why Does the Kernel Validate Arguments?

Never trust user-space input.

Suppose:

```c
write(fd, user_buffer, size);
```

The kernel must ensure that:

```text
fd
buffer
size
```

are valid for the requested operation.

For pointers, the kernel must safely interact with user memory.

Conceptually:

```text
User Pointer
     |
     v
Kernel validation/access mechanism
     |
     v
User memory
```

The kernel must not simply trust arbitrary user pointers.

---

# 15. User-Space Pointer vs Kernel-Space Pointer

Suppose:

```c
char *buf;
read(fd, buf, 100);
```

`buf` is a user-space virtual address.

The kernel cannot treat it as an arbitrary kernel pointer.

Linux provides mechanisms/macros such as:

```text
copy_to_user()
copy_from_user()
```

for safely copying data across the user/kernel boundary.

---

# 16. copy_from_user()

Suppose user space sends data to the kernel.

```text
User
 |
 | buffer
 v
Kernel
```

Conceptually:

```text
copy_from_user()
```

copies data:

```text
User Memory
     |
     | copy_from_user()
     v
Kernel Memory
```

---

# 17. copy_to_user()

When the kernel needs to return data:

```text
Kernel Memory
     |
     | copy_to_user()
     v
User Memory
```

Conceptually:

```text
Kernel
  |
  v
copy_to_user()
  |
  v
Application buffer
```

---

# 18. Why Not Just Dereference User Pointer?

Suppose:

```c
char *p = user_pointer;
```

The pointer might:

```text
Point to unmapped memory
Point to inaccessible memory
Change mappings
Cause a page fault
Be maliciously constructed
```

Therefore, kernel code must use appropriate user-memory access mechanisms.

---

# 19. read() – Important Example

Application:

```c
char buf[128];

ssize_t n = read(fd, buf, sizeof(buf));
```

Simplified:

```text
Application
    |
    v
read()
    |
    v
System Call
    |
    v
Kernel
    |
    v
fd lookup
    |
    v
file object
    |
    v
file operation
    |
    v
Filesystem / Device / Socket
    |
    v
Data
    |
    v
copy_to_user()
    |
    v
Application buffer
```

The exact path depends on the file descriptor.

---

# 20. write() – Important Example

Application:

```c
write(fd, buf, 100);
```

Simplified:

```text
Application
    |
    v
write()
    |
    v
System Call
    |
    v
Kernel
    |
    v
Validate fd/buffer
    |
    v
file object
    |
    v
Filesystem / Device / Socket
```

For regular files, the data may interact with the page cache and filesystem before eventually reaching storage.

---

# 21. openat()

Modern Linux programs frequently use:

```c
openat();
```

rather than relying only on the older `open()` interface.

Example:

```c
int fd = openat(AT_FDCWD, "file.txt", O_RDONLY);
```

Simplified:

```text
openat()
   |
   v
System Call
   |
   v
VFS
   |
   v
Path Resolution
   |
   v
dentry
   |
   v
inode
   |
   v
file object
   |
   v
file descriptor
```

This connects directly to Linux VFS internals.

---

# 22. File Descriptor

A file descriptor is a small integer used by a process to refer to an open file-like object.

Example:

```c
int fd = open("test.txt", O_RDONLY);
```

You might get:

```text
fd = 3
```

Conceptually:

```text
Process
   |
   v
File Descriptor Table
   |
   +-- 0 -> stdin
   +-- 1 -> stdout
   +-- 2 -> stderr
   +-- 3 -> file
```

The descriptor table points toward kernel-side file structures.

---

# 23. File Descriptor Flow

The important conceptual chain is:

```text
fd
 |
 v
Process FD Table
 |
 v
struct file
 |
 v
f_path
 |
 +-- dentry
 |
 +-- vfsmount
 |
 v
inode
 |
 v
Filesystem
```

This becomes very important in the VFS chapter.

---

# 24. mmap()

`mmap()` is a system call that creates a mapping in a process's virtual address space.

Conceptually:

```text
Application
     |
     v
mmap()
     |
     v
Kernel
     |
     v
Create memory mapping
     |
     v
Return virtual address
```

Physical pages may be populated later through demand paging.

---

# 25. fork()

`fork()` creates a new process.

Simplified:

```text
Parent
  |
  | fork()
  v
Parent + Child
```

Modern Linux process creation is built around the `clone`/`clone3` mechanisms, with `fork()` exposed as a traditional process-creation interface.

---

# 26. fork() and COW

The important memory behavior:

```text
Parent
   |
   +----+
        |
        v
    Physical Page
        ^
        |
   +----+
   |
Child
```

Pages are initially shared.

When one process writes:

```text
Child writes
    |
    v
Page Fault
    |
    v
Copy page
    |
    v
Child gets private copy
```

---

# 27. execve()

`execve()` replaces the current process image with a new program.

Important distinction:

```text
fork()
    |
    v
Creates another process
```

while:

```text
execve()
    |
    v
Replaces current process image
```

Typical shell sequence:

```text
shell
 |
 +-- fork()
 |
 +-- child
       |
       +-- execve()
              |
              v
          new program
```

---

# 28. exit()

A process can terminate using mechanisms ultimately involving the kernel's process-exit machinery.

Conceptually:

```text
Process
   |
   v
exit()
   |
   v
Kernel
   |
   v
Release resources
   |
   v
Process termination
```

Resources include:

```text
Memory mappings
File references
Signal-related state
Kernel objects
Other process resources
```

---

# 29. wait()

A parent can wait for a child process.

```text
Parent
   |
   v
wait()
   |
   v
Kernel
   |
   v
Wait for child
```

This is important for process lifecycle management.

---

# 30. ioctl()

`ioctl()` provides a generic interface for device-specific control operations.

Example:

```text
Application
     |
     v
ioctl(fd, command, argument)
     |
     v
Kernel
     |
     v
Device Driver
     |
     v
Hardware
```

It is commonly used when operations do not fit naturally into:

```text
read()
write()
```

---

# 31. Device Driver System Call Flow

For a device:

```text
Application
    |
    v
open()
    |
    v
Device Driver
```

Then:

```text
read()
write()
ioctl()
mmap()
```

may interact with the driver.

Conceptually:

```text
Application
     |
     v
System Call
     |
     v
VFS
     |
     v
struct file
     |
     v
file_operations
     |
     v
Device Driver
     |
     v
Hardware
```

---

# 32. socket()

Networking also uses system calls.

Example:

```c
int fd = socket(AF_INET, SOCK_STREAM, 0);
```

Conceptually:

```text
Application
    |
    v
socket()
    |
    v
Kernel
    |
    v
Socket Subsystem
    |
    v
Protocol Stack
```

Other important calls include:

```text
bind()
listen()
accept()
connect()
send()
recv()
sendto()
recvfrom()
setsockopt()
```

---

# 33. System Calls and Networking

A simplified TCP send path:

```text
Application
     |
     v
send()/write()
     |
     v
Socket Layer
     |
     v
TCP
     |
     v
IP
     |
     v
Network Device
     |
     v
NIC
```

The actual kernel path is more detailed.

---

# 34. Blocking System Calls

Some system calls can block.

Example:

```c
read(fd, buf, size);
```

If data is not available:

```text
Process
   |
   v
read()
   |
   v
No data
   |
   v
Sleep / Wait
   |
   v
Data becomes available
   |
   v
Wake process
   |
   v
read() continues
```

This is a key connection to process scheduling.

---

# 35. Nonblocking System Calls

A descriptor can be configured for nonblocking behavior.

For example:

```text
O_NONBLOCK
```

Then:

```text
read()
   |
   v
No data
   |
   v
Return immediately
```

The application can then use mechanisms such as:

```text
poll()
select()
epoll()
```

---

# 36. System Call vs Context Switch

These are **not the same thing**.

A system call means:

```text
User
  |
  v
Kernel
  |
  v
User
```

A context switch means:

```text
Process A
    |
    v
Scheduler
    |
    v
Process B
```

A system call does not necessarily cause a context switch.

A blocking system call may eventually cause the calling task to sleep, which can lead to a context switch.

---

# 37. System Call vs Interrupt

Another important distinction.

## System Call

Usually initiated intentionally by software:

```text
Application
    |
    v
syscall
    |
    v
Kernel
```

## Hardware Interrupt

Usually generated by hardware:

```text
Device
   |
   v
Interrupt
   |
   v
CPU
   |
   v
Kernel interrupt handler
```

Example:

```text
NIC receives packet
       |
       v
Interrupt / interrupt-related notification
       |
       v
Kernel
```

---

# 38. System Call vs Exception

A CPU exception can occur due to an instruction or condition during execution.

Examples:

```text
Page fault
Invalid instruction
Protection fault
```

A system call is a deliberate transition into the kernel using the architecture's system-call mechanism.

---

# 39. vDSO

Some operations can avoid a full traditional system-call transition.

Linux provides:

```text
vDSO
```

Virtual Dynamic Shared Object.

The kernel maps a special shared object into processes.

Certain operations can be performed in user space using kernel-maintained data.

A common example is time-related functionality.

Conceptually:

```text
Normal syscall:

Application
    |
    v
Kernel transition
    |
    v
Kernel
```

vDSO-assisted path:

```text
Application
    |
    v
vDSO
    |
    v
User-space execution
```

This can reduce overhead.

---

# 40. Why vDSO Exists

Some operations are:

```text
Frequent
Simple
Based on kernel-maintained state
```

Performing a full privilege transition every time would be unnecessarily expensive.

Therefore Linux can expose optimized user-space implementations.

---

# 41. System Call Overhead

A system call has overhead because it may involve:

```text
User/kernel transition
Register/state handling
Security checks
Argument validation
Kernel execution
Return transition
```

Therefore high-performance software tries to minimize unnecessary system calls.

For example:

```text
Bad:

write(fd, &c, 1);
write(fd, &c, 1);
write(fd, &c, 1);
...
```

may be less efficient than batching data:

```text
write(fd, large_buffer, size);
```

---

# 42. Batching

High-performance systems often reduce syscall frequency.

Instead of:

```text
1000 system calls
```

try, when appropriate:

```text
fewer larger operations
```

This is important in:

* Networking
* Storage
* Databases
* Logging
* IPC

---

# 43. strace

`strace` is an extremely useful Linux debugging tool.

Example:

```bash
strace ./program
```

It shows system calls made by a process.

Example output conceptually:

```text
openat(...)
read(...)
write(...)
close(...)
```

This is useful for understanding what a program is actually doing.

---

# 44. Example strace Flow

Suppose:

```c
int fd = open("test.txt", O_RDONLY);

char buf[100];

read(fd, buf, sizeof(buf));

close(fd);
```

You may observe:

```text
openat(...)
read(...)
close(...)
```

This helps connect:

```text
C code
  |
  v
Library
  |
  v
System calls
```

---

# 45. System Call Tracing Mental Model

```text
Application
    |
    v
Library
    |
    v
System Call
    |
    v
Kernel
```

`strace` observes the system-call boundary:

```text
Application
    |
    v
strace
    |
    v
System Calls
```

---

# 46. Important System Calls to Know

For senior Linux interviews, know these groups.

## Process

```text
fork()
clone()
clone3()
execve()
wait4()
_exit()
```

## Memory

```text
mmap()
munmap()
mprotect()
madvise()
brk()
```

## Files

```text
openat()
read()
write()
close()
pread64()
pwrite64()
fsync()
statx()
```

## Networking

```text
socket()
bind()
listen()
accept()
connect()
sendto()
recvfrom()
setsockopt()
```

## Multiplexing

```text
poll()
ppoll()
epoll_create1()
epoll_ctl()
epoll_wait()
```

## Synchronization / IPC

```text
futex()
event-related mechanisms
shared memory mechanisms
```

## Device

```text
ioctl()
mmap()
```

---

# 47. Important Interview Question

## What happens when you call read()?

A strong answer:

```text
Application
    |
    v
read() wrapper
    |
    v
System-call entry
    |
    v
Kernel
    |
    v
Validate arguments
    |
    v
Find file object from fd
    |
    v
Invoke appropriate file operation
    |
    v
Filesystem / device / socket
    |
    v
Obtain data
    |
    v
Copy data to user buffer
    |
    v
Return count/error
    |
    v
User space
```

---

# 48. Important Interview Question

## Does every function call cause a system call?

No.

Example:

```c
strlen()
memcpy()
strcmp()
```

can execute entirely in user space.

But:

```c
read()
write()
mmap()
```

normally require kernel interaction.

---

# 49. Important Interview Question

## Does malloc() cause a system call every time?

No.

The allocator normally manages a pool/arena of memory in user space.

It may request more memory from the kernel using mechanisms such as:

```text
mmap()
brk()
```

only when necessary.

Therefore:

```text
malloc()
  |
  +-- existing allocator memory
  |
  +-- possibly kernel request
```

---

# 50. Important Interview Question

## Does a system call always cause a context switch?

No.

A system call causes a transition between user and kernel execution.

A context switch happens when the scheduler switches execution from one task to another.

A blocking syscall may cause the current task to sleep and therefore lead to a context switch.

---

# 51. Important Interview Question

## Why can't user space directly access kernel memory?

Because the CPU's privilege and memory-protection mechanisms prevent normal user-mode access to protected kernel mappings.

This protects:

```text
Kernel
Other processes
System state
Hardware control
```

---

# 52. Important Interview Question

## What is the difference between read() and mmap()?

`read()` explicitly transfers data into a user buffer.

```text
File
 |
 v
Kernel
 |
 v
User Buffer
```

With `mmap()`:

```text
File
 |
 v
Memory Mapping
 |
 v
User Virtual Address
```

The application accesses the mapped region using normal memory operations.

---

# 53. Important Interview Question

## Why is mmap() often useful for large data?

It can avoid explicit repeated `read()`/`write()` calls and allows the application to access file contents through memory mappings.

It also integrates naturally with:

```text
Virtual Memory
Page Faults
Page Cache
```

---

# 54. Important Interview Question

## What is ioctl()?

A general-purpose interface for device- or subsystem-specific control operations that don't fit naturally into standard operations such as `read()` and `write()`.

---

# 55. Important Interview Question

## What is vDSO?

A kernel-provided shared object mapped into user processes that allows certain operations to be performed efficiently in user space using kernel-maintained information, avoiding a full syscall when possible.

---

# 56. Senior Interview Mental Model

Memorize this:

```text
             USER SPACE
                  |
                  v
          C Library / Wrapper
                  |
                  v
          System Call ABI
                  |
                  v
             CPU Entry
                  |
                  v
             KERNEL SPACE
                  |
        +---------+---------+
        |         |         |
        v         v         v
      VFS      Memory    Network
        |         |         |
        v         v         v
    Filesystem   MM       TCP/IP
        |                   |
        v                   v
     Storage                NIC
```

---

# 57. One Complete Example

Consider:

```c
int fd = open("data.txt", O_RDONLY);

char buf[4096];

ssize_t n = read(fd, buf, sizeof(buf));

close(fd);
```

Mental execution:

```text
open()
  |
  v
System Call
  |
  v
VFS
  |
  v
Path Resolution
  |
  v
dentry + inode
  |
  v
struct file
  |
  v
fd returned
```

Then:

```text
read()
  |
  v
System Call
  |
  v
fd -> struct file
  |
  v
Filesystem
  |
  v
Page Cache
  |
  +-- Hit -> data
  |
  +-- Miss -> storage
  |
  v
copy_to_user()
  |
  v
buf
```

Then:

```text
close()
  |
  v
System Call
  |
  v
Release file reference
```

This single example connects:

```text
System Calls
VFS
File Descriptors
Page Cache
Memory
Filesystem
Storage
```

---

# 58. High-Priority Topics

## Must Know

```text
User mode vs Kernel mode
System call
System-call ABI
syscall instruction
System-call arguments
System-call return value
read()
write()
openat()
close()
mmap()
fork()
execve()
ioctl()
socket()
File descriptors
copy_to_user()
copy_from_user()
strace
```

## Should Know

```text
Blocking vs nonblocking
poll()
epoll()
vDSO
System call overhead
Context switch vs syscall
System call vs interrupt
System call vs exception
```

## Deep Dive

```text
Architecture-specific syscall entry
Kernel entry/exit paths
Syscall dispatch internals
futex()
io_uring
seccomp
restartable syscalls
syscall tracing
```

---

# 59. Quick Revision

```text
User
 |
 | syscall
 v
Kernel
 |
 | validate
 v
Subsystem
 |
 | perform operation
 v
Return value
 |
 v
User
```

File:

```text
fd
 |
 v
struct file
 |
 v
dentry
 |
 v
inode
 |
 v
filesystem
```

Memory:

```text
mmap()
 |
 v
Virtual Address
 |
 v
Page Tables
 |
 v
Physical Memory
```

Process:

```text
fork()
 |
 v
COW
 |
 v
child
```

Program replacement:

```text
execve()
 |
 v
new process image
```

Device:

```text
ioctl()
 |
 v
Driver
 |
 v
Hardware
```

Networking:

```text
socket()
 |
 v
Socket Layer
 |
 v
TCP/IP
 |
 v
NIC
```

---

# 60. Chapter Summary

The most important concept is:

> **A system call is the controlled gateway from user space into the Linux kernel.**

The complete mental model is:

```text
Application
    |
    v
C Library / Wrapper
    |
    v
System Call ABI
    |
    v
CPU Privilege Transition
    |
    v
Kernel Entry
    |
    v
System Call Dispatcher
    |
    v
Kernel Subsystem
    |
    +---- VFS
    +---- Memory Management
    +---- Process Management
    +---- Networking
    +---- Device Drivers
    |
    v
Return to User Space
```

For senior interviews, the goal is not to memorize hundreds of syscall names.

You should be able to take a simple C/C++ operation such as:

```c
read()
malloc()
mmap()
fork()
socket()
ioctl()
```

and explain **what crosses the user/kernel boundary, what the kernel does, and how the operation reaches the relevant subsystem**.

---

# Next Chapter

## Chapter 5 – Linux Process Management

We will connect:

```text
Process
   |
   v
task_struct
   |
   +---- PID
   +---- Parent/Child
   +---- mm_struct
   +---- Files
   +---- Signals
   +---- Credentials
   +---- Scheduling
   |
   v
Scheduler
   |
   v
CPU
```

Topics will include:

* Process vs program
* `task_struct`
* PID/TGID
* Process creation
* `fork()`
* `clone()`
* `clone3()`
* `execve()`
* Process states
* Parent/child relationship
* Zombie/orphan processes
* Process groups
* Sessions
* Threads
* Kernel threads
* `mm_struct`
* `files_struct`
* `fs_struct`
* `signal_struct`
* Process termination
* `wait()`
* Namespaces
* Important senior interview questions
