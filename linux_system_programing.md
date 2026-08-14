# Linux System Programming & Kernel Internals

------------------------------------------------------------------------

# 1. Linux Architecture

``` text
+-----------------------------+
|       User Application      |
+-----------------------------+
              |
              v
+-----------------------------+
|       C Library / libc      |
|  printf, malloc, pthreads   |
+-----------------------------+
              |
              v
+-----------------------------+
|        System Calls         |
| read open fork mmap socket  |
+-----------------------------+
              |
              v
+-----------------------------+
|          Linux Kernel       |
|                             |
| Process Management          |
| Memory Management           |
| VFS / Filesystems            |
| Networking                  |
| IPC                         |
| Device Drivers              |
+-----------------------------+
              |
              v
+-----------------------------+
|          Hardware           |
+-----------------------------+
```

## Important distinction

### Linux System Programming

Deals mainly with:

``` text
Application
    ↓
POSIX / libc
    ↓
System calls
    ↓
Kernel services
```

Examples:

-   `fork()`
-   `exec()`
-   `open()`
-   `read()`
-   `write()`
-   `mmap()`
-   `pthread_create()`
-   `socket()`
-   `epoll()`

### Linux Kernel Internals

Deals with what happens **inside the kernel**:

-   `task_struct`
-   scheduler
-   virtual memory
-   page tables
-   VFS
-   inode
-   dentry
-   page cache
-   block layer
-   device drivers
-   kernel synchronization
-   system-call implementation

------------------------------------------------------------------------

# 2. System Calls

A system call is the controlled interface between user space and kernel
space.

Example:

``` c
read(fd, buffer, size);
```

Conceptually:

``` text
User program
     |
     | read()
     v
libc
     |
     v
system call
     |
     v
Kernel
     |
     v
VFS / filesystem
```

## Why system calls are required?

User programs cannot directly perform privileged operations.

Examples:

-   accessing hardware
-   creating processes
-   changing address spaces
-   accessing kernel-managed files
-   networking
-   memory mapping

The kernel provides controlled interfaces through system calls.

------------------------------------------------------------------------

# 3. User Space vs Kernel Space

``` text
User Space
--------------------------------
Application
Libraries
Heap
Stack
Shared libraries
--------------------------------
              |
           syscall
              |
--------------------------------
Kernel Space
--------------------------------
Scheduler
Memory Manager
VFS
Filesystem
Networking
Drivers
IPC
--------------------------------
```

## Important interview point

A system call usually causes a transition from:

``` text
User Mode
   ↓
Kernel Mode
   ↓
User Mode
```

This is different from a normal function call.

------------------------------------------------------------------------

# 4. File Descriptors

A file descriptor (FD) is a small integer used by a process to refer to
an open file/resource.

Standard descriptors:

``` text
0 → stdin
1 → stdout
2 → stderr
```

Example:

``` c
#include <unistd.h>

int main()
{
    write(1, "Hello\n", 6);
    return 0;
}
```

Here:

``` text
1 → stdout
```

------------------------------------------------------------------------

# 5. open()

``` c
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
    int fd = open("data.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    write(fd, "Hello\n", 6);

    close(fd);

    return 0;
}
```

## Important

``` text
open()
  ↓
returns file descriptor
  ↓
fd
  ↓
read/write/close
```

------------------------------------------------------------------------

# 6. File Descriptor vs Open File

Important senior interview concept.

A process has:

``` text
Process
  |
  v
File Descriptor Table
  |
  +---- fd 0
  +---- fd 1
  +---- fd 2
  +---- fd 3
             |
             v
       Open File Description
             |
             +---- file offset
             +---- status flags
             |
             v
           inode
```

Linux internally uses structures such as:

``` text
struct files_struct
struct fdtable
struct file
struct inode
struct dentry
```

Conceptually:

``` text
fd
 ↓
struct file
 ↓
dentry
 ↓
inode
 ↓
filesystem
```

This is a very important bridge between **system programming and VFS
internals**.

------------------------------------------------------------------------

# 7. read()

``` c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    int fd = open("data.txt", O_RDONLY);

    if (fd < 0)
    {
        perror("open");
        return 1;
    }

    char buffer[128];

    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);

    if (n < 0)
    {
        perror("read");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';

    printf("Read: %s", buffer);

    close(fd);

    return 0;
}
```

## Return value

``` text
> 0  → number of bytes read
0    → EOF
< 0  → error
```

------------------------------------------------------------------------

# 8. write()

``` c
ssize_t n = write(fd, buffer, size);
```

Return:

``` text
> 0 → bytes written
< 0 → error
```

Important:

A successful `write()` does not necessarily mean the data is permanently
stored on physical storage.

Data may go through:

``` text
Application
    ↓
Kernel
    ↓
Page Cache
    ↓
Filesystem
    ↓
Block Layer
    ↓
Driver
    ↓
Storage
```

------------------------------------------------------------------------

# 9. lseek()

Used to change the file offset.

``` c
off_t pos = lseek(fd, 0, SEEK_SET);
```

Common:

``` text
SEEK_SET → beginning
SEEK_CUR → current position
SEEK_END → end
```

Example:

``` c
lseek(fd, 10, SEEK_SET);
```

Moves the file offset to byte 10.

------------------------------------------------------------------------

# 10. dup() and dup2()

Used to duplicate file descriptors.

``` c
int newfd = dup(fd);
```

Typical use:

``` text
stdout
  ↓
dup2(file_fd, STDOUT_FILENO)
  ↓
printf()
  ↓
file
```

Example:

``` c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    int fd = open("output.txt",
                  O_CREAT | O_WRONLY | O_TRUNC,
                  0644);

    dup2(fd, STDOUT_FILENO);

    printf("This goes to file\n");

    close(fd);

    return 0;
}
```

------------------------------------------------------------------------

# 11. fork()

`fork()` creates a new process.

``` c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        printf("Child: PID = %d\n", getpid());
    }
    else
    {
        printf("Parent: PID = %d\n", getpid());

        wait(NULL);
    }

    return 0;
}
```

## Return value

``` text
pid < 0 → failure

pid == 0 → child

pid > 0 → parent receives child PID
```

------------------------------------------------------------------------

# 12. fork() and Copy-on-Write

Important interview question:

> Does fork() immediately copy the entire process memory?

**No.**

Linux uses **Copy-on-Write (COW)**.

Initially:

``` text
Parent
  |
  +---- page A
  +---- page B
  +---- page C

Child
  |
  +---- page A
  +---- page B
  +---- page C
```

Physical pages can initially be shared.

If child modifies a page:

``` text
Before:

Parent ----+
           |
           +---- Physical Page

Child -----+


After child writes:

Parent ------ Physical Page A

Child ------- Physical Page B
```

This reduces the cost of `fork()`.

------------------------------------------------------------------------

# 13. fork() + exec()

Very common Linux process model:

``` text
fork()
   ↓
new process
   ↓
exec()
   ↓
new program
```

Example:

``` c
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execl("/bin/ls", "ls", "-l", NULL);

        perror("execl");
    }
    else if (pid > 0)
    {
        wait(NULL);
    }

    return 0;
}
```

## Key point

`exec()` does **not** create a new process.

It replaces the current process image.

------------------------------------------------------------------------

# 14. Process Memory Layout

Typical process:

``` text
High Address
+------------------+
| Stack            |
| ↓                |
+------------------+
|                  |
| mmap/shared libs |
|                  |
+------------------+
| Heap             |
| ↑                |
+------------------+
| BSS              |
+------------------+
| Data             |
+------------------+
| Text / Code      |
+------------------+
Low Address
```

Important areas:

-   text
-   data
-   BSS
-   heap
-   stack
-   shared libraries
-   memory mappings

------------------------------------------------------------------------

# 15. Process vs Thread

## Process

Has its own:

-   virtual address space
-   page tables
-   resources
-   process context

## Thread

Threads in the same process share:

-   code
-   global data
-   heap
-   address space
-   file descriptors

Each thread has its own:

-   stack
-   registers
-   program counter
-   thread-local storage

``` text
Process
 |
 +---- Thread 1
 |
 +---- Thread 2
 |
 +---- Thread 3

Shared:
- Code
- Heap
- Global data
- Address space
- FD table
```

------------------------------------------------------------------------

# 16. pthread_create()

``` c
#include <pthread.h>
#include <stdio.h>

void* worker(void* arg)
{
    printf("Worker thread\n");
    return NULL;
}

int main()
{
    pthread_t thread;

    pthread_create(&thread, NULL, worker, NULL);

    pthread_join(thread, NULL);

    return 0;
}
```

Compile:

``` bash
gcc program.c -pthread
```

------------------------------------------------------------------------

# 17. Race Condition

Example:

``` c
int counter = 0;

void* worker(void* arg)
{
    for (int i = 0; i < 100000; i++)
        counter++;

    return NULL;
}
```

With multiple threads:

``` text
Thread 1: read counter
Thread 2: read counter
Thread 1: increment
Thread 2: increment
Thread 1: write
Thread 2: write
```

Updates can be lost.

------------------------------------------------------------------------

# 18. Mutex

``` c
#include <pthread.h>
#include <stdio.h>

int counter = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* worker(void* arg)
{
    for (int i = 0; i < 100000; i++)
    {
        pthread_mutex_lock(&lock);

        counter++;

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main()
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("counter = %d\n", counter);

    return 0;
}
```

## Mutex purpose

Provides mutual exclusion:

``` text
Thread 1
   ↓
 lock
   ↓
critical section
   ↓
unlock

Thread 2
   ↓
wait
```

------------------------------------------------------------------------

# 19. Condition Variable

Used when a thread must wait for a condition.

Typical producer/consumer:

``` text
Producer
   ↓
produce data
   ↓
signal
   ↓
Consumer wakes
```

Example:

``` c
pthread_mutex_lock(&lock);

while (!data_ready)
{
    pthread_cond_wait(&cond, &lock);
}

consume_data();

pthread_mutex_unlock(&lock);
```

Important:

Use `while`, not `if`.

Because the condition must be checked again after waking.

------------------------------------------------------------------------

# 20. Signals

Signals are asynchronous notifications sent to a process/thread.

Examples:

``` text
SIGINT
SIGTERM
SIGKILL
SIGSEGV
SIGCHLD
```

Basic example:

``` c
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void handler(int sig)
{
    printf("Signal received: %d\n", sig);
}

int main()
{
    signal(SIGINT, handler);

    while (1)
        sleep(1);

    return 0;
}
```

Press:

``` text
Ctrl+C
```

to generate `SIGINT`.

------------------------------------------------------------------------

# 21. SIGKILL vs SIGTERM

``` text
SIGTERM
    ↓
Can be handled
Can be ignored
Can be used for graceful shutdown

SIGKILL
    ↓
Cannot be caught
Cannot be ignored
Cannot be blocked
Kernel terminates process
```

------------------------------------------------------------------------

# 22. Pipe

Pipe provides unidirectional IPC.

``` text
Process A
   |
   | write()
   v
+-------+
| Pipe  |
+-------+
   |
   | read()
   v
Process B
```

Example:

``` c
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main()
{
    int fd[2];

    pipe(fd);

    write(fd[1], "Hello", 5);

    char buffer[10] = {0};

    read(fd[0], buffer, 5);

    printf("%s\n", buffer);

    close(fd[0]);
    close(fd[1]);

    return 0;
}
```

``` text
fd[0] → read
fd[1] → write
```

------------------------------------------------------------------------

# 23. Pipe with fork()

Common pattern:

``` text
Parent
   |
   | write
   v
 Pipe
   |
   | read
   v
Child
```

This is a classic interview coding problem.

------------------------------------------------------------------------

# 24. FIFO

FIFO = named pipe.

Unlike an anonymous pipe, a FIFO exists in the filesystem namespace.

Create:

``` bash
mkfifo mypipe
```

Writer:

``` bash
echo "hello" > mypipe
```

Reader:

``` bash
cat < mypipe
```

------------------------------------------------------------------------

# 25. Shared Memory

Processes can communicate through shared memory.

``` text
Process A
    |
    +--------+
             |
        Shared Memory
             |
    +--------+
    |
Process B
```

Advantages:

-   Very fast IPC
-   Avoids unnecessary copying between processes

But synchronization is still required.

Typical combination:

``` text
Shared Memory
     +
Semaphore/Mutex
```

------------------------------------------------------------------------

# 26. mmap()

Maps memory into a process address space.

Common uses:

-   file mapping
-   shared memory
-   anonymous memory
-   device memory

Example:

``` c
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    int fd = open("data.txt", O_RDWR);

    char* data = mmap(NULL,
                      4096,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED,
                      fd,
                      0);

    if (data == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    data[0] = 'X';

    munmap(data, 4096);

    close(fd);

    return 0;
}
```

------------------------------------------------------------------------

# 27. mmap() and Kernel Internals

Important relationship:

``` text
Application
    |
    | mmap()
    v
Kernel
    |
    v
Process virtual address space
    |
    v
VMA
    |
    v
Page tables
    |
    v
Physical pages
```

Modern Linux internally uses structures such as:

``` text
mm_struct
vm_area_struct
page tables
```

------------------------------------------------------------------------

# 28. Virtual Memory

Each process sees its own virtual address space.

``` text
Process A              Process B

Virtual Address        Virtual Address
      |                      |
      v                      v
 Page Tables             Page Tables
      |                      |
      v                      v
Physical Memory
```

Advantages:

-   process isolation
-   memory protection
-   virtual address abstraction
-   demand paging
-   shared memory
-   memory mapping

------------------------------------------------------------------------

# 29. Page Fault

A page fault occurs when the CPU accesses a virtual address whose
translation/page is not currently available as required.

Typical flow:

``` text
CPU accesses virtual address
          ↓
       MMU
          ↓
   Page table lookup
          ↓
      Page fault
          ↓
       Kernel
          ↓
Find/create required page
          ↓
Update page table
          ↓
Return to process
```

Important types/concepts:

-   minor fault
-   major fault
-   demand paging
-   copy-on-write fault

------------------------------------------------------------------------

# 30. Page Cache

File I/O often interacts with the page cache.

``` text
read()
  ↓
VFS
  ↓
Page Cache
  ↓
Filesystem
  ↓
Block Layer
  ↓
Driver
  ↓
Disk
```

If data is already cached:

``` text
read()
  ↓
Page Cache HIT
  ↓
Return data
```

Otherwise:

``` text
read()
  ↓
Page Cache MISS
  ↓
Filesystem
  ↓
Storage
  ↓
Page Cache populated
  ↓
Application
```

This is an important connection between **Robert Love system
programming** and **Linux kernel/storage internals**.

------------------------------------------------------------------------

# 31. VFS

VFS = Virtual File System.

Provides a common interface to different filesystems.

Examples:

``` text
ext4
XFS
Btrfs
NFS
tmpfs
```

Application sees:

``` text
open()
read()
write()
close()
```

VFS hides filesystem-specific implementation.

------------------------------------------------------------------------

# 32. Path Lookup

Example:

``` c
open("/home/user/file.txt", O_RDONLY);
```

Conceptually:

``` text
"/home/user/file.txt"
          ↓
       Path lookup
          ↓
       dentry
          ↓
       inode
          ↓
    filesystem
```

Important structures:

``` text
struct file
struct dentry
struct inode
```

------------------------------------------------------------------------

# 33. struct file

For an open file, Linux maintains a `struct file`.

Important conceptual fields:

``` text
struct file
{
    f_op
    f_inode
    f_pos
    f_flags
    f_mode
    f_path
    private_data
}
```

### Important fields

``` text
f_op
    ↓
file operations

f_inode
    ↓
inode associated with file

f_pos
    ↓
current file offset

f_flags
    ↓
open/status flags

f_path
    ↓
path information

private_data
    ↓
filesystem/device-specific data
```

------------------------------------------------------------------------

# 34. File Read Flow

``` text
Application
    |
    | read(fd, buffer, size)
    v
libc
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
f_op->read_iter()
    |
    v
Page Cache
    |
    v
Filesystem
    |
    v
Block Layer
    |
    v
Device Driver
    |
    v
Storage
```

This is a very useful **senior Linux interview flow**.

------------------------------------------------------------------------

# 35. File Write Flow

``` text
Application
     |
     | write()
     v
System Call
     |
     v
VFS
     |
     v
Filesystem
     |
     v
Page Cache
     |
     v
Dirty Pages
     |
     v
Writeback
     |
     v
Block Layer
     |
     v
Driver
     |
     v
Storage
```

Important:

`write()` may return before physical storage is updated.

For stronger durability requirements:

``` text
fsync()
fdatasync()
```

may be required.

------------------------------------------------------------------------

# 36. Socket Programming

Socket is the standard interface for network communication.

TCP server flow:

``` text
socket()
   ↓
bind()
   ↓
listen()
   ↓
accept()
   ↓
recv()/read()
   ↓
send()/write()
   ↓
close()
```

------------------------------------------------------------------------

# 37. TCP Server Skeleton

``` c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {0};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd,
         (struct sockaddr*)&addr,
         sizeof(addr));

    listen(server_fd, 10);

    int client_fd = accept(server_fd, NULL, NULL);

    char buffer[128];

    int n = read(client_fd, buffer, sizeof(buffer));

    if (n > 0)
        write(client_fd, buffer, n);

    close(client_fd);
    close(server_fd);

    return 0;
}
```

------------------------------------------------------------------------

# 38. Blocking vs Non-Blocking I/O

## Blocking

``` text
read()
  ↓
No data
  ↓
Thread sleeps
  ↓
Data arrives
  ↓
Thread wakes
```

## Non-blocking

``` text
read()
  ↓
No data
  ↓
Returns immediately
```

Set non-blocking:

``` c
fcntl(fd, F_SETFL, O_NONBLOCK);
```

------------------------------------------------------------------------

# 39. select()

Allows monitoring multiple file descriptors.

``` text
             +--- fd1
             |
select() ----+--- fd2
             |
             +--- fd3
```

Useful for I/O multiplexing.

Limitations:

-   FD-set limitations
-   scanning overhead
-   less scalable for large numbers of FDs

------------------------------------------------------------------------

# 40. poll()

Similar purpose to `select()`.

``` c
struct pollfd fds[2];

fds[0].fd = fd;
fds[0].events = POLLIN;

poll(fds, 1, 1000);
```

Advantages over `select()` include avoiding `fd_set` size limitations.

------------------------------------------------------------------------

# 41. epoll()

Linux-specific scalable I/O event mechanism.

Typical flow:

``` text
epoll_create1()
       ↓
epoll_ctl()
       ↓
epoll_wait()
       ↓
events
```

Example:

``` c
int epfd = epoll_create1(0);

struct epoll_event event;

event.events = EPOLLIN;
event.data.fd = server_fd;

epoll_ctl(epfd,
          EPOLL_CTL_ADD,
          server_fd,
          &event);

struct epoll_event events[10];

int n = epoll_wait(epfd,
                   events,
                   10,
                   -1);
```

------------------------------------------------------------------------

# 42. select vs poll vs epoll

  Feature                select        poll        epoll
  ---------------------- ------------- ----------- -------------
  Standard POSIX         Yes           Yes         No
  Linux specific         No            No          Yes
  Large FD scalability   Poor          Better      Better
  FD scanning            Yes           Yes         Event-based
  Common Linux servers   Less common   Sometimes   Very common

Senior interview answer:

> `epoll` is generally preferred for large-scale Linux event-driven
> applications because it provides scalable event notification without
> repeatedly scanning the entire FD set.

------------------------------------------------------------------------

# 43. errno

Many system calls indicate failure using:

``` text
-1
```

and set:

``` c
errno
```

Example:

``` c
int fd = open("missing.txt", O_RDONLY);

if (fd < 0)
{
    perror("open");
}
```

Or:

``` c
printf("%s\n", strerror(errno));
```

Important:

Do not assume `errno` is meaningful after a successful call.

------------------------------------------------------------------------

# 44. Zombie Process

A child becomes a zombie when:

``` text
Child terminates
      ↓
Exit status retained
      ↓
Parent has not called wait()
```

``` text
Parent
   |
   +---- Zombie Child
```

Solution:

``` c
wait(NULL);
```

or:

``` c
waitpid(child_pid, &status, 0);
```

------------------------------------------------------------------------

# 45. Orphan Process

An orphan occurs when:

``` text
Parent terminates
      ↓
Child still running
```

The child is adopted/re-parented by an appropriate system process.

Do not confuse:

``` text
Zombie → child terminated, parent hasn't collected status

Orphan → parent terminated, child still running
```

------------------------------------------------------------------------

# 46. wait() vs waitpid()

``` c
wait(NULL);
```

Waits for a child.

``` c
waitpid(pid, &status, 0);
```

Waits for a specific child.

Useful options include:

``` text
WNOHANG
```

which allows non-blocking checking.

------------------------------------------------------------------------

# 47. Process Creation Flow

Typical shell execution:

``` text
User enters command
       ↓
Shell
       ↓
fork()
       ↓
Child
       ↓
exec()
       ↓
Program starts
       ↓
Parent
       ↓
wait()
```

This is one of the most important Linux process flows.

------------------------------------------------------------------------

# 48. Memory Allocation

User-level:

``` c
malloc()
calloc()
realloc()
free()
```

Example:

``` c
int *p = malloc(10 * sizeof(int));

if (!p)
    return 1;

free(p);
```

Conceptually:

``` text
malloc()
   ↓
libc allocator
   ↓
brk()/mmap()
   ↓
Kernel
```

Important:

`malloc()` is **not itself a system call**.

The allocator may obtain memory from the kernel using mechanisms such
as:

``` text
brk()
mmap()
```

------------------------------------------------------------------------

# 49. brk() vs mmap()

Historically:

``` text
brk()/sbrk()
    ↓
heap expansion
```

`mmap()`:

``` text
anonymous mappings
file mappings
shared memory
large allocations
```

Modern allocators can use both.

------------------------------------------------------------------------

# 50. /proc

`/proc` is a virtual filesystem exposing kernel/process information.

Examples:

``` bash
cat /proc/cpuinfo
cat /proc/meminfo
cat /proc/mounts
cat /proc/<pid>/status
cat /proc/<pid>/maps
```

Useful for debugging:

``` text
/proc/<pid>/maps
/proc/<pid>/status
/proc/<pid>/fd/
/proc/<pid>/smaps
```

------------------------------------------------------------------------

# 51. /sys

`/sys` exposes kernel/device model information through sysfs.

Useful areas:

``` text
/sys/class
/sys/devices
/sys/block
/sys/bus
/sys/kernel
```

Difference:

``` text
/proc → mainly processes/system/kernel information

/sys  → mainly devices/kernel object model
```

------------------------------------------------------------------------

# 52. Device Driver Connection

Application:

``` c
read(fd, buffer, size);
```

For a device:

``` text
Application
     ↓
read()
     ↓
VFS
     ↓
struct file
     ↓
file_operations
     ↓
Driver read operation
     ↓
Hardware
```

A device driver may provide operations such as:

``` text
open
read
write
ioctl
poll
mmap
release
```

------------------------------------------------------------------------

# 53. ioctl()

Used for device-specific control operations.

``` c
ioctl(fd, COMMAND, argument);
```

Typical use:

``` text
Application
    ↓
ioctl()
    ↓
Kernel
    ↓
Driver
    ↓
Hardware
```

Unlike `read()`/`write()`, `ioctl()` is commonly used for
control/configuration operations.

------------------------------------------------------------------------

# 54. Signal-Safe Programming

Important senior-level point:

Not every libc function is safe to call from a signal handler.

Signal handlers should use async-signal-safe functions.

Example commonly safe:

``` c
write()
```

Avoid doing complex operations such as:

``` c
printf()
malloc()
free()
```

inside a signal handler.

------------------------------------------------------------------------

# 55. Synchronization Mechanisms

Linux/POSIX provides several mechanisms:

``` text
Mutex
   ↓
Protect critical section

Condition Variable
   ↓
Wait for condition

Semaphore
   ↓
Control access/count resources

Read-Write Lock
   ↓
Multiple readers / single writer

Atomic Operations
   ↓
Lock-free/small shared state operations
```

------------------------------------------------------------------------

# 56. Mutex vs Semaphore

## Mutex

Usually represents ownership:

``` text
lock
critical section
unlock
```

## Semaphore

Represents a count/resource availability.

Example:

``` text
Semaphore = 3

Thread A → acquire → 2
Thread B → acquire → 1
Thread C → acquire → 0
Thread D → waits
```

------------------------------------------------------------------------

# 57. Deadlock

Four classic conditions:

``` text
1. Mutual exclusion
2. Hold and wait
3. No preemption
4. Circular wait
```

Example:

``` text
Thread 1:
lock(A)
lock(B)

Thread 2:
lock(B)
lock(A)
```

Possible deadlock:

``` text
Thread 1 holds A → waits B

Thread 2 holds B → waits A
```

Solution:

Always acquire locks in a consistent order.

------------------------------------------------------------------------

# 58. Atomic Operation

Example:

``` c
#include <stdatomic.h>

atomic_int counter = 0;

atomic_fetch_add(&counter, 1);
```

Atomic operations avoid certain race conditions without a traditional
mutex.

But:

> Atomic does not automatically make an entire multi-step algorithm
> thread-safe.

------------------------------------------------------------------------

# 59. Memory Ordering

For advanced C++/Linux interviews:

``` text
relaxed
acquire
release
acq_rel
seq_cst
```

Basic concept:

``` text
Release
   ↓
publish data

Acquire
   ↓
observe published data
```

C++ example:

``` cpp
std::atomic<bool> ready{false};

data = 100;

ready.store(true, std::memory_order_release);
```

Another thread:

``` cpp
if (ready.load(std::memory_order_acquire))
{
    // data is visible
}
```

------------------------------------------------------------------------

# 60. TCP vs UDP

## TCP

``` text
Connection-oriented
Reliable
Ordered
Flow control
Congestion control
```

## UDP

``` text
Connectionless
No delivery guarantee
No ordering guarantee
Lower overhead
```

Typical examples:

``` text
TCP → HTTP/HTTPS, SSH
UDP → DNS, streaming/real-time applications
```

------------------------------------------------------------------------

# 61. Socket Address Flow

Server:

``` text
socket()
   ↓
bind()
   ↓
listen()
   ↓
accept()
   ↓
recv/send
   ↓
close()
```

Client:

``` text
socket()
   ↓
connect()
   ↓
send/recv
   ↓
close()
```

------------------------------------------------------------------------

# 62. Important System Calls to Know

For senior interviews, know the purpose and usage of:

``` text
Process:
fork
vfork
clone
execve
wait
waitpid
_exit
exit

File:
open
openat
read
write
close
lseek
pread
pwrite
dup
dup2
fcntl
fsync
fdatasync
stat
fstat

Memory:
mmap
munmap
mprotect
madvise
brk

Signals:
sigaction
sigprocmask
kill
raise
sigsuspend

IPC:
pipe
pipe2
shm
mmap
msg queues
semaphores

Networking:
socket
bind
listen
accept
connect
send
recv
sendmsg
recvmsg
setsockopt

I/O multiplexing:
select
poll
epoll_create1
epoll_ctl
epoll_wait
```

------------------------------------------------------------------------

# 63. Important POSIX APIs

Know these categories:

``` text
pthread_create()
pthread_join()

pthread_mutex_lock()
pthread_mutex_unlock()

pthread_cond_wait()
pthread_cond_signal()

sem_wait()
sem_post()

sigaction()

clock_gettime()

nanosleep()

getpid()
getppid()
```

------------------------------------------------------------------------

# 64. System Call vs Library Function

Example:

``` text
printf()
```

is a library function.

It may eventually use:

``` text
write()
```

which is a system call.

Similarly:

``` text
malloc()
```

is a library allocator.

It may use:

``` text
mmap()
brk()
```

to obtain memory from the kernel.

Important interview distinction:

``` text
Library function ≠ necessarily system call
```

------------------------------------------------------------------------

# 65. Context Switch

A context switch occurs when CPU execution changes from one execution
context to another.

Example:

``` text
Thread A running
      ↓
interrupt/scheduler
      ↓
save A context
      ↓
load B context
      ↓
Thread B running
```

Context includes things such as:

``` text
Registers
Program Counter
Stack Pointer
Scheduling state
Address-space information
```

A process switch can involve address-space changes.

Threads within the same process may share the address space.

------------------------------------------------------------------------

# 66. Process Scheduling

Linux uses a scheduler to decide which runnable task executes.

Conceptually:

``` text
Runnable Tasks
      ↓
 Scheduler
      ↓
 CPU
```

Important concepts:

``` text
Runnable
Running
Sleeping
Blocked
Waiting
```

For modern Linux, know:

``` text
CFS
Completely Fair Scheduler
```

and be aware that newer Linux kernels also contain newer scheduling
infrastructure such as EEVDF.

For interviews, focus on:

-   runnable tasks
-   scheduling
-   preemption
-   context switch
-   priority
-   CPU affinity

------------------------------------------------------------------------

# 67. CPU Affinity

Restricts a process/thread to selected CPUs.

Command:

``` bash
taskset -c 0 ./program
```

Useful in:

-   real-time systems
-   performance tuning
-   NUMA systems
-   CPU isolation

------------------------------------------------------------------------

# 68. Nice Value

Controls process scheduling priority indirectly.

``` bash
nice -n 10 ./program
```

Lower nice value generally means higher scheduling priority.

Higher nice value means lower priority.

------------------------------------------------------------------------

# 69. Real-Time Scheduling

Important policies:

``` text
SCHED_FIFO
SCHED_RR
```

Real-time scheduling is important in embedded Linux.

Be careful:

Real-time scheduling does not automatically mean deterministic behavior.

Other factors include:

-   interrupts
-   kernel preemption
-   locking
-   page faults
-   CPU frequency
-   hardware
-   I/O

------------------------------------------------------------------------

# 70. Blocking System Call

Example:

``` c
read(fd, buffer, size);
```

If data isn't available and FD is blocking:

``` text
Process
   ↓
read()
   ↓
sleep/block
   ↓
data becomes available
   ↓
wake
   ↓
read returns
```

The scheduler can run another task while this task is blocked.

------------------------------------------------------------------------

# 71. Important Interview Concept: Blocking Doesn't Mean CPU Busy-Wait

Bad:

``` c
while (!data_ready)
{
    // continuously check
}
```

This can waste CPU.

Blocking:

``` text
wait
 ↓
sleep
 ↓
CPU runs another task
 ↓
event
 ↓
wake
```

This is one reason blocking primitives are useful.

------------------------------------------------------------------------

# 72. epoll Event-Driven Architecture

Large server:

``` text
                +---- Client 1
                |
epoll_wait() ---+---- Client 2
                |
                +---- Client 3
                |
                +---- Client N
```

Instead of creating one blocking thread per connection, an event loop
can handle many connections.

Typical architecture:

``` text
Event Loop
    ↓
epoll_wait()
    ↓
Events
    ↓
Read/Write
    ↓
Application processing
```

------------------------------------------------------------------------

# 73. Common Interview Question

## What happens when you call read()?

Short answer:

``` text
Application
    ↓
read(fd,...)
    ↓
system call
    ↓
VFS
    ↓
struct file
    ↓
filesystem
    ↓
page cache
    ↓
possibly block I/O
    ↓
driver
    ↓
device
```

If data is already in page cache:

``` text
read()
  ↓
page cache hit
  ↓
copy data to user buffer
```

------------------------------------------------------------------------

# 74. Common Interview Question

## What happens when you call open()?

Conceptually:

``` text
open(path)
   ↓
system call
   ↓
path lookup
   ↓
dentry/inode
   ↓
filesystem
   ↓
struct file created
   ↓
FD allocated
   ↓
FD returned to application
```

------------------------------------------------------------------------

# 75. Common Interview Question

## What happens during fork()?

``` text
fork()
   ↓
Kernel creates child task
   ↓
Address-space information duplicated
   ↓
Pages initially shared using COW
   ↓
FDs inherited
   ↓
Parent and child continue execution
```

------------------------------------------------------------------------

# 76. Common Interview Question

## fork() vs clone()

`clone()` provides more control over what is shared.

Conceptually:

``` text
fork()
    ↓
new process with largely separate resources

clone()
    ↓
selectively share resources
```

Linux threads are implemented using the `clone` mechanism.

Resources that can be shared include:

``` text
Address space
File descriptors
Filesystem information
Signal handlers
```

------------------------------------------------------------------------

# 77. fork() vs pthread_create()

``` text
fork()
  ↓
new process
  ↓
separate address space

pthread_create()
  ↓
new thread
  ↓
same process address space
```

Use process when isolation is important.

Use threads when shared memory and lower communication overhead are
useful.

------------------------------------------------------------------------

# 78. vfork()

`vfork()` is a specialized process creation mechanism.

Historically, the child temporarily shares the parent's address space
until it performs `exec()` or `_exit()`.

Because of its semantics, misuse can be dangerous.

For modern application programming:

``` text
fork()
+
exec()
```

is generally easier to reason about.

------------------------------------------------------------------------

# 79. exit() vs \_exit()

Important after `fork()`.

``` text
exit()
    ↓
libc cleanup
stdio flushing
atexit handlers

_exit()
    ↓
direct process termination
```

In a child after `fork()`, particularly before `exec()`, `_exit()` is
often safer if `exec()` fails, to avoid duplicating/incorrectly flushing
inherited stdio state.

------------------------------------------------------------------------

# 80. Copying Data Across User/Kernel Boundary

Kernel cannot blindly dereference arbitrary user pointers.

Conceptually Linux uses mechanisms such as:

``` text
copy_from_user()
copy_to_user()
```

Example driver concept:

``` text
User buffer
    ↓
copy_from_user()
    ↓
Kernel buffer
```

and:

``` text
Kernel buffer
    ↓
copy_to_user()
    ↓
User buffer
```

This is important when discussing system calls and device drivers.

------------------------------------------------------------------------

# 81. Kernel Stack vs User Stack

Each thread has:

``` text
User stack
    ↓
Application function calls

Kernel stack
    ↓
Used while executing kernel code for that task
```

System-call transition:

``` text
User execution
     ↓
system call
     ↓
Kernel execution
     ↓
kernel stack
     ↓
return
     ↓
User execution
```

------------------------------------------------------------------------

# 82. Interrupt vs System Call

## System Call

Explicitly requested by application.

``` text
Application
   ↓
syscall
   ↓
Kernel
```

## Interrupt

Usually generated asynchronously by hardware.

``` text
Hardware
   ↓
Interrupt
   ↓
CPU
   ↓
Kernel interrupt handler
```

------------------------------------------------------------------------

# 83. Interrupt vs Exception

### Interrupt

Usually asynchronous.

Example:

``` text
Network packet arrives
Disk completion
Timer interrupt
```

### Exception

Synchronous with instruction execution.

Examples:

``` text
Page fault
Divide by zero
Invalid instruction
```

------------------------------------------------------------------------

# 84. Bottom Half / Deferred Work

Linux should avoid doing excessive work in the immediate interrupt
context.

Conceptually:

``` text
Hardware interrupt
       ↓
Top half
       ↓
Schedule/defer work
       ↓
Bottom-half mechanism
       ↓
More processing
```

Modern Linux mechanisms include:

``` text
softirq
tasklet (legacy/deprecated direction)
workqueue
threaded IRQ
```

------------------------------------------------------------------------

# 85. Kernel Synchronization

Kernel code uses mechanisms such as:

``` text
spinlock
mutex
rwsem
completion
atomic operations
RCU
```

Important difference:

### Mutex

Can sleep.

### Spinlock

Typically used where sleeping is not allowed.

Concept:

``` text
Spinlock:

CPU
 ↓
lock unavailable
 ↓
spin
 ↓
retry
```

Therefore holding a spinlock for a long time is bad.

------------------------------------------------------------------------

# 86. Kernel Memory Allocation

Common kernel allocators:

``` text
kmalloc()
kzalloc()
vmalloc()
```

### kmalloc()

Physically contiguous memory for suitable allocations.

### vmalloc()

Virtually contiguous memory, but physical pages need not be contiguous.

### kzalloc()

Like `kmalloc()` but memory is zeroed.

------------------------------------------------------------------------

# 87. User malloc vs Kernel kmalloc

``` text
malloc()
    ↓
User-space allocator
    ↓
Kernel mechanisms such as mmap/brk
```

Whereas:

``` text
kmalloc()
    ↓
Kernel memory allocator
```

They operate in completely different contexts.

------------------------------------------------------------------------

# 88. Kernel Module Basics

A loadable kernel module can extend kernel functionality.

Example structure:

``` c
#include <linux/module.h>
#include <linux/kernel.h>

static int __init my_init(void)
{
    printk(KERN_INFO "Module loaded\n");
    return 0;
}

static void __exit my_exit(void)
{
    printk(KERN_INFO "Module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
```

Commands:

``` bash
insmod module.ko
lsmod
rmmod module
dmesg
```

------------------------------------------------------------------------

# 89. Kernel Module vs User Program

User program:

``` text
main()
  ↓
libc
  ↓
system calls
```

Kernel module:

``` text
module_init()
  ↓
kernel
```

Kernel module:

-   runs in kernel space
-   has kernel privileges
-   must not use normal user-space APIs
-   must be extremely careful with memory and synchronization

------------------------------------------------------------------------

# 90. Kernel Panic

A kernel panic occurs when the kernel encounters a fatal condition from
which it cannot safely continue.

Potential causes:

``` text
Invalid memory access
Kernel bug
Driver bug
Corrupted kernel state
Hardware problems
```

User-space segmentation fault:

``` text
One process dies
```

Kernel panic:

``` text
Kernel cannot safely continue
```

------------------------------------------------------------------------

# 91. Important Debugging Tools

Senior Linux interviews commonly expect:

``` text
gdb
strace
ltrace
perf
valgrind
addr2line
nm
objdump
readelf
ldd
pmap
top
htop
ps
vmstat
iostat
sar
dmesg
```

------------------------------------------------------------------------

# 92. strace

Shows system calls made by a process.

Example:

``` bash
strace ./program
```

Useful:

``` bash
strace -f ./program
```

`-f` follows child processes/threads as applicable.

Example output concept:

``` text
openat(...)
read(...)
write(...)
close(...)
```

Excellent tool for understanding:

``` text
Application → System Calls → Kernel
```

------------------------------------------------------------------------

# 93. ltrace

Shows library calls.

Conceptually:

``` text
ltrace
   ↓
library calls

strace
   ↓
system calls
```

------------------------------------------------------------------------

# 94. gdb

Used for debugging:

``` text
breakpoints
stack traces
variables
registers
threads
core dumps
```

Useful commands:

``` gdb
run
break main
next
step
continue
bt
info threads
thread apply all bt
print variable
```

------------------------------------------------------------------------

# 95. Core Dump

When a process crashes, a core dump can contain process state.

Useful for:

``` text
Segmentation faults
Abort
Unexpected crashes
```

Typical workflow:

``` text
Crash
 ↓
core file
 ↓
gdb executable core
 ↓
bt
```

------------------------------------------------------------------------

# 96. perf

Used for Linux performance analysis.

Examples:

``` bash
perf stat ./program
```

``` bash
perf record ./program
perf report
```

Can help analyze:

-   CPU cycles
-   instructions
-   cache misses
-   branches
-   hotspots

------------------------------------------------------------------------

# 97. Important Performance Concepts

When debugging a slow Linux application, consider:

``` text
CPU
Memory
Cache
Lock contention
Context switches
System calls
I/O
Network
Page faults
Disk latency
```

Do not immediately assume:

``` text
CPU is slow
```

Measure first.

------------------------------------------------------------------------

# 98. Common Senior Interview Questions

### Q1. Is `malloc()` a system call?

No.

`malloc()` is a user-space library allocator. It can request memory from
the kernel through mechanisms such as `brk()` and `mmap()`.

------------------------------------------------------------------------

### Q2. Does `fork()` copy all memory?

No.

Linux uses Copy-on-Write.

------------------------------------------------------------------------

### Q3. Does `exec()` create a process?

No.

It replaces the current process image.

------------------------------------------------------------------------

### Q4. What happens to file descriptors after fork?

The child inherits copies of the parent's file descriptors.

The corresponding descriptors refer to the same underlying open file
descriptions.

------------------------------------------------------------------------

### Q5. Process vs thread?

Process:

``` text
Separate address space
```

Thread:

``` text
Shared address space within process
```

------------------------------------------------------------------------

### Q6. Why use mutex?

To protect shared data from concurrent access.

------------------------------------------------------------------------

### Q7. Mutex vs spinlock?

Mutex can sleep.

Spinlock typically busy-waits and is used where sleeping is not
permitted.

------------------------------------------------------------------------

### Q8. What is a zombie?

A terminated child whose exit status has not yet been collected by the
parent.

------------------------------------------------------------------------

### Q9. What is an orphan?

A child whose parent has terminated while the child continues running.

------------------------------------------------------------------------

### Q10. What is VFS?

Virtual File System.

It provides a common abstraction/interface over different filesystems.

------------------------------------------------------------------------

### Q11. What is page cache?

Kernel cache used to cache file-backed data/pages, reducing repeated
storage access.

------------------------------------------------------------------------

### Q12. What is `epoll`?

Linux I/O event notification mechanism used to efficiently monitor many
file descriptors.

------------------------------------------------------------------------

### Q13. What is a system call?

Controlled interface through which user-space programs request services
from the kernel.

------------------------------------------------------------------------

### Q14. What is a context switch?

Switching CPU execution from one task/thread to another by
saving/restoring execution context.

------------------------------------------------------------------------

### Q15. Why are system calls expensive compared with normal function calls?

They involve a transition between user and kernel execution contexts and
associated validation/state-management overhead.

------------------------------------------------------------------------

# 99. Must-Know Coding Problems

Practice writing these without looking at notes:

## Process

``` text
1. fork()
2. fork() twice → calculate number of processes
3. fork + exec
4. Parent waits for child
5. Zombie process
6. Orphan process
```

## Threads

``` text
7. Create two threads
8. Race condition
9. Fix race using mutex
10. Producer-consumer using condition variable
11. Reader-writer synchronization
```

## IPC

``` text
12. Pipe
13. Pipe + fork
14. FIFO
15. Shared memory
```

## File I/O

``` text
16. open/read/write/close
17. dup2() output redirection
18. File copy using read/write
19. mmap file
```

## Networking

``` text
20. TCP server
21. TCP client
22. UDP server/client
23. epoll server
```

------------------------------------------------------------------------

# 100. Senior-Level Architecture Connections

You should be able to explain these flows from memory.

## Process

``` text
fork()
 ↓
task creation
 ↓
task_struct
 ↓
address-space relationship
 ↓
COW
 ↓
scheduler
```

## File

``` text
open()
 ↓
FD
 ↓
struct file
 ↓
dentry
 ↓
inode
 ↓
filesystem
```

## Read

``` text
read()
 ↓
syscall
 ↓
VFS
 ↓
page cache
 ↓
filesystem
 ↓
block layer
 ↓
driver
 ↓
device
```

## Write

``` text
write()
 ↓
VFS
 ↓
page cache
 ↓
dirty pages
 ↓
writeback
 ↓
filesystem
 ↓
block layer
 ↓
driver
 ↓
device
```

## Network

``` text
socket()
 ↓
socket layer
 ↓
TCP/IP stack
 ↓
network device
 ↓
driver
 ↓
NIC
```

## Thread

``` text
pthread_create()
 ↓
clone mechanism
 ↓
task_struct
 ↓
shared process resources
 ↓
scheduler
```

------------------------------------------------------------------------

# 101. Robert Love vs Linux Kernel Internals

## Robert Love / System Programming

Focus heavily on:

``` text
System calls
Processes
Threads
Signals
IPC
File I/O
Memory
Sockets
I/O multiplexing
POSIX APIs
```

## Kernel Internals

Need additional knowledge of:

``` text
task_struct
scheduler
run queues
context switching
mm_struct
VMA
page tables
page faults
page cache
VFS
dentry
inode
superblock
block layer
device model
drivers
interrupts
softirq
workqueues
kernel synchronization
kernel memory allocators
```

Therefore:

``` text
Robert Love
     +
Linux Kernel Internals
     +
VFS / Filesystem
     +
Storage
     +
Networking
     +
C/C++ Multithreading
     =
Strong Senior Linux Systems Preparation
```

------------------------------------------------------------------------

# 102. Final 1-Day Revision Cheat Sheet

## Processes

``` text
fork()  → create process
exec()  → replace process image
wait()  → collect child
_exit() → terminate immediately
```

## Threads

``` text
pthread_create()
pthread_join()
mutex
condition variable
semaphore
rwlock
```

## Files

``` text
open()
read()
write()
close()
lseek()
dup()
dup2()
fcntl()
fsync()
```

## Memory

``` text
malloc()
mmap()
munmap()
mprotect()
brk()
```

## IPC

``` text
pipe
FIFO
shared memory
message queue
semaphore
```

## Signals

``` text
SIGINT
SIGTERM
SIGKILL
SIGSEGV
SIGCHLD
sigaction()
```

## Networking

``` text
socket()
bind()
listen()
accept()
connect()
send()
recv()
```

## I/O multiplexing

``` text
select()
poll()
epoll()
```

## Kernel

``` text
task_struct
mm_struct
VMA
page table
page cache
VFS
dentry
inode
superblock
block layer
driver
```

## Debugging

``` text
gdb
strace
perf
valgrind
dmesg
/proc
/sys
```

------------------------------------------------------------------------

# 103. The 10 Flows You Must Be Able to Explain

``` text
1. fork() flow
2. fork() + exec() flow
3. process context switch
4. pthread creation
5. open() flow
6. read() flow
7. write() flow
8. mmap() flow
9. TCP connection flow
10. epoll event loop
```

If you can explain these clearly at a whiteboard, you have a strong
foundation for a **Senior Linux/C/C++ Systems interview**.

------------------------------------------------------------------------

# Final Mental Model

Remember this:

``` text
                    USER SPACE
-------------------------------------------------
Application
    |
    +---- malloc()
    +---- pthread_create()
    +---- printf()
    +---- open()
    +---- read()
    +---- socket()
    |
    v
                 SYSTEM CALL
-------------------------------------------------
                    KERNEL
-------------------------------------------------
Process Management
       |
       +---- Scheduler
       +---- task_struct

Memory Management
       |
       +---- mm_struct
       +---- VMA
       +---- Page Tables
       +---- Page Cache

VFS
       |
       +---- struct file
       +---- dentry
       +---- inode

Networking
       |
       +---- Socket
       +---- TCP/IP
       +---- Network Driver

Storage
       |
       +---- Filesystem
       +---- Block Layer
       +---- Driver
-------------------------------------------------
                   HARDWARE
-------------------------------------------------
CPU | RAM | Disk | NIC | Devices
```

**Interview rule:**

Whenever an interviewer gives you a Linux API, ask yourself:

``` text
What happens in user space?
        ↓
Which system call?
        ↓
Which kernel subsystem?
        ↓
Which important kernel structure?
        ↓
Does it block?
        ↓
Does it involve memory?
        ↓
Does it involve locking?
        ↓
What happens at the hardware level?
```

That thought process is what turns basic Linux API knowledge into
**senior-level Linux systems knowledge**.

------------------------------------------------------------------------

# 104. `fcntl()` and File Descriptor Control

`fcntl()` provides operations on an open file descriptor.

``` text
fcntl()
   |
   +-- duplicate FD
   +-- get/set FD flags
   +-- get/set file status flags
   +-- file locking
```

Important commands:

``` text
F_DUPFD
F_DUPFD_CLOEXEC
F_GETFD
F_SETFD
F_GETFL
F_SETFL
```

## FD_CLOEXEC

A descriptor with `FD_CLOEXEC` is automatically closed when `exec()`
successfully replaces the process image.

``` text
fork()
  |
  +-- child
       |
      exec()
       |
       +-- CLOEXEC FD → closed
       +-- normal FD  → remains open
```

Prefer `O_CLOEXEC` when creating descriptors when the API supports it,
especially in multithreaded programs, to avoid races between descriptor
creation and setting `FD_CLOEXEC`.

------------------------------------------------------------------------

# 105. `dup()`, `dup2()`, `dup3()`

``` text
dup(fd)
    ↓
new FD → same open file description
```

`dup2(oldfd, newfd)` makes `newfd` refer to the same open file
description as `oldfd`.

Typical shell redirection:

``` text
open("out.txt")
      |
      v
dup2(fd, STDOUT_FILENO)
      |
      v
exec()
      |
      v
program stdout → out.txt
```

Important distinction:

``` text
Two FDs
   |
   +---- same open file description
             |
             +-- shared file offset
             +-- shared status flags
```

`dup3()` additionally allows `O_CLOEXEC`.

------------------------------------------------------------------------

# 106. `stat()` and File Metadata

Important APIs:

``` text
stat()
fstat()
lstat()
fstatat()
```

`struct stat` provides metadata such as:

``` text
st_mode
st_uid
st_gid
st_size
st_nlink
st_ino
st_dev
st_atime
st_mtime
st_ctime
```

## `stat()` vs `lstat()`

``` text
symlink → target
```

`stat()` follows the symbolic link.

``` text
lstat()
   |
   v
metadata of the symlink itself
```

------------------------------------------------------------------------

# 107. Directory APIs

User-space directory traversal:

``` c
DIR *dir = opendir(".");
struct dirent *entry;

while ((entry = readdir(dir)) != NULL)
    printf("%s\n", entry->d_name);

closedir(dir);
```

Important APIs:

``` text
opendir()
readdir()
closedir()
rewinddir()
```

Linux internally obtains directory information through lower-level
filesystem mechanisms such as `getdents()`.

Connection:

``` text
Application
   |
readdir()
   |
libc
   |
directory syscall/interface
   |
VFS
   |
filesystem
```

------------------------------------------------------------------------

# 108. File Permissions, Ownership and `umask`

Important APIs:

``` text
chmod()
fchmod()
chown()
fchown()
umask()
access()
```

Permission model:

``` text
        user group other
          |     |    |
         rwx   rwx  rwx
```

Special bits:

``` text
setuid
setgid
sticky bit
```

## `umask`

`umask` removes permission bits from newly created files/directories.

Conceptually:

``` text
requested mode
      &
~umask
      |
      v
actual initial mode
```

------------------------------------------------------------------------

# 109. `openat()` Family

Modern Linux code frequently uses directory-relative APIs:

``` text
openat()
fstatat()
unlinkat()
mkdirat()
renameat()
```

Concept:

``` text
directory FD
     |
     +---- openat(dirfd, "file", ...)
```

Benefits include:

-   directory-relative operations
-   avoiding repeated path traversal
-   safer filesystem operations
-   useful race-resistant designs

Related concept:

``` text
AT_FDCWD
```

means the current working directory is used.

------------------------------------------------------------------------

# 110. Process Groups, Sessions and Job Control

A process has:

``` text
PID  → process ID
PPID → parent process ID
PGID → process group ID
SID  → session ID
```

Hierarchy:

``` text
Session
   |
   +-- Process Group
   |      |
   |      +-- Process
   |      +-- Process
   |
   +-- Process Group
          |
          +-- Process
```

Important APIs:

``` text
setpgid()
getpgid()
setsid()
getsid()
tcsetpgrp()
```

This explains shell job control:

``` text
Shell
 |
 +-- foreground process group
 |
 +-- background process group
```

Terminal-generated signals such as `SIGINT` and `SIGTSTP` are associated
with the terminal's foreground process group.

------------------------------------------------------------------------

# 111. Daemon Processes

Traditional daemonization concept:

``` text
fork()
  |
  v
parent exits
  |
  v
child
  |
setsid()
  |
  v
new session
  |
chdir("/")
  |
umask()
  |
close/redirect FDs
  |
  v
daemon
```

Important considerations:

-   detach from controlling terminal
-   establish a suitable working directory
-   set file-creation mask
-   close or redirect standard descriptors
-   handle signals correctly
-   write logs through an appropriate logging mechanism

Modern Linux services are commonly supervised by `systemd`, so
understand both the traditional daemon model and service-manager-based
execution.

------------------------------------------------------------------------

# 112. `fork()` + Threads

A critical interview topic.

If a multithreaded process calls:

``` text
fork()
```

the child initially contains only the thread that called `fork()`.

Conceptually:

``` text
Parent
 |
 +-- T1
 +-- T2
 +-- T3
      |
    fork()
      |
      v
Child
 |
 +-- T3 only
```

This can be dangerous if another thread held a mutex when `fork()`
happened.

The child may inherit the locked state but not the thread that can
unlock it.

Important API:

``` text
pthread_atfork()
```

Typical safe pattern in a child is to perform minimal work and then call
`exec()`.

------------------------------------------------------------------------

# 113. POSIX Shared Memory

POSIX shared memory commonly uses:

``` text
shm_open()
ftruncate()
mmap()
munmap()
shm_unlink()
```

Architecture:

``` text
Process A
   |
   +---- mmap()
             |
             v
        Shared memory
             ^
             |
   +---- mmap()
   |
Process B
```

The shared memory itself does not automatically provide synchronization.

Combine it with:

``` text
mutex
semaphore
futex
process-shared pthread synchronization
```

when required.

------------------------------------------------------------------------

# 114. Unix Domain Sockets

Unix domain sockets provide local IPC using the socket API.

``` text
Process A
    |
    | AF_UNIX
    v
Unix Domain Socket
    |
    v
Process B
```

Common types:

``` text
SOCK_STREAM
SOCK_DGRAM
SOCK_SEQPACKET
```

Important advantage:

``` text
local IPC
+
socket semantics
+
FD passing
```

------------------------------------------------------------------------

# 115. Passing File Descriptors with `SCM_RIGHTS`

A Unix domain socket can transfer an open file descriptor between
processes.

Concept:

``` text
Process A
   |
   | sendmsg()
   | SCM_RIGHTS
   v
Unix socket
   |
   v
Process B
   |
   +---- received FD
```

This is an important senior-level Linux IPC concept.

Use cases:

-   privilege separation
-   service architectures
-   passing accepted sockets
-   passing device/file access
-   supervisor/worker designs

------------------------------------------------------------------------

# 116. `ioctl()`

`ioctl()` is commonly used for device- or subsystem-specific control
operations.

``` c
ioctl(fd, request, argument);
```

Conceptually:

``` text
User Application
      |
    ioctl()
      |
      v
    Kernel
      |
      v
VFS / subsystem / driver
```

Typical uses:

``` text
device configuration
terminal configuration
network/device control
driver-specific commands
```

`ioctl()` is not a universal replacement for `read()`/`write()`; it is
generally used for control operations that do not map naturally to
byte-stream I/O.

------------------------------------------------------------------------

# 117. Time and Clocks

Important APIs:

``` text
clock_gettime()
nanosleep()
clock_nanosleep()
timer_create()
timer_settime()
```

Important clocks:

``` text
CLOCK_REALTIME
CLOCK_MONOTONIC
```

Use `CLOCK_REALTIME` for wall-clock time.

Use `CLOCK_MONOTONIC` for elapsed-time measurement because it is not
affected by normal wall-clock adjustments.

Example:

``` text
start = CLOCK_MONOTONIC
       |
       v
operation
       |
       v
end = CLOCK_MONOTONIC
       |
       v
elapsed = end - start
```

------------------------------------------------------------------------

# 118. `timerfd`, `eventfd`, and `signalfd`

Linux provides file-descriptor-based interfaces that integrate naturally
with `poll()`/`epoll()`.

``` text
                    epoll
                  /   |   \
                 /    |    \
             socket eventfd timerfd
                        |
                     signalfd
```

## `eventfd`

Useful for event notification between threads/processes.

## `timerfd`

Represents timer expirations as readable events.

## `signalfd`

Allows signals to be consumed through a file descriptor rather than
relying only on traditional signal handlers.

These APIs are particularly useful in event-driven architectures.

------------------------------------------------------------------------

# 119. `mmap()` in More Depth

Important flags:

``` text
MAP_PRIVATE
MAP_SHARED
MAP_ANONYMOUS
```

Protection:

``` text
PROT_READ
PROT_WRITE
PROT_EXEC
```

Typical flow:

``` text
mmap()
   |
   v
Virtual Memory Area
   |
   v
First access
   |
   v
Page fault
   |
   v
Kernel resolves mapping
   |
   v
Physical page
```

## `MAP_PRIVATE`

Writes are private to the process and can involve Copy-on-Write.

## `MAP_SHARED`

Writes can be visible to other mappings of the same shared object and
can be propagated according to the mapping/storage semantics.

## `msync()`

Can be used to request synchronization of a mapped file's modified
contents with the underlying file.

------------------------------------------------------------------------

# 120. `malloc()` Internals

User-space allocation:

``` text
malloc()
   |
   v
libc allocator
   |
   +---- reuse existing heap memory
   |
   +---- brk()/heap growth
   |
   +---- mmap() for suitable allocations
```

Important concepts:

``` text
heap
arena
fragmentation
allocation overhead
thread contention
```

Do not confuse:

``` text
malloc()
```

with:

``` text
kernel memory allocation
```

`malloc()` is a user-space allocator. The kernel ultimately provides
virtual/physical memory through mechanisms such as page allocation and
virtual memory management.

------------------------------------------------------------------------

# 121. Resource Limits

Linux exposes process resource limits through:

``` text
getrlimit()
setrlimit()
prlimit()
```

Important limits:

``` text
RLIMIT_NOFILE
RLIMIT_NPROC
RLIMIT_AS
RLIMIT_CORE
```

Examples:

``` text
Too many open files
        |
RLIMIT_NOFILE
```

``` text
Core dump size
        |
RLIMIT_CORE
```

Resource limits are important when diagnosing production failures.

------------------------------------------------------------------------

# 122. Thread Lifecycle APIs

Important pthread APIs beyond `pthread_create()`:

``` text
pthread_join()
pthread_detach()
pthread_exit()
pthread_cancel()
pthread_once()
```

Lifecycle:

``` text
pthread_create()
      |
      v
   running
      |
      +---- pthread_exit()
      |
      +---- return from start routine
      |
      +---- pthread_cancel()
      |
      v
terminated
      |
      +---- pthread_join()
```

Detached threads cannot be joined.

------------------------------------------------------------------------

# 123. Thread-Local Storage

Thread-local storage provides data that is logically private to each
thread.

Concept:

``` text
Process
 |
 +-- Thread 1 → TLS-A
 |
 +-- Thread 2 → TLS-B
 |
 +-- Thread 3 → TLS-C
```

Useful APIs:

``` text
pthread_key_create()
pthread_setspecific()
pthread_getspecific()
pthread_key_delete()
```

This is useful when libraries need per-thread state without sharing one
global object.

------------------------------------------------------------------------

# 124. Condition Variables: Correct Pattern

A condition variable is used together with a mutex and a predicate.

Correct pattern:

``` c
pthread_mutex_lock(&lock);

while (!condition)
    pthread_cond_wait(&cond, &lock);

consume_or_modify_state();

pthread_mutex_unlock(&lock);
```

Why `while`?

``` text
wake up
   |
   v
reacquire mutex
   |
   v
recheck predicate
```

Reasons include:

-   spurious wakeups
-   another thread consuming/changing the condition first
-   multiple waiting threads

The condition variable itself does not store the condition. The shared
predicate does.

------------------------------------------------------------------------

# 125. Priority Inversion and Robust Synchronization

## Priority inversion

``` text
High priority thread
        |
      waits
        |
        v
mutex held by low priority thread
        ^
        |
medium priority work
```

Possible mitigation:

``` text
priority inheritance
```

Also know:

``` text
lock ordering
trylock
timeouts
short critical sections
```

## Robust mutex

Robust mutexes can help detect the case where the thread/process owning
a mutex terminates unexpectedly while holding it.

The recovery path must be designed carefully; simply detecting the
failure does not automatically restore application invariants.

------------------------------------------------------------------------

# 126. `exec()` Family and `CLOEXEC`

Important APIs:

``` text
execl()
execle()
execlp()
execv()
execvp()
execve()
```

Concept:

``` text
fork()
   |
   v
child
   |
 execve()
   |
   v
same PID
new program image
```

`exec()` does not create a new PID.

Open file descriptors normally survive `exec()` unless marked
close-on-exec.

Therefore:

``` text
fork()
   |
   +-- FD table copied
           |
         exec()
           |
           +-- normal FD → remains
           +-- CLOEXEC FD → closed
```

------------------------------------------------------------------------

# 127. Process Termination

Important APIs:

``` text
exit()
_exit()
_exit() / _Exit()
wait()
waitpid()
waitid()
```

Important distinction:

``` text
exit()
  |
  +-- user-space cleanup
  +-- stdio flushing
  +-- atexit handlers
  |
  v
kernel process termination
```

`_exit()` terminates the process without performing the normal
user-space `exit()` cleanup.

This distinction matters after `fork()`, especially before `exec()`.

------------------------------------------------------------------------

# 128. `/proc` for Runtime Debugging

Important process files:

``` text
/proc/<pid>/status
/proc/<pid>/stat
/proc/<pid>/maps
/proc/<pid>/smaps
/proc/<pid>/fd/
/proc/<pid>/fdinfo/
/proc/<pid>/limits
```

Useful connections:

``` text
/proc/<pid>/maps
      |
      v
Virtual address space
      |
      +-- executable
      +-- shared libraries
      +-- heap
      +-- stack
      +-- mmap regions
```

`/proc/<pid>/fd/` exposes symbolic links representing the process's open
descriptors.

------------------------------------------------------------------------

# 129. Namespaces, cgroups and Capabilities

These topics bridge Linux system programming with containers.

## Namespaces

Important namespace types:

``` text
PID
mount
network
UTS
IPC
user
cgroup
time
```

Concept:

``` text
Container
   |
   +-- namespace isolation
   +-- cgroups
   +-- capabilities
   |
   v
Linux Kernel
```

## PID namespace

A process can have different PID views in different namespaces.

## Network namespace

Provides an isolated network stack containing interfaces, routes, ports,
etc.

## Mount namespace

Provides an isolated view of the filesystem mount tree.

------------------------------------------------------------------------

# 130. cgroups

Control groups organize processes and control/measure resource usage.

Concept:

``` text
cgroup
 |
 +-- CPU limits/accounting
 +-- memory limits/accounting
 +-- I/O controls
 +-- process membership
```

This is a major building block used by container runtimes.

------------------------------------------------------------------------

# 131. Linux Capabilities

Traditional Unix privilege was largely represented by root/non-root.

Linux capabilities split privileged operations into smaller units.

Concept:

``` text
Process
   |
   +-- capabilities
          |
          +-- CAP_NET_ADMIN
          +-- CAP_SYS_ADMIN
          +-- CAP_NET_RAW
          +-- ...
```

This is important for understanding why containers can run with reduced
privileges.

------------------------------------------------------------------------

# 132. Seccomp

Seccomp can restrict the system calls a process is allowed to make.

Concept:

``` text
Application
    |
    v
system call
    |
    v
seccomp policy
    |
    +---- allowed → kernel service
    |
    +---- denied  → blocked
```

Common container security architecture:

``` text
Namespaces
+
cgroups
+
capabilities
+
seccomp
```

------------------------------------------------------------------------

# 133. `io_uring`

Modern Linux asynchronous I/O interface.

Core concepts:

``` text
Submission Queue (SQ)
Completion Queue (CQ)

SQE → Submission Queue Entry
CQE → Completion Queue Entry
```

Architecture:

``` text
Application
     |
     v
Submission Queue
     |
     v
Kernel
     |
     v
Completion Queue
     |
     v
Application
```

Compared with `epoll`:

``` text
epoll
  |
  +-- primarily readiness notification

io_uring
  |
  +-- submission + completion model
  +-- asynchronous operations
  +-- broad I/O operation support
```

Know the conceptual model first; deep kernel implementation can be
studied later.

------------------------------------------------------------------------

# 134. Advanced Event-Driven Architecture

A modern Linux server can combine:

``` text
                 +----------------+
                 |     epoll      |
                 +----------------+
                  /      |       \
                 /       |        \
             socket   eventfd   timerfd
                |
             network
                |
             worker threads
                |
          shared state
                |
             mutex/atomic
```

A more modern architecture may use:

``` text
io_uring
   |
   +-- network I/O
   +-- file I/O
   +-- timers
   +-- asynchronous operations
```

Interview focus:

-   avoid blocking the event loop
-   handle partial reads/writes
-   manage backpressure
-   avoid FD leaks
-   use correct synchronization
-   handle shutdown cleanly
-   monitor resource limits

------------------------------------------------------------------------

# 135. Senior Linux System Programming Checklist

Before considering System Programming complete, be able to explain:

``` text
[ ] libc vs system call
[ ] user mode vs kernel mode
[ ] FD table
[ ] struct file / open file description
[ ] inode / dentry
[ ] open/read/write/close
[ ] lseek
[ ] dup/dup2/dup3
[ ] fcntl
[ ] CLOEXEC
[ ] stat/fstat/lstat
[ ] directory APIs
[ ] permissions/umask
[ ] openat family

[ ] fork
[ ] COW
[ ] exec family
[ ] wait/waitpid
[ ] exit/_exit
[ ] zombie/orphan
[ ] process groups
[ ] sessions
[ ] job control
[ ] daemonization
[ ] resource limits

[ ] pthreads
[ ] mutex
[ ] condition variable
[ ] RW lock
[ ] semaphore
[ ] atomics
[ ] memory ordering
[ ] TLS
[ ] thread cancellation
[ ] fork + threads
[ ] priority inversion

[ ] signals
[ ] sigaction
[ ] signal masks
[ ] SIGCHLD
[ ] async-signal-safety

[ ] pipe/FIFO
[ ] shared memory
[ ] POSIX shared memory
[ ] message queues
[ ] Unix domain sockets
[ ] SCM_RIGHTS

[ ] mmap
[ ] page faults
[ ] malloc
[ ] brk
[ ] fragmentation

[ ] blocking/non-blocking I/O
[ ] select
[ ] poll
[ ] epoll
[ ] LT vs ET
[ ] eventfd
[ ] timerfd
[ ] signalfd
[ ] io_uring

[ ] TCP/UDP
[ ] socket lifecycle
[ ] socket options
[ ] Unix sockets

[ ] /proc
[ ] /sys
[ ] ioctl
[ ] termios
[ ] debugging
[ ] strace
[ ] gdb
[ ] core dumps
[ ] perf

[ ] namespaces
[ ] cgroups
[ ] capabilities
[ ] seccomp
```

------------------------------------------------------------------------

# 136. Final System Programming Mental Model

``` text
                    USER SPACE
┌─────────────────────────────────────────────┐
│ Application                                 │
│                                             │
│ C/C++                                       │
│ libc / pthreads                             │
│                                             │
│ Files ─ Processes ─ Threads ─ IPC           │
│ Memory ─ Sockets ─ epoll ─ io_uring         │
└──────────────────────┬──────────────────────┘
                       │
                  System Calls
                       │
                       ▼
                    KERNEL
┌─────────────────────────────────────────────┐
│ Process Management                          │
│ Scheduler                                   │
│ Virtual Memory / Page Tables                │
│ VFS / Filesystems / Page Cache              │
│ Networking                                  │
│ IPC                                         │
│ Block Layer                                 │
│ Device Drivers                              │
│ Interrupts                                  │
│ Synchronization                             │
│ Security / Namespaces / cgroups             │
└──────────────────────┬──────────────────────┘
                       │
                       ▼
                    HARDWARE
```

The key interview principle is:

> Don't memorize APIs in isolation. Be able to explain what happens from
> the application API, through the system call boundary, into the
> relevant kernel subsystem, and finally to the hardware when
> applicable.


# PART 0 — OPERATING SYSTEM FUNDAMENTALS COMPLETENESS ADDENDUM

> This section closes the remaining OS-theory gaps that are important for Senior Software Engineer interviews. It intentionally connects OS concepts to Linux implementation.

---

# 136. Operating System Types

An operating system manages hardware resources and provides services to applications.

## Major OS classifications

```text
Operating Systems
│
├── Batch OS
├── Multiprogramming OS
├── Multitasking OS
├── Time-Sharing OS
├── Multiprocessing OS
├── Multithreading OS
├── Distributed OS
├── Network OS
├── Embedded OS
└── Real-Time OS
    ├── Hard Real-Time
    └── Soft Real-Time
```

### Batch OS

Jobs are collected and executed with little or no interactive user involvement.

### Multiprogramming

Multiple programs are kept in memory at the same time.

```text
RAM
├── Program A
├── Program B
└── Program C

CPU
 ↓
A runs
 ↓
A waits for I/O
 ↓
B runs
 ↓
B waits for I/O
 ↓
C runs
```

**Goal:** maximize CPU utilization by running another program when one is waiting.

### Multitasking

Multiple tasks appear to execute concurrently because the CPU switches between runnable tasks.

```text
CPU
│
├── Task A
├── Task B
├── Task C
└── Task D
```

On a single CPU this is concurrency, not true simultaneous execution.

### Multiprocessing

Multiple CPUs/cores can execute tasks simultaneously.

```text
CPU Core 0 → Task A
CPU Core 1 → Task B
CPU Core 2 → Task C
CPU Core 3 → Task D
```

### Multithreading

A process contains multiple execution threads.

```text
Process
├── Thread 1
├── Thread 2
└── Thread 3
```

Threads share process resources such as the address space, while each thread has its own execution context and stack.

### Time-sharing

CPU time is divided among tasks/users to provide interactive response.

### Real-Time OS

Correctness includes meeting timing requirements.

**Hard real-time:** missing a deadline can be catastrophic.

**Soft real-time:** missing deadlines degrades quality/performance.

### Interview distinction

| Concept | Core idea |
|---|---|
| Multiprogramming | Keep multiple programs in memory; switch when one waits |
| Multitasking | Run multiple tasks through scheduling/preemption |
| Multithreading | Multiple execution flows inside one process |
| Multiprocessing | Multiple CPUs/cores execute simultaneously |
| Time-sharing | Divide CPU time for interactive fairness |
| Real-time | Timing/deadline requirements matter |

---

# 137. Process, Program and Thread

### Program

A passive executable image stored on storage.

### Process

A running instance of a program with resources and execution state.

```text
Program
   ↓ execute
Process
   ├── Address space
   ├── Registers
   ├── Stack
   ├── Heap
   ├── Open files
   ├── Signals
   └── Scheduling state
```

### Thread

The smallest schedulable execution unit in the usual process/thread model.

```text
Process
│
├── Thread A → registers + stack
├── Thread B → registers + stack
└── Thread C → registers + stack

Shared:
- address space
- code
- heap
- open-file resources
```

### Linux connection

Linux internally represents schedulable execution entities using `task_struct`; a Linux process and its threads are therefore connected to the same task/scheduler infrastructure.

---

# 138. Process Lifecycle and States

Classic OS model:

```text
             ┌──────────┐
             │   NEW    │
             └────┬─────┘
                  ↓
             ┌──────────┐
             │  READY   │◄─────────────┐
             └────┬─────┘              │
                  ↓                    │
             ┌──────────┐              │
             │ RUNNING  │              │
             └──┬────┬──┘              │
                │    │                 │
             I/O│    │exit             │
                ↓    ↓                 │
          ┌─────────┐ TERMINATED       │
          │ WAITING │                  │
          └────┬────┘                  │
               │ I/O complete          │
               └───────────────────────┘
```

Important transitions:

```text
READY → RUNNING
RUNNING → READY       (preemption)
RUNNING → WAITING     (blocking I/O / sleep)
WAITING → READY       (event completed)
RUNNING → TERMINATED
```

### Linux task states

Conceptually know:

- runnable
- interruptible sleep
- uninterruptible sleep
- stopped
- traced
- zombie

Do not assume the classic textbook state diagram maps one-to-one onto Linux's internal task-state flags.

---

# 139. CPU Scheduling Fundamentals

The scheduler decides which runnable task gets CPU time.

```text
Runnable tasks
      ↓
Scheduler
      ↓
select next task
      ↓
CPU
```

## Scheduling goals

- CPU utilization
- throughput
- fairness
- low response time
- low waiting time
- low turnaround time
- predictable latency
- deadline satisfaction for real-time workloads

## Important metrics

### Turnaround time

```text
completion time - arrival time
```

### Waiting time

Time spent waiting in the ready/runnable queue.

### Response time

```text
first CPU service - arrival time
```

### Throughput

Number of completed jobs per unit time.

---

# 140. Classical Scheduling Algorithms

Know these conceptually and be able to compare them.

## FCFS

First Come First Served.

- simple
- non-preemptive
- can cause convoy effect

## SJF

Shortest Job First.

- minimizes average waiting time when burst lengths are known/estimated
- usually non-preemptive

## SRTF

Shortest Remaining Time First.

Preemptive version of SJF.

## Priority Scheduling

Highest-priority task is selected.

Problem:

```text
Low priority task
       ↓
never gets CPU
       ↓
starvation
```

Aging can gradually increase waiting task priority.

## Round Robin

Each runnable task receives a time quantum.

```text
A → B → C → A → B → C
```

Small quantum:

- better responsiveness
- more context-switch overhead

Large quantum:

- less overhead
- behaves more like FCFS

## Multilevel Queue

Separate queues for different classes of tasks.

## Multilevel Feedback Queue

Tasks can move between queues based on behavior/priority.

---

# 141. Linux Scheduler and Runqueues

Linux does not simply implement one textbook scheduling algorithm.

Conceptually:

```text
                    Scheduler
                        │
              Scheduling classes
                 ┌──────┼──────┐
                 ↓      ↓      ↓
             Fair/    RT    Deadline
             normal
                 │
                 ↓
             Runqueue
                 │
        ┌────────┼────────┐
        ↓        ↓        ↓
      Task A   Task B   Task C
                 │
                 ↓
             CPU core
```

Modern Linux uses the fair scheduling infrastructure with **EEVDF** concepts for normal tasks, while real-time and deadline scheduling have separate policies/classes.

### Per-CPU runqueues

Conceptually:

```text
CPU 0
└── runqueue
    ├── Task A
    ├── Task B
    └── Task C

CPU 1
└── runqueue
    ├── Task D
    └── Task E
```

Important concepts:

- runnable task
- enqueue/dequeue
- wakeup
- preemption
- CPU affinity
- task migration
- load balancing
- per-CPU scheduling
- scheduling class

---

# 142. Preemption

Preemption means the currently running task can be replaced by another task.

### Voluntary blocking

```text
Task
 ↓
read() blocks
 ↓
sleep
 ↓
scheduler chooses another task
```

### Involuntary preemption

```text
Task A running
      ↓
scheduler/preemption point
      ↓
Task B becomes runnable / higher priority
      ↓
Task A preempted
      ↓
Task B runs
```

### Why preemption matters

It improves responsiveness and allows multiple runnable tasks to make progress.

---

# 143. Context Switching

A context switch changes CPU execution from one execution context to another.

```text
Task A running
      ↓
interrupt / preemption / blocking
      ↓
scheduler
      ↓
save A execution context
      ↓
select B
      ↓
restore B context
      ↓
Task B running
```

A context includes CPU execution state such as:

- registers
- instruction pointer
- stack pointer
- processor state

For process/address-space changes, additional memory-management state can change.

### Thread vs process switch

Threads in one process share the address space, so switching between them may avoid changing to a completely different address space.

A process switch can involve changing memory-management context/page-table state.

### Context-switch cost

Costs can include:

- saving/restoring registers
- scheduler work
- cache disruption
- TLB effects
- branch-prediction disruption
- address-space changes

**Interview point:** a context switch is not just "saving registers"; it can have significant microarchitectural/cache consequences.

---

# 144. Scheduler Queue, Wakeup and Sleep Flow

Understand this end-to-end flow:

```text
                 RUNNING
                    │
          ┌─────────┴─────────┐
          │                   │
       blocks              preempted
          │                   │
          ↓                   ↓
       WAITING              READY
          │                   │
      event occurs            │
          │                   │
          ↓                   │
        READY ◄───────────────┘
          │
          ↓
       RUNQUEUE
          │
          ↓
       SCHEDULER
          │
          ↓
        RUNNING
```

This connects process states, blocking, wakeups and scheduling.

---

# 145. Deadlock

Deadlock occurs when tasks wait indefinitely for resources held by each other.

## Coffman conditions

All four are required:

```text
1. Mutual exclusion
2. Hold and wait
3. No preemption
4. Circular wait
```

Example:

```text
Thread A                  Thread B

lock(A)                   lock(B)
  ↓                         ↓
wait for B                wait for A
  ↓                         ↓
          DEADLOCK
```

## Deadlock prevention

Break at least one Coffman condition.

Practical techniques:

- fixed global lock ordering
- avoid unnecessary nested locking
- minimize lock hold time
- avoid holding locks while blocking
- use try-lock/timeouts where appropriate
- use lock hierarchy
- use lockdep for kernel lock validation

## Deadlock vs race vs livelock vs starvation

| Problem | Meaning |
|---|---|
| Race condition | Result depends on timing/interleaving |
| Deadlock | Tasks wait forever |
| Livelock | Tasks keep running/changing state but make no useful progress |
| Starvation | A task waits indefinitely because others continually get resources |

---

# 146. Synchronization Problems

## Race condition

```text
Thread A          Thread B

read x            read x
add 1             add 1
write x           write x
```

Both can read the same old value.

Solution depends on the operation:

- mutex
- atomic operation
- semaphore
- rwlock
- condition variable
- lock-free algorithm

## Critical section

Code accessing shared state that must be synchronized.

```text
lock
  ↓
critical section
  ↓
unlock
```

### Important distinction

```text
Mutex      → ownership-based mutual exclusion
Semaphore  → counting/signaling primitive
RWLock     → multiple readers / exclusive writer
Condition  → wait for a condition/state change
Atomic     → indivisible atomic operation
```

---

# 147. Starvation, Fairness and Priority Inversion

## Starvation

A runnable task repeatedly fails to receive sufficient CPU/resources.

## Aging

Gradually increase the priority of a waiting task to reduce starvation.

## Priority inversion

```text
High priority H
      ↓ waits for lock
Low priority L holds lock

Medium priority M
      ↓
keeps running

H cannot progress because L cannot run
```

Potential solution:

**Priority inheritance** temporarily boosts the lock holder.

---

# 148. Concurrency vs Parallelism

These terms are frequently confused.

### Concurrency

Multiple tasks make progress over overlapping time periods.

```text
A → B → A → C → B
```

Can happen on one CPU.

### Parallelism

Multiple tasks execute simultaneously on multiple CPU cores.

```text
Core 0 → A
Core 1 → B
Core 2 → C
```

### Relationship

```text
Concurrency ≠ necessarily parallelism
Parallelism requires simultaneous execution resources
```

Multithreading can provide concurrency on one CPU and parallelism on multiple cores.

---

# 149. CPU, Core, Hardware Thread and Process

Clarify the hierarchy:

```text
Physical CPU/package
    ↓
CPU cores
    ↓
Hardware threads / logical CPUs
    ↓
Software threads/tasks scheduled by OS
```

A software thread is not the same thing as a hardware thread.

Linux schedules runnable software tasks onto logical CPUs.

---

# 150. I/O Blocking and Scheduler Interaction

A blocking system call often causes this flow:

```text
Application thread
       ↓
read()
       ↓
data unavailable
       ↓
thread sleeps/waits
       ↓
scheduler runs another task
       ↓
device performs I/O
       ↓
interrupt / deferred completion
       ↓
waiting task wakes
       ↓
task becomes runnable
       ↓
scheduler eventually runs it
       ↓
read() returns
```

This is one of the most important connections between OS theory and Linux internals.

---

# 151. Synchronization and IPC Relationship

Processes may communicate using:

```text
IPC
├── Pipe
├── FIFO
├── Message Queue
├── Shared Memory
├── Unix Domain Socket
├── Network Socket
├── Signal
├── eventfd
└── mmap/shared file
```

Shared-memory communication usually requires synchronization.

```text
Process A ─┐
           ├── shared memory
Process B ─┘
       │
       ↓
 mutex/semaphore/atomic/etc.
```

---

# 152. Resource Management

An OS manages multiple resource classes:

```text
CPU
Memory
Storage
Network
Devices
File descriptors
Processes/threads
```

For each resource, understand:

- allocation
- ownership
- contention
- blocking
- scheduling
- reclamation
- limits
- isolation

Linux examples:

```text
CPU      → scheduler / cgroups
Memory   → VM / allocator / cgroups
Storage  → VFS / filesystem / block layer
Network  → socket / network stack / cgroups
Devices  → drivers / permissions / namespaces
FDs      → per-process limits
```

---

# 153. Protection, Isolation and Privilege

OS protection mechanisms operate at multiple levels:

```text
User/Kernel mode
        ↓
Process address spaces
        ↓
File permissions
        ↓
Capabilities
        ↓
Namespaces
        ↓
cgroups
        ↓
seccomp
        ↓
LSM
```

Understand the distinction:

- **Isolation** → separate resources/views
- **Protection** → prevent unauthorized access
- **Privilege** → what an execution context is allowed to do
- **Resource control** → how much CPU/memory/etc. can be consumed

---

# 154. Interrupt, Exception and Trap

These should be clearly distinguished.

### Interrupt

Asynchronous event, commonly from hardware.

```text
NIC/device
    ↓
interrupt
    ↓
CPU
```

### Exception

Synchronous event caused by current instruction.

Examples:

- page fault
- divide-by-zero
- invalid instruction

### Trap

A synchronous transfer intentionally used to enter privileged handling; system-call mechanisms historically use trap-like mechanisms, although modern architectures provide dedicated syscall instructions.

---

# 155. Kernel Mode vs User Mode

```text
USER MODE
  ↓
system call / exception / interrupt
  ↓
KERNEL MODE
  ↓
kernel service
  ↓
return to user mode
```

Important distinction:

- **mode switch** is not necessarily a **context switch**
- a system call can enter kernel mode and return to the same thread without switching to another task
- a context switch changes which task executes

This distinction is a common interview trap.

---

# 156. Virtual Memory vs Physical Memory

### Virtual memory

Each process gets an address-space abstraction.

```text
Process A virtual address
        ↓
       MMU
        ↓
physical page

Process B virtual address
        ↓
       MMU
        ↓
different physical page
```

Benefits:

- isolation
- larger logical address space
- demand paging
- memory mapping
- shared libraries
- copy-on-write

---

# 157. Page Fault Types

A page fault does not automatically mean an error.

### Minor fault

Page can be satisfied without disk I/O.

Examples can include mapping an already resident page.

### Major fault

Requires relatively expensive I/O such as reading from storage.

### Invalid fault

Access violates the process's valid memory permissions/address space and may result in `SIGSEGV` or another fault outcome.

---

# 158. Thrashing

Thrashing occurs when the system spends excessive time moving/reclaiming memory instead of doing useful work.

Conceptually:

```text
Too much memory pressure
        ↓
page faults/reclaim increase
        ↓
I/O increases
        ↓
CPU makes less useful progress
        ↓
system becomes slow
```

Related concepts:

- working set
- page reclaim
- swap pressure
- memory overcommit

---

# 159. OS Boot Sequence

```text
Power On
   ↓
Firmware (BIOS/UEFI)
   ↓
Bootloader
   ↓
Kernel image
   ↓
Early kernel initialization
   ↓
initramfs
   ↓
Root filesystem
   ↓
PID 1
   ↓
Services
   ↓
Applications
```

Know conceptually:

- firmware initializes hardware
- bootloader loads/configures the kernel
- kernel initializes core subsystems
- initramfs provides early userspace
- PID 1 starts userspace services

---

# 160. OS Interview Must-Know Comparison Table

| Topic | Must explain |
|---|---|
| Program vs process | Passive code vs running instance |
| Process vs thread | Resource container vs execution flow |
| Multiprogramming | Multiple programs in memory |
| Multitasking | Multiple tasks scheduled |
| Multiprocessing | Multiple CPUs/cores |
| Multithreading | Multiple threads per process |
| Concurrency vs parallelism | Interleaving vs simultaneous execution |
| Ready vs waiting | Runnable vs blocked |
| Preemption | Scheduler interrupts/replaces running task |
| Context switch | Save/restore execution context |
| Mode switch | User ↔ kernel privilege transition |
| Deadlock | Circular indefinite waiting |
| Starvation | One task fails to get resources |
| Livelock | Activity without useful progress |
| Race | Timing-dependent incorrect behavior |
| Mutex vs semaphore | Ownership vs counting/signaling |
| Interrupt vs exception | Asynchronous vs synchronous |
| Virtual vs physical memory | Address abstraction vs actual RAM |
| Minor vs major page fault | No storage I/O vs storage I/O |
| Scheduling vs dispatch | Policy vs selecting/running next task |

---

# 161. OS Fundamentals — Final Completeness Checklist

Before calling the OS portion interview-ready, be able to explain:

### OS Basics
- [ ] What is an operating system?
- [ ] Kernel vs operating system
- [ ] User mode vs kernel mode
- [ ] System call
- [ ] Interrupt
- [ ] Exception
- [ ] Trap
- [ ] OS types
- [ ] Multiprogramming
- [ ] Multitasking
- [ ] Multiprocessing
- [ ] Multithreading
- [ ] Time-sharing
- [ ] Real-time OS
- [ ] Embedded OS

### Processes
- [ ] Program vs process
- [ ] Process creation
- [ ] Process states
- [ ] PCB/process metadata
- [ ] `fork()`
- [ ] `exec()`
- [ ] `wait()`
- [ ] zombie
- [ ] orphan
- [ ] process groups
- [ ] sessions
- [ ] PID 1

### Threads
- [ ] Process vs thread
- [ ] Thread lifecycle
- [ ] User vs kernel threads
- [ ] Thread-local storage
- [ ] Thread stack
- [ ] Thread cancellation
- [ ] Thread synchronization

### Scheduling
- [ ] Scheduling goals
- [ ] FCFS
- [ ] SJF
- [ ] SRTF
- [ ] Priority
- [ ] Round Robin
- [ ] Multilevel queue
- [ ] MLFQ
- [ ] Preemptive vs non-preemptive
- [ ] Runqueue
- [ ] Runnable task
- [ ] Wakeup
- [ ] CPU affinity
- [ ] Load balancing
- [ ] Task migration
- [ ] Linux scheduling classes
- [ ] CFS/EEVDF
- [ ] Real-time scheduling
- [ ] Deadline scheduling

### Context Switching
- [ ] What is a context switch?
- [ ] What gets saved/restored?
- [ ] Process vs thread switch
- [ ] Context-switch overhead
- [ ] Cache/TLB effects
- [ ] Mode switch vs context switch

### Synchronization
- [ ] Critical section
- [ ] Race condition
- [ ] Mutex
- [ ] Semaphore
- [ ] Condition variable
- [ ] RW lock
- [ ] Spinlock
- [ ] Atomic operation
- [ ] Memory ordering
- [ ] Futex
- [ ] Priority inversion
- [ ] Priority inheritance

### Deadlocks
- [ ] Four Coffman conditions
- [ ] Deadlock example
- [ ] Prevention
- [ ] Avoidance
- [ ] Detection
- [ ] Recovery
- [ ] Lock ordering
- [ ] Lockdep
- [ ] Deadlock vs starvation
- [ ] Deadlock vs livelock

### Memory
- [ ] Physical vs virtual memory
- [ ] Address translation
- [ ] MMU
- [ ] Page tables
- [ ] TLB
- [ ] Page fault
- [ ] Minor/major fault
- [ ] Demand paging
- [ ] COW
- [ ] mmap
- [ ] Page cache
- [ ] Swap
- [ ] Reclaim
- [ ] OOM
- [ ] Buddy allocator
- [ ] SLAB/SLUB
- [ ] Huge pages
- [ ] NUMA
- [ ] Thrashing

### I/O
- [ ] Blocking I/O
- [ ] Non-blocking I/O
- [ ] Synchronous vs asynchronous I/O
- [ ] Polling
- [ ] Interrupt-driven I/O
- [ ] DMA
- [ ] I/O completion
- [ ] Device drivers
- [ ] VFS
- [ ] Block layer

### IPC
- [ ] Pipe
- [ ] FIFO
- [ ] Shared memory
- [ ] Message queue
- [ ] Signal
- [ ] Unix socket
- [ ] Network socket
- [ ] `eventfd`
- [ ] `signalfd`
- [ ] `timerfd`
- [ ] FD passing

### Core Interview Flows
- [ ] fork → COW → scheduler
- [ ] pthread_create → task → scheduler
- [ ] context switch
- [ ] blocking read → sleep → wakeup
- [ ] mutex → futex → sleep/wakeup
- [ ] malloc → mmap/brk → page fault
- [ ] page fault → page allocation/mapping
- [ ] read → VFS → page cache → filesystem
- [ ] write → dirty page → writeback → block layer
- [ ] NIC → DMA → interrupt/NAPI → network stack → socket
- [ ] container → namespaces + cgroups + capabilities + seccomp

---

# 162. Senior OS Mental Model

The complete OS mental model should be:

```text
                    APPLICATION
                         │
                         ▼
                  C / C++ / POSIX
                         │
                         ▼
                   SYSTEM CALL
                         │
          ┌──────────────┼──────────────┐
          ▼              ▼              ▼
       PROCESS          MEMORY           I/O
          │              │              │
       THREADS       VM / MMU          VFS
          │              │              │
      SCHEDULER      PAGE TABLES     FILESYSTEM
          │              │              │
      RUNQUEUE           │          BLOCK LAYER
          │              │              │
      CPU/CORE           │           DRIVER
          │              │              │
          └──────────────┼──────────────┘
                         ▼
                      HARDWARE
```

The senior-level question is not merely:

> "What does `read()` do?"

It is:

> "What happens from the application call through libc and the syscall boundary, how does the kernel identify the resource, what subsystem handles it, can the task block, how is it woken, what synchronization is involved, how does the scheduler participate, and how does the request finally reach hardware?"

That is the level of understanding these notes should target.
