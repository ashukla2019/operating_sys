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
int shmid = shmget(key, 1024, IPC_CREAT | 0666);

char *ptr = (char*)shmat(shmid, NULL, 0);
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
msgsnd(msqid, &msg, sizeof(msg), 0);

msgrcv(msqid, &msg, sizeof(msg), 0, 0);
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