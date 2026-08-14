# Chapter 6 — Deadlocks

> **Three-layer approach**
>
> This chapter covers deadlocks from:
> 1. **[OS] Operating System concepts**
> 2. **[LSP] Linux System Programming + C/C++ code**
> 3. **[KERNEL] Linux Kernel Internals**
>
> Goal: understand how deadlocks occur, how to prevent/detect them, how they appear in pthread programs, and how Linux kernel developers debug locking problems.

---

# 1. What Is a Deadlock? [OS]

A **deadlock** occurs when a set of threads/processes are permanently waiting for resources held by one another.

Simple example:

```text
Thread A                    Thread B

holds Lock 1                holds Lock 2
     |                           |
     v                           v
waits for Lock 2             waits for Lock 1
```

Neither can proceed.

```text
A -> waits for B
B -> waits for A
```

The system is stuck unless some external action breaks the cycle.

---

# 2. Deadlock vs Race Condition

These are different problems.

## Race condition

Result depends on timing/interleaving.

```text
T1 and T2
   |
   v
unsafe shared access
   |
   v
incorrect/unpredictable result
```

## Deadlock

Threads wait forever.

```text
T1 -> waits for T2
T2 -> waits for T1
```

A program can have:

```text
race conditions without deadlocks
deadlocks without data races
both
neither
```

---

# 3. Deadlock vs Starvation vs Livelock

## Deadlock

Nobody in the cycle can make progress.

```text
A waits for B
B waits for A
```

## Starvation

A thread keeps getting denied the resource/CPU it needs.

```text
T1 -> waiting
T2/T3/T4 -> repeatedly get resource
```

The system may still be making progress.

## Livelock

Threads are active but make no useful progress.

Example:

```text
T1 -> detects conflict -> backs off
T2 -> detects conflict -> backs off

T1 -> retries
T2 -> retries

T1 -> backs off
T2 -> backs off
```

They are running, but the useful work never completes.

---

# 4. The Four Necessary Conditions for Deadlock [OS]

A deadlock can exist only if all four Coffman conditions hold.

```text
1. Mutual exclusion
2. Hold and wait
3. No preemption
4. Circular wait
```

Remember:

```text
M H N C
```

or simply:

```text
Mutual exclusion
Hold and wait
No preemption
Circular wait
```

---

# 5. Mutual Exclusion

At least one resource must be non-shareable.

Example:

```text
Mutex
```

Only one thread can own it at a time.

```text
Lock
 |
 +--> Thread A
```

Thread B cannot simultaneously own it.

---

# 6. Hold and Wait

A thread holds one resource while waiting for another.

Example:

```text
Thread A:

holds Lock 1
      |
      v
waits for Lock 2
```

If every thread acquires all required resources before doing work, this condition can sometimes be eliminated.

---

# 7. No Preemption

The resource cannot simply be forcibly taken from its owner.

Example:

```text
Thread A owns mutex
```

Thread B cannot normally do:

```text
"Take the mutex away from A"
```

The owner must release it.

---

# 8. Circular Wait

There is a cycle of dependencies.

Two-thread example:

```text
T1 -> waits for L2
L2 -> owned by T2
T2 -> waits for L1
L1 -> owned by T1
```

Graph:

```text
T1 ---> T2
 ^       |
 |       v
 +-------+
```

This is the condition that makes the dependency cycle explicit.

---

# 9. Two-Lock Deadlock Example [LSP]

```c
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

void *thread1(void *arg)
{
    pthread_mutex_lock(&lock1);

    sleep(1);

    pthread_mutex_lock(&lock2);

    printf("Thread 1 acquired both locks\n");

    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);

    return NULL;
}

void *thread2(void *arg)
{
    pthread_mutex_lock(&lock2);

    sleep(1);

    pthread_mutex_lock(&lock1);

    printf("Thread 2 acquired both locks\n");

    pthread_mutex_unlock(&lock1);
    pthread_mutex_unlock(&lock2);

    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}
```

Compile:

```bash
gcc deadlock.c -pthread -o deadlock
```

Likely behavior:

```text
program hangs
```

The exact timing is nondeterministic, but the code has a lock-order deadlock possibility.

---

# 10. Why the Example Deadlocks

Possible execution:

```text
T1:
lock(lock1)

T2:
lock(lock2)

T1:
wait for lock2

T2:
wait for lock1
```

Now:

```text
T1 -> lock1 -> T2
T2 -> lock2 -> T1
```

No thread can continue.

---

# 11. The Most Important Deadlock Prevention Rule

Use a **global lock ordering**.

For example:

```text
Lock 1 < Lock 2
```

Every thread must acquire:

```text
Lock 1
   ↓
Lock 2
```

Never:

```text
T1: Lock1 -> Lock2
T2: Lock2 -> Lock1
```

Instead:

```text
T1: Lock1 -> Lock2
T2: Lock1 -> Lock2
```

This removes the possibility of a circular lock-order dependency for those locks.

---

# 12. Fixed Version [LSP]

```c
void *thread1(void *arg)
{
    pthread_mutex_lock(&lock1);
    pthread_mutex_lock(&lock2);

    printf("Thread 1 acquired both locks\n");

    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);

    return NULL;
}

void *thread2(void *arg)
{
    pthread_mutex_lock(&lock1);
    pthread_mutex_lock(&lock2);

    printf("Thread 2 acquired both locks\n");

    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);

    return NULL;
}
```

Both threads use:

```text
lock1 -> lock2
```

There is no circular ordering between these two locks.

---

# 13. Lock Hierarchy [OS/KERNEL]

For larger systems, define a lock hierarchy.

Example:

```text
Global lock
    |
    v
Subsystem lock
    |
    v
Object lock
```

Rule:

```text
Acquire top-level lock before lower-level lock.
```

Avoid:

```text
Object lock -> Global lock
```

if the normal hierarchy is:

```text
Global -> Object
```

This is especially important in large kernel codebases.

---

# 14. Resource Allocation Graph [OS]

A resource allocation graph represents:

```text
Processes/threads
Resources
Allocation
Waiting
```

Example:

```text
T1 ---> L2
 ^       |
 |       v
 L1 <--- T2
```

Interpretation:

```text
T1 waits for L2
L2 is held by T2

T2 waits for L1
L1 is held by T1
```

Cycle:

```text
T1 -> L2 -> T2 -> L1 -> T1
```

A cycle is an important deadlock indicator.

---

# 15. Wait-For Graph

A simplified graph removes resource nodes.

```text
T1 -> T2
T2 -> T1
```

Meaning:

```text
T1 waits for a resource held by T2
T2 waits for a resource held by T1
```

Cycle:

```text
T1 -> T2 -> T1
```

indicates deadlock under the corresponding locking model.

---

# 16. Deadlock Prevention [OS]

Prevention means designing the system so at least one necessary condition cannot hold.

Strategies:

```text
Eliminate hold-and-wait
Allow preemption where possible
Impose resource ordering
Reduce mutual exclusion where possible
```

For mutex-based programs, **consistent lock ordering** is one of the most practical techniques.

---

# 17. Prevent Hold and Wait

Instead of:

```text
lock A
do work
lock B
```

acquire everything together:

```text
lock A
lock B
do work
```

But this only works if the program can safely acquire all required resources before proceeding.

It may increase lock holding time and reduce concurrency.

---

# 18. `pthread_mutex_trylock()` as a Deadlock-Avoidance Technique [LSP]

`trylock()` does not wait indefinitely.

Example:

```c
if (pthread_mutex_trylock(&lock2) != 0)
{
    pthread_mutex_unlock(&lock1);
    return;
}
```

Conceptually:

```text
acquire A
   |
try B
   |
failed
   |
release A
   |
retry later
```

This can break a potential hold-and-wait pattern.

However:

> `trylock()` by itself does not automatically make a design deadlock-free.

Retry logic, fairness, and livelock must also be considered.

---

# 19. `trylock()` Example [LSP]

```c
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t b = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
    for (;;)
    {
        pthread_mutex_lock(&a);

        if (pthread_mutex_trylock(&b) == 0)
        {
            printf("Acquired both locks\n");

            pthread_mutex_unlock(&b);
            pthread_mutex_unlock(&a);
            break;
        }

        pthread_mutex_unlock(&a);

        usleep(1000);
    }

    return NULL;
}
```

This uses a retry strategy rather than blocking indefinitely while holding `a`.

---

# 20. But `trylock()` Can Cause Livelock

Imagine two threads:

```text
T1:
lock A
try B -> fail
unlock A

T2:
lock B
try A -> fail
unlock B
```

Both retry at exactly the same time:

```text
T1 -> fail
T2 -> fail
T1 -> fail
T2 -> fail
...
```

They may make no useful progress.

Solutions can include:

```text
Backoff
Randomized delay
Strict lock ordering
Different work scheduling
```

Strict lock ordering is generally easier to reason about.

---

# 21. Backoff

A retrying algorithm can wait before retrying.

```text
attempt
  |
failure
  |
backoff
  |
retry
```

Backoff can be:

```text
Fixed delay
Exponential delay
Randomized delay
```

Randomization can reduce repeated synchronization collisions.

---

# 22. Deadlock Avoidance vs Prevention

These terms are related but different.

## Prevention

Design the system so deadlock cannot occur.

Example:

```text
Global lock ordering
```

## Avoidance

At runtime, decide whether granting a resource request could lead to an unsafe state.

Classic example:

```text
Banker's algorithm
```

---

# 23. Banker's Algorithm [OS]

Banker's algorithm is a classic deadlock-avoidance algorithm.

It assumes the system knows:

```text
Maximum resource requirements
Current allocations
Available resources
```

A resource request is granted only if the resulting state remains safe.

Conceptually:

```text
Request
   |
   v
Pretend to grant
   |
   v
Is system still safe?
   |
 +--+--+
 |     |
yes    no
 |     |
 v     v
grant  wait
```

It is important academically/interview-wise, but many real systems use simpler locking/resource-ordering designs.

---

# 24. Safe State vs Unsafe State

A **safe state** is one where the system has a possible sequence in which all processes can eventually complete.

An **unsafe state** does not necessarily mean deadlock has already occurred.

Important:

```text
Unsafe != necessarily deadlocked
```

But:

```text
Deadlock -> unsafe situation
```

under the corresponding model.

---

# 25. Deadlock Detection [OS]

If prevention/avoidance is not used, a system can detect deadlocks.

Typical idea:

```text
Build dependency graph
        |
        v
Find cycle
        |
        v
Potential deadlock
```

For a wait-for graph:

```text
T1 -> T2
T2 -> T3
T3 -> T1
```

Cycle exists:

```text
T1 -> T2 -> T3 -> T1
```

---

# 26. Deadlock Recovery [OS]

Once a deadlock is detected, possible recovery strategies include:

```text
Terminate one or more processes
Preempt/reclaim resources where possible
Rollback/restart work
Cancel selected operations
```

In ordinary application locking, recovery is often less attractive than preventing deadlock through good lock design.

---

# 27. Timeouts [LSP]

A timed lock can prevent indefinite waiting in some designs.

For example:

```c
pthread_mutex_timedlock(...)
```

Conceptually:

```text
try lock
   |
   v
wait up to deadline
   |
   +-- acquired
   |
   +-- timeout
```

A timeout is useful for:

```text
Fault detection
Fail-fast behavior
Distributed/resource systems
Operational safety
```

But:

> Timeout does not automatically fix the underlying deadlock.

---

# 28. Self-Deadlock [LSP]

A thread can deadlock against itself.

Example:

```c
pthread_mutex_lock(&lock);

pthread_mutex_lock(&lock);
```

With a normal non-recursive mutex, the second lock can block forever.

Graphically:

```text
Thread A
   |
   +--> Lock
   |
   +--> waits for Lock
```

The owner is itself.

---

# 29. Recursive Mutex

A recursive mutex allows the owning thread to acquire it multiple times.

Example:

```text
lock
lock
lock

unlock
unlock
unlock
```

This can prevent self-deadlock in recursive call paths.

But it can also hide design problems.

Prefer restructuring locking when possible rather than using recursive mutexes merely as a workaround.

---

# 30. Nested Locking

Nested locking means:

```text
lock A
    |
    +-- lock B
            |
            +-- critical section
```

Nested locks are not inherently wrong.

The danger is inconsistent ordering:

```text
Path 1:
A -> B

Path 2:
B -> A
```

That creates a potential cycle.

---

# 31. Deadlock Through Function Calls

A subtle bug:

```c
lock(A);

helper();
```

Suppose:

```c
helper()
{
    lock(B);
}
```

Another path:

```c
lock(B);

other_helper();
```

where:

```c
other_helper()
{
    lock(A);
}
```

Now:

```text
caller 1: A -> B
caller 2: B -> A
```

The deadlock may not be visible at the call site.

Senior engineers therefore document:

```text
Lock ordering
Lock ownership
Which functions may acquire which locks
```

---

# 32. Lock Ownership Documentation

Useful documentation:

```text
Lock A protects:
    object_list

Lock B protects:
    object_state

Ordering:
    A -> B

Never:
    B -> A
```

This reduces accidental deadlocks in large codebases.

---

# 33. Deadlock in Producer-Consumer Design

Incorrect example:

```text
Producer:
lock queue
wait for resource

Consumer:
lock resource
wait for queue
```

Possible cycle:

```text
Producer -> resource
Consumer -> queue
```

If:

```text
queue held by Producer
resource held by Consumer
```

both can block.

The lesson:

> Deadlocks are about dependency relationships, not just mutexes.

---

# 34. File Lock Deadlocks [LSP]

Linux programs can also deadlock through file/resource locking.

Examples include:

```text
fcntl locks
flock
application-level file locks
database locks
```

The same principle applies:

```text
Resource A held
Resource B requested

Resource B held
Resource A requested
```

Always define lock/resource ordering for multi-resource systems.

---

# 35. Process Synchronization Deadlocks [LSP]

Processes can deadlock using IPC resources too.

Examples:

```text
Named semaphores
POSIX semaphores
System V semaphores
Shared-memory protocols
Pipes
Sockets
Application-level locks
```

Example:

```text
Process A waits for message from B
Process B waits for message from A
```

No mutex is required for a deadlock to occur.

---

# 36. Pipe Deadlock Example

Imagine:

```text
Parent -> waits for child
Child  -> waits for parent
```

Or:

```text
Parent writes until pipe is full
Child waits for another event
Parent waits for child to read
```

If the communication protocol is incorrect, both processes can stop making progress.

This is why IPC design must consider:

```text
buffer limits
blocking behavior
reader/writer lifetime
close semantics
timeouts
```

---

# 37. Deadlocks and Blocking I/O

A lock held during blocking I/O is dangerous.

Bad:

```c
pthread_mutex_lock(&lock);

read(fd, buffer, size);  /* may block */

pthread_mutex_unlock(&lock);
```

If another thread needs the same lock to make the I/O complete:

```text
T1:
holds lock
blocks in read()

T2:
needs lock
cannot proceed
```

This can create a dependency cycle.

---

# 38. General Rule for I/O

Avoid holding locks across potentially long/blocking operations unless the design explicitly requires it.

Prefer:

```text
lock
copy/protect required state
unlock

perform slow operation

lock
update state
unlock
```

But the split must preserve correctness.

---

# 39. Deadlock and Condition Variables

A condition variable itself is not normally the source of the deadlock.

The problem is usually incorrect predicate/lock design.

Example:

```text
T1 holds A
waits for condition protected by B

T2 holds B
needs A to make condition true
```

Then:

```text
T1 -> B
T2 -> A
```

Deadlock.

---

# 40. Condition Variable Rule

Always think in terms of:

```text
Predicate
+
Mutex protecting predicate
+
Wait
+
State-changing thread
+
Notification
```

The condition variable is only part of the protocol.

---

# 41. Deadlock with Multiple Condition Variables

Suppose:

```text
T1 waits for condition A
T2 waits for condition B
```

If:

```text
T1 must change B
T2 must change A
```

and neither can proceed, the application can deadlock even though no mutex cycle is immediately obvious.

This is why state-machine design matters.

---

# 42. Linux Kernel Locking [KERNEL]

The Linux kernel has many synchronization primitives.

Important ones to know conceptually:

```text
mutex
spinlock
rwlock
rw_semaphore
semaphore
completion
wait queue
atomic operations
refcount
RCU
per-CPU mechanisms
```

The correct primitive depends heavily on:

```text
execution context
sleeping allowed?
interrupt context?
critical-section duration?
read/write ratio?
CPU scalability?
```

---

# 43. Kernel Mutex [KERNEL]

Typical pattern:

```c
mutex_lock(&lock);

/* critical section */

mutex_unlock(&lock);
```

Kernel mutexes are sleeping locks.

A task waiting for the mutex may sleep rather than busy-spin.

Do not use a sleeping mutex in contexts where sleeping is not allowed.

---

# 44. Kernel Spinlock [KERNEL]

Typical pattern:

```c
spin_lock(&lock);

/* short critical section */

spin_unlock(&lock);
```

A waiting CPU spins.

Important:

```text
short critical section
no sleeping
appropriate context
```

---

# 45. Spinlock Deadlock in the Kernel [KERNEL]

A classic mistake:

```c
spin_lock(&lock);

function_that_may_sleep();

spin_unlock(&lock);
```

If the function sleeps:

```text
CPU/task
   |
   v
holds spinlock
   |
   v
sleeps
```

Another CPU may spin indefinitely.

Kernel developers must understand which functions can sleep.

---

# 46. Interrupt-Related Deadlocks [KERNEL]

Suppose process context holds a lock:

```text
process context:
spin_lock(L)
```

An interrupt handler on the same CPU tries:

```text
interrupt:
spin_lock(L)
```

The interrupt handler may spin forever because the interrupted code cannot run to release the lock until the interrupt handler finishes.

This is one reason interrupt-safe locking patterns matter.

For appropriate situations, Linux provides forms such as:

```c
spin_lock_irqsave()
spin_unlock_irqrestore()
```

---

# 47. Lock Ordering in the Kernel [KERNEL]

Kernel subsystems often have documented lock ordering.

Conceptually:

```text
Lock A
  |
  v
Lock B
  |
  v
Lock C
```

Code must not introduce:

```text
C -> A
```

because it can create:

```text
A -> B -> C -> A
```

---

# 48. Lockdep [KERNEL]

Linux provides **lockdep**, a lock dependency validator/debugging framework.

Its purpose is to detect problematic locking patterns, including potential lock-order cycles.

Conceptually:

```text
Observed locking:
A -> B
B -> C
C -> A

lockdep:
       detects dependency cycle
```

This can catch problems before a real production deadlock occurs.

---

# 49. Lockdep Mental Model

Think of lockdep as building a graph:

```text
Lock A -> Lock B
Lock B -> Lock C
```

If code later establishes:

```text
Lock C -> Lock A
```

the dependency graph becomes:

```text
A -> B -> C -> A
```

A cycle indicates a potential locking dependency problem.

---

# 50. Kernel Completions [KERNEL]

Linux completions are useful for one task waiting for another event to complete.

Conceptually:

```text
Worker
   |
   v
does work
   |
   v
complete()

Waiter
   |
   v
wait_for_completion()
```

This is different from using a mutex simply to protect data.

---

# 51. RCU and Deadlock Design [KERNEL]

**RCU (Read-Copy-Update)** is a synchronization technique commonly used when read-side concurrency is extremely important.

High-level idea:

```text
Readers:
    access current version

Writer:
    create/update new version

After appropriate grace period:
    old version can be reclaimed
```

RCU reduces some locking overhead for read-heavy workloads.

It has specialized correctness rules and should not be treated as a generic mutex replacement.

---

# 52. Per-CPU Data [KERNEL]

One way to reduce lock contention is to avoid shared data.

Instead of:

```text
CPU0 \
CPU1  ---> shared counter + lock
CPU2 /
```

use:

```text
CPU0 -> local counter
CPU1 -> local counter
CPU2 -> local counter
```

Then aggregate when necessary.

This reduces:

```text
lock contention
cache-line bouncing
shared-state synchronization
```

---

# 53. Deadlock vs Scalability

A design can be deadlock-free and still perform badly.

Example:

```text
One global mutex
```

No deadlock:

```text
all threads use same lock order
```

But:

```text
100 threads
   |
   v
one lock
   |
   v
serialization
```

So correctness and scalability are separate concerns.

---

# 54. Deadlock Detection in a Running Linux Process [LSP]

Useful debugging approach:

```bash
ps -T -p <pid>
```

Find thread IDs.

Then:

```bash
gdb -p <pid>
```

Inside gdb:

```gdb
info threads
thread apply all bt
```

This can reveal threads blocked in synchronization functions.

Typical clues:

```text
pthread_mutex_lock
futex
pthread_cond_wait
sem_wait
```

---

# 55. GDB Deadlock Investigation

Attach:

```bash
gdb -p <pid>
```

List threads:

```gdb
info threads
```

Get all stacks:

```gdb
thread apply all bt
```

Look for:

```text
Thread 1 -> waiting for mutex
Thread 2 -> waiting for mutex
```

Then determine:

```text
Who owns the mutex?
What lock does each thread need?
What function acquired it?
```

---

# 56. `strace` and Blocking

For a process that appears stuck:

```bash
strace -f -p <pid>
```

You may see system calls related to waiting/blocking.

For example, futex operations can appear when pthread synchronization blocks.

This helps distinguish:

```text
CPU-intensive loop
vs
blocked synchronization
vs
blocked I/O
```

---

# 57. `perf` for Contention

Useful starting point:

```bash
perf stat -p <pid>
```

and:

```bash
perf record -p <pid>
```

You can investigate:

```text
CPU usage
context switches
scheduling behavior
hot functions
```

For actual lock contention, use the appropriate perf/kernel trace facilities available on the target system.

---

# 58. Thread States During Deadlock

Conceptually:

```text
T1:
sleeping/waiting for Lock 2

T2:
sleeping/waiting for Lock 1
```

The process may show low CPU usage:

```text
CPU ≈ 0%
```

Yet it is completely stuck.

This is an important debugging clue.

---

# 59. Deadlock Debugging Workflow

```text
Program appears hung
        |
        v
Check CPU usage
        |
   +----+----+
   |         |
 high       low
   |         |
   v         v
possible   possible
busy loop  blocking
             |
             v
       inspect threads
             |
             v
      capture backtraces
             |
             v
      identify wait points
             |
             v
      identify lock owners
             |
             v
       build dependency graph
             |
             v
        find cycle
```

---

# 60. Production Deadlock Checklist

When a service is stuck:

```text
1. Capture process/thread information.
2. Capture all thread stacks.
3. Identify blocking functions.
4. Identify lock/resource ownership.
5. Record lock acquisition order.
6. Build a wait-for graph.
7. Look for cycles.
8. Check blocking I/O while holding locks.
9. Check callbacks while holding locks.
10. Check condition-variable predicates.
11. Check thread shutdown/join dependencies.
12. Check IPC/resource dependencies.
```

---

# 61. A Subtle Deadlock: Join Dependency [LSP]

Example:

```text
Main thread:
holds lock
calls pthread_join(worker)

Worker:
needs same lock
```

Flow:

```text
Main -> holds lock -> waits for worker
Worker -> waits for lock
```

Deadlock.

The lesson:

> `pthread_join()` can participate in dependency cycles even though it is not a lock operation.

---

# 62. Another Subtle Deadlock: Callback

Bad pattern:

```text
lock(A);

call callback();
```

Suppose callback eventually does:

```text
lock(A);
```

Now:

```text
same thread -> waits for A
```

or another thread can create a cross-thread dependency.

Avoid invoking unknown/reentrant callbacks while holding locks unless the locking contract explicitly permits it.

---

# 63. Shutdown Deadlocks

A common production problem:

```text
Main:
request shutdown
join worker

Worker:
waits for condition
```

If shutdown does not:

```text
change predicate
signal/broadcast
```

the worker may never wake.

Correct shutdown protocols usually include:

```text
running = false
notify all waiters
join workers
destroy synchronization objects
```

---

# 64. Thread Pool Shutdown Example

Conceptually:

```c
pthread_mutex_lock(&mutex);

running = false;

pthread_cond_broadcast(&cond);

pthread_mutex_unlock(&mutex);
```

Workers:

```c
pthread_mutex_lock(&mutex);

while (queue_empty && running)
    pthread_cond_wait(&cond, &mutex);

if (!running && queue_empty)
{
    pthread_mutex_unlock(&mutex);
    return NULL;
}
```

Then the owner can:

```c
pthread_join(...)
```

This avoids workers sleeping forever during shutdown.

---

# 65. Deadlock Prevention Design Rules

Use these in real systems:

```text
1. Define lock ownership.
2. Define lock ordering.
3. Keep lock scope small.
4. Avoid nested locks where possible.
5. Never call unknown code while holding a lock.
6. Avoid blocking I/O while holding a lock.
7. Know which functions can sleep.
8. Use condition predicates correctly.
9. Keep shutdown paths explicit.
10. Prefer simpler synchronization designs.
11. Use debugging tools in development/testing.
12. Document lock dependencies.
```

---

# 66. Interview Question: Four Conditions

**Question: What are the four necessary conditions for deadlock?**

Answer:

```text
Mutual exclusion
Hold and wait
No preemption
Circular wait
```

Breaking any one of these conditions can prevent deadlock under the model.

---

# 67. Interview Question: How Do You Prevent Deadlock?

Strong answer:

> The most practical approach in lock-based software is to establish a global lock ordering and ensure every code path acquires locks in that order. Additional techniques include minimizing nested locking, avoiding blocking operations while holding locks, using trylock/timeouts where appropriate, and using lock-dependency tools such as lockdep in the Linux kernel.

---

# 68. Interview Question: Can `trylock()` Cause Livelock?

Yes.

Example:

```text
T1:
lock A
try B -> fail
unlock A

T2:
lock B
try A -> fail
unlock B
```

If both repeatedly retry in sync:

```text
no deadlock
but no useful progress
```

This is livelock.

Backoff or a fixed global ordering can help.

---

# 69. Interview Question: Is a Cycle Always a Deadlock?

Not universally.

For a simple resource/wait-for graph where all dependencies represent blocking resource ownership, a cycle indicates deadlock.

But in more complex systems:

```text
asynchronous messages
timeouts
preemption
resource cancellation
```

can change the analysis.

Senior-level answer:

> A cycle is a strong deadlock indicator when the graph accurately models blocking dependencies and the resources have the corresponding ownership semantics.

---

# 70. Interview Question: Can a Single Thread Deadlock?

Yes.

Example:

```c
pthread_mutex_lock(&m);
pthread_mutex_lock(&m);
```

A non-recursive mutex can cause self-deadlock.

Other single-thread cases can involve:

```text
waiting for an event that only the same thread can produce
joining itself
recursive resource dependency
```

---

# 71. Interview Question: Why Is Global Lock Ordering Effective?

Suppose every lock has an order:

```text
A < B < C
```

Every thread must acquire locks only in increasing order.

Then a cycle such as:

```text
A -> B -> C -> A
```

would require eventually acquiring a lower-order lock after a higher-order lock.

That violates the ordering rule.

Therefore circular wait is eliminated.

---

# 72. Interview Question: Why Not Just Kill One Thread?

In an application, forcibly terminating a thread can leave:

```text
mutexes locked
inconsistent data
partially updated state
resource leaks
corrupted invariants
```

Therefore prevention and structured recovery are usually safer.

---

# 73. Interview Question: Mutex Deadlock vs Semaphore Deadlock

Both can participate in deadlocks.

Example:

```text
T1 holds semaphore A
T1 waits for semaphore B

T2 holds semaphore B
T2 waits for semaphore A
```

The underlying principle is the same:

```text
circular resource dependency
```

Deadlock is not specific to mutexes.

---

# 74. Interview Question: Can Blocking I/O Cause Deadlock?

Yes.

Example:

```text
T1:
holds lock A
blocks on I/O

I/O completion requires T2

T2:
needs lock A
```

Then:

```text
T1 -> T2
T2 -> T1
```

Deadlock.

---

# 75. Interview Question: What Is Lock Inversion?

Lock inversion generally refers to code acquiring locks in an order inconsistent with another execution path.

Example:

```text
Path A: A -> B
Path B: B -> A
```

This can create circular wait.

In large systems, "lock ordering" is the clearer term to use when describing the dependency rule.

---

# 76. Interview Question: What Is Lockdep?

Answer:

> Lockdep is a Linux kernel lock-dependency validation framework. It tracks lock acquisition relationships and can report potential locking dependency cycles and other locking misuse. It is primarily a debugging/validation mechanism rather than a runtime deadlock recovery system.

---

# 77. Interview Question: Mutex vs Spinlock in Deadlock Context

Mutex:

```text
can block/sleep
```

Spinlock:

```text
busy waits
```

Both can be involved in deadlocks.

Spinlock deadlocks are particularly dangerous in kernel code because:

```text
owner may be unable to run
or
interrupt context may re-enter the lock
```

---

# 78. Interview Question: What Happens If a Process Holding a Mutex Dies?

Do not assume every mutex automatically becomes available.

For POSIX robust mutexes, the next locker can receive an owner-dead indication and attempt recovery.

For ordinary mutexes, application state can be left in an inconsistent/undefined recovery situation.

This is why robust mutexes and recovery protocols matter when process/thread termination is part of the design.

---

# 79. Practical Lab — Detect the Deadlock

Create:

```text
deadlock.c
```

Use two locks:

```text
T1: A -> B
T2: B -> A
```

Run:

```bash
./deadlock
```

Then attach:

```bash
gdb -p <pid>
```

Run:

```gdb
info threads
thread apply all bt
```

Identify:

```text
Thread 1 waiting for B
Thread 2 waiting for A
```

Draw the wait-for graph.

---

# 80. Practical Lab — Fix Using Ordering

Change both threads to:

```text
A -> B
```

Run again:

```bash
./deadlock
```

The deadlock should disappear for this locking dependency.

---

# 81. Practical Lab — Introduce Livelock

Implement:

```text
T1:
try A
try B
if failure:
    release
    retry

T2:
try B
try A
if failure:
    release
    retry
```

Then add:

```text
sleep/backoff
```

Observe how behavior changes.

---

# 82. Practical Lab — Shutdown Deadlock

Create:

```text
worker waits on condition variable
main holds mutex
main joins worker
```

Observe the hang.

Fix by:

```text
set shutdown predicate
broadcast
unlock
join
```

This is a valuable real-world synchronization exercise.

---

# 83. Practical Lab — Kernel Lockdep

When working with a suitable Linux kernel development/test environment, study lockdep reports for intentional lock-order violations.

Conceptually create:

```text
A -> B
B -> A
```

and observe how lock dependency validation identifies the problematic dependency.

Do this only in a safe development/test kernel environment.

---

# 84. Deadlock vs Chapter 5 Synchronization

Chapter 5:

```text
How do we synchronize?
```

Chapter 6:

```text
How can synchronization go wrong?
```

Relationship:

```text
Mutex
  |
  v
Multiple locks
  |
  v
Lock ordering
  |
  +---- correct ----> progress
  |
  +---- incorrect --> circular wait
                         |
                         v
                      deadlock
```

---

# 85. OS + Linux + Kernel Mapping

| Concept | OS | Linux System Programming | Linux Kernel |
|---|---|---|---|
| Mutex | Mutual exclusion | `pthread_mutex_*` | `struct mutex` |
| Spinlock | Busy waiting | `pthread_spin_*` | `spinlock_t` |
| Semaphore | Counting resource | `sem_*` | kernel semaphore |
| Condition wait | Predicate waiting | `pthread_cond_*` | wait queues/completions |
| Atomic | Atomic operation | C11/C++ atomics | atomic APIs |
| Deadlock | Resource graph | pthread/IPC/resource locks | lock dependencies |
| Lock ordering | Prevention | Application rule | Lockdep/kernel locking rules |
| Blocking | Scheduler involvement | futex/threads | scheduler/wait queues |
| Detection | Cycle detection | debugger/graphs | lockdep/debugging |
| Recovery | Restart/preempt/terminate | timeout/retry/restart | subsystem-specific |

---

# 86. Complete Deadlock Mental Model

```text
              Resources
                 |
        +--------+--------+
        |                 |
        v                 v
      Lock A            Lock B
        ^                 ^
        |                 |
       T1                T2
        |                 |
        +------wait-------+
```

More accurately:

```text
T1 owns A
T1 waits for B

T2 owns B
T2 waits for A

        ↓

Circular wait

        ↓

Deadlock
```

Prevention:

```text
Global ordering
A -> B
```

---

# 87. Senior-Level Design Checklist

Before adding a new lock:

```text
Why is shared state needed?
What invariant does the lock protect?
Who owns the lock?
Who can acquire it?
Can this function be called recursively?
Can it call callbacks?
Can it block?
Can it perform I/O?
Can it be called from multiple threads?
Can it be called from interrupt context? [kernel]
What other locks can be held?
What is the lock ordering?
What happens during shutdown?
What happens if a thread exits unexpectedly?
How will contention be measured?
```

---

# 88. Chapter 6 Cheat Sheet

```text
DEADLOCK
    |
    +-- Mutual exclusion
    +-- Hold and wait
    +-- No preemption
    +-- Circular wait

COMMON CAUSE
    |
    +-- inconsistent lock ordering

EXAMPLE
    |
    +-- T1: A -> B
    +-- T2: B -> A

PREVENTION
    |
    +-- global lock order
    +-- reduce nested locking
    +-- avoid blocking I/O under locks
    +-- avoid unknown callbacks under locks
    +-- use trylock/timeouts carefully
    +-- document ownership

RELATED PROBLEMS
    |
    +-- starvation
    +-- livelock
    +-- self-deadlock

LINUX DEBUGGING
    |
    +-- gdb
    +-- strace
    +-- perf
    +-- lockdep [kernel]

KERNEL
    |
    +-- mutex
    +-- spinlock
    +-- wait queues
    +-- completions
    +-- lockdep
    +-- scheduler interaction
```

---

# 89. Final Key Takeaways

1. Deadlock means a set of tasks cannot make progress because of circular resource dependencies.
2. The four Coffman conditions are mutual exclusion, hold and wait, no preemption, and circular wait.
3. Race conditions and deadlocks are different problems.
4. Starvation means a task is repeatedly denied progress; the system may still make progress.
5. Livelock means tasks are active but repeatedly fail to make useful progress.
6. The most practical lock-based prevention technique is consistent global lock ordering.
7. Nested locks are not inherently wrong; inconsistent ordering is dangerous.
8. `trylock()` can help avoid indefinite blocking but can introduce livelock.
9. Backoff can reduce repeated retry collisions.
10. Timeouts can detect/fail out from waits but do not necessarily fix the root cause.
11. A single thread can self-deadlock.
12. `pthread_join()` and condition-variable waits can participate in dependency cycles.
13. Blocking I/O while holding locks can create hidden deadlocks.
14. Unknown callbacks while holding locks are dangerous.
15. Shutdown paths need explicit wakeup and ordering.
16. Resource-allocation and wait-for graphs make deadlock dependencies visible.
17. Banker's algorithm is a classic avoidance technique based on safe states.
18. An unsafe state is not necessarily an already-deadlocked state.
19. Linux futex-based synchronization can block threads when user-space fast paths cannot acquire a lock.
20. Kernel mutexes are sleeping locks.
21. Kernel spinlocks are non-sleeping locks and must be used in appropriate contexts.
22. Interrupt context has stricter synchronization rules.
23. `spin_lock_irqsave()`/`spin_unlock_irqrestore()` are important when protecting appropriate interrupt-shared state.
24. Linux lockdep detects many lock dependency problems.
25. Deadlock-free does not mean scalable.
26. Lock contention and cache effects can still destroy performance.
27. Good synchronization design starts with invariants and dependency rules, not with blindly choosing a primitive.

---

# Chapter 7 Preview — Virtual Memory

Next:

```text
Chapter 7 -> Virtual Memory

Process virtual address space
        |
        v
Virtual address
        |
        v
Page
        |
        v
Page table
        |
        v
Physical frame
        |
        v
RAM
        |
        +--> page cache
        +--> swap
        +--> memory mapping
        +--> mmap()
        +--> malloc()
        +--> copy-on-write
        +--> demand paging
```

Chapter 7 will connect OS memory concepts to Linux system programming and kernel internals.
