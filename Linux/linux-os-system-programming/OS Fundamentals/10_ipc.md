# Chapter 10 — Inter-Process Communication (IPC)

> **Three-layer approach**
>
> This chapter covers:
> 1. **[OS] IPC fundamentals**
> 2. **[LSP] Linux System Programming + C code**
> 3. **[KERNEL] Linux Kernel IPC internals**
>
> Main idea:
>
> ```text
> Communication
>     ↓
> How do processes exchange data?
>
> Synchronization
>     ↓
> How do processes safely coordinate?
> ```

---

# 1. What Is IPC?

IPC means **Inter-Process Communication**.

Processes normally have separate virtual address spaces:

```text
Process A
+----------------+
| Virtual Memory |
+----------------+

Process B
+----------------+
| Virtual Memory |
+----------------+
```

Process A cannot normally directly dereference Process B's virtual address.

IPC provides controlled mechanisms for exchanging data or events.

---

# 2. Why Do We Need IPC?

Examples:

```text
shell → command
parent → child
producer → consumer
client → server
worker → manager
process → logging process
process → GUI
```

Typical requirements:

```text
data transfer
event notification
synchronization
request/response
shared state
client/server communication
```

---

# 3. Main IPC Categories

Linux provides several IPC mechanisms:

```text
IPC
 |
 +-- Pipe
 |
 +-- FIFO
 |
 +-- Signals
 |
 +-- Shared Memory
 |
 +-- Message Queues
 |
 +-- Semaphores
 |
 +-- eventfd
 |
 +-- Unix Domain Sockets
 |
 +-- socketpair
```

Some mechanisms primarily communicate data.

Others primarily synchronize or notify.

Many real systems combine several mechanisms.

---

# 4. IPC Design Models

Two major models:

## Message Passing

```text
Process A
    |
    | message
    v
Kernel IPC mechanism
    |
    | message
    v
Process B
```

Processes exchange discrete messages.

Examples:

```text
pipe
message queue
socket
```

---

## Shared Memory

```text
Process A ──┐
            |
            v
       Shared Pages
            ^
            |
Process B ──┘
```

Both processes map the same physical memory into their virtual address spaces.

This is usually fast for large data because processes can access shared data directly after the mapping is established.

But synchronization becomes the application's responsibility.

---

# 5. Communication vs Synchronization

These are different concepts.

### Communication

```text
A sends data to B
```

### Synchronization

```text
A waits until B finishes
A protects shared data
A signals an event to B
```

Example:

```text
Shared memory
     +
Semaphore
```

can provide:

```text
data sharing
+
coordination
```

---

# 6. Pipe

A pipe is a unidirectional byte stream.

Conceptually:

```text
Process A
   |
   | write()
   v
+----------------+
|  pipe buffer   |
+----------------+
        |
        | read()
        v
Process B
```

Classic system call:

```c
pipe(fd);
```

It creates two file descriptors:

```text
fd[0] → read end
fd[1] → write end
```

---

# 7. Basic Pipe Example

```c
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];

    if (pipe(fd) == -1)
    {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        // Child: read
        close(fd[1]);

        char buf[100];

        ssize_t n = read(fd[0], buf, sizeof(buf) - 1);

        if (n > 0)
        {
            buf[n] = '\0';
            printf("Child received: %s\n", buf);
        }

        close(fd[0]);
        return 0;
    }

    // Parent: write
    close(fd[0]);

    const char *msg = "Hello from parent";

    write(fd[1], msg, strlen(msg));

    close(fd[1]);

    wait(NULL);

    return 0;
}
```

Compile:

```bash
gcc pipe.c -o pipe
```

Run:

```bash
./pipe
```

---

# 8. How Pipe Works

The important sequence is:

```text
pipe()
   ↓
kernel creates pipe object/buffer
   ↓
returns fd[0], fd[1]
   ↓
fork()
   ↓
parent and child inherit descriptors
   ↓
parent closes read end
child closes write end
   ↓
parent write()
   ↓
kernel pipe buffer
   ↓
child read()
```

---

# 9. Why Close Unused Pipe Ends?

This is extremely important.

Suppose the child reads until EOF.

EOF is observed when all write descriptors referring to the pipe's write end are closed.

If a process accidentally keeps a write end open:

```text
reader
  |
  v
read()
  |
  v
may continue waiting
```

instead of seeing EOF.

Typical pattern:

```c
close(fd[0]);   // writer
```

and:

```c
close(fd[1]);   // reader
```

---

# 10. Pipe Is a Byte Stream

If:

```c
write(fd, "ABC", 3);
```

the reader sees bytes.

A pipe does not inherently preserve application-level records.

Conceptually:

```text
Writer:
ABCDEF

Reader:
AB
CDEF
```

A single `write()` does not mean the reader must receive exactly one matching `read()`.

For message boundaries, use a suitable message-oriented protocol/mechanism.

---

# 11. Pipe Blocking

A pipe can block.

Example:

```text
write()
   ↓
pipe buffer full
   ↓
writer may block
```

Reader:

```text
read()
   ↓
pipe empty
   ↓
reader may block
```

This naturally supports producer-consumer patterns.

---

# 12. Pipe EOF

Suppose all writers close the write end:

```text
all write descriptors closed
          |
          v
reader eventually sees EOF
          |
          v
read() returns 0
```

Example:

```c
ssize_t n = read(fd, buf, sizeof(buf));

if (n == 0)
{
    // EOF
}
```

---

# 13. Pipe and `fork()`

A common pattern:

```text
pipe()
  |
  v
fork()
  |
  +--------+
  |        |
Parent    Child
  |        |
write     read
```

The descriptors are inherited across `fork()`.

This is why pipe + fork is common for parent-child IPC.

---

# 14. Two-Way Communication

A normal pipe is one-way.

For bidirectional communication:

```text
Pipe 1:
A → B

Pipe 2:
B → A
```

Conceptually:

```text
A ---- pipe1 ----> B
A <--- pipe2 ----- B
```

But for two-way local process communication, `socketpair()` can often be cleaner.

---

# 15. FIFO / Named Pipe

A FIFO is a named pipe.

Unlike an anonymous pipe:

```text
anonymous pipe
```

the FIFO has a filesystem name.

Create:

```bash
mkfifo myfifo
```

Check:

```bash
ls -l myfifo
```

It will appear as a special FIFO file.

---

# 16. FIFO Example — Reader

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int fd = open("myfifo", O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    char buf[100];

    ssize_t n = read(fd, buf, sizeof(buf) - 1);

    if (n > 0)
    {
        buf[n] = '\0';
        printf("Received: %s\n", buf);
    }

    close(fd);

    return 0;
}
```

---

# 17. FIFO Example — Writer

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int fd = open("myfifo", O_WRONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    const char *msg = "Hello through FIFO";

    write(fd, msg, strlen(msg));

    close(fd);

    return 0;
}
```

Run:

```bash
mkfifo myfifo

./reader
```

In another terminal:

```bash
./writer
```

---

# 18. Pipe vs FIFO

| Pipe | FIFO |
|---|---|
| Usually created with `pipe()` | Created with `mkfifo()` |
| No filesystem name | Has filesystem name |
| Common with related processes | Can connect unrelated processes |
| Byte stream | Byte stream |
| Kernel-backed | Kernel-backed |

---

# 19. File Descriptors and IPC

Linux IPC frequently uses file descriptors.

Examples:

```text
pipe()
socket()
socketpair()
eventfd()
```

A file descriptor is an integer:

```text
0 → stdin
1 → stdout
2 → stderr
```

Example:

```text
fd = 5
```

The process has a descriptor table:

```text
Process
 |
 v
fd table
 |
 +-- 0 → stdin
 +-- 1 → stdout
 +-- 2 → stderr
 +-- 5 → pipe/socket/etc.
```

---

# 20. `dup()` and `dup2()`

These duplicate file descriptors.

Example:

```c
int newfd = dup(oldfd);
```

`dup2()`:

```c
dup2(oldfd, newfd);
```

This is heavily used in:

```text
shells
redirection
pipelines
IPC setup
```

---

# 21. Pipe + `dup2()` — Shell Pipeline Concept

For:

```bash
ls | grep txt
```

conceptually:

```text
ls
 |
stdout
 |
pipe
 |
stdin
 |
grep
```

The shell can use:

```text
pipe()
fork()
dup2()
exec()
```

Flow:

```text
pipe()
  |
  +--> fork ls
  |
  +--> fork grep
```

For `ls`:

```text
stdout → pipe write end
```

For `grep`:

```text
stdin → pipe read end
```

Then:

```text
exec()
```

replaces the processes with the requested programs.

---

# 22. `dup2()` Example

```c
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(void)
{
    int fd = open("output.txt",
                  O_WRONLY | O_CREAT | O_TRUNC,
                  0644);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    if (dup2(fd, STDOUT_FILENO) == -1)
    {
        perror("dup2");
        return 1;
    }

    close(fd);

    printf("This goes into output.txt\n");

    return 0;
}
```

Why?

```text
stdout (fd 1)
      |
      v
output.txt
```

---

# 23. Signals

Signals provide asynchronous event notification.

Examples:

```text
SIGINT
SIGTERM
SIGKILL
SIGCHLD
SIGUSR1
SIGUSR2
```

Send a signal:

```c
kill(pid, SIGTERM);
```

Important:

> `kill()` does not necessarily mean "kill the process"; it means send a signal.

---

# 24. Signal Example

Sender:

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main(void)
{
    pid_t pid;

    printf("Enter PID: ");
    scanf("%d", &pid);

    if (kill(pid, SIGUSR1) == -1)
    {
        perror("kill");
        return 1;
    }

    return 0;
}
```

Receiver:

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig)
{
    printf("Received signal: %d\n", sig);
}

int main(void)
{
    signal(SIGUSR1, handler);

    printf("PID = %d\n", getpid());

    while (1)
        pause();

    return 0;
}
```

For production code, prefer `sigaction()` over `signal()` because its semantics are more well-defined and controllable.

---

# 25. `sigaction()`

Example:

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig)
{
    (void)sig;
}

int main(void)
{
    struct sigaction sa = {0};

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    while (1)
        pause();

    return 0;
}
```

---

# 26. Signal Limitations

Signals are useful for:

```text
event notification
termination
child-state notification
simple asynchronous events
```

They are not normally the right mechanism for transferring large data.

For large data:

```text
shared memory
pipe
socket
message queue
```

are more appropriate.

---

# 27. Signal Safety

A signal handler runs asynchronously.

Only async-signal-safe operations should be called from a signal handler.

Avoid casually doing:

```c
printf()
malloc()
free()
```

inside signal handlers.

A common safe pattern is:

```c
volatile sig_atomic_t flag = 0;

void handler(int sig)
{
    flag = 1;
}
```

Then the main execution path handles the actual work.

---

# 28. Shared Memory

Shared memory allows multiple processes to map the same underlying memory.

Conceptually:

```text
Process A
Virtual address X
      |
      v
   Physical page
      ^
      |
Virtual address Y
Process B
```

The virtual addresses can differ.

The underlying physical memory can be shared.

---

# 29. Shared Memory Advantages

For large data:

```text
Process A
   |
   v
shared memory
   |
   v
Process B
```

There is no requirement to copy the entire application-level payload through a kernel message buffer for every exchange.

Therefore shared memory can be very efficient.

But:

```text
shared memory
+
synchronization
```

must usually be designed together.

---

# 30. POSIX Shared Memory

Typical APIs:

```text
shm_open()
ftruncate()
mmap()
munmap()
shm_unlink()
```

Example flow:

```text
shm_open()
    ↓
ftruncate()
    ↓
mmap()
    ↓
access shared region
    ↓
munmap()
    ↓
shm_unlink()
```

---

# 31. POSIX Shared Memory Example

Writer:

```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define NAME "/my_shm"
#define SIZE 4096

int main(void)
{
    int fd = shm_open(NAME, O_CREAT | O_RDWR, 0666);

    if (fd == -1)
    {
        perror("shm_open");
        return 1;
    }

    if (ftruncate(fd, SIZE) == -1)
    {
        perror("ftruncate");
        return 1;
    }

    char *p = mmap(NULL,
                   SIZE,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   fd,
                   0);

    if (p == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    close(fd);

    strcpy(p, "Hello from shared memory");

    printf("Written\n");

    munmap(p, SIZE);

    return 0;
}
```

---

# 32. Shared Memory Reader

```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define NAME "/my_shm"
#define SIZE 4096

int main(void)
{
    int fd = shm_open(NAME, O_RDONLY, 0666);

    if (fd == -1)
    {
        perror("shm_open");
        return 1;
    }

    char *p = mmap(NULL,
                   SIZE,
                   PROT_READ,
                   MAP_SHARED,
                   fd,
                   0);

    if (p == MAP_FAILED)
    {
        perror("mmap");
        return 1;
    }

    printf("Received: %s\n", p);

    munmap(p, SIZE);
    close(fd);

    shm_unlink(NAME);

    return 0;
}
```

In real applications, the writer and reader need a proper lifecycle/synchronization protocol.

---

# 33. Shared Memory Does Not Automatically Synchronize

This is a major interview point.

Suppose:

```text
Process A:
shared->value = 10;

Process B:
printf("%d", shared->value);
```

If access timing is uncontrolled, you may have a race.

Shared memory gives:

```text
shared data
```

but not automatically:

```text
mutual exclusion
ordering
notification
```

You may need:

```text
semaphore
mutex in shared memory
futex
eventfd
process-shared synchronization
```

depending on the design.

---

# 34. POSIX Semaphores

Named semaphore:

```text
sem_open()
sem_wait()
sem_post()
sem_close()
sem_unlink()
```

Unnamed process-shared semaphore can be placed in shared memory and initialized appropriately.

Conceptually:

```text
Process A
   |
sem_wait()
   |
shared resource
   |
sem_post()

Process B
   |
sem_wait()
   |
shared resource
   |
sem_post()
```

---

# 35. Semaphore Concept

A counting semaphore has a count.

Example:

```text
count = 3
```

Three resources can be acquired.

```text
wait()
   ↓
count--

post()
   ↓
count++
```

If count is zero:

```text
wait()
   ↓
block
```

until another participant posts.

---

# 36. Mutex vs Semaphore

| Mutex | Semaphore |
|---|---|
| Mutual exclusion | Resource/event counting |
| Usually ownership-oriented | No normal ownership requirement |
| Protects critical section | Can represent N resources |
| Common count = 1 concept | Count can be > 1 |

A binary semaphore and mutex can look similar, but they are not identical synchronization abstractions.

---

# 37. Message Queues

Message queues allow processes to exchange discrete messages.

Conceptually:

```text
Process A
   |
   | message
   v
+------------------+
| Message Queue    |
+------------------+
   |
   | message
   v
Process B
```

Unlike a pipe's byte stream, a message queue preserves message boundaries.

---

# 38. POSIX Message Queue

Typical APIs:

```text
mq_open()
mq_send()
mq_receive()
mq_close()
mq_unlink()
```

Conceptual example:

```c
mqd_t mq = mq_open(
    "/myqueue",
    O_CREAT | O_RDWR,
    0666,
    NULL
);
```

Send:

```c
mq_send(mq, msg, strlen(msg) + 1, 0);
```

Receive:

```c
mq_receive(mq, buffer, size, &priority);
```

Compile on systems where the required libraries/APIs are available; some older environments may require additional linker options.

---

# 39. Message Queue vs Pipe

| Pipe | Message Queue |
|---|---|
| Byte stream | Discrete messages |
| No inherent message boundaries | Message boundaries preserved |
| Simple | More structured |
| Common shell IPC | Useful for typed/prioritized messages |
| `read/write` | `mq_send/mq_receive` |

---

# 40. System V IPC

Linux also supports System V IPC:

```text
System V shared memory
System V message queues
System V semaphores
```

Examples:

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

Modern applications often prefer POSIX interfaces where appropriate, but System V IPC remains important for legacy systems and interviews.

---

# 41. POSIX vs System V IPC

| POSIX | System V |
|---|---|
| Modern interface style | Older IPC family |
| POSIX names/APIs | System V IDs/APIs |
| `shm_open()` | `shmget()` |
| `mq_open()` | `msgget()` |
| POSIX semaphores | System V semaphores |

Know both for senior Linux interviews.

---

# 42. `eventfd()`

`eventfd()` provides a file-descriptor-based event/counter mechanism.

Example:

```c
int fd = eventfd(0, 0);
```

Conceptually:

```text
Producer
   |
write()
   |
eventfd counter
   |
read()
   |
Consumer
```

It is particularly useful when event notification needs to integrate with:

```text
select()
poll()
epoll()
```

---

# 43. `eventfd()` Example

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

    if (read(fd, &received, sizeof(received)) != sizeof(received))
    {
        perror("read");
        close(fd);
        return 1;
    }

    printf("event value = %llu\n",
           (unsigned long long)received);

    close(fd);

    return 0;
}
```

---

# 44. `socketpair()`

`socketpair()` creates a pair of connected sockets, commonly for local IPC.

```c
int fd[2];

socketpair(AF_UNIX,
            SOCK_STREAM,
            0,
            fd);
```

Conceptually:

```text
Process A
   |
 fd[0]
   |
connected socket
   |
 fd[1]
   |
Process B
```

It is naturally bidirectional.

---

# 45. Unix Domain Sockets

Unix domain sockets provide local IPC using the socket interface.

They support:

```text
SOCK_STREAM
SOCK_DGRAM
```

depending on requirements.

They are widely used for:

```text
daemon communication
service managers
desktop services
database clients
local client/server applications
```

---

# 46. Unix Domain Socket vs TCP Socket

```text
TCP:
Process A
   |
   v
network stack
   |
   v
network
   |
   v
Process B
```

Unix domain socket:

```text
Process A
   |
   v
local kernel IPC/socket mechanisms
   |
   v
Process B
```

Unix domain sockets are local to the host and can avoid network-interface/network-protocol overhead associated with TCP networking.

---

# 47. Unix Domain Socket Example — Server Concept

Typical flow:

```text
socket()
   ↓
bind()
   ↓
listen()
   ↓
accept()
   ↓
read()/write()
   ↓
close()
```

Client:

```text
socket()
   ↓
connect()
   ↓
read()/write()
   ↓
close()
```

---

# 48. `socketpair()` vs Pipe

| Pipe | socketpair |
|---|---|
| Usually one-way | Bidirectional |
| Byte stream | Stream or datagram depending on type |
| Very simple | Socket semantics |
| Great for parent-child pipelines | Great for two-way local IPC |
| File descriptors | File descriptors |

---

# 49. IPC Performance — General View

A simplified conceptual ranking:

```text
Shared memory
    ↓
very efficient for large shared data
    ↓
requires synchronization

Pipe/message/socket
    ↓
kernel-mediated data movement
    ↓
simpler communication semantics
```

Do not treat this as an absolute benchmark.

Performance depends on:

```text
message size
copying
cache behavior
system calls
contention
scheduler activity
CPU architecture
implementation
```

---

# 50. Data Copying

A useful conceptual comparison.

Pipe:

```text
Producer buffer
      |
      v
kernel pipe buffer
      |
      v
Consumer buffer
```

Shared memory:

```text
Producer
   |
   v
shared region
   ^
   |
Consumer
```

Shared memory can reduce copying for large data, but synchronization and cache-coherency effects still matter.

---

# 51. Producer-Consumer Using Pipe

```text
Producer
   |
   | write()
   v
+-------------+
| pipe buffer |
+-------------+
      |
      | read()
      v
Consumer
```

If buffer is full:

```text
producer blocks
```

If buffer is empty:

```text
consumer blocks
```

This provides a simple synchronization effect naturally through blocking I/O.

---

# 52. Producer-Consumer Using Shared Memory

```text
Producer
   |
   v
+-------------------+
| shared ring buffer|
+-------------------+
   ^
   |
Consumer
```

Need additional state:

```text
head
tail
count
```

and synchronization:

```text
mutex
semaphore
futex
atomic operations
```

depending on the design.

---

# 53. Shared-Memory Ring Buffer Concept

```text
+----+----+----+----+----+
| D0 | D1 | D2 | D3 | D4 |
+----+----+----+----+----+
  ^                 ^
 read              write
```

Producer:

```text
write at head
advance head
```

Consumer:

```text
read at tail
advance tail
```

A carefully designed lock-free ring buffer may use atomics instead of a mutex, but memory-ordering correctness is critical.

---

# 54. IPC and `fork()`

`fork()` is important because parent and child inherit access to several IPC-related resources.

For file descriptors:

```text
Parent fd table
      |
      | fork
      v
Child fd table
```

The descriptors refer to the same underlying open-file descriptions where applicable.

This is why:

```text
pipe + fork
```

works naturally.

---

# 55. IPC and `exec()`

`exec()` replaces the current process image.

File descriptors generally remain open across `exec()` unless marked:

```text
FD_CLOEXEC
```

or created with close-on-exec semantics.

This matters for:

```text
shell pipelines
daemon setup
IPC descriptor passing
```

---

# 56. `FD_CLOEXEC`

Example:

```c
fcntl(fd, F_SETFD, FD_CLOEXEC);
```

This means the descriptor will be closed during successful `exec()`.

Modern APIs often provide atomic close-on-exec creation flags where available.

---

# 57. File Descriptor Passing

Unix domain sockets can pass file descriptors between processes using:

```text
sendmsg()
recvmsg()
```

with ancillary data such as:

```text
SCM_RIGHTS
```

This is a powerful Linux IPC mechanism.

Conceptually:

```text
Process A
   |
   | fd + data
   v
Unix socket
   |
   v
Process B
```

Process B obtains a new file descriptor referring to the same underlying kernel object/resource.

---

# 58. Why FD Passing Is Powerful

A privileged process can:

```text
open resource
       |
       v
pass FD
       |
       v
less-privileged worker
```

This can be used for:

```text
service architectures
privilege separation
daemon supervision
socket activation
resource delegation
```

---

# 59. Kernel View — Pipe

Conceptually:

```text
User:
write(fd, data, size)
        |
        v
system call
        |
        v
kernel pipe object
        |
        v
pipe buffer/pages
        |
        v
reader
```

The pipe has kernel-managed state.

Processes only hold file descriptors referencing the kernel-side pipe endpoints.

---

# 60. Kernel View — File Descriptor

A simplified conceptual relationship:

```text
process
   |
   v
fd table
   |
   v
file object
   |
   v
underlying kernel object
```

For a pipe:

```text
fd
 ↓
struct file
 ↓
pipe object
 ↓
pipe buffers
```

Exact kernel structures and implementation details vary across Linux versions.

---

# 61. Kernel View — Shared Memory

Conceptually:

```text
Process A virtual mapping
          |
          v
       physical pages
          ^
          |
Process B virtual mapping
```

Each process can have its own virtual address.

The MM subsystem manages the mappings.

---

# 62. Kernel View — Signals

Simplified:

```text
Process A
   |
kill()
   |
   v
kernel signal machinery
   |
   v
target task
   |
   v
signal becomes pending
   |
   v
delivery
   |
   v
signal handler/default action
```

Signal delivery is integrated with task/process management.

---

# 63. Kernel View — Message Queues

Conceptually:

```text
Sender
  |
  v
system call
  |
  v
kernel message queue
  |
  +--> messages
  +--> metadata
  +--> waiters
  |
  v
Receiver
```

The kernel manages queue state and synchronization.

---

# 64. Kernel View — Wait Queues

Blocking IPC often requires a task to sleep until some condition becomes true.

Conceptually:

```text
read()
   |
   v
condition false
   |
   v
sleep / wait
   |
   v
wait queue
   |
   v
producer changes state
   |
   v
wake up
   |
   v
reader continues
```

Wait queues are fundamental Linux kernel synchronization/wakeup mechanisms.

---

# 65. IPC and Scheduler

Suppose:

```text
consumer calls read()
```

and no data exists.

The consumer can block.

Conceptually:

```text
Running
   |
read()
   |
no data
   |
block
   |
scheduler chooses another task
```

When data arrives:

```text
producer write()
      |
      v
wake waiting reader
      |
      v
reader becomes runnable
```

IPC therefore interacts directly with process scheduling.

---

# 66. IPC and Synchronization

Shared memory example:

```text
Process A:
lock
write data
unlock

Process B:
lock
read data
unlock
```

Without synchronization:

```text
A writes
B reads partially updated state
```

This is a race.

---

# 67. Atomic Operations

For simple shared state:

```c
#include <stdatomic.h>

atomic_int counter;

atomic_fetch_add(&counter, 1);
```

Atomic operations can provide synchronization for suitable patterns.

But:

> Atomicity of one operation does not automatically make an entire multi-step algorithm thread/process safe.

---

# 68. Memory Ordering

Concurrent IPC designs can require memory-ordering guarantees.

Conceptually:

```text
Producer:
write data
publish flag

Consumer:
observe flag
read data
```

Correct synchronization must ensure the consumer observes the intended data state.

For lock-free designs, understand:

```text
acquire
release
sequential consistency
```

at an appropriate level.

---

# 69. `futex`

Linux `futex` means **fast userspace mutex**.

The basic design idea:

```text
fast uncontended path
    ↓
userspace atomic operation

contended path
    ↓
kernel futex operation
```

This is important because many userspace synchronization primitives can avoid a system call when there is no contention.

---

# 70. IPC Choice Guide

Use:

```text
pipe
```

when:

```text
simple parent-child byte stream
shell-style pipeline
```

Use:

```text
FIFO
```

when:

```text
unrelated local processes
named byte stream
```

Use:

```text
shared memory
```

when:

```text
large/high-throughput shared data
```

Use:

```text
message queue
```

when:

```text
discrete messages
priorities/structured message semantics
```

Use:

```text
Unix domain socket
```

when:

```text
local client/server
bidirectional communication
FD passing
```

Use:

```text
signal
```

when:

```text
asynchronous event notification
```

Use:

```text
eventfd
```

when:

```text
event/counter notification
integration with poll/epoll
```

---

# 71. IPC Comparison Table

| Mechanism | Data Model | Direction | Typical Use |
|---|---|---|---|
| Pipe | Byte stream | Usually one-way | Parent-child |
| FIFO | Byte stream | Usually one-way | Unrelated local processes |
| Shared memory | Shared bytes/objects | Both | High-throughput data |
| Message queue | Messages | Both via queue operations | Structured messages |
| Signal | Event | Notification | Async events |
| Semaphore | Counter/synchronization | N/A | Coordination |
| eventfd | Counter/event | Both via fd operations | Event notification |
| socketpair | Stream/datagram | Bidirectional | Local process communication |
| Unix socket | Stream/datagram | Bidirectional | Local client/server |
| System V IPC | Multiple models | Varies | Legacy/enterprise systems |

---

# 72. IPC Security Considerations

IPC objects can have permissions.

Consider:

```text
who can open?
who can read?
who can write?
who owns the object?
```

For example:

```text
FIFO permissions
POSIX shared-memory permissions
Unix socket filesystem permissions
```

Also consider:

```text
credential passing
FD passing
privilege boundaries
authentication
authorization
```

---

# 73. Debugging Pipes and FIFOs

Useful commands:

```bash
ls -l myfifo
```

For processes and descriptors:

```bash
ls -l /proc/<pid>/fd
```

Example:

```bash
ls -l /proc/1234/fd
```

You may see:

```text
pipe:[12345]
```

This indicates the descriptor refers to a kernel pipe object.

---

# 74. Debugging Unix Sockets

Useful:

```bash
ss -x
```

For Unix-domain sockets.

Also:

```bash
lsof -p <pid>
```

can help identify open descriptors and resources.

---

# 75. `strace` for IPC

A very useful interview/debugging command:

```bash
strace -f ./program
```

Look for:

```text
pipe()
fork()
read()
write()
poll()
select()
epoll_wait()
mmap()
munmap()
kill()
rt_sigaction()
```

For IPC debugging, `strace` often immediately reveals blocking and system-call behavior.

---

# 76. Pipe Blocking Debugging

Suppose a process hangs in:

```text
read()
```

Possible reason:

```text
pipe is empty
and
a writer still exists
```

Check:

```bash
ls -l /proc/<pid>/fd
```

and trace:

```bash
strace -f ./program
```

A classic bug is failing to close inherited unused pipe descriptors after `fork()`.

---

# 77. IPC Deadlock Example

Imagine:

```text
A:
write pipe1
read pipe2

B:
write pipe2
read pipe1
```

If both pipes become full before either side reads:

```text
A blocked in write()
B blocked in write()
```

This is a deadlock.

Therefore IPC design must consider:

```text
buffer capacity
blocking
ordering
backpressure
```

---

# 78. Backpressure

Suppose producer is faster:

```text
Producer
   |
   v
IPC buffer
   |
   v
Consumer
```

If consumer is slow:

```text
buffer fills
    |
    v
producer blocks/slows
```

This is backpressure.

It is important in:

```text
pipelines
network services
message queues
producer-consumer systems
```

---

# 79. IPC Reliability

Ask:

```text
What happens if producer dies?
What happens if consumer dies?
What happens if connection closes?
What happens if message is partial?
What happens if buffer is full?
What happens if process restarts?
```

Different IPC mechanisms provide different semantics.

For robust systems, define:

```text
timeouts
EOF behavior
retry behavior
message framing
error handling
cleanup
```

---

# 80. Message Framing

Byte streams require framing if the application needs messages.

For example:

```text
[length][payload]
[length][payload]
```

or:

```text
newline-delimited messages
```

or a binary header:

```text
+--------+--------+----------+
| length | type   | payload  |
+--------+--------+----------+
```

Pipes and stream sockets do not automatically preserve your application message boundaries.

---

# 81. IPC and `select/poll/epoll`

Many IPC file descriptors can integrate with I/O multiplexing.

Conceptually:

```text
pipe fd
socket fd
eventfd
other fd
   |
   v
epoll
   |
   v
one event loop
```

This enables an event-driven process to handle multiple IPC sources efficiently.

---

# 82. `poll()` Example Concept

```c
struct pollfd pfd = {
    .fd = fd,
    .events = POLLIN
};

int ret = poll(&pfd, 1, 5000);

if (ret > 0 && (pfd.revents & POLLIN))
{
    // Data available
}
```

This is useful for:

```text
pipe
FIFO
socket
eventfd
```

and other pollable descriptors.

---

# 83. IPC Through `epoll`

A server can have:

```text
client socket
client socket
pipe
eventfd
Unix socket
```

registered with one epoll instance:

```text
          +--> socket
          |
epoll <---+--> pipe
          |
          +--> eventfd
          |
          +--> Unix socket
```

The event loop waits for readiness.

---

# 84. Important IPC System Calls/APIs

### Pipes

```text
pipe()
pipe2()
```

### Descriptor operations

```text
dup()
dup2()
dup3()
fcntl()
```

### Signals

```text
kill()
sigaction()
sigprocmask()
sigsuspend()
pause()
```

### Shared memory

```text
shm_open()
ftruncate()
mmap()
munmap()
shm_unlink()
```

### POSIX queues

```text
mq_open()
mq_send()
mq_receive()
mq_close()
mq_unlink()
```

### System V

```text
shmget()
shmat()
msgget()
msgsnd()
msgrcv()
semget()
semop()
```

### Event

```text
eventfd()
```

### Local sockets

```text
socketpair()
socket()
bind()
listen()
accept()
connect()
sendmsg()
recvmsg()
```

---

# 85. Senior Interview — Pipe vs Shared Memory

**Question:** Which is faster?

Strong answer:

> Shared memory can be more efficient for large/high-throughput data because processes can directly access the shared region without copying each payload through a pipe/message buffer. However, shared memory requires explicit synchronization and can suffer from cache contention. For small messages or simple parent-child communication, pipes can be simpler and sufficiently fast.

---

# 86. Senior Interview — Why Shared Memory Needs Synchronization

Because:

```text
shared data
```

does not define:

```text
who writes
who reads
when it is valid
when it can be modified
```

Therefore use an appropriate synchronization protocol.

---

# 87. Senior Interview — Why Pipe Is Blocking

A blocking pipe operation waits when its current condition cannot be satisfied.

Examples:

```text
read + no data
    ↓
wait

write + buffer full
    ↓
wait
```

Non-blocking mode changes this behavior.

---

# 88. Senior Interview — What Happens When All Writers Close?

Reader eventually receives:

```text
read() == 0
```

which indicates EOF.

This is why closing unused descriptors after `fork()` is essential.

---

# 89. Senior Interview — How Does Shell Pipeline Work?

For:

```bash
A | B
```

conceptually:

```text
pipe()
   ↓
fork A
   ↓
fork B

A:
dup2(pipe_write, STDOUT_FILENO)

B:
dup2(pipe_read, STDIN_FILENO)

close unused fds

exec(A)
exec(B)
```

This is one of the most important Linux system-programming interview examples.

---

# 90. Senior Interview — How Does FD Passing Work?

Unix domain sockets can transfer file descriptors using ancillary data:

```text
sendmsg()
    |
    v
SCM_RIGHTS
    |
    v
recvmsg()
    |
    v
new FD in receiver
```

The receiver gets a descriptor referring to the same underlying kernel resource.

---

# 91. Senior Interview — Why Use `eventfd()` Instead of a Pipe?

`eventfd()` provides a dedicated counter/event mechanism and integrates naturally with file-descriptor-based event loops.

A pipe is more appropriate when actual byte-stream data needs to be transferred.

---

# 92. Senior Interview — Signals vs IPC Data Transfer

Signals:

```text
excellent for notification
poor for large data
```

Shared memory:

```text
excellent for large shared data
requires synchronization
```

Pipe:

```text
good for byte-stream transfer
simple
```

Message queue:

```text
good for discrete messages
```

---

# 93. Senior Interview — `socketpair()` vs Pipe

`socketpair()`:

```text
bidirectional
socket semantics
can support local stream/datagram behavior
```

Pipe:

```text
simple byte stream
typically one direction
```

For two-way parent-child IPC, `socketpair()` is often a clean choice.

---

# 94. IPC Working Summary

```text
PIPE
Process A
   |
 write()
   ↓
kernel pipe buffer
   |
 read()
   ↓
Process B
```

```text
SHARED MEMORY
Process A ─────┐
               ↓
          shared pages
               ↑
Process B ─────┘
```

```text
MESSAGE QUEUE
A
 |
 | message
 v
kernel queue
 |
 | message
 v
B
```

```text
SIGNAL
A
 |
 kill()
 v
kernel signal state
 |
 v
B
```

```text
UNIX SOCKET
A
 |
 v
kernel socket
 |
 v
B
```

---

# 95. IPC Decision Tree

```text
Need local IPC?
       |
       +-- simple byte stream?
       |       |
       |       +-- related processes → pipe
       |       |
       |       +-- unrelated processes → FIFO
       |
       +-- large/high-throughput shared data?
       |       |
       |       +-- shared memory
       |               +
       |             synchronization
       |
       +-- discrete messages?
       |       |
       |       +-- message queue
       |
       +-- asynchronous notification?
       |       |
       |       +-- signal/eventfd
       |
       +-- bidirectional local client/server?
               |
               +-- Unix domain socket/socketpair
```

---

# 96. Chapter 10 One-Minute Revision

```text
IPC = communication + coordination

PIPE
  → byte stream
  → parent-child/common pipeline
  → pipe()

FIFO
  → named pipe
  → unrelated local processes

SHARED MEMORY
  → common mapped memory
  → high throughput
  → needs synchronization

MESSAGE QUEUE
  → discrete messages
  → message boundaries preserved

SIGNAL
  → asynchronous notification
  → not for large data

EVENTFD
  → counter/event
  → file descriptor
  → epoll-friendly

UNIX SOCKET
  → local client/server
  → bidirectional
  → FD passing possible

SOCKETPAIR
  → connected local socket pair
  → bidirectional
```

---

# 97. IPC + Kernel + Scheduler Mental Model

```text
Process A
   |
   | system call
   v
Kernel IPC object
   |
   +--> buffer/state
   |
   +--> wait queue
   |
   +--> synchronization
   |
   v
Process B

If B cannot proceed:
   ↓
B blocks
   ↓
scheduler runs another task

When IPC condition changes:
   ↓
kernel wakes B
   ↓
B becomes runnable
   ↓
scheduler eventually runs B
```

This connects IPC with:

```text
Processes
Threads
Scheduling
Synchronization
Virtual memory
File descriptors
Kernel wait queues
```

---

# 98. Final Takeaways

1. IPC allows processes with separate address spaces to communicate.
2. Communication and synchronization are different concepts.
3. Pipes provide byte-stream IPC.
4. FIFOs are named pipes and can connect unrelated processes.
5. Pipes are commonly combined with `fork()`.
6. `dup2()` is fundamental for shell redirection and pipelines.
7. Signals provide asynchronous notification.
8. `sigaction()` is preferred over legacy `signal()` for robust signal handling.
9. Shared memory allows multiple processes to access common mapped pages.
10. Shared memory normally needs synchronization.
11. POSIX shared memory commonly uses `shm_open()` + `ftruncate()` + `mmap()`.
12. Message queues preserve message boundaries.
13. POSIX and System V IPC are both important for Linux interviews.
14. `eventfd()` provides a file-descriptor-based event/counter mechanism.
15. `socketpair()` provides connected bidirectional local sockets.
16. Unix domain sockets support local client/server communication.
17. Unix domain sockets can pass file descriptors using `SCM_RIGHTS`.
18. Blocking IPC interacts with scheduler and wait queues.
19. IPC buffers can introduce backpressure.
20. Byte streams need application-level framing when messages are required.
21. `strace -f` is extremely useful for debugging IPC.
22. `/proc/<pid>/fd` helps inspect process file descriptors.
23. `epoll` can integrate multiple IPC/file-descriptor sources into one event loop.
24. Shared memory is not automatically thread/process safe.
25. Lock-free IPC requires correct atomic and memory-ordering design.

---

# 99. Chapter 11 Preview — Files + I/O

Next:

```text
Chapter 11 — Files + I/O

OS
 ├── Files
 ├── File descriptors
 ├── Open/read/write
 ├── Blocking vs non-blocking I/O
 ├── Buffering
 └── I/O models

Linux System Programming
 ├── open()
 ├── read()
 ├── write()
 ├── close()
 ├── lseek()
 ├── stat()
 ├── chmod()
 ├── ioctl()
 ├── fcntl()
 ├── mmap()
 ├── sync/fsync/fdatasync
 ├── select/poll/epoll
 └── C programs

Linux Kernel Internals
 ├── VFS
 ├── struct file
 ├── inode
 ├── dentry
 ├── superblock
 ├── file_operations
 ├── page cache
 ├── read/write path
 ├── buffered vs direct I/O
 ├── block layer
 └── filesystem → block device flow
```

The key flow will be:

```text
open("file")
    ↓
system call
    ↓
VFS
    ↓
path lookup
    ↓
dentry
    ↓
inode
    ↓
struct file
    ↓
file descriptor
    ↓
read()/write()
    ↓
page cache / filesystem
    ↓
block layer
    ↓
storage device
```
