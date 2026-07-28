# Operating System - IPC (Inter-Process Communication) Handbook

> Complete interview notes covering all major IPC mechanisms in Linux/Unix with concepts, working, system calls, advantages, disadvantages, and use cases.

---

# Table of Contents

1. What is IPC?
2. Why IPC is Needed
3. IPC Mechanisms Overview
4. Unnamed Pipe
5. Named Pipe (FIFO)
6. Shared Memory
7. Message Queue
8. Socket
9. Memory-Mapped File (mmap)
10. IPC Comparison Table
11. Which IPC Should You Use?
12. Real-World Examples
13. Interview Questions


# IPC and Synchronization Mechanisms - Quick Reference
| IPC Mechanism                     | Persistence                                                                                                                                                                                         |
| --------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Unnamed Pipe**                  | Exists only as long as at least one process has the pipe open. Once all file descriptors are closed or the processes exit, the pipe is destroyed automatically.                                     |
| **Named Pipe (FIFO)**             | The FIFO file persists in the filesystem until it is explicitly removed (e.g., `unlink()` or `rm`). The data inside it exists only while there are writers/readers; the FIFO object itself remains. |
| **Message Queue (POSIX)**         | Persists in the kernel until `mq_unlink()` is called or the system reboots.                                                                                                                         |
| **System V Message Queue**        | Persists until `msgctl(..., IPC_RMID, ...)` is called or the system reboots.                                                                                                                        |
| **POSIX Shared Memory**           | Persists until `shm_unlink()` is called or the system reboots.                                                                                                                                      |
| **System V Shared Memory**        | Persists until `shmctl(..., IPC_RMID, ...)` is called or the system reboots.                                                                                                                        |
| **Semaphore (POSIX Named)**       | Persists until `sem_unlink()` is called or the system reboots.                                                                                                                                      |
| **System V Semaphore**            | Persists until `semctl(..., IPC_RMID, ...)` is called or the system reboots.                                                                                                                        |
| **Socket**                        | Exists only while the socket is open. Closing the socket destroys it.                                                                                                                               |
| **UNIX Domain Socket (pathname)** | The socket file remains in the filesystem until removed (`unlink()`), even after the process exits. The communication endpoint no longer exists once the process terminates.                        |



---

# 1. What is IPC?

**IPC (Inter-Process Communication)** is a mechanism that allows two or more processes to communicate and exchange data.

Processes normally have **separate address spaces**, so they cannot directly access each other's memory.

The Operating System provides IPC mechanisms to enable safe communication.

---

## Why IPC is Needed

Processes often need to:

- Exchange data
- Synchronize execution
- Share resources
- Notify events
- Coordinate tasks

Examples:

- Browser ↔ Renderer
- Database ↔ Application Server
- Shell ↔ Child Process
- Producer ↔ Consumer

---

# 2. IPC Mechanisms

Linux/Unix provides several IPC mechanisms.

```
Inter Process Communication

├── Unnamed Pipe
├── Named Pipe (FIFO)
├── Shared Memory
├── Message Queue
├── Socket
└── Memory-Mapped File (mmap)
```

---

# 3. IPC Overview

| IPC Mechanism | Related Processes | Unrelated Processes | Across Machines | Speed | Data Type |
|---------------|-------------------|---------------------|-----------------|-------|-----------|
| Unnamed Pipe | ✅ | ❌ | ❌ | Medium | Byte Stream |
| Named Pipe (FIFO) | ✅ | ✅ | ❌ | Medium | Byte Stream |
| Shared Memory | ✅ | ✅ | ❌ | Very Fast | Shared Memory |
| Message Queue | ✅ | ✅ | ❌ | Fast | Messages |
| Socket | ✅ | ✅ | ✅ | Medium | Stream / Datagram |
| mmap() | ✅ | ✅ | ❌ | Very Fast | Shared Memory + File |

---

# 4. Unnamed Pipe

## Concept

An unnamed pipe is the simplest IPC mechanism.

It provides **one-way communication** between **related processes**, typically a **parent** and its **child**.

The pipe exists only while the processes are running.

---

## How It Works

```
Parent Process

Write End
     │
     ▼
+-----------+
|   Pipe    |
+-----------+
     ▲
     │
Read End

Child Process
```

The parent writes data to the write end.

The child reads data from the read end.

---

## System Call

```cpp
int fd[2];

pipe(fd);
```

- `fd[0]` → Read End
- `fd[1]` → Write End

---

## Example

```cpp
int fd[2];

pipe(fd);

write(fd[1], "hello", 5);

read(fd[0], buffer, 5);
```

---

## Advantages

- Very simple
- Fast
- Low overhead
- Good for parent-child communication

---

## Disadvantages

- One-way communication
- Related processes only
- Exists only during process lifetime

---

## Use Cases

- Shell pipelines

```
ls | grep ".cpp"
```

- Parent ↔ Child communication

---

## Don't Use When

- Processes are unrelated
- Bidirectional communication is required
- Communication must survive process termination

---

# 5. Named Pipe (FIFO)

## Concept

A Named Pipe (FIFO) is similar to an unnamed pipe, but it exists as a file in the filesystem.

Because it has a name, **unrelated processes** can communicate through it.

---

## How It Works

```
Process A

      │
      ▼

 /tmp/myfifo

      ▲
      │

Process B
```

Both processes open the same FIFO file.

---

## Create FIFO

```cpp
mkfifo("myfifo", 0666);
```

---

## Example

Terminal 1

```bash
cat /tmp/myfifo
```

Terminal 2

```bash
echo "Hello" > /tmp/myfifo
```

---

## Advantages

- Works between unrelated processes
- Simple to use
- File-based communication

---

## Disadvantages

- Sequential stream only
- Slower than shared memory
- One-way by default

---

## Use Cases

- Communication between independent applications
- Simple producer-consumer systems
- Command-line utilities

---

## Don't Use When

- High throughput is required
- Random memory access is needed

---

# 6. Shared Memory

## Concept

Shared Memory is the **fastest IPC mechanism**.

Multiple processes map the same physical memory region into their address space.

No copying of data is required.

---

## How It Works

```
             Shared Memory

        +----------------------+
        |                      |
        |      Memory          |
        |                      |
        +----------------------+

          ▲                ▲

          │                │

   Process A        Process B
```

Both processes directly read and write the same memory.

---

## System Calls

System V

```cpp
shmget()

shmat()

shmdt()

shmctl()
```

POSIX

```cpp
mmap()
```

---

## Example

```cpp
# Simple Shared Memory Example in C (POSIX)

This example demonstrates how to use **POSIX Shared Memory** with `shm_open()` and `mmap()`.

---

## Writer Program (`writer.c`)

```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

int main() {
    const char *name = "/my_shared_memory";
    const int SIZE = 1024;

    // Create shared memory object
    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

    // Set the size
    ftruncate(shm_fd, SIZE);

    // Map shared memory
    char *ptr = mmap(NULL, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Write data
    strcpy(ptr, "Hello from shared memory!");

    printf("Data written: %s\n", ptr);

    munmap(ptr, SIZE);
    close(shm_fd);

    return 0;
}
```

---

## Reader Program (`reader.c`)

```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    const char *name = "/my_shared_memory";
    const int SIZE = 1024;

    // Open existing shared memory
    int shm_fd = shm_open(name, O_RDONLY, 0666);

    // Map shared memory
    char *ptr = mmap(NULL, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);

    // Read data
    printf("Data read: %s\n", ptr);

    munmap(ptr, SIZE);
    close(shm_fd);

    // Delete shared memory object (optional)
    shm_unlink(name);

    return 0;
}
```

---

## Compile

```bash
gcc writer.c -o writer -lrt
gcc reader.c -o reader -lrt
```

> **Note:** On many modern Linux systems, `-lrt` is not required.

---

## Run

```bash
./writer
./reader
```

---

## Expected Output

```text
Data written: Hello from shared memory!
Data read: Hello from shared memory!
```

---

## Functions Used

| Function | Purpose |
|----------|---------|
| `shm_open()` | Creates or opens a shared memory object. |
| `ftruncate()` | Sets the size of the shared memory object. |
| `mmap()` | Maps the shared memory into the process's address space. |
| `strcpy()` | Writes data into shared memory. |
| `munmap()` | Unmaps the shared memory. |
| `close()` | Closes the shared memory file descriptor. |
| `shm_unlink()` | Deletes the shared memory object. |

---

## Workflow

```text
Writer Process
      |
      | shm_open()
      v
+-----------------------+
|    Shared Memory      |
+-----------------------+
      ^
      | mmap()
      |
Reader Process
```

The writer creates the shared memory, writes a message into it, and exits. The reader opens the same shared memory object, reads the message, and optionally removes the shared memory using `shm_unlink()`.

```

---

## Synchronization Required

Since both processes access the same memory,

Synchronization is required.

Common tools:

- Mutex
- Semaphore
- Spinlock

Otherwise,

Race conditions occur.

---

## Advantages

- Fastest IPC
- No data copying
- Excellent for large data

---

## Disadvantages

- Requires synchronization
- Complex programming
- Local machine only

---

## Use Cases

- Multimedia applications
- Video streaming
- Sensor data
- Database shared cache

---

## Don't Use When

- Processes are on different machines
- Synchronization cannot be guaranteed

---

# 7. Message Queue

## Concept

A Message Queue stores structured messages inside the kernel.

Processes communicate by sending and receiving messages.

Communication is asynchronous.

---

## How It Works

```
Process A

     │

 Send Message

     │

     ▼

+----------------+
| Message Queue  |
+----------------+

     ▲

 Receive Message

     │

Process B
```

---

## System Calls

```cpp
msgget()

msgsnd()

msgrcv()

msgctl()
```

---

## Example

```cpp
# Simple Message Queue Example in C (POSIX)

This example demonstrates **POSIX Message Queues** using `mq_open()`, `mq_send()`, and `mq_receive()`.

---

## Sender Program (`sender.c`)

```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <string.h>

int main() {
    mqd_t mq;
    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 100;
    attr.mq_curmsgs = 0;

    // Create/Open message queue
    mq = mq_open("/myqueue", O_CREAT | O_WRONLY, 0666, &attr);

    char msg[] = "Hello from Sender!";

    // Send message
    mq_send(mq, msg, strlen(msg) + 1, 0);

    printf("Message Sent: %s\n", msg);

    mq_close(mq);

    return 0;
}
```

---

## Receiver Program (`receiver.c`)

```c
#include <stdio.h>
#include <fcntl.h>
#include <mqueue.h>

int main() {
    mqd_t mq;
    char buffer[100];

    // Open existing message queue
    mq = mq_open("/myqueue", O_RDONLY);

    // Receive message
    mq_receive(mq, buffer, 100, NULL);

    printf("Message Received: %s\n", buffer);

    mq_close(mq);

    // Delete the message queue
    mq_unlink("/myqueue");

    return 0;
}
```

---

## Compile

```bash
gcc sender.c -o sender -lrt
gcc receiver.c -o receiver -lrt
```

> **Note:** On many modern Linux systems, `-lrt` may not be required.

---

## Run

```bash
./sender
./receiver
```

---

## Expected Output

```text
Message Sent: Hello from Sender!
Message Received: Hello from Sender!
```

---

## Functions Used

| Function | Purpose |
|----------|---------|
| `mq_open()` | Creates or opens a message queue. |
| `mq_send()` | Sends a message to the queue. |
| `mq_receive()` | Receives a message from the queue. |
| `mq_close()` | Closes the message queue. |
| `mq_unlink()` | Deletes the message queue. |

---

## Workflow

```text
Sender
   |
   | mq_send()
   v
+------------------+
|  Message Queue   |
+------------------+
   ^
   | mq_receive()
   |
Receiver
```

```

---

## Advantages

- Structured communication
- Message priorities
- Built-in buffering
- Asynchronous

---

## Disadvantages

- Kernel size limits
- Slower than shared memory
- Large messages inefficient

---

## Use Cases

- Producer-Consumer
- Event notification
- Task queues
- Job scheduling

---

## Don't Use When

- Very high performance is required
- Huge data transfer is needed

---

# 8. Socket

## Concept

Sockets provide bidirectional communication between processes.

They can communicate:

- On the same machine
- Across different machines

Sockets form the basis of client-server programming.

---

## Types

### Unix Domain Socket

Local machine communication.

### TCP Socket

Reliable network communication.

### UDP Socket

Fast but unreliable communication.

---

## How It Works

```
Client

    │

 Socket

    │

 Network

    │

 Socket

    │

Server
```

---

## Common System Calls

```cpp
socket()

bind()

listen()

accept()

connect()

send()

recv()

close()
```

---

## Example

```cpp
int sock = socket(AF_UNIX, SOCK_STREAM, 0);
```

---

## Advantages

- Bidirectional
- Cross-machine communication
- Network capable
- Standard client-server architecture

---

## Disadvantages

- Slower than shared memory
- Protocol overhead

---

## Use Cases

- Web Servers
- Chat Applications
- REST APIs
- Distributed Systems
- Microservices

---

## Don't Use When

- Both processes are local
- Maximum performance is required

---

# 9. Memory-Mapped File (mmap)

## Concept

`mmap()` maps a file directly into a process's virtual memory.

Processes access the file as if it were normal memory.

Multiple processes can map the same file.

Changes automatically update the file.

---

## How It Works

```
             data.bin

                 │

          Memory Mapping

                 │

      +--------------------+
      | Shared Memory Area |
      +--------------------+

          ▲            ▲

          │            │

    Process A     Process B
```

---

## System Call

```cpp
void *mmap(
    NULL,
    size,
    PROT_READ | PROT_WRITE,
    MAP_SHARED,
    fd,
    0
);
```

---

## Example

```cpp
int fd = open("data.bin", O_RDWR);

void *ptr = mmap(
    NULL,
    size,
    PROT_READ | PROT_WRITE,
    MAP_SHARED,
    fd,
    0
);
```

---

## Advantages

- Very fast
- File persistence
- Large file support
- No explicit read/write

---

## Disadvantages

- File I/O overhead
- Local machine only
- Requires careful synchronization

---

## Use Cases

- Database engines
- Shared caches
- Large file processing
- Shared file-backed memory

---

## Don't Use When

- Persistence is unnecessary
- Simpler IPC mechanisms are sufficient

---

# 10. IPC Comparison

| Feature | Pipe | FIFO | Shared Memory | Message Queue | Socket | mmap |
|---------|------|------|---------------|---------------|--------|------|
| Parent-Child | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Unrelated Processes | ❌ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Across Machines | ❌ | ❌ | ❌ | ❌ | ✅ | ❌ |
| Bidirectional | ❌ | ❌ | ✅ | ✅ | ✅ | ✅ |
| Persistent | ❌ | FIFO file exists | ❌ | Kernel-managed | Network connection | File-backed |
| Fast | Medium | Medium | ⭐ Fastest | Fast | Medium | Very Fast |
| Synchronization Needed | ❌ | ❌ | ✅ | ❌ | Protocol-based | ✅ |

---

# 11. Which IPC Should You Use?

| Requirement | Best Choice |
|------------|-------------|
| Parent ↔ Child | Unnamed Pipe |
| Unrelated Processes | Named Pipe (FIFO) |
| Very High Speed | Shared Memory |
| Structured Messages | Message Queue |
| Client-Server | Socket |
| Cross-Machine Communication | Socket |
| Shared Data + Persistence | mmap() |

---

# 12. Real-World Examples

| Application | IPC Used |
|------------|----------|
| Linux Shell (`ls \| grep`) | Pipe |
| Independent Local Programs | FIFO |
| Database Shared Cache | Shared Memory |
| Producer-Consumer Queue | Message Queue |
| Browser ↔ Web Server | TCP Socket |
| Chat Application | Socket |
| Database File Cache | mmap() |
| Video Processing | Shared Memory |
| Distributed Microservices | Socket |

---

# 13. Interview Questions

## Basic

- What is IPC?
- Why is IPC needed?
- Name different IPC mechanisms.
- What is the fastest IPC mechanism?
- What is the difference between a pipe and a FIFO?

---

## Intermediate

- Explain shared memory.
- Why is synchronization needed in shared memory?
- How does a message queue work?
- What is a Unix domain socket?
- What is the difference between TCP and Unix sockets?
- Explain `mmap()`.

---

## Advanced

- Pipe vs Socket.
- Shared Memory vs mmap().
- Message Queue vs Shared Memory.
- Why is shared memory faster than pipes?
- Which IPC would you choose for a database cache?
- Which IPC is best for distributed systems?
- Explain IPC used in Linux shell pipelines.
- How do processes synchronize while using shared memory?
- Explain producer-consumer using message queues.
- Which IPC mechanism would you choose for transferring large video frames and why?
---------------
