# Chapter 5 — Synchronization

> **Three-layer approach**
>
> This chapter covers synchronization from:
> 1. **[OS] Operating System concepts**
> 2. **[LSP] Linux System Programming + C/C++ code**
> 3. **[KERNEL] Linux Kernel Internals**
>
> The goal is to understand not only *which primitive to use*, but also **why it works, what happens when a thread blocks, and what the kernel does underneath**.

---

# 1. Why Synchronization Is Needed [OS]

When multiple threads execute concurrently and access shared data, their operations can interleave.

Example:

```text
Thread A                    Thread B

read counter = 10
                            read counter = 10
counter = 11
                            counter = 11
```

Expected:

```text
counter = 12
```

Actual:

```text
counter = 11
```

This is a **race condition**.

Synchronization provides mechanisms to control access to shared resources.

---

# 2. Concurrency vs Parallelism [OS]

## Concurrency

Multiple tasks make progress during overlapping periods.

```text
Time →

A: ███     ███
B:    ███     ███
```

## Parallelism

Multiple tasks actually execute simultaneously on different CPUs/cores.

```text
CPU 0:  AAAAAAAA
CPU 1:  BBBBBBBB
```

Synchronization is required for both concurrent and parallel programs when shared state exists.

---

# 3. Shared Data

Examples:

```text
Global variables
Heap objects
Shared memory
Files
Sockets
Kernel data structures
Device state
Reference counters
Queues
```

Example:

```c
int counter = 0;
```

If multiple threads modify `counter`, access must be designed carefully.

---

# 4. Race Condition [OS]

A race condition occurs when the result depends on the timing/interleaving of concurrent operations.

Example:

```c
counter++;
```

It looks like one operation, but conceptually it may involve:

```text
LOAD counter
ADD 1
STORE counter
```

Two threads can interleave these operations.

```text
Thread A              Thread B

LOAD 0
                      LOAD 0
ADD 1
                      ADD 1
STORE 1
                      STORE 1
```

Final value:

```text
1
```

Expected:

```text
2
```

---

# 5. Critical Section [OS]

A critical section is a section of code that accesses shared state and must be protected against unsafe concurrent access.

```c
lock();

shared_data++;

unlock();
```

Conceptually:

```text
          Critical Section
       +--------------------+
       | shared data access |
       +--------------------+
              protected
```

---

# 6. Requirements of a Good Critical-Section Solution [OS]

Important properties:

## Mutual exclusion

Only one thread enters the protected critical section at a time.

## Progress

If no thread is inside the critical section, a suitable waiting thread should eventually be able to enter.

## Bounded waiting

A thread should not wait forever under the intended scheduling/fairness assumptions.

---

# 7. Atomicity [OS]

An operation is atomic when other threads cannot observe it in an intermediate state.

Example:

```text
Atomic:
    counter = 5 -> 6

Non-atomic conceptual sequence:
    load
    modify
    store
```

Atomicity does not automatically solve every synchronization problem.

For example:

```c
atomic_increment(counter);
```

may make an increment atomic, but a larger multi-variable invariant may still require a mutex.

---

# 8. Visibility and Ordering [OS]

Synchronization is not only about preventing simultaneous access.

It also establishes rules about:

```text
Visibility
Ordering
Memory effects
```

Example:

```text
Thread A:
data = 100;
ready = true;

Thread B:
if (ready)
    use(data);
```

Without appropriate synchronization, reasoning about visibility and ordering can be incorrect.

A mutex or correctly used atomic operations can establish the required synchronization.

---

# 9. Synchronization Primitives

Important primitives:

```text
Mutex
Semaphore
Condition variable
Spinlock
Reader-writer lock
Barrier
Atomic operations
Futex
```

At a high level:

```text
Mutex       -> mutual exclusion
Semaphore   -> counting/resource synchronization
Cond var    -> wait for a condition
Spinlock    -> busy-wait mutual exclusion
RWLock      -> multiple readers / one writer
Barrier     -> synchronize phases
Atomic      -> indivisible atomic operations
Futex       -> efficient user-space/kernel-assisted waiting
```

---

# 10. Mutex [OS]

A mutex provides mutual exclusion.

Basic pattern:

```c
lock(mutex);

/* critical section */

unlock(mutex);
```

Only one owner can hold the mutex at a time.

---

# 11. Mutex Working

Suppose:

```text
Thread A -> lock
Thread B -> lock
```

If A obtains the mutex:

```text
Mutex
 |
 +-- owner = A

A -> enters critical section
B -> waits
```

After A unlocks:

```text
A -> unlock
B -> can acquire
```

---

# 12. Mutex Does Not Protect Automatically

This is not enough:

```c
pthread_mutex_t lock;
int counter;

counter++;
```

The mutex must actually be used:

```c
pthread_mutex_lock(&lock);

counter++;

pthread_mutex_unlock(&lock);
```

All code paths accessing the protected invariant must follow the synchronization protocol.

---

# 13. POSIX Mutex Example [LSP]

```c
#include <pthread.h>
#include <stdio.h>

int counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
    for (int i = 0; i < 100000; ++i)
    {
        pthread_mutex_lock(&lock);

        counter++;

        pthread_mutex_unlock(&lock);
    }

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

    pthread_mutex_destroy(&lock);

    return 0;
}
```

Compile:

```bash
gcc mutex.c -pthread -O2 -o mutex
```

Expected:

```text
counter = 200000
```

---

# 14. Why the Mutex Example Works

Without synchronization:

```text
T1: read counter
T2: read counter
T1: write
T2: write
```

With mutex:

```text
T1:
 lock
 read
 modify
 write
 unlock

T2:
          waits
          lock
          read
          modify
          write
          unlock
```

The critical section is serialized.

---

# 15. Always Check Mutex Return Values [LSP]

For robust code:

```c
int rc = pthread_mutex_lock(&lock);

if (rc != 0)
{
    /* handle error */
}
```

Unlike many system calls, POSIX pthread functions commonly return an error number directly rather than setting `errno`.

This distinction is important in interviews.

---

# 16. `pthread_mutex_trylock()` [LSP]

```c
if (pthread_mutex_trylock(&lock) == 0)
{
    /* acquired */

    pthread_mutex_unlock(&lock);
}
else
{
    /* currently unavailable */
}
```

`trylock()` does not normally block waiting for the mutex.

Useful when:

```text
You have alternative work
You don't want to block
You are implementing specialized scheduling/work logic
```

Do not use it as a substitute for correct synchronization.

---

# 17. `pthread_mutex_timedlock()` [LSP]

A timed mutex acquisition allows waiting up to a deadline.

Conceptually:

```c
pthread_mutex_timedlock(&lock, &deadline);
```

Possible outcomes:

```text
acquired
timeout
error
```

Useful when indefinite blocking is undesirable.

---

# 18. Mutex Types [LSP]

POSIX mutex attributes can provide different behavior.

Important concepts include:

```text
Normal/default mutex
Error-checking mutex
Recursive mutex
Robust mutex
```

### Recursive mutex

Allows the same thread to lock the mutex recursively.

But:

> Recursive mutexes should not be used to hide poor lock design.

---

# 19. Robust Mutex [LSP]

A robust mutex can help recover when its owner terminates unexpectedly while holding it.

Conceptually:

```text
Thread A
  |
  | owns robust mutex
  |
  X terminates unexpectedly

Thread B
  |
  | locks
  v
owner-dead condition
```

The new owner can detect the condition and repair shared state if possible.

---

# 20. Semaphore [OS]

A semaphore maintains a counter.

Conceptually:

```text
Semaphore count = N
```

Acquire:

```text
count--
```

Release:

```text
count++
```

If count is unavailable, a thread may block.

---

# 21. Binary Semaphore vs Mutex

A binary semaphore can have values:

```text
0 or 1
```

It can sometimes look like a mutex.

But conceptually they differ:

```text
Mutex:
    ownership semantics

Semaphore:
    counting/resource signaling semantics
```

A mutex is generally the correct primitive for protecting a critical section.

---

# 22. POSIX Semaphore Example [LSP]

```c
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

sem_t sem;

void *worker(void *arg)
{
    sem_wait(&sem);

    printf("Thread entered\n");

    sem_post(&sem);

    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    sem_init(&sem, 0, 1);

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    sem_destroy(&sem);

    return 0;
}
```

---

# 23. Counting Semaphore Example

Suppose a server has:

```text
4 database connections
```

Use:

```text
semaphore count = 4
```

Threads:

```text
T1 -> acquire -> connection
T2 -> acquire -> connection
T3 -> acquire -> connection
T4 -> acquire -> connection
T5 -> waits
```

When T1 releases:

```text
T1 -> release
T5 -> can acquire
```

This is a natural semaphore use case.

---

# 24. Condition Variable [OS]

A condition variable allows a thread to wait until some condition becomes true.

Typical pattern:

```c
pthread_mutex_lock(&mutex);

while (!condition)
{
    pthread_cond_wait(&cond, &mutex);
}

/* condition is true */

pthread_mutex_unlock(&mutex);
```

Another thread changes the condition:

```c
pthread_mutex_lock(&mutex);

condition = true;

pthread_cond_signal(&cond);

pthread_mutex_unlock(&mutex);
```

---

# 25. Why Condition Variables Need a Mutex

The condition and the waiting protocol must be coordinated.

Example:

```text
shared condition
       +
    mutex
       +
condition variable
```

The mutex protects the predicate/state.

The condition variable provides the mechanism for sleeping and waking.

---

# 26. Why `pthread_cond_wait()` Uses a `while`, Not `if`

Correct:

```c
while (!ready)
{
    pthread_cond_wait(&cond, &mutex);
}
```

Not generally:

```c
if (!ready)
{
    pthread_cond_wait(&cond, &mutex);
}
```

Reasons include:

```text
Spurious wakeups
Another thread may consume/change the condition first
Multiple waiters may wake
The condition must always be rechecked
```

This is one of the most important synchronization interview questions.

---

# 27. What `pthread_cond_wait()` Does

Conceptually:

```text
Thread owns mutex
       |
       v
pthread_cond_wait()
       |
       +-- releases mutex
       |
       +-- blocks
       |
       v
another thread changes condition
       |
       v
signal/broadcast
       |
       v
waiting thread wakes
       |
       +-- reacquires mutex
       |
       v
returns from cond_wait()
       |
       v
while condition is checked again
```

The release-and-wait operation is designed to avoid a lost-wakeup race when used correctly.

---

# 28. Producer-Consumer Problem [OS/LSP]

Classic example:

```text
Producer
    |
    v
+---------+
| Buffer  |
+---------+
    |
    v
Consumer
```

Shared state:

```text
buffer
count
head
tail
```

Need synchronization.

Typical primitives:

```text
mutex
not_empty condition
not_full condition
```

---

# 29. Producer-Consumer Code [LSP]

```c
#include <pthread.h>
#include <stdio.h>

#define SIZE 5

int buffer[SIZE];
int in = 0;
int out = 0;
int count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

void *producer(void *arg)
{
    for (int i = 1; i <= 20; ++i)
    {
        pthread_mutex_lock(&mutex);

        while (count == SIZE)
            pthread_cond_wait(&not_full, &mutex);

        buffer[in] = i;
        in = (in + 1) % SIZE;
        count++;

        pthread_cond_signal(&not_empty);

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void *consumer(void *arg)
{
    for (int i = 1; i <= 20; ++i)
    {
        pthread_mutex_lock(&mutex);

        while (count == 0)
            pthread_cond_wait(&not_empty, &mutex);

        int value = buffer[out];
        out = (out + 1) % SIZE;
        count--;

        printf("Consumed %d\n", value);

        pthread_cond_signal(&not_full);

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(void)
{
    pthread_t producer_thread;
    pthread_t consumer_thread;

    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);

    return 0;
}
```

Compile:

```bash
gcc producer_consumer.c -pthread -O2 -o producer_consumer
```

---

# 30. Producer-Consumer Working

Producer:

```text
lock
 |
 | buffer full?
 +---- yes ---> wait(not_full)
 |
 v
insert item
 |
signal(not_empty)
 |
unlock
```

Consumer:

```text
lock
 |
 | buffer empty?
 +---- yes ---> wait(not_empty)
 |
 v
remove item
 |
signal(not_full)
 |
unlock
```

---

# 31. `pthread_cond_signal()` vs `pthread_cond_broadcast()` [LSP]

### signal

Wake one waiting thread.

```c
pthread_cond_signal(&cond);
```

### broadcast

Wake all waiting threads.

```c
pthread_cond_broadcast(&cond);
```

Use `broadcast()` when multiple waiters may need to reevaluate the predicate.

After waking, each thread must recheck the condition.

---

# 32. Lost Wakeup [OS]

A lost wakeup can happen with incorrect synchronization.

Bad conceptual pattern:

```text
Thread A:
check condition -> false

Thread B:
change condition
signal

Thread A:
starts waiting
```

The signal happened before A properly entered the wait.

Correct use of:

```text
mutex
+
condition predicate
+
pthread_cond_wait()
```

coordinates checking and waiting.

---

# 33. Spinlock [OS]

A spinlock waits by repeatedly checking the lock instead of sleeping.

Conceptually:

```text
while (lock is busy)
{
    spin;
}
```

Example:

```text
CPU
 |
 +-- Thread A owns lock
 |
 +-- Thread B repeatedly checks lock
```

Thread B consumes CPU while waiting.

---

# 34. Why Use a Spinlock?

Spinlocks can be useful when:

```text
Critical section is extremely short
Expected wait is very short
Sleeping is undesirable
Kernel/low-level context requires non-sleeping synchronization
```

They are usually inappropriate for long critical sections.

---

# 35. Spinlock vs Mutex

| Property | Mutex | Spinlock |
|---|---|---|
| Waiting | Can sleep/block | Busy-waits |
| CPU while waiting | Usually not continuously consumed | Consumed |
| Good for long wait | Yes | No |
| Good for very short critical section | Yes | Sometimes |
| Can sleep while held? | Depends on context/protocol; normal mutex holders can block/sleep | No |
| Common kernel use | Yes | Yes |
| User-space POSIX primitive | Yes | Yes, with caveats |

Important:

> A spinlock should not be held across operations that may sleep.

---

# 36. POSIX Spinlock [LSP]

```c
#include <pthread.h>
#include <stdio.h>

pthread_spinlock_t lock;
int counter = 0;

void *worker(void *arg)
{
    for (int i = 0; i < 100000; ++i)
    {
        pthread_spin_lock(&lock);
        counter++;
        pthread_spin_unlock(&lock);
    }

    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("%d\n", counter);

    pthread_spin_destroy(&lock);

    return 0;
}
```

---

# 37. Reader-Writer Lock [OS]

A reader-writer lock allows:

```text
Multiple readers
OR
One writer
```

Conceptually:

```text
Reader 1 ----\
Reader 2 -----+--> shared data
Reader 3 ----/

Writer --------> exclusive
```

Readers can execute concurrently if no writer owns the lock.

---

# 38. POSIX RWLock [LSP]

```c
pthread_rwlock_t rwlock;

pthread_rwlock_init(&rwlock, NULL);
```

Reader:

```c
pthread_rwlock_rdlock(&rwlock);

/* read shared data */

pthread_rwlock_unlock(&rwlock);
```

Writer:

```c
pthread_rwlock_wrlock(&rwlock);

/* modify shared data */

pthread_rwlock_unlock(&rwlock);
```

Destroy:

```c
pthread_rwlock_destroy(&rwlock);
```

---

# 39. When RWLock Helps

RWLock can help when:

```text
Reads >> Writes
```

Example:

```text
1000 readers
10 writers
```

But RWLock is not automatically faster than a mutex.

It can add:

```text
Bookkeeping
Reader/writer coordination
Cache traffic
Writer waiting
```

Measure before choosing it.

---

# 40. Barrier [OS]

A barrier synchronizes threads at a phase boundary.

Suppose:

```text
T1 ---- phase 1 ----\
T2 ---- phase 1 -----+--> barrier
T3 ---- phase 1 ----/
                         |
                         v
                    phase 2
```

All participating threads must reach the barrier before proceeding.

---

# 41. POSIX Barrier [LSP]

```c
pthread_barrier_t barrier;

pthread_barrier_init(&barrier, NULL, 3);
```

Worker:

```c
/* phase 1 */

pthread_barrier_wait(&barrier);

/* phase 2 */
```

Destroy:

```c
pthread_barrier_destroy(&barrier);
```

---

# 42. Atomic Operations [OS/LSP]

C11 provides:

```c
#include <stdatomic.h>
```

Example:

```c
atomic_int counter = 0;

atomic_fetch_add(&counter, 1);
```

Atomics are useful for simple shared-state operations.

---

# 43. Atomic vs Mutex

Atomic is not automatically "better".

Use atomic when the operation/invariant can be correctly represented using atomic operations.

Example:

```text
Simple counter
reference count
flags
state transitions
```

Mutex is usually more appropriate for complex invariants:

```text
update object A
update object B
maintain relationship between A and B
```

---

# 44. C11 Atomic Example [LSP]

```c
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

atomic_int counter = 0;

void *worker(void *arg)
{
    for (int i = 0; i < 100000; ++i)
        atomic_fetch_add(&counter, 1);

    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("counter = %d\n", atomic_load(&counter));

    return 0;
}
```

Compile:

```bash
gcc atomic.c -pthread -std=c11 -O2 -o atomic
```

---

# 45. Atomic Memory Orders [LSP]

C/C++ atomics support different memory-ordering strengths.

Important concepts:

```text
relaxed
acquire
release
acq_rel
seq_cst
```

For interview preparation:

```text
relaxed
    -> atomicity without strong synchronization ordering

release
    -> publish prior operations

acquire
    -> observe effects published by release

seq_cst
    -> strongest/simple global ordering model
```

Do not choose memory order only for performance; first establish correctness.

---

# 46. Acquire-Release Example

Producer:

```c
data = 42;
atomic_store_explicit(&ready, 1, memory_order_release);
```

Consumer:

```c
if (atomic_load_explicit(&ready, memory_order_acquire))
{
    printf("%d\n", data);
}
```

The release/acquire pair establishes the required synchronization relationship when used correctly.

---

# 47. C++ `std::mutex`

C++ provides:

```cpp
#include <mutex>
```

Example:

```cpp
std::mutex m;
int counter = 0;

void worker()
{
    for (int i = 0; i < 100000; ++i)
    {
        std::lock_guard<std::mutex> lock(m);
        ++counter;
    }
}
```

RAII automatically unlocks the mutex when `lock` goes out of scope.

---

# 48. `std::unique_lock`

Useful when you need more control:

```cpp
std::unique_lock<std::mutex> lock(m);

lock.unlock();

/* do work */

lock.lock();
```

It is commonly used with:

```cpp
std::condition_variable
```

---

# 49. C++ Condition Variable

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;
```

Wait:

```cpp
std::unique_lock<std::mutex> lock(m);

cv.wait(lock, [] {
    return ready;
});
```

Notify:

```cpp
{
    std::lock_guard<std::mutex> lock(m);
    ready = true;
}

cv.notify_one();
```

The predicate form is preferred because it handles spurious wakeups correctly.

---

# 50. Deadlock [OS]

Deadlock occurs when tasks wait forever for resources held by one another.

Example:

```text
Thread A:
holds Lock 1
waits for Lock 2

Thread B:
holds Lock 2
waits for Lock 1
```

Diagram:

```text
A ---> Lock 2
^       |
|       v
Lock 1 <--- B
```

Deadlocks are covered in detail in Chapter 6.

---

# 51. Lock Ordering [OS/LSP]

One way to prevent many lock-order deadlocks:

```text
Always acquire:

Lock A -> Lock B
```

Never:

```text
Thread 1: A -> B
Thread 2: B -> A
```

Instead:

```text
Thread 1: A -> B
Thread 2: A -> B
```

Global lock ordering is a powerful design rule.

---

# 52. `pthread_mutex_lock()` and Kernel Interaction [KERNEL]

A common misconception is:

> Every mutex lock immediately enters the kernel.

That is not generally true.

Modern Linux synchronization often uses a **fast user-space path** when the lock is uncontended.

Conceptually:

```text
pthread_mutex_lock()
        |
        v
   Try fast path
        |
   +----+----+
   |         |
success    contention
   |         |
   v         v
continue   futex/kernel
```

This is a key Linux synchronization concept.

---

# 53. Futex [KERNEL/LSP]

**futex = fast userspace mutex**

The Linux futex mechanism supports efficient blocking/waking for user-space synchronization.

Core idea:

```text
Uncontended:
user space handles it

Contended:
kernel helps block/wake waiters
```

This avoids a system call for every uncontended mutex operation.

---

# 54. Futex Conceptual Flow

```text
Thread A
   |
   | atomic operation
   v
lock acquired
   |
   v
continue
```

Contended case:

```text
Thread A owns lock

Thread B
   |
   v
cannot acquire
   |
   v
futex wait
   |
   v
kernel blocks B
   |
   |
A unlocks
   |
   v
futex wake
   |
   v
B becomes runnable
```

The exact pthread mutex implementation is more complex and can vary.

---

# 55. Linux Kernel `mutex` [KERNEL]

Kernel code has its own mutex primitive:

```c
struct mutex
```

Typical usage:

```c
mutex_lock(&my_mutex);

/* critical section */

mutex_unlock(&my_mutex);
```

A kernel mutex is a sleeping lock.

---

# 56. Kernel Spinlock [KERNEL]

Linux kernel code uses:

```c
spinlock_t
```

Typical pattern:

```c
spin_lock(&lock);

/* critical section */

spin_unlock(&lock);
```

Important rule:

> Code holding a spinlock must not perform operations that may sleep.

---

# 57. Why Kernel Spinlocks Cannot Sleep [KERNEL]

Suppose:

```text
CPU 0:
holds spinlock
```

If the owner sleeps:

```text
CPU 0:
holds lock
   |
   v
sleep
```

Another CPU:

```text
CPU 1:
tries lock
   |
   v
spins
```

The lock owner cannot run to release the lock.

This can cause severe problems/deadlock.

Therefore:

```text
spinlock held
    |
    +-- no sleeping
```

---

# 58. Process Context vs Interrupt Context [KERNEL]

This distinction is critical.

## Process context

Kernel is executing on behalf of a process/thread.

Some operations can sleep if the context permits it.

## Interrupt context

Kernel is handling an interrupt.

It cannot generally sleep like normal process context.

Therefore interrupt-related synchronization often requires non-sleeping primitives such as spinlocks, depending on the exact context.

---

# 59. Spinlock with Interrupts [KERNEL]

Kernel code may use variants such as:

```c
spin_lock_irqsave()
spin_unlock_irqrestore()
```

when protecting data shared with interrupt handlers.

Conceptually:

```text
save interrupt state
        |
disable local interrupts
        |
acquire spinlock
        |
critical section
        |
release spinlock
        |
restore interrupt state
```

The exact choice depends on the context and locking requirements.

---

# 60. Kernel Reader-Writer Locks [KERNEL]

Linux provides reader-writer synchronization primitives, including:

```text
rwlock_t
rw_semaphore
```

They have different semantics and are used in different contexts.

The important distinction:

```text
rwlock_t
    -> spin-based reader/writer locking

rw_semaphore
    -> sleeping reader/writer semaphore
```

Do not treat them as interchangeable.

---

# 61. Wait Queues [KERNEL]

Linux wait queues allow tasks to sleep until an event/condition occurs.

Conceptually:

```text
Condition false
     |
     v
wait queue
     |
     v
task sleeps

event occurs
     |
     v
wake_up()
     |
     v
task becomes runnable
```

Typical APIs include:

```c
wait_event()
wait_event_interruptible()
wake_up()
```

Wait queues are fundamental to kernel blocking mechanisms.

---

# 62. Kernel Condition-Wait Pattern [KERNEL]

Conceptually:

```text
while (!condition)
{
    sleep/wait
}

continue
```

This is the kernel equivalent of the important user-space rule:

```text
while (!predicate)
    pthread_cond_wait(...)
```

The predicate must be checked again after waking.

---

# 63. Atomic Operations in Linux Kernel [KERNEL]

Linux provides atomic types/APIs for architecture-supported atomic operations.

Conceptually:

```c
atomic_t counter;
atomic_inc(&counter);
atomic_dec(&counter);
atomic_read(&counter);
```

The exact APIs and semantics depend on the kernel API version.

Atomics are useful for simple state/counter operations, but they are not a replacement for every lock.

---

# 64. Reference Counting [KERNEL]

A common synchronization use case is reference counting.

Conceptually:

```text
object
  |
  +-- refcount = 3
```

Users acquire/release references:

```text
get -> 4
put -> 3
put -> 2
put -> 1
put -> 0
     |
     v
free object
```

Linux provides dedicated reference-counting mechanisms such as `refcount_t`.

Reference counting prevents premature object destruction when multiple users hold references.

---

# 65. Memory Barriers [KERNEL]

A memory barrier controls ordering/visibility of memory operations across CPUs and compiler/CPU reordering constraints.

Conceptually:

```text
CPU 0                         CPU 1

write data                    read flag
    |                             |
    v                             v
write flag                    read data
```

Without appropriate ordering, concurrent code can observe states differently than a naive source-code reading suggests.

Linux provides memory-ordering primitives/macros appropriate to different requirements.

---

# 66. Why Volatile Is Not a Thread Synchronization Mechanism

This is a common interview trap.

Incorrect:

```c
volatile int ready;
```

and assuming:

```text
volatile = thread safe
```

It is not.

`volatile` does not provide the required atomicity, mutual exclusion, or inter-thread memory-ordering guarantees.

Use:

```text
mutex
atomic
condition variable
appropriate synchronization
```

depending on the problem.

---

# 67. Lock Contention [OS/KERNEL]

Lock contention occurs when multiple threads frequently compete for the same lock.

```text
T1 ----\
T2 -----\
T3 ------> Lock
T4 -----/
```

High contention can cause:

```text
Waiting
Context switches
Cache-line bouncing
Lower scalability
```

---

# 68. Critical Section Length

Bad:

```c
lock();

do_expensive_computation();
read_file();
network_operation();
update_shared_data();

unlock();
```

Better:

```c
do_expensive_computation();

lock();

update_shared_data();

unlock();
```

General rule:

> Keep critical sections as small as correctness allows.

But do not split locking in a way that breaks the invariant.

---

# 69. Lock Granularity

## Coarse-grained locking

One large lock:

```text
          Lock
           |
   +-------+-------+
   |       |       |
 Data A  Data B  Data C
```

Pros:

```text
Simple
Easy to reason about
```

Cons:

```text
High contention
Poor scalability
```

## Fine-grained locking

Separate locks:

```text
Lock A -> Data A
Lock B -> Data B
Lock C -> Data C
```

Pros:

```text
More concurrency
```

Cons:

```text
Complex
Deadlock risk
More synchronization overhead
```

---

# 70. Lock-Free vs Wait-Free [OS/LSP]

## Lock-free

The system as a whole makes progress even if some individual operation is delayed.

## Wait-free

Every operation completes within a bounded number of steps.

These are stronger guarantees than simply saying:

```text
"uses atomics"
```

A program using atomics is not automatically lock-free or wait-free.

---

# 71. ABA Problem [LSP/KERNEL]

A common lock-free algorithm problem:

```text
Initial:
A

Thread 1 reads A

Thread 2:
A -> B
B -> A

Thread 1 sees A again
```

Thread 1 may incorrectly assume nothing changed.

This is the **ABA problem**.

Solutions can include:

```text
Tagged/versioned pointers
Hazard pointers
Epoch-based reclamation
Other safe memory-reclamation techniques
```

---

# 72. Synchronization and Cache Coherence

Consider:

```text
CPU 0 -> counter
CPU 1 -> counter
```

A shared cache line can bounce between CPUs.

Synchronization may therefore involve costs beyond the lock instruction itself:

```text
atomic operation
    +
cache coherence
    +
memory ordering
    +
scheduler effects
```

This is why heavily contended shared state can scale poorly.

---

# 73. Priority Inversion [OS/KERNEL]

Example:

```text
High-priority H
Medium-priority M
Low-priority L
```

L owns a mutex needed by H:

```text
L -> owns mutex
H -> blocked on mutex
M -> runnable
```

If M keeps running:

```text
H waits for L
L waits for CPU
M consumes CPU
```

H is indirectly delayed by M.

---

# 74. Priority Inheritance

With priority inheritance:

```text
L owns mutex
H waits for mutex
       |
       v
L temporarily inherits H's priority
       |
       v
L runs
       |
       v
L releases mutex
       |
       v
H runs
```

This is important in real-time systems.

---

# 75. Priority Ceiling [OS]

Another real-time synchronization technique is priority ceiling.

Conceptually:

```text
Each protected resource
    |
    v
has a defined priority ceiling
```

This can help bound priority inversion and prevent certain deadlock scenarios.

---

# 76. Synchronization in User Space vs Kernel Space

```text
User space
   |
   +-- pthread_mutex
   +-- pthread_cond
   +-- semaphore
   +-- C/C++ atomics
   |
   v
Linux synchronization implementation
   |
   +-- fast atomic operations
   +-- futex when blocking is required
   |
   v
Kernel
   |
   +-- scheduler
   +-- wait queues
   +-- mutex
   +-- spinlock
   +-- atomic operations
```

This layered view is important for Linux interviews.

---

# 77. Why Mutexes Can Be Fast

An uncontended mutex often follows a fast path:

```text
Thread
  |
  v
atomic attempt
  |
  v
success
  |
  v
continue
```

No expensive blocking operation is required.

Only when contention requires waiting does the implementation need more involved kernel-assisted behavior.

---

# 78. Why Sleeping Locks Are Useful

If a thread expects to wait for a significant time:

```text
Thread
  |
  v
cannot acquire lock
  |
  v
sleep
  |
  v
CPU executes another task
```

This avoids wasting CPU in a spin loop.

---

# 79. Why Spinlocks Can Be Faster

For extremely short waits:

```text
lock held for 50 ns
```

Putting a thread to sleep and waking it may cost more than briefly spinning.

Therefore:

```text
very short wait
    -> spinning may be useful

long/unpredictable wait
    -> sleeping may be better
```

The correct choice depends on context and workload.

---

# 80. Never Assume "Spinlock Is Faster"

This is a senior interview trap.

A spinlock can be worse because:

```text
CPU is consumed while waiting
```

Example:

```text
Critical section = 10 ms
```

Spinning for 10 ms is usually wasteful.

A mutex can let the waiter sleep and allow another task to use the CPU.

---

# 81. Synchronization and Scheduler Interaction

Example:

```text
Thread A
  |
  | lock
  v
owns mutex

Thread B
  |
  | lock
  v
cannot acquire
  |
  v
blocks
  |
  v
scheduler
  |
  v
another task runs
```

When A unlocks:

```text
A -> unlock
      |
      v
wake waiter
      |
      v
B becomes runnable
      |
      v
scheduler considers B
```

This connects synchronization directly to Chapter 4.

---

# 82. Debugging Race Conditions [LSP]

Useful tools:

```bash
gdb
strace
perf
```

For race/memory bugs, tools such as:

```text
ThreadSanitizer
Helgrind
DRD
```

can be useful when supported by the build/toolchain.

Example with GCC/Clang ThreadSanitizer:

```bash
gcc -fsanitize=thread -g race.c -pthread -o race
```

Then:

```bash
./race
```

---

# 83. Deliberate Race Condition Example [LSP]

```c
#include <pthread.h>
#include <stdio.h>

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

This has a data race.

Do not rely on observing a wrong number every time; undefined behavior means the result is not something the program can safely depend on.

---

# 84. Fix the Race with Mutex

```c
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
    for (int i = 0; i < 100000; ++i)
    {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}
```

Now access is serialized.

---

# 85. Fix a Counter with Atomics

```c
#include <stdatomic.h>

atomic_int counter = 0;

void *worker(void *arg)
{
    for (int i = 0; i < 100000; ++i)
        atomic_fetch_add(&counter, 1);

    return NULL;
}
```

This is appropriate because the required operation is simply an atomic increment.

---

# 86. Mutex vs Atomic Decision

Ask:

```text
Is this just one atomic state/counter operation?
       |
       +-- yes --> atomic may be appropriate
       |
       +-- no --> mutex/other synchronization may be needed
```

Example:

```text
counter++               -> atomic can work

update A and B together -> mutex may be required
```

---

# 87. Semaphore vs Condition Variable

### Semaphore

Represents:

```text
count/resource availability
```

Example:

```text
4 buffers available
```

### Condition variable

Represents:

```text
wait until predicate becomes true
```

Example:

```text
queue is not empty
```

Condition variables are normally used together with a mutex and a predicate.

---

# 88. Mutex vs Semaphore

| Feature | Mutex | Semaphore |
|---|---|---|
| Main purpose | Mutual exclusion | Counting/signaling |
| Ownership | Yes | No mutex-style ownership |
| Count | Binary ownership state | 0..N |
| Typical use | Critical section | Resource pool |
| Unlock/release semantics | Owner unlocks | `sem_post()` increments |

---

# 89. Mutex vs Condition Variable

They solve different problems:

```text
Mutex
    -> protects shared state

Condition variable
    -> allows waiting for a state/predicate
```

Typical combination:

```text
mutex + condition variable
```

not:

```text
condition variable alone
```

---

# 90. Mutex vs Spinlock

Use a mutex when:

```text
Waiting may be non-trivial
Thread can sleep
```

Use a spinlock when:

```text
Critical section is extremely short
Sleeping is forbidden/undesirable
Context permits spinning
```

In kernel code, context rules are especially important.

---

# 91. Common Synchronization Mistakes

## Mistake 1

Assuming:

```text
volatile = thread safe
```

Wrong.

## Mistake 2

Using a mutex only around writes but not reads that participate in the same invariant.

## Mistake 3

Using `if` instead of `while` around condition-variable waits.

## Mistake 4

Holding a lock while doing slow I/O.

## Mistake 5

Acquiring locks in inconsistent order.

## Mistake 6

Using spinlocks for long critical sections.

## Mistake 7

Assuming atomics automatically solve complex invariants.

## Mistake 8

Creating too many locks without a clear ownership/ordering design.

---

# 92. Senior Interview: What Happens When Mutex Is Contended?

Strong answer:

> The implementation first attempts a fast user-space acquisition. If the mutex is uncontended, the operation can complete without blocking in the kernel. Under contention, the implementation can use the Linux futex mechanism to coordinate waiting and waking. The waiting thread may block, allowing the scheduler to run another task. When the mutex becomes available, a waiter can be woken and compete to acquire it.

---

# 93. Senior Interview: Why Is `pthread_cond_wait()` Called with a Mutex?

Answer:

> The mutex protects the predicate being waited on and coordinates checking that predicate with entering the wait. `pthread_cond_wait()` atomically releases the mutex as part of entering the wait and reacquires it before returning. The caller then rechecks the predicate in a loop.

---

# 94. Senior Interview: Why `while`, Not `if`?

Answer:

> Because a wakeup does not prove the predicate is true for the current thread. There can be spurious wakeups, multiple waiters, or another thread may consume/change the condition before this thread reacquires the mutex. Therefore the predicate must always be checked again.

---

# 95. Senior Interview: Why Not Use Spinlock Everywhere?

Answer:

> A spinlock consumes CPU while waiting. It is useful only when the expected wait is very short or when sleeping is not permitted. For potentially longer waits, a sleeping mutex is usually more efficient because the waiting thread can block and the CPU can execute useful work.

---

# 96. Senior Interview: What Is a Futex?

Answer:

> A futex is a Linux kernel mechanism used to implement efficient blocking/waking synchronization. The uncontended path can remain in user space using atomic operations. The kernel is involved when a thread actually needs to wait or wake another waiter.

---

# 97. Senior Interview: Is `counter++` Atomic?

Generally:

```c
counter++;
```

should not be assumed atomic.

Conceptually:

```text
load
modify
store
```

For shared concurrent access, use:

```text
mutex
or
appropriate atomic operation
```

depending on the required semantics.

---

# 98. Senior Interview: Does Atomic Mean Lock-Free?

No.

An atomic API provides atomicity according to its defined semantics.

It does not automatically mean:

```text
lock-free
wait-free
scalable
contention-free
```

Always distinguish these concepts.

---

# 99. Senior Interview: What Is Priority Inversion?

Answer:

> A high-priority task waits for a lock held by a low-priority task, while a medium-priority task prevents the low-priority task from running. Priority inheritance can temporarily raise the low-priority lock holder's priority so it can finish and release the resource.

---

# 100. Senior Interview: How Do You Avoid Deadlocks?

Important techniques:

```text
Consistent lock ordering
Avoid circular dependencies
Keep lock scope small
Avoid unnecessary nested locks
Use trylock carefully where appropriate
Establish ownership rules
Use timeouts where appropriate
Use deadlock-detection/debugging tools
```

Chapter 6 covers deadlocks in detail.

---

# 101. Senior Interview: How Would You Debug a Production Lock Contention Problem?

A practical approach:

```text
1. Identify the process/thread
2. Observe CPU and context switches
3. Check thread states
4. Inspect application lock metrics
5. Use perf where appropriate
6. Capture stack traces
7. Identify lock ownership/waiters
8. Measure critical-section duration
9. Look for excessive contention
10. Check CPU affinity/NUMA effects
11. Reduce lock scope or redesign shared state
```

Useful tools:

```bash
top -H -p <pid>
pidstat -w -p <pid> 1
perf stat ...
perf record ...
gdb
```

---

# 102. Practical Exercise 1 — Race Condition

Create:

```text
race.c
```

Implement:

```text
2 threads
shared counter
100000 increments each
```

Run repeatedly.

Then fix it using:

```text
mutex
```

Then compare against:

```text
atomic
```

Questions:

```text
Which is faster?
Why?
What happens as thread count increases?
```

---

# 103. Practical Exercise 2 — Producer Consumer

Implement:

```text
bounded queue
1 producer
4 consumers
```

Use:

```text
mutex
not_empty
not_full
```

Observe:

```text
buffer empty
buffer full
threads waiting
threads waking
```

---

# 104. Practical Exercise 3 — Reader Writer

Implement:

```text
10 readers
2 writers
```

Compare:

```text
mutex
vs
rwlock
```

Measure throughput.

Question:

> Does RWLock always perform better?

Answer:

> No. It depends on read/write ratio, contention, critical-section size, scheduling, and implementation overhead.

---

# 105. Practical Exercise 4 — Spinlock vs Mutex

Implement the same short critical section using:

```text
pthread_mutex
pthread_spinlock
```

Measure:

```bash
time ./program
```

Then increase critical-section duration.

Observe how the relative behavior changes.

---

# 106. Practical Exercise 5 — Condition Variable

Implement:

```text
worker threads
+
task queue
+
condition variable
```

Workers:

```text
while (running)
{
    lock

    while (queue_empty)
        wait

    get task

    unlock

    process task
}
```

This is the foundation of a thread pool.

---

# 107. Thread Pool Mental Model

```text
                Task producers
                      |
                      v
               +-------------+
               | Task Queue  |
               +-------------+
                 ^    ^    ^
                 |    |    |
                W1   W2   W3
                 |    |    |
                 +----+----+
                      |
                   Workers
```

Synchronization:

```text
mutex
+
condition variable
+
queue
```

This pattern is extremely common in real systems.

---

# 108. Synchronization Decision Tree

```text
Need to protect shared data?
          |
         yes
          |
          v
Is operation simple and atomic?
          |
     +----+----+
    yes        no
     |          |
     v          v
 atomic       mutex
                |
                v
Need to wait for a condition?
                |
               yes
                |
                v
       mutex + condition variable

Need counting/resource control?
                |
               yes
                |
                v
             semaphore

Need multiple readers?
                |
               yes
                |
                v
             RWLock

Need extremely short non-sleeping lock?
                |
               yes
                |
                v
            spinlock
```

This is a conceptual guide, not a substitute for workload/context analysis.

---

# 109. Three-Layer Summary

## [OS]

Know:

```text
Race condition
Critical section
Mutual exclusion
Atomicity
Visibility
Memory ordering
Mutex
Semaphore
Condition variable
Spinlock
RWLock
Barrier
Starvation
Priority inversion
Priority inheritance
Lock ordering
Deadlock relationship
Lock contention
Lock granularity
Lock-free
Wait-free
```

## [LSP]

Know:

```text
pthread_mutex_*
pthread_cond_*
sem_*
pthread_rwlock_*
pthread_spin_*
pthread_barrier_*
C11 atomics
C++ std::mutex
C++ condition_variable
std::atomic
std::lock_guard
std::unique_lock

futex concept
ThreadSanitizer
Helgrind
gdb
perf
```

## [KERNEL]

Know:

```text
struct mutex
spinlock_t
rwlock_t
rw_semaphore
atomic_t
refcount_t
wait queues
futex
scheduler interaction
process context
interrupt context
spinlock + interrupt rules
memory barriers
priority inheritance
lock contention
per-CPU/cache effects
```

---

# 110. Important Commands Cheat Sheet

```bash
# Compile pthread program
gcc program.c -pthread -o program

# Compile with debug symbols
gcc program.c -pthread -g -O0 -o program

# ThreadSanitizer
gcc program.c -pthread -fsanitize=thread -g -o program

# Show threads
ps -T -p <pid>

# Interactive thread view
top -H -p <pid>

# Context switches
pidstat -w -p <pid> 1

# Performance statistics
perf stat ./program

# Record performance profile
perf record ./program

# Debug with gdb
gdb ./program

# Attach gdb
gdb -p <pid>
```

---

# 111. Final Mental Model

Synchronization is not simply:

```text
"Put a mutex around the variable."
```

The correct mental model is:

```text
                Shared State
                     |
                     v
             What invariant?
                     |
                     v
              Who accesses it?
                     |
                     v
             What concurrency?
                     |
          +----------+----------+
          |                     |
          v                     v
       Atomic?               Complex?
          |                     |
          v                     v
       Atomic              Mutex/RWLock
                                |
                                v
                       Need to wait?
                                |
                                v
                       Condition variable
                                |
                                v
                       Blocking/wakeup
                                |
                                v
                             Futex
                                |
                                v
                            Scheduler
                                |
                                v
                              CPU
```

Kernel-level view:

```text
User Thread
    |
    v
pthread synchronization
    |
    +---- fast uncontended path
    |
    +---- contention
             |
             v
           futex
             |
             v
          Kernel
             |
      +------+------+
      |             |
      v             v
 wait queue      scheduler
      |             |
      +------+------+
             |
             v
          wakeup
             |
             v
        runnable task
```

---

# Chapter 5 — Key Takeaways

1. Synchronization protects shared state and establishes correct concurrency semantics.
2. A race condition occurs when correctness depends on unsafe timing/interleaving.
3. A critical section accesses shared state that must be protected.
4. Mutexes provide mutual exclusion.
5. Semaphores are useful for counting resources/signaling.
6. Condition variables allow threads to wait for predicates.
7. Always use a predicate and normally a `while` loop with a condition variable.
8. `pthread_cond_wait()` releases the mutex while waiting and reacquires it before returning.
9. Spinlocks busy-wait and therefore consume CPU while waiting.
10. Spinlocks are useful only in appropriate short/non-sleeping contexts.
11. RWLocks permit concurrent readers but serialize writers.
12. Barriers synchronize execution phases.
13. Atomics are excellent for simple atomic state transitions/counters.
14. Atomics do not automatically replace mutexes for complex invariants.
15. `volatile` is not a thread-synchronization mechanism.
16. Lock contention can hurt scalability.
17. Keep critical sections as short as correctness permits.
18. Coarse locks simplify design but may reduce scalability.
19. Fine-grained locks improve concurrency but increase complexity.
20. Consistent lock ordering helps prevent deadlocks.
21. Priority inversion can be mitigated by priority inheritance.
22. User-space mutexes commonly use a fast path and kernel-assisted waiting when contended.
23. Linux futexes are fundamental to efficient user-space blocking synchronization.
24. Kernel mutexes can sleep.
25. Kernel spinlocks cannot be held across sleeping operations.
26. Process context and interrupt context have different synchronization rules.
27. Linux wait queues implement many blocking/wakeup patterns.
28. Memory barriers matter when reasoning about ordering on multicore systems.
29. Lock-free and wait-free are different concepts.
30. ABA is an important lock-free algorithm problem.
31. Synchronization interacts directly with the scheduler.
32. For senior Linux interviews, know both the API and the kernel mechanism underneath.

---

# Chapter 6 Preview — Deadlocks

The next chapter will connect directly to synchronization:

```text
Chapter 5
Synchronization
      |
      v
Multiple locks/resources
      |
      v
Incorrect acquisition order
      |
      v
Deadlock
```

Chapter 6 will cover:

```text
Deadlock conditions
Resource allocation graph
Wait-for graph
Lock-order deadlocks
Self-deadlock
Livelock
Starvation
Deadlock prevention
Deadlock avoidance
Deadlock detection
Deadlock recovery
pthread deadlock examples
trylock()
Lock ordering
Linux kernel deadlocks
Lockdep
Circular dependencies
Senior interview problems
```
