# Linux IPC (Inter-Process Communication)

## 1. What Is IPC?
IPC stands for **Inter-Process Communication**. Processes normally have separate virtual address spaces:
```
Process A --> Address Space A
Process B --> Address Space B
```
Therefore, processes need kernel-supported mechanisms to communicate and synchronize. Common IPC mechanisms: Pipes, FIFOs, Signals, Message Queues, Shared Memory, Semaphores, Sockets, Unix Domain Sockets.

## 2. Why Do We Need IPC?
Suppose `Process A --produces data--> Process B --processes data--> Process C`. IPC allows them to exchange information: data transfer, synchronization, notification, event signaling, request/response communication.

## 3. IPC Categories
```
IPC
 +-- Data Transfer: Pipe, FIFO, Message Queue, Socket
 +-- Shared Data: Shared Memory
 +-- Synchronization: Semaphore, Mutex
 +-- Notification: Signal
```

## 4. IPC Through the Kernel
Most traditional IPC mechanisms involve the kernel:
```
Process A --> System Call --> Linux Kernel --> IPC Object --> Process B
```
Shared memory is different because, after setup, processes can access the same physical memory through their virtual address spaces without copying every byte through the kernel.

---

## 5. Pipe
A pipe provides a byte stream between processes:
```
Process A --write()--> Pipe --read()--> Process B
```
Create with:
```c
int fd[2];
pipe(fd);
```
Usually `fd[0] → read`, `fd[1] → write`.

## 6. Pipe Example
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
Conceptually: `Parent --write()--> Pipe --read()--> Child`.

## 7. Pipe Characteristics
Important properties: byte stream, usually unidirectional, kernel-managed buffer, file-descriptor based, supports blocking I/O. A pipe does not preserve message boundaries — if the writer performs `write("ABC")` then `write("DEF")`, the reader sees a byte stream rather than two guaranteed application-level messages.

## 8. Blocking Pipe
If the pipe is empty: `Reader --read()--> Pipe empty --> Reader sleeps`. When data arrives: `Writer --write()--> Pipe --> Wake reader`. This is an important example of IPC + scheduling interaction.

## 9. Anonymous Pipe
An ordinary pipe is commonly used between related processes: `Parent --fork()--> Child`. Both processes inherit the pipe file descriptors (each keeps its own `fd`).

## 10. Named Pipe – FIFO
A FIFO is a named pipe that appears in the filesystem namespace. Create with `mkfifo myfifo`. Then: `Process A --> myfifo --> Process B`. Unlike an anonymous pipe, unrelated processes can open the FIFO by name.

## 11. FIFO vs Pipe
| Feature | Pipe | FIFO |
|---|---|---|
| Name in filesystem | No | Yes |
| Related processes | Common use | Not required |
| Byte stream | Yes | Yes |
| Kernel buffer | Yes | Yes |
| File descriptor based | Yes | Yes |

---

## 12. Signals
A signal is a lightweight asynchronous notification, e.g. `Process A --SIGTERM--> Process B`. Common signals: `SIGINT`, `SIGTERM`, `SIGKILL`, `SIGSTOP`, `SIGCONT`, `SIGSEGV`, `SIGCHLD`.

## 13. Signal Delivery
```
Sender --kill()--> Kernel --> Target task --> Signal becomes pending --> Signal handling
```
The kernel manages signal state and delivery.

## 14. kill() Does Not Necessarily Kill
An interview trap — `kill(pid, SIGTERM);` does not mean the function always kills the process. `kill()` sends a signal: `SIGSTOP` → stop, `SIGCONT` → continue, `SIGTERM` → termination request, `SIGUSR1` → application-defined notification.

## 15. SIGTERM vs SIGKILL
**SIGTERM** — graceful termination request; can be handled; can perform cleanup.
**SIGKILL** — immediate forced termination; cannot be caught; cannot be ignored.

Use `SIGTERM` before `SIGKILL` when graceful shutdown is possible.

---

## 16. Message Queues
Message queues allow processes to exchange discrete messages:
```
Process A --message--> Message Queue --message--> Process B
```
Unlike pipes, message queues preserve message boundaries.

## 17. Pipe vs Message Queue
Pipe: byte stream (`ABCDEF...`). Message queue: message-oriented (`[MSG1] [MSG2] [MSG3]`). The distinction is important for protocol design.

## 18. POSIX Message Queues
Linux provides POSIX message queues. Typical APIs: `mq_open()`, `mq_send()`, `mq_receive()`, `mq_close()`, `mq_unlink()`:
```
Producer --mq_send()--> Message Queue --mq_receive()--> Consumer
```

## 19. System V IPC
Linux also supports System V IPC mechanisms: System V shared memory, System V semaphores, System V message queues. Modern applications often prefer POSIX APIs or other mechanisms depending on requirements. For interviews, know both names and their basic differences.

---

## 20. Shared Memory
Shared memory allows multiple processes to map the same physical memory into their virtual address spaces:
```
Process A: Virtual Address A ---+
                                  v
                          Shared Physical Memory
                                  ^
Process B: Virtual Address B ---+
```
This can provide very high throughput.

## 21. Why Shared Memory Is Fast
A pipe: `Process A --> Kernel buffer --> Process B` — data typically moves through kernel-managed buffering. With shared memory, both processes can directly access the shared mapped region:
```
Process A ---> Shared Memory <--- Process B
```
Therefore large data transfers can avoid repeated copying.

## 22. Shared Memory Does NOT Solve Synchronization
Extremely important. If Process A writes shared data and Process B reads it without synchronization → race condition. Shared memory must usually be combined with synchronization: Shared Memory + Semaphore/Mutex + Memory ordering.

## 23. Shared Memory Producer-Consumer
A common design:
```
Producer --> Shared Memory (Ring Buffer) --> Consumer
```
Producer: write data, update producer index. Consumer: read data, update consumer index. Synchronization is required to safely coordinate ownership.

## 24. Ring Buffer IPC
A ring buffer is frequently used for high-performance IPC:
```
| D0 | D1 | D2 | D3 | D4 |
  ^read           ^write
```
When the end is reached, write wraps around — this avoids continuously allocating/freeing buffers.

---

## 25. Semaphore
A semaphore is primarily a synchronization/counting mechanism: `Semaphore = count`. Operations: `wait/down`, `signal/up`. E.g. `count = 3` means up to three units of some resource are available.

## 26. Binary Semaphore
A semaphore with two logical states (`1 → available`, `0 → unavailable`) can be used for synchronization. However, for mutual exclusion in modern Linux user-space applications, a mutex is generally a better semantic choice.

## 27. Mutex
A mutex provides mutual exclusion:
```
Thread A: lock --> Critical Section --> unlock
Thread B: lock (waits until available)
```
Only one owner can hold the mutex at a time.

## 28. Mutex vs Semaphore
**Mutex** — ownership, mutual exclusion, protect critical section.
**Semaphore** — counting, resource availability, synchronization.

Do not simply describe both as "locks."

## 29. IPC and Synchronization
For `Process A --write--> Shared Memory <--read-- Process B`, synchronization is needed — e.g. Shared Memory + Semaphore, or Shared Memory + process-shared pthread mutex when configured appropriately.

---

## 30. Unix Domain Sockets
Unix domain sockets provide IPC through the socket interface:
```
Process A --socket--> Unix Domain Socket --> Process B
```
Particularly useful for request/response, bidirectional communication, structured protocols, credential passing, and local service communication.

## 31. Unix Domain Socket vs TCP
**Unix domain socket** — same machine, no IP networking required, uses kernel socket infrastructure.
**TCP socket** — can communicate across machines, uses IP networking stack.

For local IPC, Unix domain sockets can avoid the overhead of IP routing and are commonly used by system services.

## 32. Socket Types
Unix domain sockets support common socket semantics: `SOCK_STREAM` (reliable byte stream), `SOCK_DGRAM` (datagram/message-oriented communication), `SOCK_SEQPACKET`.

---

## 33. File Descriptors and IPC
Many Linux IPC mechanisms use file descriptors: `pipe()`, `socket()`, `eventfd()`, `signalfd()`:
```
Process --- fd 3 → pipe
         --- fd 4 → socket
         --- fd 5 → file
```
A powerful Linux design principle: many kernel resources are exposed through file descriptors.

## 34. eventfd
`eventfd` provides a lightweight event notification mechanism using a file descriptor:
```
Producer --write/update eventfd--> eventfd --read--> Consumer
```
Useful for thread notification, event notification, polling/epoll integration, and kernel/user synchronization.

## 35. eventfd + epoll
A powerful Linux pattern:
```
Application --- socket, eventfd, pipe --> epoll --> Wait for events
```
The application can monitor multiple event sources using one event loop.

## 36. signalfd
`signalfd` allows signals to be consumed through a file descriptor:
```
Signal --> signalfd --> read() --> Application
```
This can integrate signal handling into an event-driven architecture.

## 37. IPC Through epoll
Linux applications often build event-driven systems around `epoll`:
```
epoll --- socket, eventfd, pipe
```
One thread can wait for events from many sources.

---

## 38. IPC Performance
A rough conceptual comparison:
- **Shared Memory** — very high throughput, needs synchronization
- **Pipe** — simple byte-stream IPC
- **Unix Domain Socket** — flexible local IPC, request/response friendly
- **Message Queue** — message-oriented IPC
- **Signal** — small notification

The "fastest" mechanism depends on workload and implementation details.

## 39. IPC Choice
- **Pipe** — simple parent-child stream
- **FIFO** — unrelated processes, simple stream
- **Shared memory** — large/high-throughput data, low-copy design
- **Unix domain socket** — local client/server, bidirectional communication
- **Message queue** — discrete messages, priority/message semantics
- **Signal** — simple asynchronous notification

---

## 40. IPC Architecture Example
```
Application --request--> IPC Service --process request--> Worker --result--> Application
```
Possible implementation: Unix Domain Socket + Shared Memory + eventfd — socket for control messages, shared memory for large data, eventfd for notification. This combination is common in high-performance designs.

## 41. IPC and Kernel Scheduling
IPC often causes tasks to block and wake:
```
Producer --write()--> Consumer wakes --> Scheduler --> Consumer runs
```
So: `IPC → Blocking/wakeup → Scheduler` is an important Linux internals relationship.

## 42. IPC and Memory Ordering
Shared-memory IPC introduces another issue: even if CPU 0 (Producer) and CPU 1 (Consumer) access the same memory, correct synchronization requires appropriate memory ordering:
```
Producer: write data --> publish state
Consumer: observe state --> read data
```
Without correct ordering, the consumer may observe inconsistent state. Especially important for lock-free queues, ring buffers, shared-memory IPC, and multicore systems.

## 43. Race Condition Example
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
Without proper synchronization, this can be incorrect on a multicore system. Correct designs use mutexes, semaphores, condition variables, atomic operations, and memory barriers/appropriate memory ordering.

## 44. IPC Failure Modes
Senior engineers should think about: deadlock, race condition, lost notification, buffer overflow, backpressure, priority inversion, resource exhaustion, process crash, peer disappearance, partial messages, ordering, and memory visibility. IPC design is not simply about transferring data.

## 45. Backpressure
If a producer is fast and a consumer is slow, the IPC buffer eventually fills up. The system needs a policy: block producer, drop data, overwrite old data, expand buffer, or apply flow control. This is called handling **backpressure**.

## 46. Blocking vs Nonblocking IPC
**Blocking:** no data → process sleeps.
**Nonblocking:** no data → return immediately (e.g. `O_NONBLOCK`).

Nonblocking IPC is commonly combined with `poll()`, `select()`, `epoll()`.

## 47. IPC with poll/epoll
Instead of `read(socket1)`, `read(socket2)`, `read(pipe)` and potentially blocking on the wrong one:
```
epoll --- socket1 ready, socket2 not ready, pipe ready
```
The application processes only ready sources. This is fundamental to scalable Linux servers.

---

## 48. IPC Security
IPC also requires security considerations: file permissions, UID/GID, capabilities, namespaces, SELinux/AppArmor, socket permissions. For example, a Unix domain socket can have filesystem permissions controlling which users can connect.

## 49. IPC and Namespaces
Containers use IPC namespaces to isolate certain IPC resources: `Container A --> IPC namespace A`, `Container B --> IPC namespace B`. This prevents processes in different namespaces from automatically seeing the same namespace-scoped IPC resources — important for container internals.

## 50. IPC in Containers
A containerized application may use Unix sockets, shared memory, signals, pipes, eventfd — but namespace isolation determines visibility of many resources: `Containers → Namespaces → IPC isolation`.

---

## 51. IPC Interview Question: Why is shared memory faster than a pipe?
A good answer: shared memory allows processes to map the same memory region and exchange data without requiring the same data-transfer path through kernel buffering for every operation. This can provide very high throughput, but synchronization and memory ordering become the application's responsibility.

## 52. IPC Interview Question: Pipe vs shared memory?
**Pipe** — simple, kernel-managed buffer, byte stream, easier synchronization semantics.
**Shared memory** — very high throughput, large data, requires explicit synchronization, more complex.

## 53. IPC Interview Question: Why use Unix domain sockets instead of TCP on the same machine?
Because they provide socket-based local IPC without going through the full IP networking path and are well suited to local client/server communication. They also integrate naturally with `poll()`, `epoll()`, `select()`.

## 54. IPC Interview Question: What happens when a process reads from an empty blocking pipe?
```
read() --> Pipe empty --> Task sleeps --> Writer writes --> Reader wakes --> Scheduler --> Reader runs
```
This answer demonstrates understanding of IPC + kernel wait queues + scheduler.

## 55. IPC Interview Question: Can shared memory have race conditions?
Absolutely. If Process A writes and Process B reads without synchronization → race. Shared memory must be combined with appropriate synchronization and memory-ordering mechanisms.

## 56. IPC Interview Question: Signal vs message queue?
**Signal** — small notification, asynchronous, limited payload semantics.
**Message Queue** — actual messages, message boundaries, can carry structured application data.

## 57. IPC Interview Question: Why is a mutex different from a semaphore?
**Mutex** — ownership, mutual exclusion.
**Semaphore** — counter/resource availability, synchronization.

A mutex is normally the clearer primitive for protecting a critical section.

## 58. IPC Interview Question: What IPC mechanisms are common in high-performance Linux systems?
A strong answer: shared memory, Unix domain sockets, eventfd, epoll, lock-free/ring-buffer designs. The exact combination depends on whether the workload is control-plane, data-plane, high-throughput, low-latency, or event-driven.

---

## 59. Senior-Level IPC Mental Model
Memorize:
```
                 IPC
       +----------+----------+
       |          |          |
    Stream      Message    Shared Memory
       |          |          |
     Pipe       MQ/Sock     Ring Buffer
       +----------+----------+
                  |
           Synchronization
       +----------+----------+
       |          |          |
     Mutex    Semaphore    Atomics
                  |
             Memory Order
```

## 60. Final IPC Flow
For senior Linux interviews, think of IPC as:
```
Process A --System Call/Shared Mapping--> Linux Kernel/Shared Memory
   +-- Buffer
   +-- Wait Queue
   +-- Synchronization
   +-- Wakeup
   --> Process B --> Scheduler --> CPU
```

The most important topics to master:
```
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

The key senior-level connection:
```
IPC → Kernel object/shared memory → Blocking or notification → Wait queue → Wakeup → Scheduler → CPU
```
Understanding this chain is much more valuable than memorizing individual IPC APIs.
