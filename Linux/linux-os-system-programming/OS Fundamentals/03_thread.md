# Chapter 3 — Threads

> **Three-layer approach**
>
> This chapter covers Threads from:
> 1. **[OS] Operating System concepts**
> 2. **[LSP] Linux System Programming**
> 3. **[KERNEL] Linux Kernel Internals**
>
> It also includes practical C code, Linux commands, working/flows, interview questions, and quick revision.

---

# 1. What Is a Thread? [OS]

A **thread is an execution unit within a process**.

A process provides the resource/address-space environment, while threads provide independent execution contexts.

Conceptually:

```text
Process
|
+-- Shared Address Space
|     |
|     +-- Code
|     +-- Global Data
|     +-- Heap
|     +-- Shared Libraries
|
+-- Shared Resources
|     |
|     +-- Open Files
|     +-- Signals / process resources
|     +-- Other process-level resources
|
+-- Thread 1
|     +-- Registers
|     +-- Program Counter
|     +-- Stack
|     +-- Scheduling State
|
+-- Thread 2
|     +-- Registers
|     +-- Program Counter
|     +-- Stack
|     +-- Scheduling State
|
+-- Thread 3
      +-- Registers
      +-- Program Counter
      +-- Stack
      +-- Scheduling State
```

The key idea:

```text
Process = resource/address-space container
Thread  = execution context
```

This is a simplified conceptual model; exact ownership/sharing semantics depend on the OS and threading model.

---

# 2. Why Do We Need Threads? [OS]

Suppose a server must handle:

```text
Client A
Client B
Client C
Client D
```

A single execution flow could process them sequentially:

```text
A -> B -> C -> D
```

If A performs slow I/O:

```text
A ---- waiting for I/O ----
          |
          +-- B/C/D cannot progress
```

With multiple threads:

```text
Thread 1 -> Client A
Thread 2 -> Client B
Thread 3 -> Client C
Thread 4 -> Client D
```

While one thread waits, another can execute.

On a multicore CPU, multiple runnable threads can also execute in parallel.

---

# 3. Concurrency vs Parallelism [OS]

These are often confused.

## Concurrency

Multiple tasks make progress during overlapping time periods.

```text
Time --->

Thread A:  A A   A       A
Thread B:    B B   B B
```

They may be interleaved on one CPU.

## Parallelism

Multiple tasks execute simultaneously on multiple CPU cores.

```text
CPU 0:  Thread A  Thread A  Thread A

CPU 1:  Thread B  Thread B  Thread B
```

Therefore:

```text
Concurrency != necessarily parallelism

Parallelism requires multiple execution resources
```

A multicore system can provide both.

---

# 4. Process vs Thread [OS]

| Process | Thread |
|---|---|
| Resource/address-space environment | Execution unit |
| Own virtual address space | Usually shares process address space |
| Higher isolation | Less isolation |
| Process creation is relatively expensive | Thread creation is generally cheaper |
| Communication often requires IPC | Shared memory is naturally available |
| Has process identity | Has thread identity |
| Own process-level resources | Has own execution state |
| Failure is generally isolated by process boundary | Memory corruption can affect sibling threads |

Important:

> "Threads are always cheaper than processes" is an oversimplification. Creation cost, synchronization, scheduling, memory footprint, and workload all matter.

---

# 5. What Does Each Thread Have? [OS]

Each thread needs its own execution context.

Typically:

```text
Thread
|
+-- Program Counter / Instruction Pointer
+-- CPU Registers
+-- Stack
+-- Stack Pointer
+-- Scheduling state
+-- Thread-local storage
+-- Thread identity
```

The stack is especially important.

Example:

```c
void worker()
{
    int x = 10;
}
```

Each thread calling `worker()` gets its own stack frame.

Conceptually:

```text
Thread 1 Stack             Thread 2 Stack
+----------------+         +----------------+
| worker() frame |         | worker() frame |
| x = 10         |         | x = 20         |
+----------------+         +----------------+
```

---

# 6. What Do Threads Share? [OS]

Threads in the same process generally share:

```text
+-------------------------+
| Process Address Space   |
+-------------------------+
| Code                    |
| Global variables        |
| Heap                    |
| Shared libraries        |
| Memory mappings         |
+-------------------------+
```

They may also share process-level resources such as:

```text
Open file descriptions / file-descriptor state
Signal dispositions
Credentials
Other process resources
```

But exact semantics need care.

For example, each thread has its own:

```text
Stack
Registers
Program counter
Thread-local storage
Scheduling state
```

---

# 7. Why Shared Memory Is Both Powerful and Dangerous [OS]

Suppose:

```c
int counter = 0;
```

Two threads execute:

```c
counter++;
```

Conceptually:

```text
Thread 1                 Thread 2

read counter = 0
                         read counter = 0

calculate 1
                         calculate 1

write 1
                         write 1
```

Final value:

```text
1
```

Expected:

```text
2
```

This is a **race condition**.

Synchronization is therefore essential when multiple threads access shared mutable state.

Chapter 5 covers synchronization in depth.

---

# 8. Thread Lifecycle [OS]

A simplified lifecycle:

```text
                 create
                   |
                   v
             +-----------+
             |   Ready   |
             +-----------+
                   |
                   | scheduler
                   v
             +-----------+
             |  Running  |
             +-----------+
              |    |    |
              |    |    |
       wait/I/O    |    | exit
              |    |    |
              v    |    v
          Blocked  |  Terminated
              |    |
       event occurs |
              |    |
              +----+
                |
                v
              Ready
```

A thread can:

- Be created
- Become runnable
- Run
- Block
- Become runnable again
- Exit

The exact state names are OS-dependent.

---

# 9. User-Level Threads vs Kernel-Level Threads [OS]

There are two broad implementation models.

## User-Level Threads

Thread management is primarily performed in user space.

```text
Application
|
+-- User thread library
     |
     +-- Thread A
     +-- Thread B
     +-- Thread C
```

The kernel may not see each user-level thread as an independently schedulable entity.

Advantages can include:

- Low overhead for user-space switching
- Flexible scheduling

Potential problems include:

- Blocking system calls can block the underlying execution resource
- Kernel may not independently schedule every user thread
- Multicore execution can be limited depending on implementation

---

# 10. Kernel-Level Threads [OS]

The kernel knows about the execution entities.

```text
Application
|
+-- Thread A ----\
+-- Thread B -----+--> Kernel scheduler
+-- Thread C ----/
```

Advantages:

- Kernel can schedule individual threads
- Better multicore support
- Blocking one thread need not block all sibling threads

Costs:

- Kernel involvement
- Scheduling/context-switch overhead
- More kernel-managed state

Modern Linux pthreads use kernel-visible tasks.

---

# 11. Linux Thread Model [KERNEL]

Linux uses a unified task model.

The kernel represents schedulable execution entities using:

```c
struct task_struct
```

This structure is central to both processes and threads.

A useful mental model is:

```text
Linux task
    |
    +-- task_struct
    |
    +-- scheduling state
    +-- identity
    +-- relationships
    +-- credentials
    +-- signal state
    +-- file-system/resource references
    +-- memory-management reference
```

This is one of the most important Linux kernel concepts.

---

# 12. Linux Does Not Have a Completely Separate "Thread Struct" [KERNEL]

For interview purposes, avoid saying:

> "Linux has one structure for processes and another completely separate structure for threads."

A better model is:

```text
Process / Thread
       |
       v
    task_struct
```

Linux uses tasks as the fundamental execution entities.

The distinction between a process and its threads is represented through how tasks share resources and are grouped.

---

# 13. `task_struct` and `mm_struct` [KERNEL]

A simplified relationship:

```text
                Process / Thread Group
                         |
                +--------+--------+
                |        |        |
              Task 1   Task 2   Task 3
                |        |        |
                +--------+--------+
                         |
                    shared mm_struct
                         |
                         v
                  Process Address Space
```

Conceptually:

```text
task_struct
    |
    +----> mm_struct
                |
                +-- virtual memory information
                +-- memory mappings
                +-- page-table-related information
```

Threads in the same process normally share the same memory-management context.

This is a major difference from separate processes.

---

# 14. Linux Thread Group [KERNEL]

Linux groups related tasks into a thread group.

Conceptually:

```text
Thread Group
|
+-- Main thread
|
+-- Worker thread
|
+-- Worker thread
|
+-- Worker thread
```

The process-level identity is associated with the thread group.

For Linux internals, understand the distinction between:

```text
task ID / TID
thread-group ID / TGID
```

For a multithreaded program:

```text
TGID = process/thread-group identifier
TID  = individual task/thread identifier
```

The main thread's TID is typically equal to the TGID.

---

# 15. `getpid()` and `gettid()` [LSP]

For Linux system programming:

```c
getpid()
```

returns the process/thread-group identity exposed as the PID to the application.

Linux also provides:

```c
gettid()
```

to obtain the calling thread's thread ID.

`gettid()` is Linux-specific.

Example:

```c
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

int main(void)
{
    printf("PID  = %d\n", getpid());
    printf("TID  = %ld\n", syscall(SYS_gettid));

    return 0;
}
```

Compile:

```bash
gcc tid.c -o tid
```

Run:

```bash
./tid
```

---

# 16. POSIX Threads — `pthread`

The standard Linux user-space threading API is commonly:

```c
pthread
```

Include:

```c
#include <pthread.h>
```

Important APIs:

```text
pthread_create()
pthread_join()
pthread_exit()
pthread_self()
pthread_detach()
pthread_cancel()
pthread_mutex_*
pthread_cond_*
```

Compile with:

```bash
gcc program.c -pthread
```

or commonly:

```bash
gcc program.c -lpthread
```

Prefer:

```bash
-pthread
```

because it enables the appropriate compiler/linker threading behavior.

---

# 17. Creating a Thread — `pthread_create()` [LSP]

Basic signature:

```c
int pthread_create(
    pthread_t *thread,
    const pthread_attr_t *attr,
    void *(*start_routine)(void *),
    void *arg
);
```

Conceptually:

```text
pthread_create()
       |
       +-- create thread
       |
       +-- start function
       |
       v
Thread begins execution
```

---

# 18. First Thread Program [LSP]

```c
#include <stdio.h>
#include <pthread.h>

void *worker(void *arg)
{
    printf("Worker thread running\n");
    return NULL;
}

int main(void)
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, worker, NULL) != 0)
    {
        perror("pthread_create");
        return 1;
    }

    pthread_join(thread, NULL);

    printf("Main thread exiting\n");

    return 0;
}
```

Compile:

```bash
gcc thread.c -pthread -o thread
```

Run:

```bash
./thread
```

Flow:

```text
main()
 |
 +-- pthread_create()
 |       |
 |       +---- worker()
 |
 +-- pthread_join()
 |
 v
exit
```

---

# 19. Why `pthread_join()`? [LSP]

`pthread_join()` allows one thread to wait for another thread to terminate.

Example:

```c
pthread_join(thread, NULL);
```

Conceptually:

```text
Main Thread
    |
    | join
    v
wait for worker
    |
    | worker exits
    v
continue
```

Without synchronization, the main thread could terminate the process before the worker has finished.

---

# 20. Thread Return Value [LSP]

A thread can return a value:

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *worker(void *arg)
{
    int *result = malloc(sizeof(int));

    if (result == NULL)
        return NULL;

    *result = 42;

    return result;
}

int main(void)
{
    pthread_t thread;
    void *ret;

    pthread_create(&thread, NULL, worker, NULL);

    pthread_join(thread, &ret);

    if (ret != NULL)
    {
        printf("Result = %d\n", *(int *)ret);
        free(ret);
    }

    return 0;
}
```

Important:

The returned pointer must point to valid memory after the worker returns.

Do not return the address of a local stack variable:

```c
void *worker(void *arg)
{
    int value = 42;
    return &value;       // WRONG
}
```

because `value` ceases to exist after the thread function returns.

---

# 21. Passing Arguments to Threads [LSP]

Example:

```c
#include <stdio.h>
#include <pthread.h>

void *worker(void *arg)
{
    int value = *(int *)arg;

    printf("Value = %d\n", value);

    return NULL;
}

int main(void)
{
    pthread_t thread;
    int value = 100;

    pthread_create(&thread, NULL, worker, &value);

    pthread_join(thread, NULL);

    return 0;
}
```

Important issue:

The pointed-to object must remain valid while the thread uses it.

---

# 22. Multiple Threads [LSP]

```c
#include <stdio.h>
#include <pthread.h>

void *worker(void *arg)
{
    int id = *(int *)arg;

    printf("Worker %d running\n", id);

    return NULL;
}

int main(void)
{
    pthread_t threads[3];
    int ids[3] = {1, 2, 3};

    for (int i = 0; i < 3; ++i)
    {
        pthread_create(&threads[i],
                       NULL,
                       worker,
                       &ids[i]);
    }

    for (int i = 0; i < 3; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
```

Possible output:

```text
Worker 1 running
Worker 3 running
Worker 2 running
```

The order is not guaranteed.

---

# 23. Why Is Thread Output Order Random? [OS]

The scheduler decides which runnable thread gets CPU time.

Therefore:

```text
Thread 1
Thread 2
Thread 3
```

does not imply:

```text
1 -> 2 -> 3
```

The actual order may be:

```text
2 -> 1 -> 3
```

or:

```text
3 -> 2 -> 1
```

or another order.

Do not rely on scheduling order unless synchronization establishes an ordering relationship.

---

# 24. Thread Safety Problem [LSP]

Example:

```c
#include <stdio.h>
#include <pthread.h>

int counter = 0;

void *worker(void *arg)
{
    for (int i = 0; i < 100000; ++i)
        counter++;

    return NULL;
}

int main(void)
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

Expected mathematically:

```text
200000
```

But unsynchronized concurrent access creates a data race in C/C++ and the program has undefined behavior under the language memory model.

This is not merely a Linux scheduling issue.

The correct solution is synchronization, such as a mutex or suitable atomic operations.

Synchronization is covered in Chapter 5.

---

# 25. Thread-Local Storage [OS/LSP]

Sometimes each thread needs its own copy of a variable.

Example:

```c
__thread int thread_value;
```

Each thread can have a different value:

```text
Thread 1 -> thread_value = 10
Thread 2 -> thread_value = 20
Thread 3 -> thread_value = 30
```

In portable POSIX programming, thread-specific data can also be implemented using:

```text
pthread_key_create()
pthread_setspecific()
pthread_getspecific()
```

---

# 26. POSIX Thread-Specific Data [LSP]

Example:

```c
#include <stdio.h>
#include <pthread.h>

pthread_key_t key;

void *worker(void *arg)
{
    int id = *(int *)arg;

    pthread_setspecific(key, (void *)(long)id);

    long value = (long)pthread_getspecific(key);

    printf("Thread-specific value = %ld\n", value);

    return NULL;
}

int main(void)
{
    pthread_t t1, t2;
    int a = 10;
    int b = 20;

    pthread_key_create(&key, NULL);

    pthread_create(&t1, NULL, worker, &a);
    pthread_create(&t2, NULL, worker, &b);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_key_delete(key);

    return 0;
}
```

The important idea is:

```text
One key
   |
   +-- Thread 1 -> value A
   +-- Thread 2 -> value B
```

---

# 27. Joinable vs Detached Threads [LSP]

A newly created POSIX thread is normally joinable unless created with detached attributes.

## Joinable

Another thread can call:

```c
pthread_join()
```

to wait for termination and collect its return value.

## Detached

A detached thread's resources are automatically reclaimed when it terminates.

You cannot later join it.

Conceptually:

```text
Joinable:

Thread
  |
  v
exit
  |
  v
pthread_join()
  |
  v
resources collected


Detached:

Thread
  |
  v
exit
  |
  v
resources automatically reclaimed
```

---

# 28. Detaching a Thread [LSP]

```c
pthread_detach(thread);
```

Example:

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *worker(void *arg)
{
    sleep(2);
    printf("Worker finished\n");
    return NULL;
}

int main(void)
{
    pthread_t thread;

    pthread_create(&thread, NULL, worker, NULL);

    pthread_detach(thread);

    printf("Main continues\n");

    sleep(3);

    return 0;
}
```

A detached thread cannot later be joined.

---

# 29. Thread Cancellation [LSP]

POSIX provides:

```c
pthread_cancel()
```

But cancellation should be treated carefully.

Example:

```c
pthread_cancel(thread);
```

Cancellation can be:

- Deferred
- Asynchronous

Deferred cancellation is generally safer because cancellation occurs at defined cancellation points.

A robust application should understand cleanup handlers and resource ownership before using cancellation.

---

# 30. `pthread_exit()` [LSP]

A thread can explicitly terminate itself:

```c
pthread_exit(NULL);
```

Example:

```c
void *worker(void *arg)
{
    printf("Worker\n");

    pthread_exit(NULL);
}
```

Returning from the thread start routine also terminates that thread.

---

# 31. Important Difference: `pthread_exit()` vs `exit()` [LSP]

This is a common interview question.

If one thread calls:

```c
pthread_exit(NULL);
```

that thread terminates.

Other threads in the same process can continue.

But:

```c
exit(0);
```

terminates the **entire process**, which means all its threads terminate.

Conceptually:

```text
pthread_exit()
    |
    +-- current thread exits

exit()
    |
    +-- entire process exits
         |
         +-- all threads terminate
```

---

# 32. Linux `clone()` [KERNEL/LSP]

Linux provides a lower-level process/task creation interface:

```c
clone()
```

It allows the caller to control which resources are shared.

Conceptually:

```text
clone()
 |
 +-- share memory?
 +-- share file descriptors?
 +-- share signal-related state?
 +-- share filesystem state?
 +-- etc.
```

This flexibility is central to Linux's unified task model.

At a conceptual level:

```text
clone()
   |
   +-- less sharing -> process-like behavior
   |
   +-- more sharing -> thread-like behavior
```

Do not reduce `clone()` to simply "the system call used by pthreads"; the actual modern implementation path can involve additional kernel/libc mechanisms and flags.

---

# 33. Process vs Thread Through `clone()` [KERNEL]

A simplified mental model:

```text
                 clone()
                    |
          +---------+---------+
          |                   |
     fewer shared          more shared
       resources             resources
          |                   |
          v                   v
      process-like        thread-like
```

Examples of resources that can be shared include:

```text
CLONE_VM
CLONE_FILES
CLONE_FS
CLONE_SIGHAND
CLONE_THREAD
```

These flags have specific kernel semantics.

For interviews, understand the concept rather than memorizing every flag without context.

---

# 34. `CLONE_VM` [KERNEL]

`CLONE_VM` means the child shares the memory descriptor with the caller.

Conceptually:

```text
Task A ----+
           |
           +----> same address space
           |
Task B ----+
```

This is a major ingredient of thread-like behavior.

Without shared memory, two tasks have separate address spaces.

---

# 35. `CLONE_FILES` [KERNEL]

This controls sharing of the file descriptor table.

Conceptually:

```text
Task A ----+
           |
           +----> shared file-descriptor table
           |
Task B ----+
```

Again, exact semantics are kernel-defined.

---

# 36. `CLONE_THREAD` [KERNEL]

`CLONE_THREAD` places the new task in the same thread group as the caller.

Conceptually:

```text
Thread Group
|
+-- Task A
+-- Task B
+-- Task C
```

This is important for understanding Linux's process/thread model.

---

# 37. Linux Threads and Scheduling [KERNEL]

Linux schedules runnable tasks.

Conceptually:

```text
Runnable tasks
|
+-- Thread A
+-- Thread B
+-- Thread C
+-- Thread D
       |
       v
   Scheduler
       |
       v
     CPU(s)
```

The scheduler does not simply schedule a "process" as one indivisible entity and then run all of its threads together.

Individual schedulable tasks compete for CPU time according to scheduler policy.

---

# 38. Thread Context [OS/KERNEL]

Each thread requires CPU execution state.

Simplified:

```text
Thread
 |
 +-- Program Counter
 +-- Stack Pointer
 +-- General-purpose registers
 +-- Processor flags/state
 +-- Kernel scheduling state
```

When switching:

```text
Thread A running
      |
      | save execution state
      v
    Scheduler
      |
      | restore state
      v
Thread B running
```

This is part of **context switching**.

Context switching is covered in more depth in Chapter 4.

---

# 39. Thread Stack [OS/KERNEL]

Each thread requires a stack for its own call frames.

```text
Process
|
+-- Thread 1
|     |
|     +-- Stack 1
|
+-- Thread 2
|     |
|     +-- Stack 2
|
+-- Thread 3
      |
      +-- Stack 3
```

But the stacks are all mapped into the process's shared virtual address space.

This is why stack corruption in one thread can potentially affect the process.

---

# 40. Thread Stack Overflow [LSP]

Recursive code can exhaust a thread's stack.

Example:

```c
void recurse(void)
{
    char buffer[1024];

    recurse();
}
```

Eventually the thread can hit its stack limit/guard region and receive a fault such as:

```text
SIGSEGV
```

The exact manifestation depends on the platform and memory mappings.

---

# 41. Thread Attributes [LSP]

POSIX provides:

```text
pthread_attr_t
```

Example attributes include:

```text
Detach state
Stack size
Stack address
Scheduling-related attributes
Guard size
```

Example:

```c
pthread_attr_t attr;

pthread_attr_init(&attr);

pthread_attr_setdetachstate(
    &attr,
    PTHREAD_CREATE_DETACHED
);

pthread_create(&thread, &attr, worker, NULL);

pthread_attr_destroy(&attr);
```

---

# 42. Thread Affinity [LSP/Linux]

Linux can restrict a thread to selected CPUs.

Useful APIs include:

```text
pthread_setaffinity_np()
pthread_getaffinity_np()
```

Example concept:

```text
Thread
   |
   +-- CPU 2 only
```

This is useful for:

- Performance tuning
- Real-time workloads
- Cache locality
- CPU isolation experiments

The `_np` suffix means it is non-portable.

---

# 43. CPU Affinity Experiment [LSP]

Example:

```c
#define _GNU_SOURCE

#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

void *worker(void *arg)
{
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(0, &set);

    if (pthread_setaffinity_np(
            pthread_self(),
            sizeof(set),
            &set) != 0)
    {
        perror("pthread_setaffinity_np");
        return NULL;
    }

    printf("Running on CPU %d\n", sched_getcpu());

    return NULL;
}

int main(void)
{
    pthread_t thread;

    pthread_create(&thread, NULL, worker, NULL);
    pthread_join(thread, NULL);

    return 0;
}
```

This is Linux-specific.

---

# 44. Thread IDs and `/proc` [LSP/KERNEL]

For a process:

```bash
ls /proc/<pid>/task
```

You may see:

```text
1234
1235
1236
1237
```

Each directory represents a task/thread ID.

Conceptually:

```text
/proc/PID/task/
       |
       +-- TID1
       +-- TID2
       +-- TID3
```

Inspect a particular thread:

```bash
cat /proc/<pid>/task/<tid>/status
```

---

# 45. Observe Threads with `ps` [LSP]

Run:

```bash
ps -T -p <pid>
```

or:

```bash
ps -eLf
```

Useful columns include:

```text
PID
LWP
NLWP
```

The exact output varies.

Typical meanings:

```text
PID  -> process/thread-group identity
LWP  -> lightweight process / thread identifier
NLWP -> number of lightweight processes/threads
```

---

# 46. `top` and Threads [LSP]

Run:

```bash
top
```

To show individual threads:

```text
H
```

in `top` on many Linux systems toggles thread display.

Alternatively:

```bash
top -H -p <pid>
```

This is useful for identifying which thread consumes CPU.

---

# 47. `gdb` and Threads [LSP]

Start:

```bash
gdb ./program
```

Useful commands:

```gdb
info threads
thread <number>
bt
thread apply all bt
```

Very useful interview/debugging command:

```gdb
thread apply all bt
```

It prints a backtrace for all threads.

---

# 48. `strace` and Threads [LSP]

Run:

```bash
strace -f ./program
```

The `-f` option follows processes/tasks created by the traced program.

This is useful for observing system calls from multiple threads.

---

# 49. Kernel View of a POSIX Thread [KERNEL]

Simplified conceptual flow:

```text
pthread_create()
       |
       v
C library / threading implementation
       |
       v
Linux task creation mechanism
       |
       v
task_struct for new task
       |
       +-- same address space
       +-- shared process resources
       +-- own execution state
       |
       v
Runnable
       |
       v
Scheduler
```

Do not assume every libc/kernel version has exactly the same internal call sequence.

The important interview concept is the mapping:

```text
POSIX thread
     |
     v
Linux task
     |
     v
task_struct
     |
     v
scheduler
```

---

# 50. Why Threads Can Be Faster Than Processes [OS]

Threads can avoid some costs associated with separate address spaces.

For example, threads in one process can directly access shared memory:

```text
Thread A
   |
   +----------+
              |
              v
          Shared Heap
              ^
              |
   +----------+
   |
Thread B
```

No explicit IPC is required just to access ordinary shared memory.

However, synchronization becomes necessary.

Therefore:

```text
Threads
  |
  +-- cheaper communication
  |
  +-- shared memory
  |
  +-- synchronization complexity
```

---

# 51. Why Processes Can Be Safer [OS]

Separate processes provide stronger memory isolation.

```text
Process A
+------------------+
| Address Space A  |
+------------------+

Process B
+------------------+
| Address Space B  |
+------------------+
```

A memory bug in A generally does not directly overwrite B's ordinary user-space memory.

With threads:

```text
Process
+---------------------------+
| Shared address space      |
|                           |
| Thread A <-> Thread B     |
+---------------------------+
```

A bad pointer can corrupt shared process state.

So:

```text
Processes -> stronger isolation
Threads   -> easier sharing
```

---

# 52. Thread Pools [OS/LSP]

Creating a new thread for every request may be inefficient.

Instead:

```text
                  +--> Worker 1
Requests ---> Queue ---> Worker 2
                  +--> Worker 3
                  +--> Worker 4
```

A thread pool:

- Creates a fixed/controlled number of workers
- Queues work
- Reuses threads
- Avoids excessive creation/destruction

Common in:

- Web servers
- Network services
- Database systems
- Background workers

---

# 53. Producer-Consumer with Threads [OS/LSP]

Concept:

```text
Producer
   |
   v
+---------+
|  Queue  |
+---------+
   |
   v
Consumer Threads
```

The queue is shared state.

Therefore synchronization is required.

Typical primitives:

```text
Mutex
Condition variable
Semaphore
Atomic operations
```

Detailed implementation is in Chapter 5.

---

# 54. Thread Cancellation and Cleanup [LSP]

If a thread owns resources:

```text
Thread
 |
 +-- lock
 +-- file
 +-- memory
 +-- socket
```

and is cancelled incorrectly:

```text
cancel
  |
  v
resource leak / locked mutex
```

POSIX provides cleanup handlers such as:

```text
pthread_cleanup_push()
pthread_cleanup_pop()
```

The key interview concept:

> Cancellation is not simply "kill the thread immediately." Resource cleanup and cancellation points matter.

---

# 55. Signal Handling with Threads [OS/LSP]

Signals become more subtle in multithreaded programs.

A signal can be:

```text
Process-directed
```

or:

```text
Thread-directed
```

POSIX/Linux provides mechanisms such as:

```text
pthread_kill()
pthread_sigmask()
sigwait()
```

A common design is to dedicate one thread to synchronously wait for selected signals using `sigwait()`.

Avoid designing complex asynchronous signal handlers unless necessary.

---

# 56. Thread Scheduling [OS]

The scheduler chooses runnable threads/tasks.

Simplified:

```text
Thread A ----+
Thread B ----+
Thread C ----+----> Scheduler ----> CPU
Thread D ----+
```

Important factors can include:

- Priority
- Scheduling policy
- CPU affinity
- Runtime
- Blocking
- Preemption

Chapter 4 will cover scheduling deeply.

---

# 57. Context Switching [OS/KERNEL]

A context switch changes execution from one task/thread to another.

Simplified:

```text
Thread A
   |
   | save state
   v
Kernel / Scheduler
   |
   | restore state
   v
Thread B
```

The saved state includes architecture-dependent execution state.

The switch can involve:

```text
Registers
Stack pointer
Instruction pointer
Processor state
Address-space context where required
```

Important:

> A context switch is not simply "save all registers and load all registers." Modern CPUs and Linux use architecture-specific mechanisms, and switching between tasks that share an address space can differ from switching between tasks with different address spaces.

---

# 58. Thread Context Switch vs Process Context Switch [OS]

The terminology can be misleading because Linux schedules tasks.

Conceptually:

```text
Thread A -> Thread B
```

may share the same address space.

While:

```text
Process A -> Process B
```

may require switching memory-management context.

The exact hardware work depends on architecture, address-space sharing, scheduler state, CPU features, and kernel implementation.

---

# 59. Thread Creation Cost [OS]

Thread creation requires:

```text
Kernel task state
Stack
Scheduling state
Resource references
Thread-local state
```

Therefore it is not free.

Thread pools are often used to amortize creation cost.

---

# 60. Important Linux Kernel Structures [KERNEL]

For interview preparation, know these names:

```text
struct task_struct
struct mm_struct
struct files_struct
struct fs_struct
struct signal_struct
struct sighand_struct
```

Conceptually:

```text
task_struct
 |
 +-- mm_struct       -> memory
 |
 +-- files_struct    -> file descriptors
 |
 +-- fs_struct       -> filesystem-related state
 |
 +-- signal-related structures
 |
 +-- scheduling state
```

Exact references and sharing rules are kernel-version dependent.

---

# 61. Process vs Thread in Linux — Mental Model [KERNEL]

Think:

```text
                Linux Tasks
                    |
          +---------+---------+
          |                   |
       Process-like        Thread-like
       task group           tasks
          |                   |
          |                   |
     separate mm         shared mm
          |                   |
          v                   v
    separate address     shared address
       space                space
```

This is a conceptual model, not a literal kernel class hierarchy.

---

# 62. Thread Creation Flow — Full Three-Layer View

```text
                APPLICATION
                    |
                    | pthread_create()
                    v
          LINUX SYSTEM PROGRAMMING
                    |
                    v
             libc / threading
               implementation
                    |
                    v
                  KERNEL
                    |
                    v
              task creation
                    |
                    v
              task_struct
                    |
             +------+------+
             |             |
             v             v
        shared mm      own CPU state
             |             |
             v             v
      Address space     Thread stack
             |             |
             +------+------+
                    |
                    v
                Runnable
                    |
                    v
                Scheduler
                    |
                    v
                  CPU
```

This is the model you should be able to explain in a senior interview.

---

# 63. Thread Termination Flow

```text
Thread running
      |
      | return / pthread_exit()
      v
Thread termination
      |
      v
Kernel cleans thread-specific state
      |
      v
Joinable?
   /       \
 yes       detached
  |           |
  v           v
join       resources
required   reclaimed
```

For a thread group, process termination occurs when the overall process/thread-group termination conditions are met, such as the main process exiting or an explicit process-wide `exit()`.

---

# 64. `exit()` in a Multithreaded Process

Suppose:

```text
Process
 |
 +-- Thread A
 +-- Thread B
 +-- Thread C
```

If Thread A calls:

```c
exit(0);
```

the process terminates.

Therefore:

```text
exit()
   |
   +-- terminate process
         |
         +-- Thread A
         +-- Thread B
         +-- Thread C
```

If Thread A returns from its start routine or calls:

```c
pthread_exit(NULL);
```

only Thread A terminates.

---

# 65. Common Thread Interview Traps

## Trap 1 — Threads have separate heaps

Normally no.

Threads in the same process share the process address space, including the heap.

---

## Trap 2 — Threads have the same stack

No.

Each thread needs its own stack.

---

## Trap 3 — `pthread_join()` kills a thread

No.

It waits for a thread to terminate and can collect its return value.

---

## Trap 4 — `pthread_exit()` terminates the process

No.

It terminates the calling thread.

---

## Trap 5 — `exit()` terminates only the current thread

No.

It terminates the process.

---

## Trap 6 — Threads always run in parallel

No.

They may execute concurrently on one CPU or in parallel on multiple CPUs.

---

## Trap 7 — Thread execution order is deterministic

No.

The scheduler controls execution order unless synchronization establishes ordering.

---

## Trap 8 — Shared memory automatically means safe communication

No.

Shared memory creates the possibility of fast communication, but synchronization is required for shared mutable state.

---

## Trap 9 — Linux has a completely separate kernel object for every thread called `thread_struct`

Do not describe Linux that way.

The central schedulable entity is `task_struct`. Architecture-specific thread state also exists, but `task_struct` is the key kernel task representation.

---

## Trap 10 — `clone()` simply means "create a thread"

Too simplistic.

`clone()` provides configurable sharing semantics. Different combinations produce process-like or thread-like behavior.

---

# 66. Practical Experiment — Create 5 Threads [LSP]

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

void *worker(void *arg)
{
    long id = (long)arg;

    printf("worker %ld: PID=%d TID=%ld\n",
           id,
           getpid(),
           syscall(SYS_gettid));

    sleep(2);

    return NULL;
}

int main(void)
{
    pthread_t threads[5];

    for (long i = 0; i < 5; ++i)
    {
        pthread_create(&threads[i],
                       NULL,
                       worker,
                       (void *)i);
    }

    for (int i = 0; i < 5; ++i)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
```

Compile:

```bash
gcc threads.c -pthread -o threads
```

Run:

```bash
./threads
```

Observe:

```text
PID
```

is generally the same for all threads, while:

```text
TID
```

is different.

---

# 67. Inspect the Threads

While the program is running:

```bash
ps -T -p <PID>
```

Also:

```bash
ls /proc/<PID>/task
```

You should see multiple task IDs.

This demonstrates:

```text
One process
    |
    +-- multiple Linux tasks/threads
```

---

# 68. Debug All Threads with GDB

Compile with debug symbols:

```bash
gcc -g threads.c -pthread -o threads
```

Start:

```bash
gdb ./threads
```

Inside GDB:

```gdb
run
info threads
```

Select a thread:

```gdb
thread 2
```

Backtrace:

```gdb
bt
```

All thread backtraces:

```gdb
thread apply all bt
```

This is an essential senior Linux debugging skill.

---

# 69. Observe System Calls

Run:

```bash
strace -f ./threads
```

You can inspect the system-call activity associated with the multithreaded application.

The exact thread-creation system calls visible depend on the libc/kernel implementation.

---

# 70. Senior Interview Question — Process vs Thread

A good answer:

> A process provides an isolated virtual address-space and resource environment, while a thread is an execution context within that environment. Threads of the same process normally share the address space, heap, global data and many process resources, but each thread has its own registers, program counter, stack and scheduling state. Threads make communication efficient through shared memory but introduce synchronization and fault-isolation challenges.

---

# 71. Senior Interview Question — What Does `pthread_create()` Do?

A good answer:

> `pthread_create()` creates a POSIX thread and starts execution at the supplied start routine. The user-space threading implementation requests creation of a Linux task with appropriate sharing semantics. The new task has its own execution state and stack but normally shares the process address space and relevant resources. It becomes runnable and is scheduled independently.

---

# 72. Senior Interview Question — Why Does Each Thread Need Its Own Stack?

Because each thread has independent function-call state.

For example:

```text
Thread A:
main()
  -> foo()
       -> bar()

Thread B:
main()
  -> worker()
       -> process()
```

Their stack frames must not overwrite each other.

Therefore:

```text
Thread A -> Stack A
Thread B -> Stack B
```

while:

```text
Thread A
Thread B
    |
    +---- shared heap/global memory
```

---

# 73. Senior Interview Question — What Does Linux Schedule?

A strong answer:

> Linux schedules runnable tasks. In a multithreaded process, individual threads are represented as tasks and can be scheduled independently. The scheduler selects runnable tasks according to scheduling policy, priority, CPU affinity and other factors.

---

# 74. Senior Interview Question — Why Does a Thread Have a Different TID but Usually the Same PID?

Because Linux distinguishes the individual task identity from the thread-group/process identity.

Conceptually:

```text
Process/thread group
TGID = 1000

Main thread:
TID = 1000

Worker:
TID = 1001

Worker:
TID = 1002
```

The exact user-space representation should be discussed in terms of Linux PID/TID semantics and namespaces when necessary.

---

# 75. Senior Interview Question — What Is the Biggest Advantage of Threads?

Efficient sharing of the process's address space and resources.

This makes communication through shared memory very fast compared with many explicit IPC mechanisms.

But the trade-off is:

```text
shared memory
    +
concurrent execution
    |
    v
synchronization required
```

---

# 76. Senior Interview Question — What Is the Biggest Risk of Threads?

Shared mutable state.

A bug in one thread can corrupt shared process memory.

Examples:

```text
Race condition
Deadlock
Data corruption
Use-after-free
Double free
Lock inversion
```

This is why synchronization and ownership design are critical.

---

# 77. Senior Interview Question — Does Creating More Threads Always Improve Performance?

No.

Too many threads can cause:

```text
Context-switch overhead
Scheduler overhead
Cache misses
Lock contention
Memory consumption
Oversubscription
```

Example:

```text
4 CPU cores

1000 runnable CPU-bound threads
```

does not mean 1000-way parallelism.

Only a limited number can execute simultaneously.

---

# 78. Thread Count and CPU Cores

Suppose:

```text
CPU cores = 4
Threads = 100
```

At most approximately four runnable threads can execute simultaneously on four logical CPUs.

The remaining runnable threads wait for CPU time.

```text
100 runnable threads
       |
       v
Scheduler
       |
       +--> CPU 0 -> Thread A
       +--> CPU 1 -> Thread B
       +--> CPU 2 -> Thread C
       +--> CPU 3 -> Thread D
```

The others remain runnable/waiting for CPU.

---

# 79. CPU-Bound vs I/O-Bound Threads

## CPU-bound

Example:

```text
large computation
compression
encryption
```

More threads than available CPUs may hurt due to contention/context switching.

## I/O-bound

Example:

```text
network
disk
waiting for external service
```

Multiple threads can be useful because some threads can wait while others execute.

But asynchronous I/O/event-driven designs may sometimes scale better than one-thread-per-operation.

---

# 80. Thread Pool Mental Model

For a server:

```text
                 Incoming Requests
                        |
                        v
                  +-----------+
                  | Work Queue|
                  +-----------+
                   /    |    \
                  /     |     \
                 v      v      v
             Worker  Worker  Worker
                1       2       3
```

Advantages:

```text
bounded thread count
thread reuse
controlled resource consumption
```

---

# 81. Three-Layer Summary

## [OS]

```text
Thread
 |
 +-- Execution unit
 +-- Own registers
 +-- Own PC
 +-- Own stack
 +-- Shares process address space
 +-- Can execute concurrently/parallel
```

## [LSP]

Know:

```text
pthread_create()
pthread_join()
pthread_exit()
pthread_self()
pthread_detach()
pthread_cancel()

pthread_mutex_*        -> synchronization
pthread_cond_*         -> waiting/signaling
pthread_sigmask()
pthread_kill()
pthread_setaffinity_np()
```

Practical tools:

```bash
ps -T
top -H
/proc/<pid>/task
gdb
strace -f
```

## [KERNEL]

Know:

```text
task_struct
mm_struct
thread group
PID/TID/TGID
clone()
CLONE_VM
CLONE_FILES
CLONE_SIGHAND
CLONE_THREAD
scheduler
task state
thread stack
context switch
```

---

# 82. Interview Flow to Memorize

When asked:

> "How does a POSIX thread get created in Linux?"

Answer using this sequence:

```text
Application
    |
    | pthread_create()
    v
POSIX/libc threading layer
    |
    v
Linux task creation
    |
    v
new task_struct
    |
    +-- own execution state
    +-- own stack
    +-- thread identity
    |
    +-- share process mm/resources
    |
    v
Runnable task
    |
    v
Linux scheduler
    |
    v
CPU
    |
    v
thread start routine
```

Then mention:

> The exact implementation path is libc and kernel-version dependent, so the diagram is a conceptual model rather than a promise of one exact system-call sequence.

This is a strong senior-level answer.

---

# 83. Chapter 3 Quick Revision

## One-line definitions

```text
Thread:
Execution unit inside a process.

Process:
Resource/address-space environment containing one or more execution units.

Concurrency:
Multiple tasks making progress over overlapping time.

Parallelism:
Multiple tasks executing simultaneously on multiple execution resources.

Race condition:
Program behavior depends on unsynchronized timing/order of concurrent operations.

Thread-safe:
Code behaves correctly under the intended concurrent access model.

Thread pool:
Reusable set of worker threads processing queued work.
```

---

# 84. Must-Know APIs

```text
pthread_create()
pthread_join()
pthread_exit()
pthread_self()
pthread_detach()
pthread_cancel()

pthread_attr_init()
pthread_attr_destroy()

pthread_key_create()
pthread_setspecific()
pthread_getspecific()

pthread_setaffinity_np()
pthread_getaffinity_np()

pthread_sigmask()
pthread_kill()
```

Know the purpose and basic usage of each.

---

# 85. Must-Know Linux Concepts

```text
task_struct
mm_struct
thread group
TGID
TID
clone()
CLONE_VM
CLONE_FILES
CLONE_SIGHAND
CLONE_THREAD
scheduler
runnable task
thread stack
context switch
CPU affinity
/proc/<pid>/task
```

---

# 86. Must-Practice Programs

Before moving to Chapter 4, implement these yourself:

```text
1. Create one POSIX thread
2. Create 5 threads
3. Pass arguments to threads
4. Return a result from a thread
5. Use pthread_join()
6. Create detached thread
7. Demonstrate shared-variable race
8. Fix race using mutex
9. Implement producer-consumer
10. Print PID/TID of each thread
11. Inspect threads through /proc
12. Debug threads using GDB
13. Observe thread-related syscalls with strace
14. Set thread CPU affinity
15. Build a simple thread pool
```

The synchronization programs will be revisited and expanded in Chapter 5.

---

# 87. Final Mental Model

```text
                         PROCESS
                            |
             +--------------+--------------+
             |              |              |
             v              v              v
           CODE           HEAP          GLOBAL DATA
             |              |              |
             +--------------+--------------+
                            |
                     SHARED ADDRESS SPACE
                            |
          +-----------------+-----------------+
          |                 |                 |
          v                 v                 v
      THREAD 1          THREAD 2          THREAD 3
          |                 |                 |
      +---+---+         +---+---+         +---+---+
      |       |         |       |         |       |
     PC    Registers   PC    Registers   PC    Registers
      |       |         |       |         |       |
      +-------+         +-------+         +-------+
          |
        Stack
          |
          v
       Scheduler
          |
          v
        CPU(s)
```

Linux kernel view:

```text
POSIX thread
     |
     v
Linux task
     |
     v
task_struct
     |
     +----> scheduling state
     +----> thread identity
     +----> execution state
     +----> stack
     |
     +----> shared mm_struct
                 |
                 v
          Process address space
```

---

# Chapter 3 — Key Takeaways

1. A thread is an execution unit within a process.
2. Threads of the same process normally share the address space.
3. Each thread has its own execution state and stack.
4. Threads enable concurrency and can enable parallelism on multicore systems.
5. Shared memory makes communication efficient but creates synchronization challenges.
6. `pthread_create()` creates a POSIX thread.
7. `pthread_join()` waits for a joinable thread.
8. `pthread_exit()` terminates the calling thread.
9. `exit()` terminates the entire process.
10. Detached threads cannot be joined.
11. Linux represents schedulable tasks using `task_struct`.
12. Linux uses a unified task model for processes and threads.
13. Threads in the same process normally share `mm_struct`.
14. Linux distinguishes individual task IDs from thread-group/process identity.
15. `clone()` provides configurable resource-sharing semantics.
16. `CLONE_VM` is associated with sharing the address space.
17. `CLONE_THREAD` places tasks in the same thread group.
18. Linux schedules runnable tasks, including individual threads.
19. Each thread needs its own stack and CPU execution state.
20. Context switching changes execution from one task to another.
21. Too many threads can reduce performance due to scheduling, cache and synchronization overhead.
22. Thread pools control thread creation and reuse workers.
23. `/proc/<pid>/task` is useful for inspecting Linux threads.
24. `gdb` and `strace -f` are important tools for multithreaded debugging.
25. The most important senior-level connection is:

```text
POSIX pthread
      ↓
Linux task
      ↓
task_struct
      ↓
shared process resources + private execution state
      ↓
scheduler
      ↓
CPU
```

---

# Next Chapter

## Chapter 4 — CPU Scheduling

Planned three-layer coverage:

```text
[OS]
- Scheduling goals
- CPU/I/O burst
- Preemptive vs non-preemptive scheduling
- FCFS
- SJF
- SRTF
- Round Robin
- Priority scheduling
- Multilevel queues
- Multilevel feedback queues
- Starvation
- Aging
- Response/turnaround/waiting time
- Context switching

[LSP]
- sched_* APIs
- nice()
- getpriority()
- setpriority()
- sched_yield()
- CPU affinity
- Real-time scheduling APIs
- Practical experiments
- top/ps/chrt/taskset

[KERNEL]
- Linux scheduler concepts
- runnable tasks
- scheduling classes
- CFS concepts
- fair scheduling
- real-time scheduling
- per-CPU run queues
- task migration
- context-switch path
- scheduler-related kernel structures
- modern Linux scheduler concepts

[INTERVIEW]
- Senior-level scheduler questions
- Context-switch questions
- Linux scheduler questions
- Performance/debugging scenarios
