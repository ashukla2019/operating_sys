
# Operating Systems + Linux System Programming + Linux Kernel Internals

## Senior C/C++ Linux Systems Interview Preparation Roadmap

---

# 0. How to Use This Roadmap

The goal is not just to memorize Linux commands or APIs.

For every topic, learn it in this order:

```text
1. OS CONCEPT
       ↓
2. WHY IT EXISTS
       ↓
3. LINUX IMPLEMENTATION
       ↓
4. SYSTEM CALL / POSIX API
       ↓
5. C/C++ CODE
       ↓
6. KERNEL INTERNALS
       ↓
7. DEBUGGING / PERFORMANCE
       ↓
8. INTERVIEW QUESTIONS
```

Example:

```text
Process
   ↓
Virtual address space
   ↓
fork()
   ↓
task_struct
   ↓
mm_struct
   ↓
page tables
   ↓
COW
   ↓
exec()
   ↓
ELF loader
```

This approach gives much better preparation for senior Linux interviews.

---

# 1. MASTER ROADMAP

```text
                    ┌───────────────────────┐
                    │     C / C++ Basics    │
                    └───────────┬───────────┘
                                │
                                ▼
                    ┌───────────────────────┐
                    │   Operating Systems   │
                    └───────────┬───────────┘
                                │
          ┌─────────────────────┼─────────────────────┐
          ▼                     ▼                     ▼
      Processes             Memory                Threads
          │                     │                     │
          ▼                     ▼                     ▼
       fork()                 mmap()              pthreads
       exec()                 malloc              mutex
       wait()                 mmap                condvar
       signals                page cache           rwlock
          │                     │                     │
          └─────────────────────┼─────────────────────┘
                                ▼
                    ┌───────────────────────┐
                    │ Linux System Calls    │
                    │       / POSIX         │
                    └───────────┬───────────┘
                                │
        ┌───────────────┬───────┼───────────────┐
        ▼               ▼       ▼               ▼
      Files           IPC     Signals        Sockets
        │               │       │               │
        └───────────────┴───────┼───────────────┘
                                ▼
                    ┌───────────────────────┐
                    │ Linux Kernel Internals│
                    └───────────┬───────────┘
                                │
        ┌──────────────┬────────┼───────────┬─────────────┐
        ▼              ▼        ▼           ▼             ▼
     Process          MM      VFS       Networking     Drivers
        │              │        │           │             │
        ▼              ▼        ▼           ▼             ▼
   task_struct      mm_struct  inode     sk_buff       modules
   scheduler        page      dentry     TCP/IP        ioctl
   runqueue         tables     file       sockets       device
        │              │        │           │
        └──────────────┴────────┼───────────┴─────────────┘
                                ▼
                    ┌───────────────────────┐
                    │ Debugging & Perf      │
                    │ gdb / strace / perf   │
                    │ ftrace / /proc / sys  │
                    └───────────────────────┘
```

---

# 2. PHASE 1 — OPERATING SYSTEM FUNDAMENTALS

## 2.1 What is an Operating System?

An OS provides:

```text
Application
     ↓
System Call / API
     ↓
Operating System
     ↓
Hardware
```

Main responsibilities:

* Process management
* Memory management
* File systems
* I/O
* Device management
* Networking
* Security
* Resource allocation

---

# 3. USER MODE VS KERNEL MODE

Modern CPUs provide privilege levels.

Typical Linux model:

```text
USER MODE
   │
   │ system call
   ▼
KERNEL MODE
   │
   ▼
Hardware
```

User programs cannot directly:

* Access arbitrary physical memory
* Execute privileged instructions
* Directly control hardware
* Modify kernel memory

They use system calls.

Example:

```c
write(fd, buffer, size);
```

Flow:

```text
Application
    ↓
write()
    ↓
C library
    ↓
syscall instruction
    ↓
Kernel
    ↓
VFS
    ↓
Filesystem
    ↓
Block layer
    ↓
Device driver
    ↓
Hardware
```

---

# 4. PROCESS MANAGEMENT

## 4.1 Process

A process is an executing program together with:

* Address space
* CPU state
* Open file descriptors
* Scheduling information
* Credentials
* Signal state
* Resources

Linux represents a process using:

```c
struct task_struct
```

Important concept:

```text
Program
   +
Execution state
   +
Resources
   =
Process
```

---

# 5. PROCESS ADDRESS SPACE

Typical process:

```text
High Address
+-------------------+
|       Stack       |
+-------------------+
|                   |
|       mmap        |
|                   |
+-------------------+
|       Heap        |
+-------------------+
|       BSS         |
+-------------------+
|       Data        |
+-------------------+
|       Text        |
+-------------------+
Low Address
```

Important terms:

* Text
* Read-only data
* Data
* BSS
* Heap
* mmap region
* Stack

---

# 6. fork()

`fork()` creates a new process.

Example:

```c
#include <stdio.h>
#include <unistd.h>

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
        printf("Parent: PID = %d, Child = %d\n",
               getpid(), pid);
    }

    return 0;
}
```

## fork() return value

```text
Parent:
fork() returns child PID

Child:
fork() returns 0

Failure:
fork() returns -1
```

---

# 7. fork() INTERNALS

Conceptually:

```text
Parent Process
      |
      | fork()
      ▼
+-------------+
| task_struct |
+-------------+
      |
      +------------------+
      |                  |
      ▼                  ▼
   Parent              Child
```

The child does NOT normally copy every physical memory page immediately.

Linux uses:

# Copy-On-Write

Initially:

```text
Parent Page Table ─────┐
                       ├── Physical Page
Child Page Table ──────┘
```

Pages are marked appropriately so that when one process writes:

```text
Child writes
     ↓
Page fault
     ↓
Kernel allocates new page
     ↓
Copy data
     ↓
Update page table
     ↓
Continue execution
```

Important interview point:

> `fork()` creates a new process and logically duplicates the address space, but physical pages are initially shared using Copy-On-Write.

---

# 8. exec()

`fork()` creates a process.

`exec()` replaces the current process image.

Typical shell execution:

```text
Shell
 |
 | fork()
 ▼
Child
 |
 | exec()
 ▼
New Program
```

Example:

```c
#include <unistd.h>

int main()
{
    execl("/bin/ls", "ls", "-l", NULL);

    return 0;
}
```

After successful `exec()`:

```text
Old program
    ↓
Address space destroyed/replaced
    ↓
New ELF program loaded
```

The PID normally remains the same.

---

# 9. wait()

Parent waits for child:

```c
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        printf("Child running\n");
        return 42;
    }

    int status;

    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
    {
        printf("Exit code = %d\n",
               WEXITSTATUS(status));
    }

    return 0;
}
```

Important:

```text
fork()
exec()
wait()
```

are extremely important for Linux interviews.

---

# 10. ZOMBIE AND ORPHAN

## Zombie

Child has terminated but parent has not collected its exit status.

```text
Child exits
    ↓
Zombie
    ↓
Parent wait()
    ↓
Zombie removed
```

## Orphan

Parent terminates before child.

The orphan is adopted by another process/reaper mechanism.

---

# 11. PROCESS VS THREAD

## Process

Has its own address space.

## Thread

Threads of the same process share:

```text
Code
Data
Heap
Open files
Address space
```

But each thread has its own:

```text
Stack
Registers
Program counter
Thread-local storage
```

Diagram:

```text
Process
+---------------------------+
| Address Space             |
|                           |
| Code                      |
| Data                      |
| Heap                      |
|                           |
| Thread 1 ─ Stack          |
| Thread 2 ─ Stack          |
| Thread 3 ─ Stack          |
+---------------------------+
```

---

# 12. pthread CODE

```c
#include <stdio.h>
#include <pthread.h>

void* worker(void* arg)
{
    printf("Worker thread\n");
    return NULL;
}

int main()
{
    pthread_t tid;

    pthread_create(&tid, NULL, worker, NULL);

    pthread_join(tid, NULL);

    return 0;
}
```

Compile:

```bash
gcc thread.c -pthread
```

---

# 13. PROCESS VS THREAD INTERVIEW TABLE

| Feature       | Process          | Thread             |
| ------------- | ---------------- | ------------------ |
| Address space | Separate         | Shared             |
| Creation      | More expensive   | Cheaper            |
| Communication | IPC required     | Shared memory      |
| Isolation     | High             | Lower              |
| Stack         | Separate         | Separate           |
| Heap          | Separate         | Shared             |
| Crash impact  | Usually isolated | Can affect process |
| Scheduling    | Scheduled entity | Scheduled entity   |

---

# 14. CPU SCHEDULING

Important concepts:

* Scheduler
* Context switch
* Preemption
* Time slice
* Priority
* Fair scheduling
* Real-time scheduling

Linux concepts to eventually understand:

```text
task_struct
    ↓
Scheduling class
    ↓
Run queue
    ↓
Scheduler
    ↓
CPU
```

Important distinction:

> Context switching between threads/processes is not the same as switching between user mode and kernel mode.

---

# 15. CONTEXT SWITCH

CPU changes from one execution context to another.

Conceptually:

```text
Thread A running
      ↓
Save CPU state
      ↓
Scheduler
      ↓
Select Thread B
      ↓
Restore CPU state
      ↓
Thread B running
```

Costs can include:

* Register state changes
* Cache effects
* TLB effects
* Scheduler overhead

---

# 16. SYNCHRONIZATION

Learn these in order:

```text
Race Condition
     ↓
Mutex
     ↓
Semaphore
     ↓
Condition Variable
     ↓
RW Lock
     ↓
Spinlock
     ↓
Atomics
     ↓
Memory Ordering
```

---

# 17. RACE CONDITION

```c
counter++;
```

Looks atomic but conceptually:

```text
LOAD
ADD
STORE
```

Two threads can interleave:

```text
T1: LOAD 0
T2: LOAD 0
T1: ADD
T2: ADD
T1: STORE 1
T2: STORE 1
```

Expected:

```text
2
```

Actual:

```text
1
```

---

# 18. MUTEX

```c
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

---

# 19. CONDITION VARIABLE

Used when a thread must wait for a condition.

Typical pattern:

```text
Producer
   ↓
produce data
   ↓
signal condition
   ↓
Consumer wakes
   ↓
consume data
```

Code:

```c
pthread_mutex_lock(&lock);

while (!ready)
{
    pthread_cond_wait(&cond, &lock);
}

pthread_mutex_unlock(&lock);
```

Always prefer:

```c
while (!condition)
```

rather than:

```c
if (!condition)
```

because of spurious wakeups and rechecking the predicate.

---

# 20. DEADLOCK

Four Coffman conditions:

```text
1. Mutual exclusion
2. Hold and wait
3. No preemption
4. Circular wait
```

Example:

```text
Thread 1:
lock(A)
lock(B)

Thread 2:
lock(B)
lock(A)
```

Potential deadlock:

```text
T1 owns A → waits B
T2 owns B → waits A
```

Prevention:

```text
Always acquire locks in a global order.
```

---

# 21. MEMORY MANAGEMENT

Important OS concepts:

```text
Virtual Memory
    ↓
Virtual Address
    ↓
Page Table
    ↓
Physical Address
```

---

# 22. VIRTUAL MEMORY

Each process sees its own virtual address space.

```text
Process A
Virtual 0x1000 ─────┐
                    │
                    ▼
                 Physical
                    Page

Process B
Virtual 0x1000 ─────┘
```

Same virtual address can map to different physical pages.

Benefits:

* Isolation
* Protection
* Larger address space
* Memory overcommit
* Shared memory
* mmap
* Copy-on-write

---

# 23. PAGE AND PAGE TABLE

Typical flow:

```text
Virtual Address
      ↓
Page Table
      ↓
Physical Frame
```

Virtual address conceptually:

```text
+-------------------+------------+
| Page Number       | Offset     |
+-------------------+------------+
```

The page number is translated.

The offset remains unchanged.

---

# 24. PAGE FAULT

Page fault occurs when required memory mapping/page is not immediately available.

Possible reasons:

* Demand allocation
* Copy-on-write
* File-backed page not present
* Invalid access

Conceptual flow:

```text
CPU accesses address
        ↓
MMU
        ↓
Page table lookup
        ↓
Fault
        ↓
CPU enters kernel
        ↓
Page fault handler
        ↓
Resolve fault
        ↓
Update mapping
        ↓
Retry instruction
```

---

# 25. mmap()

`mmap()` maps memory into a process address space.

Example:

```c
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main()
{
    size_t size = 4096;

    char* p = mmap(NULL,
                   size,
                   PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS,
                   -1,
                   0);

    if (p == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    strcpy(p, "Hello mmap");

    printf("%s\n", p);

    munmap(p, size);

    return 0;
}
```

---

# 26. MALLOC VS MMAP

```text
malloc()
   ↓
C library allocator
   ↓
May use brk()/mmap()
   ↓
Kernel virtual memory
```

Important interview point:

> `malloc()` is a user-space allocator. It is not itself a system call.

---

# 27. IPC

Learn IPC in this order:

```text
Pipe
 ↓
FIFO
 ↓
Signal
 ↓
Shared Memory
 ↓
Semaphore
 ↓
Message Queue
 ↓
Unix Domain Socket
```

---

# 28. PIPE

```c
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main()
{
    int fd[2];

    pipe(fd);

    write(fd[1], "hello", 5);

    char buffer[10] = {0};

    read(fd[0], buffer, 5);

    printf("%s\n", buffer);

    return 0;
}
```

Concept:

```text
Process
   |
   +--> fd[1] write
   |
   +--> Kernel pipe buffer
              |
              +--> fd[0] read
```

---

# 29. FILE DESCRIPTORS

Linux represents an open file using a file descriptor.

Example:

```text
0 → stdin
1 → stdout
2 → stderr
```

Open:

```c
int fd = open("test.txt", O_RDONLY);
```

Read:

```c
read(fd, buffer, size);
```

Close:

```c
close(fd);
```

---

# 30. FILE DESCRIPTOR INTERNALS

Important Linux mapping:

```text
Process
   |
   ▼
fd table
   |
   ▼
struct file
   |
   ├── f_inode
   ├── f_op
   ├── f_pos
   ├── f_flags
   ├── f_path
   └── private_data
```

This becomes extremely important when learning VFS.

---

# 31. FILE I/O SYSTEM CALLS

Master these:

```text
open()
close()
read()
write()
pread()
pwrite()
lseek()
stat()
fstat()
fsync()
fcntl()
ioctl()
dup()
dup2()
dup3()
```

---

# 32. dup() / dup2()

Used to duplicate file descriptors.

Classic shell redirection:

```bash
program > output.txt
```

Conceptually:

```text
open(output.txt)
       ↓
dup2(fd, STDOUT_FILENO)
       ↓
exec(program)
```

Now:

```text
stdout → output.txt
```

---

# 33. SIGNALS

Important signals:

```text
SIGINT
SIGTERM
SIGKILL
SIGSTOP
SIGSEGV
SIGCHLD
SIGPIPE
SIGALRM
```

Important distinction:

```text
SIGTERM → can be handled
SIGKILL → cannot be caught/ignored
SIGSTOP → cannot be caught/ignored
```

---

# 34. SIGNAL CODE

```c
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void handler(int sig)
{
    printf("Received signal %d\n", sig);
}

int main()
{
    signal(SIGINT, handler);

    while (1)
        pause();

    return 0;
}
```

For robust applications, learn:

```text
sigaction()
```

rather than relying on `signal()` semantics.

---

# 35. SOCKET PROGRAMMING

Master:

```text
socket()
bind()
listen()
accept()
connect()
send()
recv()
shutdown()
close()
```

TCP server:

```text
socket
   ↓
bind
   ↓
listen
   ↓
accept
   ↓
recv/send
   ↓
close
```

TCP client:

```text
socket
   ↓
connect
   ↓
send/recv
   ↓
close
```

---

# 36. I/O MULTIPLEXING

Learn:

```text
select()
    ↓
poll()
    ↓
epoll()
```

For Linux server development, understand `epoll` deeply.

Concept:

```text
Many sockets
     ↓
epoll
     ↓
Ready events
     ↓
Application processes ready FDs
```

---

# 37. BLOCKING VS NON-BLOCKING

Blocking:

```c
read(fd, buffer, size);
```

may wait.

Non-blocking:

```c
fcntl(fd, F_SETFL, O_NONBLOCK);
```

Then:

```text
read()
   ↓
data available?
   │
   ├── yes → return data
   │
   └── no → EAGAIN/EWOULDBLOCK
```

---

# 38. PHASE 2 — LINUX KERNEL INTERNALS

Now move from APIs to implementation.

---

# 39. LINUX KERNEL ARCHITECTURE

```text
+----------------------------------+
| User Applications                |
+----------------------------------+
| C Library / Runtime              |
+----------------------------------+
| System Call Interface            |
+----------------------------------+
|                                  |
| Linux Kernel                     |
|                                  |
| Process / Scheduler              |
| Memory Management                |
| VFS / Filesystems                |
| Networking                       |
| IPC                              |
| Drivers                          |
| Security                         |
|                                  |
+----------------------------------+
| Hardware                         |
+----------------------------------+
```

---

# 40. SYSTEM CALL FLOW

Example:

```c
read(fd, buffer, 100);
```

Conceptually:

```text
Application
    ↓
glibc wrapper
    ↓
syscall
    ↓
CPU enters kernel mode
    ↓
sys_read()
    ↓
VFS
    ↓
file->f_op->read_iter()
    ↓
Filesystem
    ↓
Block layer
    ↓
Driver
    ↓
Device
```

Exact internal functions vary by kernel version/filesystem, but this architecture is the important interview model.

---

# 41. task_struct

Linux process/thread representation:

```c
struct task_struct
```

Conceptually contains/accesses information related to:

```text
PID
Parent
State
Scheduling
Memory
Credentials
Files
Signals
Namespaces
```

Important associated structures:

```text
task_struct
     |
     +── mm_struct
     |
     +── files_struct
     |
     +── fs_struct
     |
     +── signal_struct
     |
     +── cred
```

---

# 42. PROCESS CREATION INTERNALS

```text
fork()
  ↓
system call
  ↓
kernel process creation
  ↓
task_struct created
  ↓
resources/metadata prepared
  ↓
address-space references established
  ↓
COW mappings
  ↓
child runnable
```

---

# 43. clone()

Linux uses the concept of `clone()` to control which resources are shared.

Conceptually:

```text
clone()
  |
  +-- share VM?
  +-- share files?
  +-- share filesystem info?
  +-- share signal handlers?
```

Threads can therefore be viewed as tasks sharing selected resources.

---

# 44. SCHEDULER INTERNALS

Understand:

```text
task_struct
     ↓
Scheduling policy/class
     ↓
Run queue
     ↓
Scheduler
     ↓
CPU
```

Learn:

* Preemption
* Scheduling classes
* Runnable tasks
* Context switching
* CPU affinity
* Priority
* Real-time scheduling
* CFS concepts
* `SCHED_FIFO`
* `SCHED_RR`
* `SCHED_OTHER`

---

# 45. MEMORY MANAGEMENT INTERNALS

Important structures:

```text
mm_struct
     |
     +── virtual memory areas
     |
     +── page tables
     |
     +── memory mappings
```

Important concepts:

```text
mm_struct
vm_area_struct
page tables
struct page
page fault
COW
anonymous memory
file-backed memory
```

---

# 46. VMA

A VMA describes a virtual memory region.

Examples:

```text
Code
Heap
Stack
Shared library
mmap region
```

Conceptually:

```text
Process address space

+------------------+
| VMA: stack       |
+------------------+
| VMA: mmap        |
+------------------+
| VMA: heap        |
+------------------+
| VMA: executable  |
+------------------+
```

---

# 47. PAGE CACHE

Very important for Linux filesystem interviews.

Conceptually:

```text
Application
     ↓
read()
     ↓
VFS
     ↓
Filesystem
     ↓
Page Cache
     │
     ├── hit → return data
     │
     └── miss
           ↓
       Filesystem
           ↓
       Block layer
           ↓
       Disk
```

The page cache caches filesystem file data in memory.

---

# 48. VFS

Virtual File System provides a common interface to filesystems.

Applications do:

```text
open()
read()
write()
close()
```

without caring whether the filesystem is:

```text
ext4
xfs
tmpfs
proc
sysfs
NFS
...
```

---

# 49. VFS CORE OBJECTS

Master these:

```text
super_block
inode
dentry
file
```

Relationship:

```text
Filesystem
    |
    ▼
super_block
    |
    ▼
inode
    |
    ▼
dentry
    |
    ▼
file
```

But remember they represent different concepts.

---

# 50. PATH LOOKUP

For:

```text
/home/user/test.txt
```

Conceptually:

```text
Path
 ↓
Dentry lookup
 ↓
Directory hierarchy
 ↓
Final dentry
 ↓
inode
```

Then:

```text
open()
   ↓
struct file
```

---

# 51. struct file

Important fields:

```c
struct file
{
    f_op;
    f_inode;
    f_pos;
    f_flags;
    f_mode;
    f_path;
    private_data;
};
```

Conceptually:

```text
FD
 ↓
struct file
 ├── f_op
 ├── f_inode
 ├── f_pos
 ├── f_flags
 ├── f_mode
 ├── f_path
 └── private_data
```

---

# 52. FILE OPEN FLOW

```text
open("/home/user/a.txt")
        ↓
System call
        ↓
Path lookup
        ↓
Dentry
        ↓
Inode
        ↓
Filesystem
        ↓
struct file created
        ↓
FD assigned
        ↓
FD returned to process
```

---

# 53. READ FLOW

```text
read(fd)
    ↓
fd table
    ↓
struct file
    ↓
f_op
    ↓
VFS
    ↓
filesystem
    ↓
page cache
    ↓
block layer if needed
    ↓
storage driver
    ↓
device
```

---

# 54. WRITE FLOW

```text
write(fd)
    ↓
VFS
    ↓
filesystem
    ↓
page cache
    ↓
dirty page
    ↓
writeback
    ↓
filesystem/block layer
    ↓
driver
    ↓
disk
```

Important:

> A successful `write()` does not necessarily mean the data has reached stable storage.

Learn:

```text
fsync()
fdatasync()
O_SYNC
O_DSYNC
```

---

# 55. FILESYSTEM MOUNT

Example:

```bash
mkfs.ext4 /dev/sdb1
mount /dev/sdb1 /mnt/data
```

Conceptual flow:

```text
Block device
    ↓
ext4 filesystem
    ↓
mount()
    ↓
VFS
    ↓
filesystem mount operation
    ↓
super_block
    ↓
root inode/dentry
    ↓
mounted filesystem
```

---

# 56. EXT4 INTERNAL MODEL

Understand:

```text
Application
    ↓
VFS
    ↓
ext4
    ↓
Page Cache
    ↓
Block Layer
    ↓
Block Device Driver
    ↓
Disk
```

Learn:

* Superblock
* Block groups
* Inodes
* Extents
* Journaling
* Directory structures
* Delayed allocation
* Writeback

---

# 57. BLOCK LAYER

Storage path:

```text
Application
    ↓
VFS
    ↓
Filesystem
    ↓
Block layer
    ↓
I/O scheduler
    ↓
Block driver
    ↓
Device
```

Important concepts:

```text
bio
request
blk-mq
I/O scheduler
queue
block device
```

---

# 58. DEVICE DRIVERS

Understand:

```text
User application
      ↓
system call
      ↓
VFS / subsystem
      ↓
driver
      ↓
hardware
```

Important APIs/concepts:

```text
open
read
write
ioctl
mmap
poll
```

For character drivers:

```text
file_operations
```

---

# 59. LINUX KERNEL MODULE

Basic module:

```c
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

Build/load concept:

```bash
make
sudo insmod mymodule.ko
lsmod
dmesg
sudo rmmod mymodule
```

---

# 60. KERNEL SYNCHRONIZATION

User-space:

```text
pthread_mutex
pthread_rwlock
condition variable
semaphore
atomic
```

Kernel:

```text
mutex
spinlock
rwlock
semaphore
completion
atomic_t
refcount_t
RCU
```

Understand when to use:

```text
mutex     → may sleep
spinlock  → short critical section
RCU       → read-heavy synchronization
atomic    → simple atomic operations
```

---

# 61. INTERRUPTS

Hardware:

```text
Device
   ↓
Interrupt
   ↓
CPU
   ↓
Interrupt handler
```

Linux historically distinguishes:

```text
Top half
Bottom half
```

Modern mechanisms include:

```text
softirq
tasklet
workqueue
threaded interrupt
```

Important principle:

> Interrupt context cannot arbitrarily sleep.

---

# 62. KERNEL MEMORY ALLOCATION

Important APIs/concepts:

```text
kmalloc()
kzalloc()
vmalloc()
alloc_pages()
GFP_KERNEL
GFP_ATOMIC
slab/slub
```

Understand:

```text
kmalloc
   ↓
physically contiguous memory for allocation purposes

vmalloc
   ↓
virtually contiguous
   ↓
physical pages need not be contiguous
```

---

# 63. KERNEL BOOT FLOW

High-level:

```text
Firmware
   ↓
Bootloader
   ↓
Kernel image
   ↓
Kernel initialization
   ↓
Memory initialization
   ↓
Scheduler
   ↓
Drivers/subsystems
   ↓
init/systemd
   ↓
User space
```

Learn:

```text
BIOS/UEFI
GRUB
kernel decompression
start_kernel()
initcalls
init/systemd
```

---

# 64. /proc AND /sys

`/proc`:

```text
Process/kernel runtime information
```

Examples:

```bash
/proc/cpuinfo
/proc/meminfo
/proc/<pid>/maps
/proc/<pid>/fd
/proc/<pid>/status
```

`/sys`:

```text
Kernel device/driver/subsystem information
```

Important distinction:

```text
/proc → process/kernel information
/sys  → device/kernel object model
```

---

# 65. DEBUGGING TOOLCHAIN

Master these:

```text
gdb
strace
ltrace
perf
ftrace
dmesg
/proc
/sys
ps
top
htop
vmstat
iostat
sar
ss
tcpdump
```

---

# 66. GDB

Important commands:

```gdb
run
break main
next
step
continue
print
info locals
info threads
thread <id>
bt
frame
watch
x
disassemble
```

For multithreading:

```gdb
info threads
thread 2
bt
```

---

# 67. STRACE

See system calls:

```bash
strace ./program
```

Example:

```text
openat(...)
read(...)
write(...)
close(...)
```

Excellent for understanding:

```text
Application
     ↓
System calls
     ↓
Kernel
```

---

# 68. PERF

Basic examples:

```bash
perf stat ./program
perf record ./program
perf report
```

Learn:

```text
CPU cycles
instructions
cache misses
branch misses
context switches
page faults
```

---

# 69. INTERVIEW-IMPORTANT QUESTIONS

## Processes

1. What is a process?
2. What happens during fork()?
3. Why is fork() fast?
4. What is Copy-On-Write?
5. What happens during exec()?
6. Difference between fork() and exec()?
7. Zombie vs orphan?
8. What happens when parent exits?
9. What is task_struct?
10. Process vs thread?

## Threads

1. What does a thread share?
2. What does a thread not share?
3. Why are threads cheaper?
4. What is a race condition?
5. Mutex vs semaphore?
6. Mutex vs spinlock?
7. Condition variable?
8. Deadlock?
9. Starvation?
10. Priority inversion?

## Memory

1. Virtual vs physical memory?
2. What is a page?
3. What is a page table?
4. What is TLB?
5. What is a page fault?
6. What is Copy-On-Write?
7. malloc vs mmap?
8. mmap private vs shared?
9. What is memory fragmentation?
10. What is page cache?

## Linux System Programming

Master:

```text
open
close
read
write
pread
pwrite
lseek
stat
fcntl
ioctl
dup
dup2
pipe
fork
exec
wait
waitpid
mmap
munmap
brk
select
poll
epoll
socket
bind
listen
accept
connect
send
recv
shutdown
```

## Kernel

1. What is task_struct?
2. What is mm_struct?
3. What is vm_area_struct?
4. How does fork implement COW?
5. How does open() work internally?
6. What is VFS?
7. inode vs dentry vs file?
8. What is struct file?
9. What is page cache?
10. What happens during read()?
11. What happens during write()?
12. What is writeback?
13. What is the block layer?
14. What is bio?
15. What is a kernel module?
16. Mutex vs spinlock?
17. What is interrupt context?
18. What is RCU?
19. What is workqueue?
20. What happens during a system call?

---

# 70. PRACTICAL CODING ROADMAP

Implement these yourself.

## Level 1 — Basic System Calls

```text
1. File copy using open/read/write
2. File statistics
3. File descriptor duplication
4. stdin/stdout redirection
5. mmap allocation
```

## Level 2 — Processes

```text
6. fork example
7. fork + exec
8. parent/child synchronization
9. zombie demonstration
10. process tree
```

## Level 3 — IPC

```text
11. Pipe
12. FIFO
13. Shared memory
14. POSIX semaphore
15. Message queue
16. Unix domain socket
```

## Level 4 — Threads

```text
17. Thread creation
18. Mutex protected counter
19. Producer-consumer
20. Reader-writer problem
21. Thread pool
22. Deadlock demonstration
23. Deadlock prevention
```

## Level 5 — Networking

```text
24. TCP server
25. TCP client
26. UDP server/client
27. Non-blocking socket
28. epoll server
29. Multi-client server
```

## Level 6 — Linux Internals

```text
30. Kernel module
31. Character driver skeleton
32. ioctl example
33. procfs entry
34. Workqueue example
35. Kernel synchronization example
```

---

# 71. MUST-KNOW CODE PATTERNS

For senior interviews, be able to write from memory:

```text
fork()
exec()
waitpid()

open()
read()
write()
close()

dup2()

pipe()

mmap()
munmap()

pthread_create()
pthread_join()

pthread_mutex_lock()
pthread_mutex_unlock()

pthread_cond_wait()
pthread_cond_signal()

socket()
bind()
listen()
accept()
connect()

fcntl(O_NONBLOCK)

epoll_create1()
epoll_ctl()
epoll_wait()
```

---

# 72. FINAL INTEGRATED EXAMPLE

Suppose the interviewer asks:

> "Explain what happens when a process reads a file."

Start from the application:

```text
read(fd, buffer, size)
```

Then explain:

```text
User Process
     ↓
System Call
     ↓
Kernel Mode
     ↓
FD table
     ↓
struct file
     ↓
VFS
     ↓
inode / filesystem
     ↓
Page Cache
     │
     ├── HIT
     │     ↓
     │   Copy data to user buffer
     │
     └── MISS
           ↓
       Filesystem
           ↓
       Block Layer
           ↓
       Driver
           ↓
       Storage Device
           ↓
       Data returned
           ↓
       Page Cache
           ↓
       User Buffer
```

Then discuss:

```text
copy_to_user()
page faults
blocking
I/O scheduling
filesystem
cache
performance
```

This is the level expected from a senior Linux systems engineer.

---

# 73. RECOMMENDED STUDY ORDER

Follow this exact sequence:

```text
PHASE 1
C/C++ fundamentals
        ↓
PHASE 2
OS fundamentals
        ↓
PHASE 3
Processes + threads
        ↓
PHASE 4
Memory management
        ↓
PHASE 5
Synchronization
        ↓
PHASE 6
Files + file descriptors
        ↓
PHASE 7
IPC
        ↓
PHASE 8
Signals
        ↓
PHASE 9
Sockets + epoll
        ↓
PHASE 10
System-call internals
        ↓
PHASE 11
Linux process internals
        ↓
PHASE 12
Linux memory management
        ↓
PHASE 13
VFS + filesystems
        ↓
PHASE 14
Page cache + block layer
        ↓
PHASE 15
Device drivers + kernel modules
        ↓
PHASE 16
Interrupts + synchronization
        ↓
PHASE 17
Kernel boot
        ↓
PHASE 18
Debugging + performance
        ↓
PHASE 19
Networking internals
        ↓
PHASE 20
Senior-level system design
```

---

# 74. PRIORITY FOR SENIOR LINUX INTERVIEWS

## Tier 1 — Must Master

```text
★★★★★

Processes
Threads
fork/exec/wait
Virtual memory
Page tables
COW
Mutex/condition variables
Deadlocks
File descriptors
open/read/write
mmap
IPC
Sockets
epoll
System calls
VFS
inode/dentry/file
Page cache
task_struct
mm_struct
Scheduler
Kernel synchronization
Debugging
```

## Tier 2 — Strong Knowledge

```text
★★★★☆

Signals
ioctl
fcntl
splice
sendfile
io_uring
Block layer
bio
ext4
Writeback
Interrupts
Workqueues
RCU
Kernel modules
Device drivers
Namespaces
cgroups
```

## Tier 3 — Advanced

```text
★★★☆☆

NUMA
Huge pages
THP
Memory reclaim
SLUB internals
RCU internals
Kernel networking internals
eBPF
io_uring internals
Advanced scheduler internals
Storage internals
Virtualization
```

---

# 75. THE MOST IMPORTANT CONNECTIONS TO MEMORIZE

## Process

```text
fork()
 ↓
task_struct
 ↓
mm_struct
 ↓
COW
 ↓
page tables
```

## Thread

```text
pthread_create()
 ↓
Linux task
 ↓
shared address space
 ↓
mm_struct
```

## File

```text
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

## Memory

```text
Virtual Address
 ↓
Page Table
 ↓
Physical Page
 ↓
RAM
```

## File Read

```text
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

## Network

```text
socket()
 ↓
socket FD
 ↓
VFS
 ↓
socket subsystem
 ↓
TCP/IP
 ↓
NIC driver
 ↓
NIC
```

## System Call

```text
User Mode
 ↓
syscall
 ↓
Kernel Mode
 ↓
Kernel subsystem
 ↓
Hardware
```

---

# 76. FINAL REVISION MAP

Before an interview, revise in this order:

```text
                    ┌───────────────┐
                    │   OS BASICS   │
                    └───────┬───────┘
                            ↓
                 ┌────────────────────┐
                 │ PROCESS + THREAD    │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ MEMORY MANAGEMENT   │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ SYNCHRONIZATION    │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ FILES + VFS        │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ IPC + SIGNALS      │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ SOCKETS + EPOLL    │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ KERNEL INTERNALS   │
                 └─────────┬──────────┘
                           ↓
              ┌──────────────────────────┐
              │ VFS / MM / SCHEDULER /   │
              │ BLOCK / DRIVER / NET     │
              └────────────┬─────────────┘
                           ↓
                 ┌────────────────────┐
                 │ DEBUG + PERFORMANCE │
                 └─────────┬──────────┘
                           ↓
                 ┌────────────────────┐
                 │ SYSTEM DESIGN      │
                 └────────────────────┘
```

# 77. Core Objective

By the end, you should be able to answer questions at three levels:

### Level 1 — API

> How do I create a process?

```c
fork();
```

### Level 2 — OS

> What does fork do?

```text
Creates a child execution context
and uses Copy-On-Write for memory.
```

### Level 3 — Kernel

> How does Linux implement it?

```text
fork()
 ↓
kernel process creation
 ↓
task_struct
 ↓
address-space/resource handling
 ↓
page-table/COW setup
 ↓
child becomes runnable
```

That **API → OS → kernel implementation → code → debugging** progression is the core of the entire preparation.
