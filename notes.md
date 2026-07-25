# Senior Embedded Linux Interview Notes
# Part 1A – Inter Process Communication (IPC)

> **Target:** Senior Embedded Linux Engineer (5–10+ Years)
> **Companies:** Qualcomm, NVIDIA, Intel, AMD, Samsung, Google, Apple, Meta, Amazon Lab126

---

# Table of Contents

1. Introduction to IPC
2. IPC Mechanisms
3. Pipe (Unnamed Pipe)
4. FIFO (Named Pipe)
5. Pipe vs FIFO
6. Interview Questions

---

# 1. Inter Process Communication (IPC)

## What is IPC?

Inter Process Communication (IPC) is a mechanism that enables two or more independent processes to exchange data and synchronize their execution.

Since every Linux process has its own virtual address space, one process cannot directly access another process's memory. IPC provides controlled ways to communicate.

---

## Why IPC is Required?

IPC is used for:

- Data sharing
- Resource sharing
- Synchronization
- Event notification
- Client-server communication
- Parallel processing

Example:

```
+-----------+        IPC        +-----------+
| Process A | <---------------> | Process B |
+-----------+                   +-----------+
```

---

# Linux IPC Mechanisms

```
                        IPC
                         |
 -------------------------------------------------------
 |         |          |          |         |           |
 Pipe     FIFO   Shared Mem   Msg Queue  Semaphore   Socket
                                          |
                                        Mutex
```

---

## IPC Comparison

| IPC | Speed | Related Process | Unrelated Process | Data Copy | Synchronization |
|------|-------|-----------------|-------------------|-----------|-----------------|
| Pipe | Medium | Yes | No | Yes | No |
| FIFO | Medium | Yes | Yes | Yes | No |
| Shared Memory | Fastest | Yes | Yes | No | External |
| Message Queue | Medium | Yes | Yes | Yes | Built-in |
| Socket | Slow | Yes | Yes | Yes | Protocol |
| Semaphore | N/A | Yes | Yes | No | Yes |
| Mutex | N/A | Threads | No | No | Yes |

---

## Which IPC Should You Use?

| Scenario | Preferred IPC |
|----------|----------------|
| Parent ↔ Child | Pipe |
| Unrelated Processes | FIFO |
| High-Speed Data Sharing | Shared Memory |
| Ordered Messages | Message Queue |
| Synchronization | Semaphore |
| Thread Synchronization | Mutex |
| Network Communication | Socket |

---

# 2. Pipe (Unnamed Pipe)

## Definition

A pipe is a unidirectional communication channel used between related processes.

Created using:

```c
#include <unistd.h>

int pipe(int fd[2]);
```

Returns:

```
fd[0] --> Read End

fd[1] --> Write End
```

---

## Architecture

```
          Parent

            |

        write(fd[1])

            |

      +-------------+

      |    PIPE     |

      +-------------+

            |

        read(fd[0])

            |

          Child
```

---

## Working

1. Parent creates pipe.
2. Parent calls `fork()`.
3. Child inherits pipe file descriptors.
4. Parent writes.
5. Child reads.

---

## Example

```c
#include <stdio.h>
#include <unistd.h>

int main()
{
    int fd[2];
    char buffer[20];

    pipe(fd);

    if(fork()==0)
    {
        read(fd[0],buffer,sizeof(buffer));
        printf("%s\n",buffer);
    }
    else
    {
        write(fd[1],"Hello",6);
    }

    return 0;
}
```

---

## Pipe Characteristics

- Half duplex
- Kernel managed
- Byte stream
- Temporary
- Parent-child communication
- Anonymous

---

## Advantages

- Very simple API
- Fast communication
- Low overhead
- Kernel synchronization

---

## Limitations

- Works only between related processes
- Half duplex
- No message boundaries
- Limited kernel buffer

---

## Kernel Buffer

Typical size:

```
64 KB
```

Can be queried using:

```c
fcntl(fd,F_GETPIPE_SZ);
```

Can be changed using:

```c
fcntl(fd,F_SETPIPE_SZ,size);
```

---

## Blocking Behavior

### Read

If pipe is empty:

```
read()

↓

Blocks
```

---

### Write

If pipe is full:

```
write()

↓

Blocks
```

---

## Non-blocking Pipe

```c
fcntl(fd,F_SETFL,O_NONBLOCK);
```

Now

```
read()

↓

Returns immediately
```

---

## Full Duplex Communication

A single pipe supports only one direction.

Need two pipes.

```
Parent ---------> Child

Parent <--------- Child
```

---

# Common Interview Questions

### Q1. Why does pipe require fork()?

Because the child inherits the file descriptors created by the parent.

---

### Q2. Why can't unrelated processes use unnamed pipes?

Unnamed pipes have no filesystem entry, so unrelated processes cannot obtain their file descriptors.

---

### Q3. Why is pipe called half duplex?

Because communication occurs only in one direction.

---

### Q4. Can multiple writers write into the same pipe?

Yes.

However,

- Data larger than `PIPE_BUF` may interleave.
- Writes up to `PIPE_BUF` are atomic.

---

### Q5. What happens if all writers close the pipe?

The reader receives:

```
EOF

(read returns 0)
```

---

### Q6. What happens if a process writes to a pipe with no reader?

Kernel sends:

```
SIGPIPE
```

and

```
write()

↓

returns -1

errno = EPIPE
```

---

# 3. FIFO (Named Pipe)

## Definition

FIFO (First In First Out) is a named pipe stored in the filesystem.

Unlike unnamed pipes, unrelated processes can communicate through it.

---

## Creating FIFO

Using command:

```bash
mkfifo myfifo
```

Using C:

```c
mkfifo("myfifo",0666);
```

---

## Architecture

```
Process A

      |

      |

+---------------+

|     FIFO      |

+---------------+

      |

      |

Process B
```

---

## Features

- Has pathname
- Persistent until deleted
- Half duplex
- Supports unrelated processes
- Byte stream communication

---

## Opening FIFO

Writer

```c
open("myfifo",O_WRONLY);
```

Reader

```c
open("myfifo",O_RDONLY);
```

---

## Example

Writer

```c
int fd=open("myfifo",O_WRONLY);

write(fd,"Linux",6);
```

Reader

```c
int fd=open("myfifo",O_RDONLY);

read(fd,buffer,100);
```

---

## Advantages

- Easy to use
- Persistent
- Supports unrelated processes
- Simple API

---

## Limitations

- Slower than shared memory
- Kernel copy involved
- Half duplex
- Sequential communication

---

# Pipe vs FIFO

| Feature | Pipe | FIFO |
|----------|------|------|
| Named | No | Yes |
| Filesystem Entry | No | Yes |
| Parent-Child | Yes | Not Required |
| Unrelated Processes | No | Yes |
| Lifetime | Until process exits | Until removed |
| Created Using | `pipe()` | `mkfifo()` |

---

# Qualcomm / FAANG Interview Questions

### Basic

- What is IPC?
- Why is IPC needed?
- Explain pipe.
- Explain FIFO.
- Difference between pipe and FIFO.
- Why are unnamed pipes anonymous?

---

### Intermediate

- How does Linux implement pipes internally?
- Explain the pipe buffer.
- Why are writes smaller than `PIPE_BUF` atomic?
- What happens when a reader exits unexpectedly?
- Explain blocking vs non-blocking pipes.

---

### Advanced

- How does Linux wake blocked readers?
- Explain reference counting for pipes.
- What is `SIGPIPE`?
- How does `epoll()` work with pipes?
- How does the kernel synchronize concurrent writers?

---

# Quick Revision

| Topic | Key Point |
|--------|-----------|
| Pipe | Parent-child communication |
| FIFO | Unrelated process communication |
| Pipe Type | Byte stream |
| Direction | Half duplex |
| Fastest IPC | Shared Memory (covered in Part 1B) |
| Pipe Buffer | Typically 64 KB |
| Atomic Write | Up to `PIPE_BUF` |
| Pipe Error | `SIGPIPE` if no reader |

---

## Next Part

**Part 1B** covers:

- Shared Memory
- Message Queue
- POSIX vs System V IPC
- Performance Comparison
- Qualcomm Interview Questions
