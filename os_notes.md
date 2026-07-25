# Senior Embedded Linux Interview Notes
# Part 1 – Inter Process Communication (IPC)

> **Target Audience:** Senior Embedded Linux Engineers (5–10+ years)
> **Interview Focus:** Qualcomm, NVIDIA, Intel, AMD, Google, Apple, Amazon Lab126, Samsung, Meta, Microsoft

---

# Table of Contents

1. Introduction to IPC
2. Types of IPC
3. Pipe (Unnamed Pipe)
4. Named Pipe (FIFO)
5. Shared Memory
6. Message Queue
7. Semaphore
8. Mutex
9. Reader-Writer Lock
10. Deadlock
11. Race Condition
12. Priority Inversion
13. IPC Comparison Table
14. POSIX IPC vs System V IPC
15. Common Interview Questions

---

# 1. What is IPC?

**IPC (Inter Process Communication)** is a mechanism that allows two or more processes to exchange data and synchronize their execution.

## Why IPC?

- Data sharing
- Resource sharing
- Process synchronization
- Event notification
- Client-server communication

---

## IPC Overview

```
                 IPC
                  |
------------------------------------------------------
|       |         |         |        |               |
Pipe   FIFO   Shared Mem  Msg Queue Semaphore     Socket
                                   |
                                 Mutex
```

---

# Choosing an IPC Mechanism

| IPC | Speed | Related Process | Unrelated Process | Kernel Copy | Synchronization |
|------|------|----------------|-------------------|-------------|-----------------|
| Pipe | ⭐⭐ | Yes | No | Yes | No |
| FIFO | ⭐⭐ | Yes | Yes | Yes | No |
| Shared Memory | ⭐⭐⭐⭐⭐ | Yes | Yes | No | External |
| Message Queue | ⭐⭐⭐ | Yes | Yes | Yes | Built-in |
| Socket | ⭐⭐ | Yes | Yes | Yes | Protocol |
| Semaphore | - | Yes | Yes | No | Yes |
| Mutex | - | Threads | No | No | Yes |

---

# 2. Pipe (Unnamed Pipe)

A pipe is a **unidirectional communication channel** between related processes.

Created using:

```c
#include <unistd.h>

int fd[2];

pipe(fd);
```

Returns

```
fd[0] --> Read End

fd[1] --> Write End
```

Architecture

```
Parent
   |
 Write
   |
 Pipe
   |
 Read
   |
Child
```

Example

```c
int fd[2];

pipe(fd);

write(fd[1], "Hello", 5);

read(fd[0], buffer, 5);
```

---

## Features

- Half duplex
- Parent-child communication
- Kernel buffer
- Exists until processes terminate

---

## Advantages

- Simple
- Fast
- Kernel managed

---

## Limitations

- Related processes only
- No message boundaries
- Half duplex

---

## Senior Interview Questions

### Q1 Why is pipe only for related processes?

Because file descriptors are inherited through `fork()`.

---

### Q2 Pipe size?

Typically

```
64 KB
```

Can be modified using

```
fcntl()
```

---

### Q3 Can pipe be bidirectional?

No.

Need two pipes.

```
Parent -----> Child

Child -----> Parent
```

---

# 3. Named Pipe (FIFO)

FIFO stands for

```
First In First Out
```

Created using

```bash
mkfifo myfifo
```

or

```c
mkfifo("myfifo",0666);
```

Architecture

```
Process A

      FIFO

Process B
```

---

## Features

- Exists in filesystem
- Communication between unrelated processes
- Half duplex

---

## Advantages

- Persistent
- Simple

---

## Disadvantages

- Slower than shared memory
- Sequential communication

---

## Pipe vs FIFO

| Pipe | FIFO |
|------|------|
| Anonymous | Named |
| Parent-child only | Any process |
| Temporary | Persistent |
| Created by pipe() | Created by mkfifo() |

---

# 4. Shared Memory

Fastest IPC mechanism.

Kernel allocates a memory segment.

Both processes map the same physical memory.

Architecture

```
          Shared Memory

+-----------------------------+

P1 --------------------------->

                              Memory

P2 <---------------------------

+-----------------------------+
```

---

## System Calls

```c
shmget()

shmat()

shmdt()

shmctl()
```

---

## Example

```c
int id = shmget(key,1024,0666);

char *ptr = shmat(id,NULL,0);

strcpy(ptr,"Hello");
```

---

## Advantages

- Fastest IPC
- Zero-copy after mapping
- Large data transfer

---

## Disadvantages

- Requires synchronization
- Complex cleanup

---

## Why is Shared Memory Fast?

Other IPC

```
User

↓

Kernel

↓

User
```

Shared Memory

```
Process A

↓

RAM

↑

Process B
```

No repeated copying.

---

# 5. Message Queue

Kernel stores messages with priority.

Architecture

```
Message Queue

+----------------------+

Message 1

Message 2

Message 3

+----------------------+
```

---

## System Calls

```c
msgget()

msgsnd()

msgrcv()

msgctl()
```

---

## Message Structure

```c
struct msg {

long type;

char data[100];

};
```

---

## Advantages

- Ordered communication
- Message priority
- Synchronization built in

---

## Disadvantages

- Kernel copy required
- Slower than shared memory

---

## Interview Question

When should Message Queue be preferred?

Answer:

- Ordered communication
- Prioritized messages
- Simpler synchronization

---

# 6. Semaphore

Semaphore controls access to shared resources.

Imagine a parking lot.

```
5 Parking Slots

Car enters

Count--

Car leaves

Count++
```

---

## Types

### Binary Semaphore

```
0

1
```

Acts similar to mutex.

---

### Counting Semaphore

```
0

1

2

3

...

N
```

Used for multiple resources.

---

## Operations

```
wait()

signal()
```

Also called

```
P()

V()
```

---

## Example

```c
sem_wait(&sem);

/* Critical Section */

sem_post(&sem);
```

---

## Use Cases

- Producer Consumer
- Shared Memory
- Resource Pool

---

# 7. Mutex

Mutex means

```
Mutual Exclusion
```

Only one thread owns it.

```
Thread A

↓

Lock

↓

Critical Section

↓

Unlock

↓

Thread B enters
```

---

## Features

- Ownership exists
- Only owner unlocks
- Thread synchronization

---

## Mutex vs Semaphore

| Mutex | Semaphore |
|---------|------------|
| Ownership | No ownership |
| One owner | Multiple users |
| Thread synchronization | Process/Thread synchronization |
| Only owner unlocks | Any thread may signal |

---

# 8. Reader-Writer Lock

Optimized for

```
Many Readers

Few Writers
```

```
Reader1

Reader2

Reader3

      ||

 Shared Data

      ||

Writer waits
```

---

## Benefits

- Better performance
- Multiple concurrent readers
- Exclusive writer

---

## Use Cases

- Databases
- Routing tables
- Configuration data
- Cache

---

# 9. Deadlock

Deadlock occurs when processes wait forever.

Example

```
P1 waits for P2

P2 waits for P1
```

---

## Four Necessary Conditions

1. Mutual Exclusion
2. Hold and Wait
3. No Preemption
4. Circular Wait

---

## Prevention

- Lock ordering
- Timeout
- Trylock
- Resource hierarchy

---

# 10. Race Condition

Occurs when multiple threads access shared data simultaneously.

Example

```
counter = 5

Thread A reads 5

Thread B reads 5

Thread A writes 6

Thread B writes 6

Expected = 7

Actual = 6
```

Solution

- Mutex
- Semaphore
- Atomic operations

---

# 11. Priority Inversion

Scenario

```
High Priority Task

↓

Waiting

↓

Low Priority Task owns mutex

↓

Medium Priority Task keeps executing
```

High priority task gets blocked.

---

## Solution

Priority Inheritance Protocol

Interviewers love asking this topic.

---

# 12. POSIX IPC vs System V IPC

| POSIX | System V |
|--------|----------|
| shm_open() | shmget() |
| sem_open() | semget() |
| mq_open() | msgget() |
| Easier API | Older API |
| Portable | Legacy |

---

# 13. IPC Performance Comparison

| IPC | Speed | Copy | Synchronization |
|------|------|------|----------------|
| Pipe | Medium | Kernel | No |
| FIFO | Medium | Kernel | No |
| Shared Memory | Fastest | No | External |
| Message Queue | Medium | Kernel | Yes |
| Socket | Slow | Kernel | Protocol |

---

# 14. Common Qualcomm / FAANG Interview Questions

## Basic

- What is IPC?
- Which IPC is fastest?
- Pipe vs FIFO?
- Mutex vs Semaphore?
- Binary Semaphore vs Mutex?
- Why use Reader-Writer Lock?

---

## Intermediate

- Explain shared memory implementation.
- Why is shared memory faster?
- Explain producer-consumer problem.
- Explain semaphore internals.
- Explain message queue priority.
- What happens if shared memory creator dies?
- Difference between POSIX and System V IPC?

---

## Advanced

- Explain cache coherency in shared memory.
- How does Linux synchronize multicore shared memory?
- Explain memory barriers.
- Explain futex.
- Explain robust mutex.
- Explain lock-free programming.
- Explain spinlock vs mutex.
- Explain RCU (Read Copy Update).
- Explain priority inversion in Linux.
- Explain how scheduler interacts with mutexes.

---

# Quick Revision Sheet

| IPC | Best For |
|------|-----------|
| Pipe | Parent-child communication |
| FIFO | Unrelated processes |
| Shared Memory | Large data transfer |
| Message Queue | Ordered communication |
| Semaphore | Synchronization |
| Mutex | Thread locking |
| RW Lock | Many readers |

---

# One-Line Interview Answers

**Fastest IPC?**

Shared Memory.

---

**Pipe vs FIFO?**

Pipe is anonymous and for related processes.
FIFO has a filesystem entry and supports unrelated processes.

---

**Semaphore vs Mutex?**

Semaphore controls resource count.
Mutex provides ownership-based mutual exclusion.

---

**Why is Shared Memory fastest?**

Because processes access the same mapped physical memory without repeated kernel copying.

---

**Why is mutex preferred over binary semaphore?**

Because mutex has ownership semantics, preventing accidental unlocks by other threads.

---

**When should Reader-Writer Lock be used?**

When read operations significantly outnumber write operations.

---

# End of Part 1
