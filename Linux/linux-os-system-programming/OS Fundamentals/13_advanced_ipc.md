# Chapter 13 — Advanced Linux IPC + Synchronization

> **Three-layer approach**
>
> This chapter covers:
> 1. **[OS] IPC concepts**
> 2. **[LSP] Linux System Programming + C code**
> 3. **[KERNEL] Linux Kernel Internals**
>
> The goal is senior Linux / embedded / systems interview preparation.

---

# 1. What Is IPC?

IPC = **Inter-Process Communication**.

Processes normally have separate virtual address spaces:

```text
Process A
+------------------+
| Virtual Address  |
| Space            |
+------------------+

Process B
+------------------+
| Virtual Address  |
| Space            |
+------------------+
```

IPC mechanisms allow them to exchange:

```text
data
events
synchronization
file descriptors
signals
```

---

# 2. Why Do We Need IPC?

Processes are isolated for safety.

But applications often need cooperation.

Example:

```text
Process A
   |
   | request
   v
Process B
   |
   | response
   v
Process A
```

Examples:

```text
parent ↔ child
client ↔ server
producer ↔ consumer
worker processes
container ↔ host
service ↔ service
```

---

# 3. Major Linux IPC Mechanisms

```text
IPC
 |
 +-- Pipes
 |
 +-- FIFOs
 |
 +-- Unix Domain Sockets
 |
 +-- Shared Memory
 |
 +-- Message Queues
 |
 +-- Signals
 |
 +-- eventfd
 |
 +-- signalfd
 |
 +-- timerfd
 |
 +-- File descriptor passing
```

Synchronization mechanisms often used with IPC:

```text
mutex
semaphore
condition variable
futex
file locks
```

---

# 4. IPC Classification

A useful interview classification:

| Mechanism | Data | Synchronization | Typical scope |
|---|---|---|---|
| Pipe | Byte stream | Yes, implicitly | Related processes |
| FIFO | Byte stream | Yes, implicitly | Unrelated local processes |
| Unix socket | Stream/datagram | Yes | Local processes |
| Shared memory | Shared bytes | No, separately synchronize | Local processes |
| Message queue | Messages | Queue semantics | Local processes |
| Signal | Small event/info | Yes | Local processes |
| eventfd | Counter/event | Yes | Local processes/threads |
| signalfd | Signal events | Yes | Local process |
| timerfd | Timer events | Yes | Local process |

---

# 5. Pipe

A pipe is a unidirectional byte stream.

Create:

```c
int pipefd[2];

pipe(pipefd);
```

Convention:

```text
pipefd[0] → read
pipefd[1] → write
```

Architecture:

```text
Process A
   |
 write()
   |
   v
+---------+
|  PIPE   |
+---------+
   |
 read()
   |
   v
Process B
```

---

# 6. Pipe Example

```c
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int fd[2];

    if (pipe(fd) == -1)
    {
        perror("pipe");
        return 1;
    }

    const char *msg = "hello";

    write(fd[1], msg, strlen(msg));

    char buffer[100] = {0};

    read(fd[0], buffer, sizeof(buffer) - 1);

    printf("Received: %s\n", buffer);

    close(fd[0]);
    close(fd[1]);

    return 0;
}
```

---

# 7. Pipe + `fork()`

This is the classic IPC example.

```text
Parent
  |
  | write
  v
 pipe
  |
  | read
  v
Child
```

Example:

```c
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];

    pipe(fd);

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }

    if (pid > 0)
    {
        close(fd[0]);

        const char *msg = "Hello child";

        write(fd[1], msg, strlen(msg) + 1);

        close(fd[1]);

        wait(NULL);
    }
    else
    {
        close(fd[1]);

        char buffer[100] = {0};

        read(fd[0], buffer, sizeof(buffer));

        printf("Child received: %s\n", buffer);

        close(fd[0]);
    }

    return 0;
}
```

---

# 8. Why Close Unused Pipe Ends?

Suppose:

```text
Parent writes
Child reads
```

Parent should close:

```c
close(fd[0]);
```

Child should close:

```c
close(fd[1]);
```

This is important because EOF for a pipe is observed only when all write references have been closed.

If an unwanted writer FD remains open, the reader may continue waiting instead of seeing EOF.

---

# 9. Pipe Blocking

A pipe can block.

### Reader

If no data is available:

```text
read()
   ↓
sleep
```

### Writer

If the pipe cannot accept more data:

```text
write()
   ↓
may block
```

With non-blocking mode:

```text
EAGAIN/EWOULDBLOCK
```

may be returned.

---

# 10. Pipe and File Descriptors

After:

```c
pipe(fd);
```

the process receives two file descriptors.

```text
Process FD table

fd 3 ─────> pipe read end
fd 4 ─────> pipe write end
```

After `fork()`:

```text
Parent FD table       Child FD table

fd 3 ──┐              fd 3 ──┐
       ├── pipe              ├── pipe
fd 4 ──┘              fd 4 ──┘
```

This explains why unused ends must be closed in both processes.

---

# 11. FIFO

FIFO = named pipe.

Create:

```bash
mkfifo myfifo
```

or:

```c
mkfifo("myfifo", 0666);
```

Unlike an anonymous pipe, a FIFO has a filesystem pathname.

```text
Process A
   |
open("myfifo")
   |
   v
FIFO
   |
   v
Process B
```

---

# 12. FIFO Example

Writer:

```c
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int fd = open("myfifo", O_WRONLY);

    const char *msg = "Hello FIFO";

    write(fd, msg, strlen(msg) + 1);

    close(fd);

    return 0;
}
```

Reader:

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd = open("myfifo", O_RDONLY);

    char buffer[100] = {0};

    read(fd, buffer, sizeof(buffer) - 1);

    printf("Received: %s\n", buffer);

    close(fd);

    return 0;
}
```

Create:

```bash
mkfifo myfifo
```

Run reader and writer separately.

---

# 13. Pipe vs FIFO

| Feature | Pipe | FIFO |
|---|---|---|
| Named | No | Yes |
| Filesystem pathname | No | Yes |
| Related processes | Common | Related/unrelated |
| Byte stream | Yes | Yes |
| `read/write` | Yes | Yes |

---

# 14. Shared Memory

Shared memory allows multiple processes to map the same physical memory into their virtual address spaces.

```text
Process A                Process B

Virtual memory           Virtual memory
     |                        |
     +----------+-------------+
                |
                v
        Shared physical pages
```

It is usually one of the fastest IPC mechanisms for large data because processes can access shared data directly after mapping.

---

# 15. Important Warning: Shared Memory Is Not Synchronization

This is a critical interview point.

Shared memory provides:

```text
shared data
```

It does not automatically provide:

```text
mutual exclusion
ordering
condition signaling
```

Therefore:

```text
Shared memory
     +
mutex/semaphore/futex/etc.
```

is often required.

---

# 16. POSIX Shared Memory

Typical APIs:

```c
shm_open()
ftruncate()
mmap()
munmap()
close()
shm_unlink()
```

Flow:

```text
shm_open()
    ↓
ftruncate()
    ↓
mmap()
    ↓
shared memory access
    ↓
munmap()
    ↓
close()
    ↓
shm_unlink()
```

---

# 17. Shared Memory Example

Creator:

```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int fd = shm_open("/my_shm",
                      O_CREAT | O_RDWR,
                      0666);

    if (fd == -1)
    {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(fd, 4096) == -1)
    {
        perror("ftruncate");
        return 1;
    }

    char *ptr = mmap(NULL,
                     4096,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     fd,
                     0);

    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    strcpy(ptr, "Hello from shared memory");

    munmap(ptr, 4096);
    close(fd);

    return 0;
}
```

Another process can open the same object:

```c
int fd = shm_open("/my_shm", O_RDWR, 0666);
```

and map it using:

```c
mmap(..., MAP_SHARED, fd, 0);
```

---

# 18. Why `MAP_SHARED`?

For IPC shared memory:

```c
MAP_SHARED
```

allows updates to the mapped region to be visible through the shared mapping.

With:

```c
MAP_PRIVATE
```

writes use private copy-on-write semantics and are not intended for shared writable IPC.

---

# 19. `mmap()` and IPC

`mmap()` maps an object into a process's virtual address space.

Conceptually:

```text
Process A VA
     |
     v
+------------+
| shared map |
+------------+
     |
     v
physical pages
     ^
     |
+------------+
| shared map |
+------------+
     ^
     |
Process B VA
```

The virtual addresses do not need to be identical in both processes.

This is important when storing pointers in shared memory.

---

# 20. Shared Memory and Pointers

Do not normally store ordinary process-local pointers in shared memory.

Bad:

```c
struct data
{
    char *ptr;
};
```

Why?

```text
Process A:
ptr = 0x7f123...

Process B:
0x7f123... may mean something completely different.
```

Prefer:

```text
offsets
indexes
fixed-size structures
relative pointers
```

---

# 21. POSIX vs System V IPC

Linux provides both historical System V IPC and POSIX IPC APIs.

System V examples:

```text
shmget()
shmat()
shmdt()
shmctl()

msgget()
msgsnd()
msgrcv()
msgctl()

semget()
semop()
semctl()
```

POSIX examples:

```text
shm_open()
mmap()

mq_open()
mq_send()
mq_receive()

sem_open()
sem_wait()
sem_post()
```

For modern application development, POSIX APIs are often easier to reason about, but senior Linux interviews may ask about both.

---

# 22. Message Queues

Message queues provide message-oriented IPC.

Unlike pipes:

```text
pipe → byte stream
message queue → messages
```

Conceptually:

```text
Producer
   |
   | message
   v
+-------------+
| Message Q   |
+-------------+
   |
   | message
   v
Consumer
```

---

# 23. POSIX Message Queue

APIs:

```c
mq_open()
mq_send()
mq_receive()
mq_close()
mq_unlink()
```

Messages can have priorities.

Conceptually:

```text
priority 10 → message A
priority  5 → message B
priority  1 → message C
```

---

# 24. System V Message Queue

APIs:

```c
msgget()
msgsnd()
msgrcv()
msgctl()
```

System V queues are identified by IPC identifiers rather than normal pathname-based objects.

---

# 25. Pipe vs Message Queue

| Feature | Pipe | Message Queue |
|---|---|---|
| Data model | Byte stream | Messages |
| Message boundaries | No | Yes |
| Priority | No inherent message priority | POSIX MQ supports priorities |
| API | `read/write` | MQ-specific |
| Typical use | Streams | Commands/events/messages |

---

# 26. Signals

Signals are asynchronous notifications delivered to a process/thread.

Examples:

```text
SIGINT
SIGTERM
SIGKILL
SIGSEGV
SIGCHLD
SIGUSR1
SIGUSR2
```

Example:

```text
Process A
   |
   | kill(pid, SIGUSR1)
   v
Process B
```

---

# 27. Installing a Signal Handler

Prefer `sigaction()`.

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig)
{
    printf("Received signal %d\n", sig);
}

int main(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = handler;

    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);

    while (1)
        pause();

    return 0;
}
```

---

# 28. Why `sigaction()`?

`sigaction()` provides more control than the historical `signal()` interface.

It allows configuration of:

```text
handler
signal mask
flags
additional signal behavior
```

For robust Linux code, prefer:

```c
sigaction()
```

---

# 29. Async-Signal Safety

Signal handlers have severe restrictions.

Do not casually call functions such as:

```c
printf()
malloc()
free()
```

inside an asynchronous signal handler.

Many library functions are not async-signal-safe.

A safer pattern is often:

```text
signal handler
     ↓
set atomic flag / write to event FD
     ↓
normal application loop handles event
```

---

# 30. `sig_atomic_t`

A traditional pattern:

```c
volatile sig_atomic_t stop = 0;

void handler(int sig)
{
    stop = 1;
}
```

Main loop:

```c
while (!stop)
{
    ...
}
```

For more complex multithreaded designs, use appropriate C/C++ atomic and synchronization mechanisms.

---

# 31. `kill()`

Despite the name:

```c
kill(pid, signal);
```

does not necessarily mean terminate.

It sends a signal.

Examples:

```c
kill(pid, SIGTERM);
kill(pid, SIGUSR1);
```

---

# 32. SIGTERM vs SIGKILL

### SIGTERM

```text
request termination
```

The process can catch it and clean up.

### SIGKILL

```text
force termination
```

Cannot be caught, blocked, or ignored.

Typical graceful shutdown:

```text
SIGTERM
   ↓
cleanup
   ↓
exit
```

---

# 33. `SIGCHLD`

Parent processes can receive:

```text
SIGCHLD
```

when child state changes.

Common use:

```text
child exits
   ↓
SIGCHLD
   ↓
wait()/waitpid()
   ↓
reap child
```

This connects signals with process lifecycle.

---

# 34. Signals Are Not General Data Channels

Signals are excellent for:

```text
events
notifications
control
```

They are not suitable for transferring large amounts of data.

For data use:

```text
pipe
socket
shared memory
message queue
```

---

# 35. `eventfd`

Linux provides:

```c
eventfd()
```

for event/counter notification.

Example:

```c
int fd = eventfd(0, 0);
```

Conceptually:

```text
Thread A
   |
   | write()
   v
eventfd counter
   |
   | read()
   v
Thread B
```

Because it is an FD, it can integrate with:

```text
poll
select
epoll
```

---

# 36. `eventfd` Example

```c
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/eventfd.h>

int main(void)
{
    int fd = eventfd(0, 0);

    if (fd == -1)
    {
        perror("eventfd");
        return 1;
    }

    uint64_t value = 1;

    if (write(fd, &value, sizeof(value)) != sizeof(value))
    {
        perror("write");
        close(fd);
        return 1;
    }

    uint64_t received;

    if (read(fd, &received, sizeof(received))
        != sizeof(received))
    {
        perror("read");
        close(fd);
        return 1;
    }

    printf("eventfd value = %llu\n",
           (unsigned long long)received);

    close(fd);

    return 0;
}
```

---

# 37. Why `eventfd` Is Useful

It provides a simple bridge between:

```text
event notification
```

and:

```text
FD-based event loops
```

For example:

```text
worker thread
   ↓
eventfd
   ↓
epoll
   ↓
event-loop thread
```

This is extremely useful in Linux system programming.

---

# 38. `signalfd`

`signalfd()` converts signals into an FD-oriented interface.

Conceptually:

```text
signal
   ↓
signalfd
   ↓
read()
   ↓
event loop
```

This is useful when building event-driven applications.

Typical architecture:

```text
epoll
 |
 +-- network sockets
 |
 +-- timerfd
 |
 +-- eventfd
 |
 +-- signalfd
```

One event loop can process different event sources.

---

# 39. `timerfd`

Linux provides:

```c
timerfd_create()
```

to expose timers as file descriptors.

Conceptually:

```text
timer
  ↓
timerfd
  ↓
read()
  ↓
epoll event
```

This avoids relying entirely on signal-based timer handling.

---

# 40. Unified Linux Event Loop

A powerful senior-level pattern:

```text
                 epoll
                   |
       +-----------+-----------+
       |           |           |
      TCP        eventfd     timerfd
      socket        |           |
       |         worker       timer
       |
      client
```

Potentially also:

```text
signalfd
Unix sockets
other pollable FDs
```

This unifies many event sources.

---

# 41. Futex

Futex = **fast userspace mutex**.

The fundamental idea:

```text
fast path
→ user-space atomic operation

slow path
→ kernel wait/wake
```

This avoids a system call for every uncontended lock operation.

Conceptually:

```text
User space
   |
   | atomic compare/exchange
   |
   +---- uncontended → done
   |
   +---- contended
            ↓
          futex
            ↓
      kernel sleep/wake
```

---

# 42. Why Futex Matters

Futexes are a fundamental primitive used by many higher-level synchronization implementations.

Conceptually:

```text
pthread mutex
pthread condition variable
other synchronization primitives
       ↓
    futex
       ↓
kernel wait/wake
```

Do not assume every implementation detail is identical across all libc/kernel versions; understand the abstraction.

---

# 43. Futex System Call

The Linux system call is:

```c
syscall(SYS_futex, ...);
```

The direct futex API is low-level and easy to misuse.

For application code, prefer:

```text
pthread_mutex
pthread_cond
C++ std::mutex
C++ condition_variable
```

unless you specifically need futex-level control.

---

# 44. Wait Queues

Linux kernel code frequently needs:

```text
sleep until condition becomes true
```

Wait queues provide a kernel mechanism for this pattern.

Conceptually:

```text
Task
 ↓
wait queue
 ↓
sleep

event occurs
 ↓
wake_up()
 ↓
task becomes runnable
```

This is closely related to:

```text
blocking I/O
device drivers
networking
process synchronization
```

---

# 45. User-Space Condition Variable vs Kernel Wait Queue

Conceptually:

```text
pthread_cond_wait()
        ↓
libc synchronization
        ↓
futex
        ↓
kernel wait/wake mechanisms
```

Kernel code can directly use:

```text
wait queues
```

The exact implementation path depends on the primitive and situation.

---

# 46. Shared Memory + Mutex

A common design:

```text
Process A                 Process B

producer                  consumer
   |                         |
   +-----------+-------------+
               |
        shared memory
               |
             mutex
               |
          condition
```

The shared memory stores:

```text
data
head
tail
state
```

Synchronization protects:

```text
concurrent access
```

---

# 47. Producer-Consumer Using Shared Memory

Conceptually:

```text
Producer
   |
   | lock
   v
+------------------+
| shared ring buf  |
+------------------+
   |
   | unlock
   v
Consumer
```

A real implementation typically needs:

```text
mutex
condition variable/semaphore
head
tail
capacity
shutdown state
```

---

# 48. Ring Buffer

A ring buffer is commonly used for IPC.

```text
       +---+---+---+---+---+
       | A | B | C |   |   |
       +---+---+---+---+---+
           ^           ^
          tail        head
```

When reaching the end:

```text
index = (index + 1) % capacity;
```

Advantages:

```text
bounded memory
cache-friendly
efficient producer/consumer model
```

---

# 49. Shared-Memory Ring Buffer

Typical layout:

```c
struct ring
{
    size_t head;
    size_t tail;
    size_t capacity;
    char data[...];
};
```

For multiple processes, synchronization and memory-ordering rules are critical.

For multi-producer/multi-consumer lock-free designs, use carefully designed atomic algorithms rather than simply making `head` and `tail` volatile.

---

# 50. `volatile` Is Not Synchronization

Very important interview point.

This:

```c
volatile int ready;
```

does not provide:

```text
atomicity
mutual exclusion
memory ordering
```

Use appropriate:

```text
C atomics
pthread synchronization
semaphores
futexes
```

instead.

---

# 51. IPC and Memory Ordering

With shared memory between processes, concurrent access must obey synchronization rules.

You may need:

```text
atomic operations
acquire/release ordering
mutexes
semaphores
memory barriers
```

The exact mechanism depends on the algorithm.

---

# 52. POSIX Named Semaphore

Create/open:

```c
sem_t *sem = sem_open("/mysem",
                      O_CREAT,
                      0666,
                      1);
```

Wait:

```c
sem_wait(sem);
```

Post:

```c
sem_post(sem);
```

Close:

```c
sem_close(sem);
```

Remove:

```c
sem_unlink("/mysem");
```

---

# 53. Semaphore Example

```c
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>

int main(void)
{
    sem_t *sem = sem_open("/demo_sem",
                          O_CREAT,
                          0666,
                          1);

    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        return 1;
    }

    sem_wait(sem);

    printf("Critical section\n");

    sem_post(sem);

    sem_close(sem);
    sem_unlink("/demo_sem");

    return 0;
}
```

---

# 54. Mutex vs Semaphore

### Mutex

Usually represents ownership:

```text
one owner locks
same owner unlocks
```

Use for:

```text
protecting shared state
```

### Semaphore

Represents a count/resource availability.

Example:

```text
semaphore = 5
```

can represent:

```text
5 available resources
```

Do not use the terms interchangeably in design discussions.

---

# 55. IPC Namespaces

Linux namespaces isolate IPC-related resources.

An IPC namespace can isolate System V IPC and POSIX message queue namespaces.

Useful for:

```text
containers
process isolation
multi-tenant environments
```

Conceptually:

```text
Host
 |
 +-- IPC namespace A
 |
 +-- IPC namespace B
```

Processes in different namespaces may not see the same IPC objects.

---

# 56. File Descriptor Passing

Unix domain sockets can transfer file descriptors between processes.

This is a powerful Linux IPC feature.

Conceptually:

```text
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
   | receives FD
   v
FD usable by B
```

This can transfer access to:

```text
files
sockets
pipes
eventfds
other file-descriptor-backed objects
```

---

# 57. Why FD Passing Is Powerful

It allows a privileged process to create a resource and pass access to another process.

Example:

```text
Privileged service
       |
       | creates socket/device FD
       |
       v
Unix domain socket
       |
       v
Worker process
```

The worker does not necessarily need the same privileges required to create the original resource.

---

# 58. `sendmsg()` / `recvmsg()`

File descriptor passing uses ancillary/control data.

Conceptually:

```text
sendmsg()
   |
   +-- payload
   |
   +-- control message
          |
          +-- SCM_RIGHTS
```

Receiver:

```text
recvmsg()
   ↓
inspect control messages
   ↓
extract received FD
```

---

# 59. Unix Socket + FD Passing Architecture

```text
             Unix Domain Socket
                    |
        +-----------+-----------+
        |                       |
     Process A              Process B
        |                       |
    privileged              unprivileged
        |
     opens FD
        |
        +------ SCM_RIGHTS ---->
                                |
                              new FD
```

This pattern appears in real Linux systems and service architectures.

---

# 60. IPC Performance Comparison

A rough conceptual comparison:

```text
Fast for large shared data
        ↓
Shared memory
        ↓
Unix domain sockets
        ↓
Pipes / FIFOs
        ↓
Message queues
        ↓
Signals for data
```

Do not treat this as a universal benchmark ranking.

Performance depends on:

```text
message size
copy count
synchronization
cache behavior
contention
system-call frequency
CPU/NUMA topology
```

---

# 61. Shared Memory vs Socket

### Shared memory

Advantages:

```text
excellent for large data
avoids repeated copying through a pipe/socket abstraction
high throughput
```

Disadvantages:

```text
synchronization is your responsibility
complex lifecycle
data structures must be designed carefully
```

### Unix socket

Advantages:

```text
simple API
built-in stream/datagram semantics
FD passing
works naturally with epoll
```

Disadvantages:

```text
kernel buffering/copying overhead
```

---

# 62. Pipe vs Unix Socket

Pipe:

```text
simple byte stream
usually one-way
```

Unix socket:

```text
bidirectional
stream/datagram
credentials and FD passing
epoll integration
client/server architecture
```

For sophisticated local IPC, Unix domain sockets are often more flexible.

---

# 63. IPC and File Descriptors

A useful Linux mental model:

```text
                    FD
                     |
       +-------------+-------------+
       |             |             |
      file          pipe         socket
       |             |             |
      VFS            IPC        networking
                     |
               eventfd/timerfd
```

Many Linux mechanisms intentionally expose an FD so that they can integrate with:

```text
poll()
epoll()
select()
```

---

# 64. Unified Event-Driven Architecture

A senior Linux service can look like:

```text
                    epoll
                      |
      +---------------+----------------+
      |               |                |
 TCP socket        eventfd          timerfd
      |               |                |
 clients           worker          timeout
      |
 Unix socket
      |
 control/admin
```

Signal handling can also be integrated through:

```text
signalfd
```

This avoids scattering event handling across many unrelated mechanisms.

---

# 65. IPC Error Handling

Always check return values.

Examples:

```text
pipe()
fork()
read()
write()
mmap()
shm_open()
sem_wait()
mq_send()
socket()
sendmsg()
recvmsg()
```

Typical errors:

```text
EINTR
EAGAIN
EWOULDBLOCK
EPIPE
EBADF
ENOMEM
EINVAL
```

For blocking operations, understand what happens when interrupted by a signal.

---

# 66. `EINTR`

A blocking system call may be interrupted by a signal.

Example:

```c
ssize_t n = read(fd, buf, sizeof(buf));

if (n == -1 && errno == EINTR)
{
    /* retry if appropriate */
}
```

Do not blindly retry every system call forever; understand the operation and application semantics.

---

# 67. IPC Security

IPC is also a security boundary.

Consider:

```text
permissions
ownership
namespaces
credentials
FD inheritance
Unix socket permissions
authentication
authorization
```

For Unix sockets, filesystem permissions may be part of access control for pathname sockets.

---

# 68. `fork()` + IPC

A common pattern:

```text
pipe()
   ↓
fork()
   ↓
parent/child share pipe FDs
   ↓
close unused ends
   ↓
communicate
```

This works because `fork()` duplicates the process's FD references.

---

# 69. `exec()` + IPC

After `exec()`:

```text
program image changes
```

but file descriptors normally remain open unless marked close-on-exec.

Use:

```c
O_CLOEXEC
```

or:

```c
FD_CLOEXEC
```

to avoid unintended FD inheritance.

This is important for:

```text
pipes
sockets
eventfds
files
```

---

# 70. IPC and `FD_CLOEXEC`

Example:

```c
int fd = open("file",
              O_RDONLY | O_CLOEXEC);
```

This helps prevent:

```text
parent FD
   ↓
exec()
   ↓
unexpected child program owns FD
```

For security-sensitive services, unintended descriptor inheritance can be a serious bug.

---

# 71. IPC Interview Question

> Why is shared memory usually faster than a pipe?

Answer:

```text
A pipe uses a kernel-managed communication buffer and the
application interacts with it through system calls.

Shared memory allows both processes to map the same memory pages
and directly access the shared data. This can reduce data-copying
and system-call overhead.

However, shared memory requires explicit synchronization and careful
memory-ordering/lifetime management.
```

---

# 72. IPC Interview Question

> Why use Unix domain sockets instead of TCP on the same machine?

Answer:

```text
Unix domain sockets are designed for local IPC. They avoid the need
for IP routing and network interfaces and provide a convenient local
socket API. They also support useful Linux features such as passing
file descriptors and peer credential mechanisms.

They are often preferable for local service-to-service communication.
```

---

# 73. IPC Interview Question

> Why use eventfd instead of a pipe for notification?

Answer:

```text
eventfd provides a lightweight counter/event abstraction and is
naturally represented as a file descriptor, so it integrates cleanly
with epoll.

It is useful when the application needs notification rather than
arbitrary byte-stream data.
```

---

# 74. IPC Interview Question

> What is a futex?

Answer:

```text
A futex is a Linux synchronization primitive designed around a
userspace fast path and kernel-assisted wait/wake operations.

Uncontended synchronization can often be handled using atomic
userspace operations without entering the kernel. The kernel is
needed when a thread/process must sleep or be woken because of
contention.
```

---

# 75. IPC Interview Question

> Why can't we simply use volatile for shared-memory synchronization?

Answer:

```text
volatile does not provide atomicity, mutual exclusion or the
required inter-thread/inter-process memory ordering.

Synchronization requires proper atomics, mutexes, semaphores,
futexes or another appropriate synchronization mechanism.
```

---

# 76. IPC Interview Question

> How can you integrate signals into an epoll-based server?

Answer:

```text
One option is signalfd.

Block the signals normally, create a signalfd for the desired signal
set, add that FD to epoll, and process signal events from the normal
event loop.

This keeps asynchronous signal handling integrated with the rest of
the FD-based event model.
```

---

# 77. IPC Interview Question

> How can one process pass a socket to another process?

Answer:

```text
Use a Unix domain socket and sendmsg()/recvmsg() with SCM_RIGHTS.

The receiving process obtains a new file descriptor referring to the
same underlying open resource.
```

---

# 78. IPC Interview Question

> What happens if a pipe writer closes its FD?

If all write references are closed:

```text
reader eventually observes EOF
```

For example:

```c
read(fd[0], buffer, sizeof(buffer));
```

returns:

```text
0
```

when there is no remaining data and all writers have closed their ends.

---

# 79. IPC Interview Question

> Can two unrelated processes use shared memory?

Yes.

They can both open/map a shared-memory object, for example through:

```text
shm_open()
mmap(MAP_SHARED)
```

The processes still need synchronization.

---

# 80. IPC Interview Question

> Why are pointers dangerous in shared memory?

Because virtual addresses are process-specific.

Example:

```text
Process A:
0x70000000 → object A

Process B:
0x70000000 → something else
```

Use:

```text
offsets
indexes
relative pointers
```

or map the region consistently when an appropriate design requires it.

---

# 81. Kernel Internals — IPC Mental Model

High-level:

```text
User process
     |
     | system call
     v
+----------------------+
| Linux IPC subsystem  |
+----------------------+
     |
     +--> pipe
     |
     +--> socket
     |
     +--> shared memory
     |
     +--> message queue
     |
     +--> futex
     |
     +--> eventfd
     |
     +--> signal subsystem
```

---

# 82. Pipe Kernel Internals

Conceptually:

```text
write()
   ↓
kernel pipe state
   ↓
pipe buffer
   ↓
wake reader
   ↓
read()
```

The pipe has kernel-managed state and buffering.

Blocking writers/readers interact with kernel wait mechanisms.

---

# 83. Pipe + Wait Queues

Simplified:

```text
Reader
  |
  | read()
  | no data
  v
wait queue
  |
 sleep
  |
  | writer adds data
  v
wake reader
```

Writer:

```text
write()
  ↓
pipe buffer
  ↓
wake readers
```

This connects IPC with the kernel scheduler and wait queues.

---

# 84. Socket Kernel Internals

Conceptually:

```text
FD
 ↓
struct file
 ↓
socket
 ↓
struct sock
 ↓
TCP/UDP state
 ↓
buffers / queues
```

For network packets:

```text
NIC
 ↓
driver/NAPI
 ↓
skb
 ↓
protocol stack
 ↓
socket
```

---

# 85. Shared Memory Kernel Internals

At a high level:

```text
shm object / backing pages
        |
        v
process A page tables
        |
        +------ physical pages
        |
        v
process B page tables
```

The two processes can have different virtual addresses that map to the same underlying pages.

This connects:

```text
IPC
+
virtual memory
+
page tables
+
TLB
```

from earlier chapters.

---

# 86. Futex Kernel Internals

Simplified:

```text
userspace atomic state
        |
        | contention
        v
     futex()
        |
        v
kernel futex wait structure
        |
        v
task sleeps
        |
        v
wake operation
        |
        v
task runnable
```

The kernel does not need to be entered for every uncontended lock operation.

---

# 87. Eventfd Kernel Internals

Conceptually:

```text
eventfd FD
   |
   v
kernel counter
   |
   +--> write increments
   |
   +--> read consumes/returns value
   |
   +--> wait queue for blocking
   |
   +--> poll/epoll readiness
```

This explains why eventfd integrates naturally with event loops.

---

# 88. Signalfd Kernel Internals

Conceptually:

```text
signal generation
      ↓
signal pending state
      ↓
signalfd
      ↓
FD readable
      ↓
read()
```

The application can process the signal as normal event-loop input.

---

# 89. Timerfd Kernel Internals

Conceptually:

```text
kernel timer
    ↓
expiration
    ↓
timerfd state
    ↓
FD becomes readable
    ↓
epoll event
    ↓
read()
```

The read returns timer-expiration information according to timerfd semantics.

---

# 90. IPC + Scheduler

Many IPC operations eventually interact with scheduling.

Example:

```text
Process A
   |
   | read()
   | no data
   v
sleep
   |
   | scheduler
   v
Process B runs
   |
   | write()
   v
wake A
   |
   v
A becomes runnable
```

This is why IPC knowledge cannot be separated from:

```text
processes
threads
scheduler
wait queues
```

---

# 91. IPC + Virtual Memory

Shared memory demonstrates the connection:

```text
Process A VA
      ↓
   page table
      ↓
physical page
      ↑
   page table
      ↑
Process B VA
```

The same physical memory can be mapped into multiple processes.

This connects Chapter 7/8 with Chapter 13.

---

# 92. IPC + Networking

Unix domain sockets use the socket abstraction:

```text
Application
   ↓
socket API
   ↓
Unix domain socket
   ↓
kernel
   ↓
other process
```

TCP networking uses:

```text
socket API
   ↓
TCP/IP
   ↓
NIC
   ↓
network
```

Same high-level socket API, different protocol family/path.

---

# 93. Choosing an IPC Mechanism

Use a decision process.

### Need simple parent-child stream?

```text
pipe
```

### Need named local stream?

```text
FIFO
```

### Need rich local client/server IPC?

```text
Unix domain socket
```

### Need very large/high-throughput shared data?

```text
shared memory
```

### Need message boundaries/priorities?

```text
message queue
```

### Need simple event notification?

```text
eventfd
```

### Need integrate signals into epoll?

```text
signalfd
```

### Need timers in an event loop?

```text
timerfd
```

### Need synchronization primitive?

```text
mutex / semaphore / futex / condition variable
```

---

# 94. IPC Decision Table

| Requirement | Good candidate |
|---|---|
| Parent-child simple stream | Pipe |
| Named byte stream | FIFO |
| Local client/server | Unix socket |
| Large shared data | Shared memory |
| Message semantics | Message queue |
| Event notification | eventfd |
| Signal event loop | signalfd |
| Timer event loop | timerfd |
| Mutual exclusion | Mutex |
| Resource counting | Semaphore |
| Low-level wait/wake | Futex |

---

# 95. Senior Practice Programs

Implement these in order:

```text
1. pipe + fork
2. FIFO reader/writer
3. POSIX shared memory producer/consumer
4. shared-memory ring buffer
5. POSIX message queue
6. signal handling with sigaction
7. eventfd + pthread
8. timerfd + epoll
9. signalfd + epoll
10. Unix socket client/server
11. Unix socket FD passing
12. epoll server using socket + eventfd + timerfd
```

---

# 96. Most Important APIs to Memorize

## Pipes

```c
pipe()
read()
write()
close()
```

## FIFO

```c
mkfifo()
open()
read()
write()
unlink()
```

## Shared memory

```c
shm_open()
ftruncate()
mmap()
munmap()
shm_unlink()
```

## Message queues

```c
mq_open()
mq_send()
mq_receive()
mq_close()
mq_unlink()
```

## Signals

```c
sigaction()
sigprocmask()
sigwaitinfo()
kill()
```

## Linux FD-based events

```c
eventfd()
signalfd()
timerfd_create()
```

## Synchronization

```c
pthread_mutex_lock()
pthread_mutex_unlock()
sem_wait()
sem_post()
```

## FD passing

```c
sendmsg()
recvmsg()
SCM_RIGHTS
```

---

# 97. One-Minute Revision

```text
Pipe
→ unnamed byte stream

FIFO
→ named pipe

Shared memory
→ shared mapped memory; synchronization is separate

Message queue
→ message-oriented IPC

Unix domain socket
→ powerful local IPC + FD passing

Signal
→ asynchronous event/notification

eventfd
→ counter/event represented as FD

signalfd
→ signal events represented as FD

timerfd
→ timer events represented as FD

futex
→ userspace fast path + kernel wait/wake

wait queue
→ kernel sleep/wake mechanism

SCM_RIGHTS
→ pass FDs over Unix sockets

volatile
→ NOT synchronization

MAP_SHARED
→ shared mapping semantics

FD_CLOEXEC
→ prevent unintended FD inheritance across exec
```

---

# 98. Final Mental Model

```text
                         LINUX IPC
                            |
        +-------------------+-------------------+
        |                   |                   |
      DATA                 EVENTS          SYNCHRONIZATION
        |                   |                   |
   +----+----+          +---+---+          +----+----+
   |         |          |       |          |         |
 pipe      shared     signal  eventfd    mutex    semaphore
 socket    memory
 queue                   |       |
   |                      |       |
   +----------+-----------+-------+
              |
              v
         file descriptors
              |
              v
             epoll
              |
              v
        event-driven app
```

---

# 99. Chapter 13 — Senior Interview Checklist

Before moving forward, be able to explain without notes:

```text
[ ] Pipe vs FIFO
[ ] Why close unused pipe ends
[ ] Pipe blocking behavior
[ ] Shared memory architecture
[ ] Why shared memory needs synchronization
[ ] mmap(MAP_SHARED)
[ ] Why pointers are dangerous in shared memory
[ ] POSIX vs System V IPC
[ ] Message queues
[ ] Signals and sigaction
[ ] Async-signal-safe functions
[ ] SIGTERM vs SIGKILL
[ ] eventfd
[ ] signalfd
[ ] timerfd
[ ] Futex
[ ] Wait queues
[ ] FD passing / SCM_RIGHTS
[ ] IPC namespaces
[ ] FD_CLOEXEC
[ ] IPC + scheduler relationship
[ ] IPC + virtual memory relationship
[ ] IPC + epoll relationship
[ ] Choosing IPC based on requirements
```

---

# 100. Chapter 13 Core Takeaway

The most important Linux IPC mental model is:

```text
Process isolation
       ↓
Need communication
       ↓
Choose IPC mechanism
       ↓
+-------------------------------+
| pipe / FIFO                   |
| Unix socket                   |
| shared memory                 |
| message queue                 |
| signal                        |
| eventfd / signalfd / timerfd  |
+-------------------------------+
       ↓
Synchronization if required
       ↓
mutex / semaphore / futex
       ↓
Kernel wait/wake mechanisms
       ↓
Scheduler
```

For senior Linux interviews, do not learn IPC as a list of APIs.

Understand the **complete path**:

```text
User API
   ↓
system call / libc
   ↓
kernel IPC object
   ↓
buffer / shared memory / queue
   ↓
wait queue / wakeup when required
   ↓
scheduler
   ↓
other process/thread
```

That connects **OS fundamentals + Linux system programming + Linux kernel internals** into one model.
