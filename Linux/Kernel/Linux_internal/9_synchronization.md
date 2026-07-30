# Chapter 9 – Linux Kernel Synchronization

---

# 1. Why Synchronization?

Linux is highly concurrent.

Multiple execution contexts can access the same data:

```text
CPU 0 ──┐
CPU 1 ──┤
CPU 2 ──┼──> Shared Kernel Data
CPU 3 ──┘
```

Also:

```text
Process Context
Interrupt Context
Softirq
Workqueue
Kernel Threads
```

If multiple contexts modify shared data without synchronization, race conditions can occur.

---

# 2. Race Condition

A race condition occurs when the result depends on the timing/order of concurrent operations.

Example:

```c
counter++;
```

This looks like one operation but conceptually involves:

```text
READ counter
ADD 1
WRITE counter
```

Suppose:

```text
CPU 0                  CPU 1

READ counter = 10
                       READ counter = 10
ADD 1
                       ADD 1
WRITE 11
                       WRITE 11
```

Expected:

```text
12
```

Actual:

```text
11
```

This is a race condition.

---

# 3. Critical Section

A critical section is code that accesses shared state and must be protected from conflicting concurrent access.

```text
              Shared Data
                   |
                   v
          +----------------+
          | Critical       |
          | Section        |
          +----------------+
```

Example:

```c
lock();

counter++;

unlock();
```

Only one allowed execution context enters the protected region at a time, depending on the lock type.

---

# 4. Synchronization

Synchronization provides controlled access to shared resources.

Common Linux mechanisms:

```text
Spinlocks
Mutexes
Semaphores
Read-write locks
Atomic operations
Completions
Wait queues
RCU
Memory barriers
```

The correct mechanism depends on:

```text
Can the code sleep?
How long is the critical section?
Reader/writer ratio?
Interrupt context?
SMP?
Performance requirements?
```

---

# 5. Locking Context Matters

One of the most important Linux interview concepts:

```text
Process Context
    |
    +-- Can potentially sleep
    |
    +-- Mutex allowed

Hard IRQ Context
    |
    +-- Cannot sleep
    |
    +-- Mutex NOT allowed
    |
    +-- Spinlock/atomic mechanisms
```

Always ask:

> **Can this code sleep?**

before selecting a synchronization mechanism.

---

# 6. Mutex

A mutex provides exclusive ownership.

Conceptually:

```text
CPU 0
 |
 | mutex_lock()
 v
[ LOCKED ]
 |
 | critical section
 v
mutex_unlock()
```

Another thread attempting to acquire the mutex may sleep until the mutex becomes available.

---

# 7. Mutex Example

```c
struct device_data {
    struct mutex lock;
    int value;
};

void update_value(struct device_data *dev)
{
    mutex_lock(&dev->lock);

    dev->value++;

    mutex_unlock(&dev->lock);
}
```

The shared value is protected.

---

# 8. Why Mutex Can Sleep

Suppose CPU 0 owns the mutex:

```text
CPU 0
 |
 +-- mutex locked
 |
 +-- long operation
```

CPU 1 tries:

```text
CPU 1
 |
 +-- mutex_lock()
 |
 +-- mutex unavailable
 |
 +-- sleep
```

The scheduler can run another task.

This avoids wasting CPU spinning.

---

# 9. Mutex Rules

A mutex is generally appropriate when:

```text
Process context
Critical section can be relatively long
Sleeping is allowed
Only one owner is required
```

Do not use a normal mutex in hard IRQ context.

---

# 10. Spinlock

A spinlock protects a short critical section by spinning instead of sleeping.

Conceptually:

```text
CPU 0
 |
 +-- owns lock
 |
 +-- critical section

CPU 1
 |
 +-- tries lock
 |
 +-- spins
 |
 +-- spins
 |
 +-- lock released
 |
 +-- enters
```

---

# 11. Spinlock Example

```c
spin_lock(&lock);

shared_data++;

spin_unlock(&lock);
```

The waiting CPU actively checks the lock.

---

# 12. Why Spin Instead of Sleep?

For a very short critical section:

```text
Lock held for 100 ns
```

sleeping and waking a task may cost more than simply waiting briefly.

Therefore:

```text
Short critical section
        ↓
Spinlock can be appropriate
```

---

# 13. Spinlock Rules

Spinlocks are appropriate when:

```text
Critical section is short
Code cannot sleep
Interrupt/atomic context may be involved
```

Never do long/blocking operations while holding a spinlock.

---

# 14. Mutex vs Spinlock

| Property                      | Mutex           | Spinlock                  |
| ----------------------------- | --------------- | ------------------------- |
| Can sleep?                    | Yes             | No                        |
| Waiting method                | Sleep           | Spin                      |
| Suitable for IRQ context?     | No              | Yes, with correct variant |
| Critical section              | Can be longer   | Should be short           |
| CPU consumption while waiting | Low             | Higher                    |
| Typical use                   | Process context | Short atomic sections     |

Interview rule:

```text
Can sleep?
    |
    +-- YES → mutex may be appropriate
    |
    +-- NO  → spinlock/atomic mechanism
```

---

# 15. `spin_lock_irqsave()`

Suppose shared data is accessed from both:

```text
Process Context
```

and:

```text
Interrupt Context
```

A common pattern is:

```c
unsigned long flags;

spin_lock_irqsave(&lock, flags);

shared_data++;

spin_unlock_irqrestore(&lock, flags);
```

This protects the critical section and saves/restores the local interrupt state.

---

# 16. Why Disable Local Interrupts?

Consider:

```text
Process Context
    |
    | acquires spinlock
    |
    v
CPU
    |
    | IRQ arrives
    v
IRQ Handler
    |
    | tries same lock
    v
spin forever
```

The interrupted process cannot resume to release the lock.

This can deadlock.

Using an IRQ-safe locking pattern prevents this specific local interrupt re-entry problem.

---

# 17. `spin_lock_irq()`

Another variant is:

```c
spin_lock_irq(&lock);
```

It disables local interrupts while holding the lock.

But it does not preserve the previous interrupt state.

Therefore:

```text
spin_lock_irqsave()
```

is commonly preferred when the previous interrupt state must be restored correctly.

---

# 18. Bottom-Half Synchronization

Synchronization can also involve:

```text
Hard IRQ
Softirq
Tasklet
Workqueue
Process
```

For example:

```text
Process
   |
   v
Shared data
   ^
   |
IRQ Handler
```

The locking mechanism must be safe for the contexts accessing the data.

---

# 19. Semaphore

A semaphore maintains a count.

Conceptually:

```text
Semaphore = N available resources
```

Example:

```text
Semaphore count = 3
```

Three users can acquire it simultaneously.

```text
Thread A → acquire → count 2
Thread B → acquire → count 1
Thread C → acquire → count 0
Thread D → wait
```

---

# 20. Binary Semaphore

A semaphore with a count of 1 behaves somewhat like an exclusive gate.

However:

> In modern Linux driver code, a mutex is generally preferred when the requirement is mutual exclusion.

A mutex has ownership semantics that a semaphore does not.

---

# 21. Mutex vs Semaphore

| Property          | Mutex            | Semaphore          |
| ----------------- | ---------------- | ------------------ |
| Ownership         | Yes              | No                 |
| Typical use       | Mutual exclusion | Resource counting  |
| Multiple holders  | No               | Yes, if count > 1  |
| Can sleep         | Yes              | Yes                |
| Common driver use | Very common      | Specific use cases |

---

# 22. Read-Write Lock

Sometimes many readers access data while writers are rare.

Example:

```text
Readers:
CPU 0 ──┐
CPU 1 ──┤
CPU 2 ──┤──> Shared Data
CPU 3 ──┘

Writer:
CPU 4 ─────────> Shared Data
```

A read-write lock allows multiple readers simultaneously but excludes writers.

---

# 23. Read Lock

Multiple readers can hold the read side:

```text
Reader 1 ──┐
Reader 2 ──┤
Reader 3 ──┼──> READ LOCK
Reader 4 ──┘
```

No writer can enter while readers hold the lock.

---

# 24. Write Lock

A writer requires exclusive access:

```text
Writer
  |
  v
WRITE LOCK
  |
  +-- No readers
  +-- No other writers
```

---

# 25. Read-Write Lock Example

Conceptually:

```c
read_lock(&rwlock);

read_shared_data();

read_unlock(&rwlock);
```

Writer:

```c
write_lock(&rwlock);

modify_shared_data();

write_unlock(&rwlock);
```

---

# 26. Read-Write Lock Trade-Off

Read-write locking is useful when:

```text
Many readers
Few writers
Read operations are significant
```

But it is not automatically faster than a mutex.

Lock overhead, contention, cache behavior, and workload matter.

---

# 27. Atomic Operations

For simple shared variables, atomic operations can avoid a full lock.

Example:

```c
atomic_inc(&counter);
```

Conceptually:

```text
CPU 0 ── atomic increment
CPU 1 ── atomic increment
CPU 2 ── atomic increment
```

The operation is performed atomically with respect to other atomic operations on that variable.

---

# 28. Why Atomics?

Consider:

```c
counter++;
```

This is not necessarily atomic.

Instead:

```c
atomic_inc(&counter);
```

provides an atomic increment operation.

For simple counters and flags, atomics can be more efficient and simpler than locks.

---

# 29. Atomic vs Lock

Use atomic operations for:

```text
Simple counters
Flags
Reference counts
Simple state transitions
```

Use locks for:

```text
Multiple related variables
Complex invariants
Larger critical sections
Multiple operations that must happen together
```

Example:

```text
counter++
```

may need only an atomic operation.

But:

```text
update list
update count
update state
```

may require a lock to maintain consistency across all fields.

---

# 30. Atomicity vs Ordering

Important distinction:

```text
Atomicity
```

means an operation cannot be observed as partially completed.

```text
Ordering
```

means operations occur with required visibility/order relative to other CPUs.

An atomic operation does not automatically mean:

> "Every memory operation around it has exactly the ordering I want."

Understand both concepts separately.

---

# 31. Memory Barriers

Memory barriers enforce ordering constraints.

Examples include:

```text
smp_mb()
smp_rmb()
smp_wmb()
```

Conceptually:

```text
CPU 0

write data
    |
    v
MEMORY BARRIER
    |
    v
write flag
```

Another CPU can use appropriate synchronization to ensure it observes the intended ordering.

---

# 32. Locking Often Provides Ordering

Kernel locking primitives generally provide the required memory-ordering semantics around lock acquisition/release.

Therefore, do not add memory barriers blindly around locks.

First understand:

```text
What synchronization is already providing?
```

---

# 33. Wait Queues

A wait queue allows a process to sleep until a condition becomes true.

Example:

```text
Process
   |
   v
Condition false
   |
   v
Sleep
```

Later:

```text
Interrupt
   |
   v
Data available
   |
   v
Wake process
```

---

# 34. Wait Queue Example

Conceptually:

```c
wait_event(queue, data_ready);
```

The process sleeps until:

```c
data_ready == true
```

Another context can wake it:

```c
wake_up(&queue);
```

---

# 35. Wait Queue Pattern

The standard pattern is:

```text
Check condition
      |
      +-- true --> continue
      |
      +-- false
             |
             v
           sleep
             |
             v
          wake up
             |
             v
       check condition again
```

The condition must be checked again after waking.

---

# 36. Why Recheck the Condition?

A wakeup means:

> Something may have changed.

It does not necessarily mean:

> Your condition is guaranteed to be true.

Therefore:

```text
Wakeup
  ↓
Recheck condition
```

is essential.

---

# 37. Completion

A completion is useful when one execution context needs to wait until another finishes an operation.

Example:

```text
Process
   |
   v
Start hardware operation
   |
   v
wait_for_completion()
```

Hardware completes:

```text
IRQ
 |
 v
Driver
 |
 v
complete()
 |
 v
Wake process
```

---

# 38. Completion vs Wait Queue

### Wait Queue

Used for:

```text
Wait until a condition becomes true
```

### Completion

Used for:

```text
Wait until a specific operation/event completes
```

Completion often expresses the intent more clearly.

---

# 39. Reference Counting

Reference counting tracks how many users hold references to an object.

Example:

```text
Object
  |
  +-- Reference 1
  +-- Reference 2
  +-- Reference 3
```

Count:

```text
3
```

When references disappear:

```text
3 → 2 → 1 → 0
```

At zero, the object can be released.

Linux provides:

```text
refcount_t
```

for reference counting.

---

# 40. Why Reference Counting?

Suppose:

```text
Thread A → using object
Thread B → using object
```

Thread A must not free the object while B is still using it.

Reference counting prevents premature destruction.

---

# 41. Reference Counting Example

Conceptually:

```text
Object created
   |
   v
refcount = 1

Another user
   |
   v
refcount = 2

User A releases
   |
   v
refcount = 1

User B releases
   |
   v
refcount = 0

Object destroyed
```

---

# 42. RCU

RCU means:

```text
Read-Copy-Update
```

It is a synchronization mechanism optimized for read-heavy workloads.

Conceptually:

```text
Many Readers
   |
   +-- Read shared structure
   |
   +-- Read shared structure
   |
   +-- Read shared structure

Rare Writer
   |
   v
Create new version
   |
   v
Publish new version
```

Readers can often proceed without taking a traditional lock.

---

# 43. RCU Basic Idea

Suppose readers use:

```text
Object A
```

A writer creates:

```text
Object B
```

Then publishes B.

Existing readers can finish using A.

Later, when it is safe:

```text
Object A
```

can be reclaimed.

---

# 44. RCU Mental Model

```text
             Shared Pointer
                  |
             +----+----+
             |         |
         Readers     Writer
             |         |
             v         v
          Version A  Version B
                       |
                       v
                    Publish
                       |
                       v
                    New Readers
```

Old readers finish before the old object is reclaimed.

---

# 45. Why RCU?

RCU is useful when:

```text
Reads are extremely frequent
Writes are relatively rare
Read-side latency matters
Traditional locking would create contention
```

Common kernel use cases include read-heavy data structures.

---

# 46. RCU Trade-Off

RCU is powerful but complex.

You must understand:

```text
Read-side critical sections
Grace periods
Deferred reclamation
Pointer publication
Memory ordering
```

Do not use RCU simply because it is "faster."

---

# 47. Lock Ordering

Suppose code needs two locks:

```text
lock A
lock B
```

Correct:

```text
A → B
```

Another path must not do:

```text
B → A
```

Otherwise:

```text
CPU 0:
lock A
wait for B

CPU 1:
lock B
wait for A
```

Deadlock.

---

# 48. Deadlock

Four classic conditions are commonly associated with deadlock:

```text
1. Mutual exclusion
2. Hold and wait
3. No preemption
4. Circular wait
```

If all are present, deadlock can occur.

---

# 49. Deadlock Example

```text
CPU 0                     CPU 1

lock A                    lock B
   |                          |
   v                          v
wait for B                wait for A
   |                          |
   +---------- DEADLOCK ------+
```

Neither can proceed.

---

# 50. Preventing Lock-Order Deadlocks

Define a global lock order:

```text
A < B < C
```

Then every code path acquires locks in that order:

```text
A → B
A → C
B → C
```

Never:

```text
B → A
C → A
C → B
```

unless the locking design explicitly allows it.

---

# 51. Lock Contention

Contention occurs when many CPUs compete for the same lock.

```text
CPU 0 ──┐
CPU 1 ──┤
CPU 2 ──┼──> SAME LOCK
CPU 3 ──┘
```

High contention can cause:

```text
CPU waste
Cache-line bouncing
Reduced scalability
Increased latency
```

---

# 52. Lock Granularity

### Coarse-grained locking

```text
One large lock
      |
      +-- protects many things
```

Advantages:

```text
Simple
Less locking complexity
```

Disadvantages:

```text
High contention
Poor scalability
```

### Fine-grained locking

```text
Lock A → data A
Lock B → data B
Lock C → data C
```

Advantages:

```text
More concurrency
Better scalability
```

Disadvantages:

```text
More complexity
Potential deadlocks
More overhead
```

---

# 53. Lockless Does Not Mean Synchronization-Free

A lockless algorithm still requires synchronization.

It may use:

```text
Atomic operations
Memory barriers
RCU
Compare-and-swap
Per-CPU data
```

Therefore:

> Lockless ≠ no synchronization.

---

# 54. Compare-and-Swap

A common atomic primitive is:

```text
Compare-And-Swap
```

Conceptually:

```text
if (value == expected)
    value = new_value;
```

The comparison and update happen atomically.

This is useful for lock-free algorithms.

---

# 55. CAS Example

Suppose:

```text
value = 10
```

We want:

```text
10 → 20
```

CAS:

```text
compare(value, 10)

if equal:
    value = 20
```

If another CPU changed it first:

```text
value = 15
```

the CAS fails.

The algorithm can retry or take another path.

---

# 56. Per-CPU Data

Sometimes the best way to avoid synchronization is to avoid sharing data.

Instead of:

```text
All CPUs
   |
   v
One shared counter
```

use:

```text
CPU0 → counter0
CPU1 → counter1
CPU2 → counter2
CPU3 → counter3
```

This reduces contention.

---

# 57. Per-CPU Mental Model

```text
CPU 0 → local data
CPU 1 → local data
CPU 2 → local data
CPU 3 → local data
```

Global aggregation can happen later.

This is useful for:

```text
Statistics
Counters
Caches
High-frequency kernel data
```

---

# 58. False Sharing

Two CPUs can modify different variables that happen to reside on the same cache line.

```text
Cache Line
+-------------------------+
| counterA | counterB     |
+-------------------------+
     ↑           ↑
    CPU0        CPU1
```

Even though the variables are logically independent, cache coherence traffic can cause performance degradation.

This is called:

```text
False Sharing
```

---

# 59. Synchronization and Cache Coherency

Locks involve shared memory.

Therefore:

```text
CPU 0
 |
 +-- modifies lock/data
 |
 v
Cache coherence traffic
 |
 v
CPU 1
```

High lock contention can produce substantial cache-line movement.

This is one reason why scalability can degrade as CPU count increases.

---

# 60. Atomic Operations vs Spinlocks

Atomic:

```text
atomic_inc()
```

is ideal for:

```text
One simple variable
```

Spinlock:

```text
spin_lock()
...
spin_unlock()
```

is better for:

```text
Multiple related variables
Complex invariants
Several operations that must be protected together
```

---

# 61. Example: Counter + List

Suppose:

```text
list
count
```

must remain consistent.

This is unsafe:

```text
list_add();
atomic_inc(&count);
```

if another CPU can observe the intermediate state and the relationship requires both operations to appear as one protected update.

A lock may be appropriate:

```c
spin_lock(&lock);

list_add(...);
count++;

spin_unlock(&lock);
```

---

# 62. Synchronization in Device Drivers

A typical driver may have:

```text
User Process
     |
     v
ioctl/read/write
     |
     v
Driver
     |
     +-- mutex
     |
     +-- hardware
     |
     +-- DMA
     |
     +-- interrupt
             |
             v
        spinlock
             |
             v
        shared state
```

The same driver can legitimately use different synchronization mechanisms for different contexts.

---

# 63. Example Driver Design

Suppose:

```text
read()
```

starts an operation and an IRQ completes it.

Process context:

```c
mutex_lock(&dev->lock);

start_device();

wait_for_completion(&dev->done);

mutex_unlock(&dev->lock);
```

Interrupt context:

```c
irq_handler(...)
{
    spin_lock(&dev->irq_lock);

    update_status();

    spin_unlock(&dev->irq_lock);

    complete(&dev->done);
}
```

The important point is that each context uses an appropriate synchronization primitive.

---

# 64. Common Synchronization Mistakes

### Mistake 1

Using mutex in interrupt context.

```text
IRQ
 |
 v
mutex_lock()
 |
 X
```

Invalid because mutex acquisition can sleep.

---

### Mistake 2

Holding spinlock while sleeping.

```text
spin_lock()
   |
   v
sleep()
   |
   X
spin_unlock()
```

Invalid.

---

### Mistake 3

Inconsistent lock order.

```text
Path A: A → B
Path B: B → A
```

Potential deadlock.

---

### Mistake 4

Overusing locks.

Too much locking can cause:

```text
Contention
Poor scalability
Complexity
```

---

### Mistake 5

Insufficient locking.

This causes:

```text
Race conditions
Corruption
Use-after-free
Inconsistent state
```

---

# 65. Debugging Race Conditions

Look for:

```text
Shared mutable data
Multiple execution contexts
Missing locking
Incorrect lock scope
Interrupt/process interaction
Lifetime issues
```

Useful kernel tools/mechanisms include:

```text
lockdep
KASAN
KCSAN
ftrace
tracepoints
debugging instrumentation
```

---

# 66. Lockdep

Lockdep is the Linux kernel lock dependency validator.

It can detect problems such as:

```text
Potential deadlocks
Lock ordering problems
Incorrect lock usage
```

Conceptually:

```text
Lock A → Lock B

Lock B → Lock A

        ↓
Potential circular dependency
```

Lockdep helps identify such patterns.

---

# 67. KCSAN

KCSAN means:

```text
Kernel Concurrency Sanitizer
```

It helps detect data races in kernel code.

Conceptually:

```text
CPU 0 → write shared data
CPU 1 → read shared data
             |
             v
        Possible race
```

This is particularly useful for concurrency debugging.

---

# 68. Synchronization Decision Tree

Use this mental model:

```text
Need to protect shared state?
          |
          v
        YES
          |
          v
Can code sleep?
     /           \
   YES            NO
    |              |
    v              v
  Mutex       Spinlock / Atomic
                   |
                   v
          Is operation simple?
             /          \
           YES          NO
            |            |
            v            v
         Atomic       Spinlock
```

For read-heavy workloads:

```text
Many readers?
      |
      v
Read-write lock / RCU
```

For waiting:

```text
Wait for condition?
      |
      v
Wait queue
```

For event completion:

```text
Wait for operation?
      |
      v
Completion
```

---

# 69. Synchronization Hierarchy

Think of synchronization like this:

```text
Simple shared value
       |
       v
    Atomic

Short critical section
       |
       v
   Spinlock

Sleepable critical section
       |
       v
     Mutex

Multiple readers
       |
       v
Read-write lock / RCU

Wait for condition
       |
       v
 Wait Queue

Wait for event completion
       |
       v
 Completion
```

This is a very useful interview framework.

---

# 70. Senior Interview Questions

## Q1. Mutex vs spinlock?

```text
Mutex:
    Can sleep
    Process context
    Longer critical sections

Spinlock:
    Cannot sleep
    Short critical sections
    IRQ/atomic contexts
```

---

## Q2. Can you use a mutex in an interrupt handler?

No.

Hard IRQ handlers cannot sleep, and mutex acquisition may sleep.

---

## Q3. Why use `spin_lock_irqsave()`?

When a lock is shared between contexts where local interrupt re-entry must be prevented, it disables local interrupts while holding the lock and restores the previous interrupt state afterward.

---

## Q4. Atomic operation vs spinlock?

Atomic operations are ideal for simple independent updates.

Spinlocks protect larger critical sections and relationships among multiple pieces of state.

---

## Q5. What is deadlock?

A situation where execution contexts wait indefinitely for resources held by each other.

---

## Q6. How do you prevent deadlock?

Use:

```text
Consistent lock ordering
Avoid unnecessary nested locks
Keep critical sections small
Use lockdep
Design ownership carefully
```

---

## Q7. What is RCU?

A synchronization mechanism optimized for read-heavy workloads where readers can often proceed without taking a traditional lock.

---

## Q8. What is a wait queue?

A mechanism that allows a task to sleep until a condition becomes true.

---

## Q9. What is a completion?

A synchronization mechanism used when one execution context needs to wait for another operation/event to finish.

---

## Q10. What is lock contention?

Multiple CPUs/threads competing for the same lock, potentially reducing scalability and increasing latency.

---

# 71. Senior-Level Scenario

### Question:

A driver occasionally crashes because shared device state becomes corrupted. How would you investigate?

Answer:

```text
1. Identify all accesses to shared state.
2. Identify execution contexts:
       process
       IRQ
       workqueue
       kernel thread
3. Determine whether accesses can occur concurrently.
4. Check existing locking.
5. Check whether the lock is valid for each context.
6. Check lock scope.
7. Check interrupt-safe locking.
8. Check lock ordering.
9. Check lifetime/reference counting.
10. Use lockdep/KCSAN where appropriate.
11. Reproduce under SMP/high concurrency.
```

This demonstrates senior-level debugging rather than simply saying:

> "Add a mutex."

---

# 72. Most Important Topics to Master

```text
★★★★★ Race conditions
★★★★★ Critical sections
★★★★★ Mutex
★★★★★ Spinlocks
★★★★★ IRQ-safe locking
★★★★★ Atomic operations
★★★★★ Memory ordering
★★★★★ Deadlocks
★★★★★ Lock ordering
★★★★★ Wait queues
★★★★★ Completions
★★★★★ Reference counting
★★★★★ RCU
★★★★☆ Read-write locks
★★★★☆ Per-CPU data
★★★★☆ Lock contention
★★★★☆ False sharing
★★★★☆ lockdep
★★★★☆ KCSAN
```

---

# 73. Final Mental Model

Memorize this:

```text
                 SHARED DATA
                     |
                     v
             Need synchronization?
                     |
          +----------+----------+
          |                     |
         YES                    NO
          |                     |
          v                     v
      What context?          No lock needed
          |
     +----+----+
     |         |
 Can sleep?   Cannot sleep
     |             |
    YES            NO
     |             |
     v             v
   MUTEX       SPINLOCK
                   |
             Simple operation?
               /        \
             YES         NO
              |           |
              v           v
           ATOMIC      SPINLOCK

Read-heavy:
    ↓
RWLOCK / RCU

Wait for condition:
    ↓
WAIT QUEUE

Wait for event:
    ↓
COMPLETION
```

---

# Chapter 9 Summary

The most important Linux synchronization principle is:

> **Choose synchronization based on execution context, whether sleeping is allowed, the type of shared state, contention, and the required concurrency.**

The key chain is:

```text
Race Condition
      ↓
Critical Section
      ↓
Synchronization
      ↓
+---------+---------+---------+---------+
|         |         |         |         |
Mutex   Spinlock  Atomic     RCU    Wait Queue
|         |         |         |         |
Sleep   No Sleep  Simple    Readers   Condition
```

For senior Linux interviews at companies such as **Qualcomm, AMD, NVIDIA, Intel and other systems/embedded companies**, you should be able to take a real driver scenario and explain **which synchronization primitive you would use, why, what context the code runs in, and what failure occurs if the wrong primitive is chosen**.
