# Chapter 4 – Linux System Calls

## 1. What Is a System Call?
A **system call** is the controlled interface through which a user-space program requests a service from the Linux kernel. Examples: `read()`, `write()`, `openat()`, `close()`, `mmap()`, `fork()`, `clone()`, `execve()`, `socket()`, `ioctl()`.

The basic flow:
```
User Application --> C Library / Wrapper --> System Call --> CPU Privilege Transition
--> Linux Kernel --> Kernel Subsystem --> Hardware / Memory / Filesystem / Network
```

## 2. Why Do We Need System Calls?
User applications cannot directly perform privileged operations — an application should not be able to directly access arbitrary physical memory, modify page tables, program hardware registers arbitrarily, access disk hardware directly, or change CPU control state. Instead:
```
Application --> System Call --> Kernel validates request --> Kernel performs operation
```
This provides security, isolation, hardware abstraction, resource management, and controlled access to kernel services.

## 3. User Mode and Kernel Mode
Modern CPUs provide privilege levels. The important distinction for Linux:
```
User Mode (Restricted) --> Kernel Mode (Privileged) --> Hardware
```
Applications normally execute in **user mode**; the Linux kernel executes in **kernel mode**.

---

## 4. Example: read()
```c
char buf[100];
read(fd, buf, 100);
```
The application is asking the kernel to read data from this file descriptor into this buffer:
```
Application --read(fd, buf, 100)--> C Library --> System Call Interface --> Kernel
--> VFS --> Filesystem --> Block Layer --> Storage
```
The actual path depends on the type of file descriptor.

## 5. System Call vs Library Function
Not every function you call is a system call. `printf("Hello");` is a C library function that may eventually use `write()` to output data. Similarly, `malloc()` is generally a C library allocator function that may obtain more virtual memory using system calls such as `mmap()`/`brk()`. So:
```
Application --> Library Function --> System Call --> Kernel
```

## 6. System Call Interface
Linux exposes a system-call interface to user space: `read`, `write`, `openat`, `close`, `mmap`, `munmap`, `clone`, `execve`, `wait4`, `socket`, `connect`, `sendto`, `recvfrom`, `ioctl`. The exact system-call ABI depends on the architecture.

## 7. System Call Number
The kernel needs to know which system call was requested, via a system-call number:
```
System Call Number --> Kernel System Call Table --> System Call Handler
```
i.e. `syscall number --> system_call_table --> appropriate kernel implementation`. The exact internal implementation and table details vary by architecture and kernel version.

## 8. System Call Arguments
For `read(fd, buffer, size);`, the kernel needs `fd`, `buffer`, `size`. The architecture's system-call ABI specifies how the number and arguments are passed, commonly using CPU registers:
```
Registers: [syscall number | argument 1 | argument 2 | argument 3] --> Kernel Entry
```

---

## 9. CPU Privilege Transition
A system call requires transitioning from user execution to kernel execution:
```
User Mode --syscall instruction--> Kernel Entry --> Kernel Mode
```
On x86-64 Linux, the `syscall` instruction is commonly used; other architectures use their own mechanisms.

## 10. x86-64 Example
A simplified x86-64 system-call sequence:
```
User Code --> Registers prepared --> syscall instruction --> CPU enters kernel
--> Kernel syscall entry --> System call dispatcher --> Requested syscall
```
The exact register usage is defined by the Linux x86-64 syscall ABI.

## 11. System Call Return
After the kernel finishes: `Kernel --> Return value --> Return to user mode --> Application continues`. For `ssize_t n = read(fd, buf, 100);` — success means `n > 0`; on error, `read()` returns `-1` and `errno` is set appropriately.

---

## 12. Complete System Call Flow
The important mental model:
```
             USER SPACE
  Application: read(fd, buf, size)
                |
  C Library / Syscall Wrapper
                |
        syscall instruction
      ====== CPU BOUNDARY ======
                |
          Kernel Entry
                |
        Syscall Dispatcher
                |
     Specific Kernel Subsystem
                |
  Filesystem / Network / Memory
```

## 13. What Happens During a System Call?
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

---

## 14. Why Does the Kernel Validate Arguments?
Never trust user-space input. For `write(fd, user_buffer, size);`, the kernel must ensure `fd`, `buffer`, and `size` are valid for the requested operation. For pointers, the kernel must safely interact with user memory rather than simply trusting arbitrary user pointers:
```
User Pointer --> Kernel validation/access mechanism --> User memory
```

## 15. User-Space Pointer vs Kernel-Space Pointer
For `char *buf; read(fd, buf, 100);` — `buf` is a user-space virtual address; the kernel cannot treat it as an arbitrary kernel pointer. Linux provides mechanisms such as `copy_to_user()` and `copy_from_user()` for safely copying data across the user/kernel boundary.

## 16. copy_from_user()
When user space sends data to the kernel:
```
User Memory --copy_from_user()--> Kernel Memory
```

## 17. copy_to_user()
When the kernel needs to return data:
```
Kernel Memory --copy_to_user()--> User Memory
```

## 18. Why Not Just Dereference User Pointer?
`char *p = user_pointer;` — the pointer might point to unmapped or inaccessible memory, have changed mappings, cause a page fault, or be maliciously constructed. Therefore kernel code must use appropriate user-memory access mechanisms.

---

## 19. read() – Important Example
```c
char buf[128];
ssize_t n = read(fd, buf, sizeof(buf));
```
```
Application --> read() --> System Call --> Kernel --> fd lookup --> file object
--> file operation --> Filesystem/Device/Socket --> Data --> copy_to_user() --> Application buffer
```
The exact path depends on the file descriptor.

## 20. write() – Important Example
```c
write(fd, buf, 100);
```
```
Application --> write() --> System Call --> Kernel --> Validate fd/buffer
--> file object --> Filesystem/Device/Socket
```
For regular files, the data may interact with the page cache and filesystem before eventually reaching storage.

## 21. openat()
Modern Linux programs frequently use `openat()` rather than relying only on the older `open()` interface, e.g. `int fd = openat(AT_FDCWD, "file.txt", O_RDONLY);`
```
openat() --> System Call --> VFS --> Path Resolution --> dentry --> inode --> file object --> file descriptor
```
This connects directly to Linux VFS internals.

---

## 22. File Descriptor
A file descriptor is a small integer used by a process to refer to an open file-like object, e.g. `int fd = open("test.txt", O_RDONLY);` might give `fd = 3`:
```
Process --> File Descriptor Table
              +-- 0 -> stdin
              +-- 1 -> stdout
              +-- 2 -> stderr
              +-- 3 -> file
```
The descriptor table points toward kernel-side file structures.

## 23. File Descriptor Flow
The important conceptual chain:
```
fd --> Process FD Table --> struct file --> f_path
                                              +-- dentry
                                              +-- vfsmount
                                             --> inode --> Filesystem
```
This becomes very important in the VFS chapter.

---

## 24. mmap()
`mmap()` is a system call that creates a mapping in a process's virtual address space:
```
Application --> mmap() --> Kernel --> Create memory mapping --> Return virtual address
```
Physical pages may be populated later through demand paging.

## 25. fork()
`fork()` creates a new process: `Parent --fork()--> Parent + Child`. Modern Linux process creation is built around the `clone`/`clone3` mechanisms, with `fork()` exposed as a traditional process-creation interface.

## 26. fork() and COW
The important memory behavior:
```
Parent ----+
           v
      Physical Page
           ^
Child -----+
```
Pages are initially shared. When one process writes: `Child writes --> Page Fault --> Copy page --> Child gets private copy`.

## 27. execve()
`execve()` replaces the current process image with a new program. Important distinction: `fork()` creates another process, while `execve()` replaces the current process image. Typical shell sequence:
```
shell --fork()--> child --execve()--> new program
```

## 28. exit()
A process can terminate using mechanisms ultimately involving the kernel's process-exit machinery:
```
Process --exit()--> Kernel --> Release resources --> Process termination
```
Resources include memory mappings, file references, signal-related state, kernel objects, and other process resources.

## 29. wait()
A parent can wait for a child process: `Parent --wait()--> Kernel --> Wait for child`. This is important for process lifecycle management.

---

## 30. ioctl()
`ioctl()` provides a generic interface for device-specific control operations:
```
Application --ioctl(fd, command, argument)--> Kernel --> Device Driver --> Hardware
```
Commonly used when operations don't fit naturally into `read()`/`write()`.

## 31. Device Driver System Call Flow
For a device: `Application --> open() --> Device Driver`, then `read()`, `write()`, `ioctl()`, `mmap()` may interact with the driver:
```
Application --> System Call --> VFS --> struct file --> file_operations --> Device Driver --> Hardware
```

## 32. socket()
Networking also uses system calls, e.g. `int fd = socket(AF_INET, SOCK_STREAM, 0);`
```
Application --> socket() --> Kernel --> Socket Subsystem --> Protocol Stack
```
Other important calls: `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `sendto()`, `recvfrom()`, `setsockopt()`.

## 33. System Calls and Networking
A simplified TCP send path:
```
Application --> send()/write() --> Socket Layer --> TCP --> IP --> Network Device --> NIC
```
The actual kernel path is more detailed.

---

## 34. Blocking System Calls
Some system calls can block, e.g. `read(fd, buf, size);`. If data isn't available:
```
Process --> read() --> No data --> Sleep/Wait --> Data becomes available --> Wake process --> read() continues
```
This is a key connection to process scheduling.

## 35. Nonblocking System Calls
A descriptor can be configured for nonblocking behavior (`O_NONBLOCK`). Then: `read() --> No data --> Return immediately`. The application can then use mechanisms such as `poll()`, `select()`, `epoll()`.

## 36. System Call vs Context Switch
These are **not the same thing**. A system call: `User --> Kernel --> User`. A context switch: `Process A --> Scheduler --> Process B`. A system call does not necessarily cause a context switch, though a blocking system call may eventually cause the calling task to sleep, which can lead to a context switch.

## 37. System Call vs Interrupt
**System Call** — usually initiated intentionally by software: `Application --syscall--> Kernel`.
**Hardware Interrupt** — usually generated by hardware: `Device --> Interrupt --> CPU --> Kernel interrupt handler`, e.g. `NIC receives packet --> Interrupt/interrupt-related notification --> Kernel`.

## 38. System Call vs Exception
A CPU exception can occur due to an instruction or condition during execution (page fault, invalid instruction, protection fault). A system call is a deliberate transition into the kernel using the architecture's system-call mechanism.

---

## 39. vDSO
Some operations can avoid a full traditional system-call transition. Linux provides the **vDSO** (Virtual Dynamic Shared Object) — the kernel maps a special shared object into processes, allowing certain operations to be performed in user space using kernel-maintained data. A common example is time-related functionality.
```
Normal syscall: Application --> Kernel transition --> Kernel
vDSO-assisted:  Application --> vDSO --> User-space execution
```
This can reduce overhead.

## 40. Why vDSO Exists
Some operations are frequent, simple, and based on kernel-maintained state — performing a full privilege transition every time would be unnecessarily expensive. Therefore Linux can expose optimized user-space implementations.

## 41. System Call Overhead
A system call has overhead: user/kernel transition, register/state handling, security checks, argument validation, kernel execution, return transition. High-performance software tries to minimize unnecessary system calls — e.g. `write(fd, &c, 1);` repeated many times is less efficient than batching data with `write(fd, large_buffer, size);`.

## 42. Batching
High-performance systems often reduce syscall frequency — instead of 1000 system calls, use fewer larger operations when appropriate. Important in networking, storage, databases, logging, and IPC.

---

## 43. strace
`strace` is an extremely useful Linux debugging tool, e.g. `strace ./program`. It shows system calls made by a process — conceptually: `openat(...)`, `read(...)`, `write(...)`, `close(...)`. Useful for understanding what a program is actually doing.

## 44. Example strace Flow
```c
int fd = open("test.txt", O_RDONLY);
char buf[100];
read(fd, buf, sizeof(buf));
close(fd);
```
You may observe: `openat(...)`, `read(...)`, `close(...)`. This helps connect: `C code --> Library --> System calls`.

## 45. System Call Tracing Mental Model
```
Application --> Library --> System Call --> Kernel
```
`strace` observes the system-call boundary: `Application --> strace --> System Calls`.

---

## 46. Important System Calls to Know
For senior Linux interviews, know these groups.

- **Process:** `fork()`, `clone()`, `clone3()`, `execve()`, `wait4()`, `_exit()`
- **Memory:** `mmap()`, `munmap()`, `mprotect()`, `madvise()`, `brk()`
- **Files:** `openat()`, `read()`, `write()`, `close()`, `pread64()`, `pwrite64()`, `fsync()`, `statx()`
- **Networking:** `socket()`, `bind()`, `listen()`, `accept()`, `connect()`, `sendto()`, `recvfrom()`, `setsockopt()`
- **Multiplexing:** `poll()`, `ppoll()`, `epoll_create1()`, `epoll_ctl()`, `epoll_wait()`
- **Synchronization / IPC:** `futex()`, event-related mechanisms, shared memory mechanisms
- **Device:** `ioctl()`, `mmap()`

---

## 47. Important Interview Question: What happens when you call read()?
A strong answer:
```
Application --> read() wrapper --> System-call entry --> Kernel --> Validate arguments
--> Find file object from fd --> Invoke appropriate file operation --> Filesystem/device/socket
--> Obtain data --> Copy data to user buffer --> Return count/error --> User space
```

## 48. Important Interview Question: Does every function call cause a system call?
No. `strlen()`, `memcpy()`, `strcmp()` can execute entirely in user space, but `read()`, `write()`, `mmap()` normally require kernel interaction.

## 49. Important Interview Question: Does malloc() cause a system call every time?
No — the allocator normally manages a pool/arena of memory in user space. It may request more memory from the kernel using mechanisms such as `mmap()`/`brk()` only when necessary:
```
malloc() --- existing allocator memory --- possibly kernel request
```

## 50. Important Interview Question: Does a system call always cause a context switch?
No. A system call causes a transition between user and kernel execution. A context switch happens when the scheduler switches execution from one task to another. A blocking syscall may cause the current task to sleep and therefore lead to a context switch.

## 51. Important Interview Question: Why can't user space directly access kernel memory?
Because the CPU's privilege and memory-protection mechanisms prevent normal user-mode access to protected kernel mappings — this protects the kernel, other processes, system state, and hardware control.

## 52. Important Interview Question: What is the difference between read() and mmap()?
`read()` explicitly transfers data into a user buffer: `File --> Kernel --> User Buffer`. With `mmap()`: `File --> Memory Mapping --> User Virtual Address` — the application accesses the mapped region using normal memory operations.

## 53. Important Interview Question: Why is mmap() often useful for large data?
It can avoid explicit repeated `read()`/`write()` calls and allows the application to access file contents through memory mappings, integrating naturally with Virtual Memory, Page Faults, and Page Cache.

## 54. Important Interview Question: What is ioctl()?
A general-purpose interface for device- or subsystem-specific control operations that don't fit naturally into standard operations such as `read()` and `write()`.

## 55. Important Interview Question: What is vDSO?
A kernel-provided shared object mapped into user processes that allows certain operations to be performed efficiently in user space using kernel-maintained information, avoiding a full syscall when possible.

---

## 56. Senior Interview Mental Model
Memorize this:
```
             USER SPACE
                  |
          C Library / Wrapper
                  |
          System Call ABI
                  |
             CPU Entry
                  |
             KERNEL SPACE
        +---------+---------+
        |         |         |
      VFS      Memory    Network
        |         |         |
    Filesystem   MM       TCP/IP
        |                   |
     Storage                NIC
```

---

## 57. One Complete Example
```c
int fd = open("data.txt", O_RDONLY);
char buf[4096];
ssize_t n = read(fd, buf, sizeof(buf));
close(fd);
```
Mental execution:
```
open() --> System Call --> VFS --> Path Resolution --> dentry + inode --> struct file --> fd returned
```
```
read() --> System Call --> fd -> struct file --> Filesystem --> Page Cache
   +-- Hit -> data
   +-- Miss -> storage
--> copy_to_user() --> buf
```
```
close() --> System Call --> Release file reference
```
This single example connects: System Calls, VFS, File Descriptors, Page Cache, Memory, Filesystem, Storage.

---

## 58. High-Priority Topics

**Must Know:** User mode vs Kernel mode, System call, System-call ABI, syscall instruction, System-call arguments, System-call return value, `read()`, `write()`, `openat()`, `close()`, `mmap()`, `fork()`, `execve()`, `ioctl()`, `socket()`, File descriptors, `copy_to_user()`, `copy_from_user()`, `strace`

**Should Know:** Blocking vs nonblocking, `poll()`, `epoll()`, vDSO, System call overhead, Context switch vs syscall, System call vs interrupt, System call vs exception

**Deep Dive:** Architecture-specific syscall entry, Kernel entry/exit paths, Syscall dispatch internals, `futex()`, io_uring, seccomp, restartable syscalls, syscall tracing

---

## 59. Quick Revision
```
User --syscall--> Kernel --validate--> Subsystem --perform operation--> Return value --> User
```
File: `fd --> struct file --> dentry --> inode --> filesystem`
Memory: `mmap() --> Virtual Address --> Page Tables --> Physical Memory`
Process: `fork() --> COW --> child`
Program replacement: `execve() --> new process image`
Device: `ioctl() --> Driver --> Hardware`
Networking: `socket() --> Socket Layer --> TCP/IP --> NIC`

---

## 60. Chapter Summary
The most important concept: **a system call is the controlled gateway from user space into the Linux kernel.**

The complete mental model:
```
Application --> C Library/Wrapper --> System Call ABI --> CPU Privilege Transition
--> Kernel Entry --> System Call Dispatcher --> Kernel Subsystem
       +---- VFS
       +---- Memory Management
       +---- Process Management
       +---- Networking
       +---- Device Drivers
--> Return to User Space
```
For senior interviews, the goal is not to memorize hundreds of syscall names. You should be able to take a simple C/C++ operation such as `read()`, `malloc()`, `mmap()`, `fork()`, `socket()`, `ioctl()` and explain **what crosses the user/kernel boundary, what the kernel does, and how the operation reaches the relevant subsystem**.

---
 
