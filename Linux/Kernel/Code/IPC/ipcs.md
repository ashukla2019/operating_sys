# Linux IPC + Reader-Writer Lock — C++ Interview Notes

Simple and small working examples for:

1. Anonymous Pipe
2. Named Pipe (FIFO)
3. Shared Memory
4. POSIX Message Queue
5. Reader-Writer Lock
6. Reader-Writer Lock implemented using Mutex + Condition Variable

---

# 1. IPC Overview

IPC = Inter-Process Communication.

It allows different processes to communicate.

Common IPC mechanisms:

```text
Process A
   |
   +---- Pipe
   |
   +---- Named Pipe (FIFO)
   |
   +---- Shared Memory
   |
   +---- Message Queue
   |
   +---- Socket
   |
   v
Process B
```

Important:

```text
Pipe
    -> Usually parent/child relationship

Named Pipe (FIFO)
    -> Can be used by unrelated processes

Shared Memory
    -> Fastest IPC
    -> Need synchronization

Message Queue
    -> Send messages between processes

Reader-Writer Lock
    -> Synchronization
    -> NOT itself an IPC mechanism
```

---

# 2. Anonymous Pipe

A pipe provides one-way communication.

Basic model:

```text
Process A
   |
   | write()
   v
+---------+
|  PIPE   |
+---------+
   |
   | read()
   v
Process B
```

A pipe has two file descriptors:

```cpp
pipefd[0]  -> read
pipefd[1]  -> write
```

---

# 3. Simple Pipe Example

Parent writes.

Child reads.

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main()
{
    int pipefd[2];

    pipe(pipefd);

    pid_t pid = fork();

    if (pid == 0)
    {
        // Child

        close(pipefd[1]);

        char buffer[100];

        read(
            pipefd[0],
            buffer,
            sizeof(buffer)
        );

        cout << "Child received: "
             << buffer
             << endl;

        close(pipefd[0]);
    }
    else
    {
        // Parent

        close(pipefd[0]);

        const char* msg =
            "Hello from parent";

        write(
            pipefd[1],
            msg,
            18
        );

        close(pipefd[1]);

        wait(nullptr);
    }

    return 0;
}
```

Compile:

```bash
g++ pipe.cpp -o pipe
```

Run:

```bash
./pipe
```

Output:

```text
Child received: Hello from parent
```

---

# 4. Pipe Important Functions

Create:

```cpp
pipe(pipefd);
```

Read:

```cpp
read(
    pipefd[0],
    buffer,
    size
);
```

Write:

```cpp
write(
    pipefd[1],
    data,
    size
);
```

Close:

```cpp
close(pipefd[0]);
close(pipefd[1]);
```

---

# 5. Pipe File Descriptors

After:

```cpp
int pipefd[2];

pipe(pipefd);
```

we have:

```text
pipefd[0]
    |
    +-- READ END


pipefd[1]
    |
    +-- WRITE END
```

Therefore:

```cpp
read(pipefd[0], ...);
```

and:

```cpp
write(pipefd[1], ...);
```

---

# 6. Why Do We Close Unused Ends?

Parent only writes:

```cpp
close(pipefd[0]);
```

Child only reads:

```cpp
close(pipefd[1]);
```

This is good practice.

Conceptually:

```text
Parent
   |
   | WRITE
   v
+------+
| PIPE |
+------+
   |
   | READ
   v
Child
```

Parent doesn't need the read end.

Child doesn't need the write end.

---

# 7. Pipe Is Usually One-Way

Example:

```text
Parent
   |
   | write
   v
 PIPE
   |
   | read
   v
Child
```

For two-way communication, commonly use two pipes:

```text
Parent
   |             ^
   | pipe1       | pipe2
   v             |
Child
```

---

# 8. Two-Way Pipe Example

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main()
{
    int p1[2];
    int p2[2];

    pipe(p1);
    pipe(p2);

    pid_t pid = fork();

    if (pid == 0)
    {
        // Child

        close(p1[1]);
        close(p2[0]);

        char buffer[100];

        read(
            p1[0],
            buffer,
            sizeof(buffer)
        );

        cout << "Child received: "
             << buffer
             << endl;

        const char* reply =
            "Hello parent";

        write(
            p2[1],
            reply,
            13
        );

        close(p1[0]);
        close(p2[1]);
    }
    else
    {
        // Parent

        close(p1[0]);
        close(p2[1]);

        const char* msg =
            "Hello child";

        write(
            p1[1],
            msg,
            12
        );

        char buffer[100];

        read(
            p2[0],
            buffer,
            sizeof(buffer)
        );

        cout << "Parent received: "
             << buffer
             << endl;

        close(p1[1]);
        close(p2[0]);

        wait(nullptr);
    }

    return 0;
}
```

Output:

```text
Child received: Hello child
Parent received: Hello parent
```

---

# 9. Named Pipe / FIFO

A normal pipe exists through file descriptors.

A named pipe has a name in the filesystem.

Example:

```text
/tmp/myfifo
```

Create:

```cpp
mkfifo(
    "/tmp/myfifo",
    0666
);
```

Then:

```text
Process A
    |
    | write
    v
/tmp/myfifo
    |
    | read
    v
Process B
```

Unlike an anonymous pipe, unrelated processes can use a FIFO.

---

# 10. Named Pipe — Writer

File:

```text
writer.cpp
```

```cpp
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

int main()
{
    const char* fifo =
        "/tmp/myfifo";

    mkfifo(
        fifo,
        0666
    );

    int fd = open(
        fifo,
        O_WRONLY
    );

    const char* msg =
        "Hello from writer";

    write(
        fd,
        msg,
        18
    );

    close(fd);

    return 0;
}
```

---

# 11. Named Pipe — Reader

File:

```text
reader.cpp
```

```cpp
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

int main()
{
    const char* fifo =
        "/tmp/myfifo";

    int fd = open(
        fifo,
        O_RDONLY
    );

    char buffer[100] = {};

    read(
        fd,
        buffer,
        sizeof(buffer)
    );

    cout << "Received: "
         << buffer
         << endl;

    close(fd);

    return 0;
}
```

Compile:

```bash
g++ writer.cpp -o writer
g++ reader.cpp -o reader
```

Run reader first:

```bash
./reader
```

Then in another terminal:

```bash
./writer
```

Output:

```text
Received: Hello from writer
```

---

# 12. Named Pipe Cleanup

Delete the FIFO:

```bash
rm /tmp/myfifo
```

Or from C++:

```cpp
unlink("/tmp/myfifo");
```

---

# 13. Anonymous Pipe vs Named Pipe

| Feature | Pipe | Named Pipe |
|---|---|---|
| Also called | Anonymous pipe | FIFO |
| Filesystem name | No | Yes |
| Parent-child communication | Common | Possible |
| Unrelated processes | Usually no | Yes |
| Creation | `pipe()` | `mkfifo()` |
| Read | `read()` | `read()` |
| Write | `write()` | `write()` |

Remember:

```text
pipe()
    -> anonymous pipe

mkfifo()
    -> named pipe
```

---

# 14. Shared Memory

Shared memory allows multiple processes to access the same memory region.

Conceptually:

```text
Process A
    |
    |\
    | \
    |  \
    v   v
+----------------+
| Shared Memory  |
+----------------+
    ^   ^
    |   |
    |   |
Process B
```

It is generally very fast because processes access the same memory instead of repeatedly copying data through the kernel.

But:

```text
Shared memory
    +
Synchronization
```

are usually needed together.

---

# 15. POSIX Shared Memory

Important functions:

```cpp
shm_open()
ftruncate()
mmap()
munmap()
shm_unlink()
```

Basic flow:

```text
shm_open()
     |
     v
ftruncate()
     |
     v
mmap()
     |
     v
Access shared memory
     |
     v
munmap()
     |
     v
shm_unlink()
```

---

# 16. Very Simple Shared Memory Example

Parent creates shared memory.

Child reads it.

```cpp
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main()
{
    const char* name =
        "/my_shared_memory";

    int fd = shm_open(
        name,
        O_CREAT | O_RDWR,
        0666
    );

    ftruncate(
        fd,
        1024
    );

    char* memory = static_cast<char*>(
        mmap(
            nullptr,
            1024,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            0
        )
    );

    pid_t pid = fork();

    if (pid == 0)
    {
        // Child

        sleep(1);

        cout << "Child received: "
             << memory
             << endl;

        munmap(
            memory,
            1024
        );

        close(fd);
    }
    else
    {
        // Parent

        strcpy(
            memory,
            "Hello from shared memory"
        );

        wait(nullptr);

        munmap(
            memory,
            1024
        );

        close(fd);

        shm_unlink(name);
    }

    return 0;
}
```

Compile:

```bash
g++ shared_memory.cpp -o shared_memory
```

Run:

```bash
./shared_memory
```

Output:

```text
Child received: Hello from shared memory
```

---

# 17. Shared Memory Important Functions

Create/open:

```cpp
shm_open(
    name,
    O_CREAT | O_RDWR,
    0666
);
```

Set size:

```cpp
ftruncate(
    fd,
    1024
);
```

Map memory:

```cpp
mmap(
    nullptr,
    1024,
    PROT_READ | PROT_WRITE,
    MAP_SHARED,
    fd,
    0
);
```

Unmap:

```cpp
munmap(
    memory,
    1024
);
```

Remove:

```cpp
shm_unlink(name);
```

---

# 18. Why Do We Need `ftruncate()`?

When we create shared memory:

```cpp
shm_open();
```

we have an object, but we need to give it a size.

Therefore:

```cpp
ftruncate(
    fd,
    1024
);
```

means:

```text
Make shared memory 1024 bytes.
```

---

# 19. Why Do We Need `mmap()`?

`shm_open()` gives us a file descriptor.

We need to map that shared-memory object into the process's address space.

That's what:

```cpp
mmap()
```

does.

Conceptually:

```text
Shared memory object
        |
        | mmap()
        v
Process virtual address space
        |
        v
char* memory
```

Then:

```cpp
strcpy(
    memory,
    "Hello"
);
```

can access the shared memory.

---

# 20. Shared Memory Needs Synchronization

Suppose:

```cpp
process A -> writes
process B -> reads
```

If both access the memory at the same time, synchronization may be required.

Possible synchronization mechanisms:

```text
Mutex
Semaphore
Condition variable
Process-shared pthread synchronization
```

Shared memory gives you the data-sharing mechanism.

It does NOT automatically solve synchronization.

---

# 21. Message Queue

A message queue allows processes to send messages.

Conceptually:

```text
Process A
    |
    | send message
    v
+----------------+
| Message Queue  |
+----------------+
    |
    | receive
    v
Process B
```

Unlike shared memory, the receiver doesn't directly access the sender's memory.

---

# 22. POSIX Message Queue

POSIX APIs include:

```cpp
mq_open()
mq_send()
mq_receive()
mq_close()
mq_unlink()
```

Need:

```cpp
#include <mqueue.h>
```

Compile with:

```bash
g++ mq.cpp -o mq -lrt
```

On some modern Linux systems:

```bash
g++ mq.cpp -o mq
```

may also work.

---

# 23. Simple Message Queue — Sender

File:

```text
sender.cpp
```

```cpp
#include <iostream>
#include <mqueue.h>
#include <cstring>

using namespace std;

int main()
{
    const char* name =
        "/my_queue";

    mqd_t mq = mq_open(
        name,
        O_CREAT | O_WRONLY,
        0666,
        nullptr
    );

    const char* msg =
        "Hello from sender";

    mq_send(
        mq,
        msg,
        strlen(msg) + 1,
        0
    );

    mq_close(mq);

    return 0;
}
```

---

# 24. Simple Message Queue — Receiver

File:

```text
receiver.cpp
```

```cpp
#include <iostream>
#include <mqueue.h>

using namespace std;

int main()
{
    const char* name =
        "/my_queue";

    mqd_t mq = mq_open(
        name,
        O_RDONLY
    );

    char buffer[100] = {};

    mq_receive(
        mq,
        buffer,
        sizeof(buffer),
        nullptr
    );

    cout << "Received: "
         << buffer
         << endl;

    mq_close(mq);

    mq_unlink(name);

    return 0;
}
```

Compile:

```bash
g++ sender.cpp -o sender -lrt
g++ receiver.cpp -o receiver -lrt
```

Run receiver:

```bash
./receiver
```

Then sender:

```bash
./sender
```

Output:

```text
Received: Hello from sender
```

---

# 25. Message Queue Important APIs

Create/open:

```cpp
mq_open()
```

Send:

```cpp
mq_send()
```

Receive:

```cpp
mq_receive()
```

Close:

```cpp
mq_close()
```

Delete:

```cpp
mq_unlink()
```

---

# 26. Message Queue Priority

POSIX message queues support message priorities.

Send:

```cpp
mq_send(
    mq,
    msg,
    strlen(msg) + 1,
    10
);
```

The last argument:

```cpp
10
```

is the priority.

Receive:

```cpp
unsigned int priority;

mq_receive(
    mq,
    buffer,
    sizeof(buffer),
    &priority
);
```

Higher-priority messages can be received before lower-priority messages.

---

# 27. Pipe vs Message Queue

| Feature | Pipe | Message Queue |
|---|---|---|
| Data model | Byte stream | Messages |
| Message boundaries | No | Yes |
| Priority | No | Yes |
| FIFO | Yes | Yes |
| API | `read/write` | `mq_send/mq_receive` |
| Named version | FIFO | Queue name |

Important interview point:

```text
Pipe:
    "bytes"

Message Queue:
    "messages"
```

---

# 28. Shared Memory vs Message Queue

## Shared Memory

```text
Fast
+
Direct memory access
+
Need synchronization
```

## Message Queue

```text
Messages
+
Kernel manages queue
+
No direct shared memory access
```

Simple comparison:

```text
Shared Memory
    -> Fastest
    -> More synchronization responsibility

Message Queue
    -> Easier message-based communication
    -> Kernel manages messages
```

---

# 29. Pipe vs Shared Memory vs Message Queue

| Feature | Pipe | Shared Memory | Message Queue |
|---|---|---|---|
| Communication | Byte stream | Shared data | Messages |
| Speed | Good | Very fast | Good |
| Synchronization | Pipe semantics | Need synchronization | Queue semantics |
| Message boundaries | No | Application-defined | Yes |
| Unrelated processes | FIFO needed | Yes | Yes |
| Main APIs | `pipe/read/write` | `shm_open/mmap` | `mq_send/mq_receive` |

---

# 30. Reader-Writer Lock

A reader-writer lock is a synchronization mechanism.

It allows:

```text
Multiple readers
    OR
One writer
```

but not:

```text
Readers + Writer simultaneously
```

Conceptually:

```text
             LOCK
               |
       +-------+-------+
       |               |
     READ            WRITE
       |               |
 Multiple readers    One writer
 allowed             allowed
       |               |
       +-------+-------+
               |
            UNLOCK
```

---

# 31. Why Reader-Writer Lock?

Suppose:

```text
Database / shared configuration

90% operations = READ
10% operations = WRITE
```

A normal mutex:

```text
Reader 1 -> lock
Reader 2 -> waits
Reader 3 -> waits
```

Only one reader works at a time.

A reader-writer lock allows:

```text
Reader 1 ----\
Reader 2 -----+--> Read simultaneously
Reader 3 ----/
```

But:

```text
Writer
   |
   v
Must wait until readers finish.
```

---

# 32. POSIX Reader-Writer Lock

POSIX provides:

```cpp
pthread_rwlock_t
```

Important APIs:

```cpp
pthread_rwlock_init()
pthread_rwlock_rdlock()
pthread_rwlock_wrlock()
pthread_rwlock_unlock()
pthread_rwlock_destroy()
```

---

# 33. Simple Reader-Writer Lock Example

```cpp
#include <iostream>
#include <pthread.h>
#include <unistd.h>

using namespace std;

int data = 0;

pthread_rwlock_t rwlock =
    PTHREAD_RWLOCK_INITIALIZER;

void* reader(void* arg)
{
    pthread_rwlock_rdlock(&rwlock);

    cout << "Reader sees: "
         << data
         << endl;

    sleep(1);

    pthread_rwlock_unlock(&rwlock);

    return nullptr;
}

void* writer(void* arg)
{
    pthread_rwlock_wrlock(&rwlock);

    data++;

    cout << "Writer changed data to: "
         << data
         << endl;

    pthread_rwlock_unlock(&rwlock);

    return nullptr;
}

int main()
{
    pthread_t r1;
    pthread_t r2;
    pthread_t w1;

    pthread_create(
        &r1,
        nullptr,
        reader,
        nullptr
    );

    pthread_create(
        &r2,
        nullptr,
        reader,
        nullptr
    );

    pthread_create(
        &w1,
        nullptr,
        writer,
        nullptr
    );

    pthread_join(r1, nullptr);
    pthread_join(r2, nullptr);
    pthread_join(w1, nullptr);

    pthread_rwlock_destroy(&rwlock);

    return 0;
}
```

Compile:

```bash
g++ rwlock.cpp -pthread -o rwlock
```

Run:

```bash
./rwlock
```

---

# 34. Reader Lock

Use:

```cpp
pthread_rwlock_rdlock(
    &rwlock
);
```

Multiple readers can hold the lock simultaneously.

Example:

```cpp
pthread_rwlock_rdlock(&rwlock);

cout << data << endl;

pthread_rwlock_unlock(&rwlock);
```

---

# 35. Writer Lock

Use:

```cpp
pthread_rwlock_wrlock(
    &rwlock
);
```

Only one writer can hold the lock.

Readers cannot access the protected data while the writer holds the lock.

Example:

```cpp
pthread_rwlock_wrlock(&rwlock);

data++;

pthread_rwlock_unlock(&rwlock);
```

---

# 36. Reader-Writer Lock Rules

Remember:

```text
Reader + Reader
    -> Allowed

Reader + Writer
    -> NOT allowed

Writer + Writer
    -> NOT allowed
```

Table:

| Current | New Reader | New Writer |
|---|---:|---:|
| Reader | Allowed | Wait |
| Writer | Wait | Wait |

---

# 37. Implement Reader-Writer Lock Yourself

For interviews, you may be asked:

```text
Implement a reader-writer lock using mutex and condition variable.
```

Basic state:

```cpp
int readers = 0;
bool writer = false;
```

Meaning:

```text
readers
    -> Number of active readers

writer
    -> Is a writer currently active?
```

---

# 38. Simple Reader-Writer Lock Implementation

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

class RWLock
{
private:

    pthread_mutex_t mutex =
        PTHREAD_MUTEX_INITIALIZER;

    pthread_cond_t cond =
        PTHREAD_COND_INITIALIZER;

    int readers = 0;

    bool writer = false;

public:

    void readLock()
    {
        pthread_mutex_lock(&mutex);

        while (writer)
        {
            pthread_cond_wait(
                &cond,
                &mutex
            );
        }

        readers++;

        pthread_mutex_unlock(&mutex);
    }

    void readUnlock()
    {
        pthread_mutex_lock(&mutex);

        readers--;

        if (readers == 0)
        {
            pthread_cond_signal(&cond);
        }

        pthread_mutex_unlock(&mutex);
    }

    void writeLock()
    {
        pthread_mutex_lock(&mutex);

        while (
            writer ||
            readers > 0
        )
        {
            pthread_cond_wait(
                &cond,
                &mutex
            );
        }

        writer = true;

        pthread_mutex_unlock(&mutex);
    }

    void writeUnlock()
    {
        pthread_mutex_lock(&mutex);

        writer = false;

        pthread_cond_broadcast(
            &cond
        );

        pthread_mutex_unlock(&mutex);
    }
};

int main()
{
    RWLock lock;

    int data = 100;

    lock.readLock();

    cout << "Reading data: "
         << data
         << endl;

    lock.readUnlock();

    lock.writeLock();

    data++;

    cout << "Writing data: "
         << data
         << endl;

    lock.writeUnlock();

    return 0;
}
```

Compile:

```bash
g++ rwlock_custom.cpp -pthread -o rwlock_custom
```

Run:

```bash
./rwlock_custom
```

Output:

```text
Reading data: 100
Writing data: 101
```

---

# 39. Custom Reader Lock Explained

This is the important part:

```cpp
void readLock()
{
    pthread_mutex_lock(&mutex);

    while (writer)
    {
        pthread_cond_wait(
            &cond,
            &mutex
        );
    }

    readers++;

    pthread_mutex_unlock(&mutex);
}
```

Meaning:

```text
1. Lock mutex
2. If writer is active:
       wait
3. Otherwise:
       increase reader count
4. Unlock mutex
```

Multiple readers can increase:

```text
readers = 1
readers = 2
readers = 3
...
```

Therefore multiple readers can read simultaneously.

---

# 40. Custom Reader Unlock

```cpp
void readUnlock()
{
    pthread_mutex_lock(&mutex);

    readers--;

    if (readers == 0)
    {
        pthread_cond_signal(&cond);
    }

    pthread_mutex_unlock(&mutex);
}
```

When the final reader leaves:

```cpp
readers == 0
```

we notify a waiting thread.

---

# 41. Custom Writer Lock

```cpp
void writeLock()
{
    pthread_mutex_lock(&mutex);

    while (
        writer ||
        readers > 0
    )
    {
        pthread_cond_wait(
            &cond,
            &mutex
        );
    }

    writer = true;

    pthread_mutex_unlock(&mutex);
}
```

Writer waits if:

```text
writer == true
```

OR:

```text
readers > 0
```

Therefore:

```text
Existing writer?
    -> WAIT

Existing readers?
    -> WAIT

No readers + no writer?
    -> WRITE
```

---

# 42. Custom Writer Unlock

```cpp
void writeUnlock()
{
    pthread_mutex_lock(&mutex);

    writer = false;

    pthread_cond_broadcast(
        &cond
    );

    pthread_mutex_unlock(&mutex);
}
```

The writer is finished.

We wake waiting threads.

---

# 43. Important Limitation of Simple RW Lock

The simple implementation above is easy to understand, but it is **not necessarily fair**.

For example:

```text
Many readers keep arriving.

Writer waits.

New readers may continue entering.
```

This can cause:

```text
Writer starvation
```

A production-quality RW lock may track waiting writers and prevent new readers from continuously entering.

For interviews, first understand the simple implementation.

Then discuss fairness if asked.

---

# 44. Fair Reader-Writer Lock Idea

To improve fairness, maintain:

```cpp
int readers;
int waitingWriters;
bool writer;
```

Reader should wait when:

```cpp
writer || waitingWriters > 0
```

Writer increments:

```cpp
waitingWriters++;
```

before waiting.

This prevents new readers from continuously bypassing a waiting writer.

---

# 45. Reader-Writer Lock Interview Implementation

A slightly fairer version:

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

class RWLock
{
private:

    pthread_mutex_t mutex =
        PTHREAD_MUTEX_INITIALIZER;

    pthread_cond_t cond =
        PTHREAD_COND_INITIALIZER;

    int readers = 0;

    int waitingWriters = 0;

    bool writer = false;

public:

    void readLock()
    {
        pthread_mutex_lock(&mutex);

        while (
            writer ||
            waitingWriters > 0
        )
        {
            pthread_cond_wait(
                &cond,
                &mutex
            );
        }

        readers++;

        pthread_mutex_unlock(&mutex);
    }

    void readUnlock()
    {
        pthread_mutex_lock(&mutex);

        readers--;

        if (readers == 0)
        {
            pthread_cond_broadcast(
                &cond
            );
        }

        pthread_mutex_unlock(&mutex);
    }

    void writeLock()
    {
        pthread_mutex_lock(&mutex);

        waitingWriters++;

        while (
            writer ||
            readers > 0
        )
        {
            pthread_cond_wait(
                &cond,
                &mutex
            );
        }

        waitingWriters--;

        writer = true;

        pthread_mutex_unlock(&mutex);
    }

    void writeUnlock()
    {
        pthread_mutex_lock(&mutex);

        writer = false;

        pthread_cond_broadcast(
            &cond
        );

        pthread_mutex_unlock(&mutex);
    }
};

int main()
{
    RWLock lock;

    int data = 10;

    lock.readLock();

    cout << "Read: "
         << data
         << endl;

    lock.readUnlock();

    lock.writeLock();

    data++;

    cout << "Write: "
         << data
         << endl;

    lock.writeUnlock();

    return 0;
}
```

Compile:

```bash
g++ rwlock_fair.cpp -pthread -o rwlock_fair
```

---

# 46. IPC Quick Comparison

| IPC | Main Idea | Speed | Message Based? |
|---|---|---|---|
| Pipe | Byte stream | Good | No |
| Named Pipe | Named byte stream | Good | No |
| Shared Memory | Shared memory region | Very fast | No |
| Message Queue | Kernel-managed messages | Good | Yes |

---

# 47. When Would You Use What?

## Pipe

Use when:

```text
Parent <-> Child
```

Example:

```text
shell commands
```

---

## Named Pipe

Use when:

```text
Two unrelated local processes
```

Example:

```text
Process A
    |
    v
/tmp/myfifo
    |
    v
Process B
```

---

## Shared Memory

Use when:

```text
Large amount of data
+
High performance required
```

Example:

```text
Large data structures
Image/video buffers
High-speed IPC
```

But remember:

```text
Need synchronization.
```

---

## Message Queue

Use when:

```text
Small discrete messages
+
Message boundaries matter
```

Example:

```text
Request
Response
Command
Event
```

---

# 48. IPC Interview Questions

## Pipe

1. What is a pipe?
2. What does `pipe()` return?
3. What is `pipefd[0]`?
4. What is `pipefd[1]`?
5. Is a pipe bidirectional?
6. How can you implement two-way communication?
7. Why close unused pipe ends?

## Named Pipe

8. What is a FIFO?
9. Difference between pipe and FIFO?
10. What does `mkfifo()` do?
11. Can unrelated processes communicate using FIFO?
12. How do you remove a FIFO?

## Shared Memory

13. What is shared memory?
14. Why is shared memory fast?
15. What does `shm_open()` do?
16. Why use `ftruncate()`?
17. What does `mmap()` do?
18. Why is synchronization required with shared memory?
19. What does `shm_unlink()` do?

## Message Queue

20. What is a message queue?
21. Difference between pipe and message queue?
22. What does `mq_send()` do?
23. What does `mq_receive()` do?
24. Can messages have priority?

## Reader-Writer Lock

25. What is a reader-writer lock?
26. Can multiple readers enter simultaneously?
27. Can multiple writers enter simultaneously?
28. Can a reader and writer enter simultaneously?
29. How would you implement RW lock using mutex + condition variable?
30. What is writer starvation?
31. How can you improve fairness?

---

# 49. Most Important Interview Differences

## Pipe vs FIFO

```text
pipe()
    -> Anonymous pipe

mkfifo()
    -> Named pipe
```

---

## Pipe vs Message Queue

```text
Pipe
    -> Byte stream

Message Queue
    -> Separate messages
```

---

## Message Queue vs Shared Memory

```text
Message Queue
    -> Send/receive messages

Shared Memory
    -> Processes access same memory
```

---

## Mutex vs RW Lock

```text
Mutex
    -> One thread at a time

RW Lock
    -> Multiple readers
    -> One writer
```

---

# 50. The Four Patterns to Memorize

## Pattern 1: Pipe

```cpp
int fd[2];

pipe(fd);

write(
    fd[1],
    data,
    size
);

read(
    fd[0],
    buffer,
    size
);
```

---

## Pattern 2: Shared Memory

```cpp
int fd = shm_open(
    name,
    O_CREAT | O_RDWR,
    0666
);

ftruncate(
    fd,
    size
);

void* memory = mmap(
    nullptr,
    size,
    PROT_READ | PROT_WRITE,
    MAP_SHARED,
    fd,
    0
);
```

---

## Pattern 3: Message Queue

```cpp
mqd_t mq = mq_open(
    name,
    O_CREAT | O_RDWR,
    0666,
    nullptr
);

mq_send(
    mq,
    message,
    size,
    priority
);

mq_receive(
    mq,
    buffer,
    size,
    nullptr
);
```

---

## Pattern 4: Reader-Writer Lock

Reader:

```cpp
pthread_rwlock_rdlock(
    &rwlock
);

// read

pthread_rwlock_unlock(
    &rwlock
);
```

Writer:

```cpp
pthread_rwlock_wrlock(
    &rwlock
);

// write

pthread_rwlock_unlock(
    &rwlock
);
```

---

# 51. Final Mental Model

Think of IPC like this:

```text
             IPC
              |
      +-------+-------+
      |       |       |
      v       v       v
    Pipe    Shared   Message
            Memory   Queue
      |       |       |
      v       v       v
    Bytes   Memory   Messages
```

And synchronization:

```text
             Synchronization
                    |
          +---------+---------+
          |                   |
          v                   v
        Mutex              RW Lock
                              |
                    +---------+---------+
                    |                   |
                  Reader              Writer
                    |                   |
                 Multiple              One
```

The key interview sentence to remember:

```text
Pipe = byte stream
FIFO = named pipe
Shared memory = fastest data sharing, but needs synchronization
Message queue = message-based IPC
RW lock = multiple readers OR one writer
```

---

# 52. Compile Commands

## Pipe

```bash
g++ pipe.cpp -o pipe
./pipe
```

## FIFO

```bash
g++ writer.cpp -o writer
g++ reader.cpp -o reader
```

Run:

```bash
./reader
```

Then:

```bash
./writer
```

## Shared Memory

```bash
g++ shared_memory.cpp -o shared_memory
./shared_memory
```

## Message Queue

```bash
g++ sender.cpp -o sender -lrt
g++ receiver.cpp -o receiver -lrt
```

## Reader-Writer Lock

```bash
g++ rwlock.cpp -pthread -o rwlock
./rwlock
```

Custom RW lock:

```bash
g++ rwlock_custom.cpp -pthread -o rwlock_custom
./rwlock_custom
```

---

# 53. One-Page Revision

```text
PIPE
----
pipe(fd)

fd[0] = READ
fd[1] = WRITE

read(fd[0], ...)
write(fd[1], ...)


FIFO / NAMED PIPE
-----------------
mkfifo()

open()
read()
write()
close()

unlink()


SHARED MEMORY
-------------
shm_open()
ftruncate()
mmap()

access memory

munmap()
close()
shm_unlink()


MESSAGE QUEUE
-------------
mq_open()
mq_send()
mq_receive()
mq_close()
mq_unlink()


READER-WRITER LOCK
------------------
Reader:
    rdlock()
    read
    unlock()

Writer:
    wrlock()
    write
    unlock()


CUSTOM RW LOCK
--------------
readers = number of active readers
writer = whether writer is active

Reader waits if:
    writer == true

Writer waits if:
    writer == true
    OR
    readers > 0
```

---

# End

