# Chapter 12 – Linux IPC (Inter-Process Communication)

---

# 1. What Is IPC?

IPC stands for:

```text
Inter-Process Communication
```

Processes normally have separate virtual address spaces.

```text
Process A                    Process B
   |                            |
   v                            v
Address Space A             Address Space B
```

Therefore, processes need kernel-supported mechanisms to communicate and synchronize.

Common IPC mechanisms:

```text
Pipes
FIFOs
Signals
Message Queues
Shared Memory
Semaphores
Sockets
Unix Domain Sockets
```

---

# 2. Why Do We Need IPC?

Suppose:

```text
Process A
   |
   | produces data
   v
Process B
   |
   | processes data
   v
Process C
```

IPC allows them to exchange information.

IPC can provide:

```text
Data transfer
Synchronization
Notification
Event signaling
Request/response communication
```

---

# 3. IPC Categories

A useful classification:

```text
IPC
 |
 +-- Data Transfer
 |     |
 |     +-- Pipe
 |     +-- FIFO
 |     +-- Message Queue
 |     +-- Socket
 |
 +-- Shared Data
 |     |
 |     +-- Shared Memory
 |
 +-- Synchronization
 |     |
 |     +-- Semaphore
 |     +-- Mutex
 |
 +-- Notification
       |
       +-- Signal
```

---

# 4. IPC Through the Kernel

Most traditional IPC mechanisms involve the kernel.

Conceptually:

```text
Process A
    |
    v
 System Call
    |
    v
 Linux Kernel
    |
    v
 IPC Object
    |
    v
Process B
```

Shared memory is different because, after setup, processes can access the same physical memory through their virtual address spaces without copying every byte through the kernel.

---

# 5. Pipe

A pipe provides a byte stream between processes.

Typical flow:

```text
Process A
    |
    | write()
    v
+-----------+
|   Pipe    |
+-----------+
    |
    | read()
    v
Process B
```

Create:

```c
int fd[2];

pipe(fd);
```

Usually:

```text
fd[0] → read
fd[1] → write
```

---

# 6. Pipe Example

```c
int fd[2];

pipe(fd);

if (fork() == 0) {
    close(fd[1]);

    char buf[100];
    read(fd[0], buf, sizeof(buf));

    close(fd[0]);
}
else {
    close(fd[0]);

    write(fd[1], "hello", 5);

    close(fd[1]);
}
```

Conceptually:

```text
Parent
  |
 write()
  |
  v
 Pipe
  |
 read()
  |
  v
Child
```

---

# 7. Pipe Characteristics

Important properties:

```text
Byte stream
Usually unidirectional
Kernel-managed buffer
File-descriptor based
Supports blocking I/O
```

A pipe does not preserve message boundaries.

If the writer performs:

```text
write("ABC")
write("DEF")
```

the reader sees a byte stream rather than two guaranteed application-level messages.

---

# 8. Blocking Pipe

Suppose the pipe is empty:

```text
Reader
  |
 read()
  |
  v
Pipe empty
  |
  v
Reader sleeps
```

When data arrives:

```text
Writer
  |
 write()
  |
  v
Pipe
  |
  v
Wake reader
```

This is an important example of IPC + scheduling interaction.

---

# 9. Anonymous Pipe

An ordinary pipe is commonly used between related processes.

Typical pattern:

```text
Parent
  |
 fork()
  |
  +---- Child
```

Both processes inherit the pipe file descriptors.

```text
Parent
   |
   +-- fd
   |
 Child
   |
   +-- fd
```

---

# 10. Named Pipe – FIFO

A FIFO is a named pipe.

It appears in the filesystem namespace.

Create:

```bash
mkfifo myfifo
```

Then:

```text
Process A
   |
   v
myfifo
   |
   v
Process B
```

Unlike an anonymous pipe, unrelated processes can open the FIFO by name.

---

# 11. FIFO vs Pipe

| Feature               | Pipe       | FIFO         |
| --------------------- | ---------- | ------------ |
| Name in filesystem    | No         | Yes          |
| Related processes     | Common use | Not required |
| Byte stream           | Yes        | Yes          |
| Kernel buffer         | Yes        | Yes          |
| File descriptor based | Yes        | Yes          |

---

# 12. Signals

A signal is a lightweight asynchronous notification.

Example:

```text
Process A
   |
   | SIGTERM
   v
Process B
```

Common signals:

```text
SIGINT
SIGTERM
SIGKILL
SIGSTOP
SIGCONT
SIGSEGV
SIGCHLD
```

---

# 13. Signal Delivery

Conceptually:

```text
Sender
  |
  | kill()
  v
Kernel
  |
  v
Target task
  |
  v
Signal becomes pending
  |
  v
Signal handling
```

The kernel manages signal state and delivery.

---

# 14. `kill()` Does Not Necessarily Kill

This is an interview trap.

```c
kill(pid, SIGTERM);
```

does not mean the function always kills the process.

`kill()` sends a signal.

For example:

```text
SIGSTOP → stop
SIGCONT → continue
SIGTERM → termination request
SIGUSR1 → application-defined notification
```

---

# 15. SIGTERM vs SIGKILL

### SIGTERM

```text
Graceful termination request
Can be handled
Can perform cleanup
```

### SIGKILL

```text
Immediate forced termination
Cannot be caught
Cannot be ignored
```

Use:

```text
SIGTERM
```

before:

```text
SIGKILL
```

when graceful shutdown is possible.

---

# 16. Message Queues

Message queues allow processes to exchange discrete messages.

Conceptually:

```text
Process A
   |
   | message
   v
+-------------+
| Message     |
| Queue       |
+-------------+
   |
   | message
   v
Process B
```

Unlike pipes, message queues preserve message boundaries.

---

# 17. Pipe vs Message Queue

Pipe:

```text
byte stream
```

Message queue:

```text
message-oriented
```

Example:

```text
Pipe:
ABCDEF...

Message queue:
[MSG1]
[MSG2]
[MSG3]
```

The distinction is important for protocol design.

---

# 18. POSIX Message Queues

Linux provides POSIX message queues.

Typical APIs include:

```c
mq_open()
mq_send()
mq_receive()
mq_close()
mq_unlink()
```

Conceptually:

```text
Producer
   |
 mq_send()
   |
   v
Message Queue
   |
 mq_receive()
   |
   v
Consumer
```

---

# 19. System V IPC

Linux also supports System V IPC mechanisms.

Examples:

```text
System V shared memory
System V semaphores
System V message queues
```

Modern applications often prefer POSIX APIs or other mechanisms depending on requirements.

For interviews, know both names and their basic differences.

---

# 20. Shared Memory

Shared memory allows multiple processes to map the same physical memory into their virtual address spaces.

Conceptually:

```text
Process A                  Process B
    |                          |
    v                          v
Virtual Address A          Virtual Address B
    |                          |
    +-----------+--------------+
                |
                v
         Shared Physical Memory
```

This can provide very high throughput.

---

# 21. Why Shared Memory Is Fast

Consider a pipe:

```text
Process A
   |
   v
Kernel buffer
   |
   v
Process B
```

Data typically moves through kernel-managed buffering.

With shared memory:

```text
Process A
    \
     \
      v
 Shared Memory
      ^
     /
    /
Process B
```

Both processes can directly access the shared mapped region.

Therefore large data transfers can avoid repeated copying.

---

# 22. Shared Memory Does NOT Solve Synchronization

This is extremely important.

Suppose:

```text
Process A
    |
    +-- writes shared data

Process B
    |
    +-- reads shared data
```

Without synchronization:

```text
Race condition
```

Shared memory must usually be combined with synchronization.

For example:

```text
Shared Memory
      +
Semaphore / Mutex
      +
Memory ordering
```

---

# 23. Shared Memory Producer-Consumer

A common design:

```text
Producer
   |
   v
+----------------+
| Shared Memory  |
|                |
| Ring Buffer    |
+----------------+
   |
   v
Consumer
```

Producer:

```text
write data
update producer index
```

Consumer:

```text
read data
update consumer index
```

Synchronization is required to safely coordinate ownership.

---

# 24. Ring Buffer IPC

A ring buffer is frequently used for high-performance IPC.

```text
+----+----+----+----+----+
| D0 | D1 | D2 | D3 | D4 |
+----+----+----+----+----+
  ^                   ^
  |                   |
read                write
```

When the end is reached:

```text
write → wraps around
```

This avoids continuously allocating/freeing buffers.

---

# 25. Semaphore

A semaphore is primarily a synchronization/counting mechanism.

Conceptually:

```text
Semaphore = count
```

Operations:

```text
wait/down
signal/up
```

Example:

```text
count = 3
```

means up to three units of some resource are available.

---

# 26. Binary Semaphore

A semaphore with two logical states can be used for synchronization.

Conceptually:

```text
1 → available
0 → unavailable
```

However, for mutual exclusion in modern Linux user-space applications, a mutex is generally a better semantic choice.

---

# 27. Mutex

A mutex provides mutual exclusion.

```text
Thread A
   |
 lock
   |
   v
 Critical Section
   |
 unlock
   |
   v

Thread B
   |
 lock
```

Only one owner can hold the mutex at a time.

---

# 28. Mutex vs Semaphore

### Mutex

```text
Ownership
Mutual exclusion
Protect critical section
```

### Semaphore

```text
Counting
Resource availability
Synchronization
```

Do not simply describe both as "locks."

---

# 29. IPC and Synchronization

Consider:

```text
Process A
    |
    | write
    v
Shared Memory
    ^
    | read
    |
Process B
```

Need:

```text
Synchronization
```

For example:

```text
Shared Memory
     +
Semaphore
```

or:

```text
Shared Memory
     +
Process-shared pthread mutex
```

when configured appropriately.

---

# 30. Unix Domain Sockets

Unix domain sockets provide IPC through the socket interface.

```text
Process A
    |
    | socket
    v
Unix Domain Socket
    |
    v
Process B
```

They are particularly useful when applications need:

```text
Request/response
Bidirectional communication
Structured protocols
Credential passing
Local service communication
```

---

# 31. Unix Domain Socket vs TCP

Unix domain socket:

```text
Same machine
No IP networking required
Uses kernel socket infrastructure
```

TCP socket:

```text
Can communicate across machines
Uses IP networking stack
```

For local IPC, Unix domain sockets can avoid the overhead of IP routing and are commonly used by system services.

---

# 32. Socket Types

Unix domain sockets support common socket semantics such as:

```text
SOCK_STREAM
SOCK_DGRAM
SOCK_SEQPACKET
```

For example:

```text
SOCK_STREAM
    ↓
Reliable byte stream

SOCK_DGRAM
    ↓
Datagram/message-oriented communication
```

---

# 33. File Descriptors and IPC

Many Linux IPC mechanisms use file descriptors.

For example:

```text
pipe()
socket()
eventfd()
signalfd()
```

Conceptually:

```text
Process
   |
   +-- fd 3 → pipe
   +-- fd 4 → socket
   +-- fd 5 → file
```

This is a powerful Linux design principle:

> Many kernel resources are exposed through file descriptors.

---

# 34. `eventfd`

`eventfd` provides a lightweight event notification mechanism using a file descriptor.

Conceptually:

```text
Producer
   |
   | write/update eventfd
   v
eventfd
   |
   | read
   v
Consumer
```

It is useful for:

```text
Thread notification
Event notification
Polling/epoll integration
Kernel/user synchronization
```

---

# 35. `eventfd` + `epoll`

A powerful Linux pattern:

```text
Application
    |
    +-- socket
    +-- eventfd
    +-- pipe
    |
    v
  epoll
    |
    v
Wait for events
```

The application can monitor multiple event sources using one event loop.

---

# 36. `signalfd`

`signalfd` allows signals to be consumed through a file descriptor.

Conceptually:

```text
Signal
   |
   v
signalfd
   |
   v
read()
   |
   v
Application
```

This can integrate signal handling into an event-driven architecture.

---

# 37. IPC Through `epoll`

Linux applications often build event-driven systems around:

```text
epoll
```

For example:

```text
             epoll
               |
       +-------+-------+
       |       |       |
       v       v       v
    socket   eventfd  pipe
```

One thread can wait for events from many sources.

---

# 38. IPC Performance

A rough conceptual comparison:

```text
Shared Memory
    ↓
Very high throughput
Needs synchronization

Pipe
    ↓
Simple byte-stream IPC

Unix Domain Socket
    ↓
Flexible local IPC
Request/response friendly

Message Queue
    ↓
Message-oriented IPC

Signal
    ↓
Small notification
```

The "fastest" mechanism depends on workload and implementation details.

---

# 39. IPC Choice

Use:

### Pipe

When:

```text
Simple parent-child stream
```

### FIFO

When:

```text
Unrelated processes
Simple stream
```

### Shared memory

When:

```text
Large/high-throughput data
Low-copy design
```

### Unix domain socket

When:

```text
Local client/server
Bidirectional communication
```

### Message queue

When:

```text
Discrete messages
Priority/message semantics
```

### Signal

When:

```text
Simple asynchronous notification
```

---

# 40. IPC Architecture Example

Consider:

```text
Application
     |
     | request
     v
IPC Service
     |
     | process request
     v
Worker
     |
     | result
     v
Application
```

Possible implementation:

```text
Unix Domain Socket
        +
Shared Memory
        +
eventfd
```

For example:

```text
Socket → control messages
Shared memory → large data
eventfd → notification
```

This combination is common in high-performance designs.

---

# 41. IPC and Kernel Scheduling

IPC often causes tasks to block and wake.

Example:

```text
Producer
   |
 write()
   |
   v
Consumer wakes
   |
   v
Scheduler
   |
   v
Consumer runs
```

Therefore:

```text
IPC
 ↓
Blocking/wakeup
 ↓
Scheduler
```

is an important Linux internals relationship.

---

# 42. IPC and Memory Ordering

Shared-memory IPC introduces another issue:

```text
CPU 0
 |
 +-- Producer

CPU 1
 |
 +-- Consumer
```

Even if both access the same memory, correct synchronization requires appropriate memory ordering.

Conceptually:

```text
Producer:
write data
    ↓
publish state

Consumer:
observe state
    ↓
read data
```

Without correct ordering, the consumer may observe inconsistent state.

This becomes especially important for:

```text
Lock-free queues
Ring buffers
Shared-memory IPC
Multicore systems
```

---

# 43. Race Condition Example

Bad design:

```c
shared_data = 10;
ready = 1;
```

Consumer:

```c
if (ready)
    printf("%d", shared_data);
```

Without proper synchronization, this can be incorrect on a multicore system.

Correct designs use mechanisms such as:

```text
Mutex
Semaphore
Condition variable
Atomic operations
Memory barriers / appropriate memory ordering
```

---

# 44. IPC Failure Modes

Senior engineers should think about:

```text
Deadlock
Race condition
Lost notification
Buffer overflow
Backpressure
Priority inversion
Resource exhaustion
Process crash
Peer disappearance
Partial messages
Ordering
Memory visibility
```

IPC design is not simply about transferring data.

---

# 45. Backpressure

Suppose:

```text
Producer
   |
   | FAST
   v
IPC Buffer
   |
   | SLOW
   v
Consumer
```

Eventually:

```text
Buffer full
```

The system needs a policy:

```text
Block producer
Drop data
Overwrite old data
Expand buffer
Apply flow control
```

This is called handling **backpressure**.

---

# 46. Blocking vs Nonblocking IPC

Blocking:

```text
No data
  |
  v
Process sleeps
```

Nonblocking:

```text
No data
  |
  v
Return immediately
```

Example:

```text
O_NONBLOCK
```

Nonblocking IPC is commonly combined with:

```text
poll()
select()
epoll()
```

---

# 47. IPC with `poll`/`epoll`

Instead of:

```text
read(socket1)
read(socket2)
read(pipe)
```

and potentially blocking on the wrong one:

```text
epoll
 |
 +-- socket1 ready
 +-- socket2 not ready
 +-- pipe ready
```

The application processes only ready sources.

This is fundamental to scalable Linux servers.

---

# 48. IPC Security

IPC also requires security considerations.

Linux uses:

```text
File permissions
UID/GID
Capabilities
Namespaces
SELinux/AppArmor
Socket permissions
```

For example, a Unix domain socket can have filesystem permissions controlling which users can connect.

---

# 49. IPC and Namespaces

Containers use IPC namespaces to isolate certain IPC resources.

Conceptually:

```text
Container A
   |
   +-- IPC namespace A

Container B
   |
   +-- IPC namespace B
```

This prevents processes in different namespaces from automatically seeing the same namespace-scoped IPC resources.

This is important for container internals.

---

# 50. IPC in Containers

A containerized application may use:

```text
Unix sockets
Shared memory
Signals
Pipes
eventfd
```

but namespace isolation determines visibility of many resources.

Therefore:

```text
Containers
   |
   v
Namespaces
   |
   v
IPC isolation
```

---

# 51. IPC Interview Question

### Why is shared memory faster than a pipe?

A good answer:

> Shared memory allows processes to map the same memory region and exchange data without requiring the same data-transfer path through kernel buffering for every operation. This can provide very high throughput, but synchronization and memory ordering become the application's responsibility.

---

# 52. IPC Interview Question

### Pipe vs shared memory?

```text
Pipe
    Simple
    Kernel-managed buffer
    Byte stream
    Easier synchronization semantics

Shared memory
    Very high throughput
    Large data
    Requires explicit synchronization
    More complex
```

---

# 53. IPC Interview Question

### Why use Unix domain sockets instead of TCP on the same machine?

Because they provide socket-based local IPC without going through the full IP networking path and are well suited to local client/server communication.

They also integrate naturally with:

```text
poll()
epoll()
select()
```

---

# 54. IPC Interview Question

### What happens when a process reads from an empty blocking pipe?

Conceptually:

```text
read()
  |
  v
Pipe empty
  |
  v
Task sleeps
  |
  v
Writer writes
  |
  v
Reader wakes
  |
  v
Scheduler
  |
  v
Reader runs
```

This answer demonstrates understanding of:

```text
IPC
+
Kernel wait queues
+
Scheduler
```

---

# 55. IPC Interview Question

### Can shared memory have race conditions?

Absolutely.

Example:

```text
Process A → writes
Process B → reads
```

Without synchronization:

```text
Race
```

Shared memory must be combined with appropriate synchronization and memory-ordering mechanisms.

---

# 56. IPC Interview Question

### Signal vs message queue?

```text
Signal
    Small notification
    Asynchronous
    Limited payload semantics

Message Queue
    Actual messages
    Message boundaries
    Can carry structured application data
```

---

# 57. IPC Interview Question

### Why is a mutex different from a semaphore?

```text
Mutex
    Ownership
    Mutual exclusion

Semaphore
    Counter/resource availability
    Synchronization
```

A mutex is normally the clearer primitive for protecting a critical section.

---

# 58. IPC Interview Question

### What IPC mechanisms are common in high-performance Linux systems?

A strong answer:

```text
Shared memory
Unix domain sockets
eventfd
epoll
Lock-free/ring-buffer designs
```

The exact combination depends on whether the workload is:

```text
Control-plane
Data-plane
High-throughput
Low-latency
Event-driven
```

---

# 59. Senior-Level IPC Mental Model

Memorize:

```text
                 IPC
                  |
       +----------+----------+
       |          |          |
       v          v          v
    Stream      Message    Shared Memory
       |          |          |
       v          v          v
    Pipe       MQ/Sock     Ring Buffer
       |          |          |
       +----------+----------+
                  |
                  v
           Synchronization
                  |
       +----------+----------+
       |          |          |
       v          v          v
     Mutex    Semaphore    Atomics
                  |
                  v
             Memory Order
```

---

# 60. Final IPC Flow

For senior Linux interviews, think of IPC as:

```text
Process A
   |
   | System Call / Shared Mapping
   v
Linux Kernel / Shared Memory
   |
   +-- Buffer
   +-- Wait Queue
   +-- Synchronization
   +-- Wakeup
   |
   v
Process B
   |
   v
Scheduler
   |
   v
CPU
```

The most important topics to master are:

```text
★★★★★ Pipes
★★★★★ Shared Memory
★★★★★ Unix Domain Sockets
★★★★★ Signals
★★★★★ Mutex/Semaphore
★★★★★ Blocking vs Nonblocking
★★★★★ Wait queues and wakeups
★★★★★ eventfd
★★★★★ epoll-based IPC
★★★★★ Race conditions
★★★★☆ Message Queues
★★★★☆ Ring Buffers
★★★★☆ Memory Ordering
★★★★☆ IPC Namespaces
★★★★☆ Backpressure
```

The key senior-level connection is:

```text
IPC
 ↓
Kernel object / shared memory
 ↓
Blocking or notification
 ↓
Wait queue
 ↓
Wakeup
 ↓
Scheduler
 ↓
CPU
```

Understanding this chain is much more valuable than memorizing individual IPC APIs.
