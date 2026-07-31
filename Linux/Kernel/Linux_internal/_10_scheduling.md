# Chapter 10 – Linux Scheduling

---

# 1. What Is Scheduling?

Linux can have hundreds or thousands of runnable tasks, but a CPU core can execute only one task at a time.

The scheduler decides:

```text
Which task should run?
        |
        v
On which CPU?
        |
        v
For how long?
```

Conceptually:

```text
          Runnable Tasks
        /      |       \
       v       v        v
    Task A   Task B   Task C
        \      |       /
         \     |      /
          v    v     v
            Scheduler
                |
        +-------+-------+
        |       |       |
        v       v       v
      CPU 0   CPU 1   CPU 2
```

---

# 2. Main Goals of a Scheduler

A scheduler tries to balance several competing goals:

```text
Fairness
Throughput
Low latency
Responsiveness
CPU utilization
Real-time requirements
Cache locality
NUMA locality
```

There is no single scheduling strategy that maximizes all of them simultaneously.

---

# 3. Runnable vs Running

This distinction is fundamental.

### Running

A task is currently executing on a CPU.

```text
CPU 0
 |
 +-- Task A ← RUNNING
```

### Runnable

A task is ready to execute but is waiting for CPU time.

```text
Runnable:
Task B
Task C
Task D
```

Therefore:

```text
Runnable
    |
    | scheduler selects
    v
Running
```

---

# 4. Sleeping Tasks

A task may not be runnable because it is waiting for something.

Examples:

```text
I/O
Timer
Lock
Condition
Event
```

Conceptually:

```text
Running
   |
   v
Wait for I/O
   |
   v
Sleeping
   |
   | I/O completes
   v
Runnable
```

A sleeping task does not consume CPU time while it waits.

---

# 5. Scheduling Decision

A simplified model:

```text
             Scheduler
                 |
       +---------+---------+
       |         |         |
       v         v         v
    Task A    Task B    Task C
       |
       v
    Selected
       |
       v
      CPU
```

The scheduler considers things such as:

```text
Scheduling policy
Priority
Runnable state
CPU affinity
Task class
Runtime
Load
```

---

# 6. `task_struct`

Linux represents a task using:

```c
struct task_struct
```

The scheduler-related information is stored in the task's scheduling state.

Conceptually:

```text
task_struct
     |
     +-- state
     +-- scheduling policy
     +-- priority
     +-- scheduling class
     +-- CPU information
     +-- runtime information
```

You do not need to memorize every field for a senior interview.

Understand the relationships.

---

# 7. Scheduling Classes

Linux supports different scheduling classes.

Conceptually:

```text
             Scheduler
                 |
        +--------+--------+
        |        |        |
        v        v        v
     Deadline   RT      Fair
        |
        +----------------+
                 |
                Idle
```

Important classes include:

```text
Stop
Deadline
Real-time
Fair
Idle
```

The exact internal implementation evolves between kernel versions.

---

# 8. Why Scheduling Classes?

Different workloads need different scheduling behavior.

### Normal applications

Want:

```text
Fair CPU sharing
```

### Real-time applications

Want:

```text
Predictable priority-based execution
```

### Deadline workloads

Want:

```text
Execution before specified deadlines
```

Therefore Linux provides multiple scheduling policies.

---

# 9. Normal Scheduling

Normal applications generally use:

```text
SCHED_NORMAL
```

Historically, Linux implemented normal scheduling using the:

```text
Completely Fair Scheduler
(CFS)
```

Modern kernels have evolved the internals toward newer fair-scheduling mechanisms, so interviews should focus on the **fair scheduling principles** rather than assuming an older kernel's exact implementation.

---

# 10. Fair Scheduling

The goal is approximately:

> Give runnable tasks a fair share of CPU time according to their scheduling weights.

Example:

```text
Task A
Task B
Task C
```

If they have equal scheduling weight and remain runnable, they should receive approximately equal CPU share over time.

```text
CPU time:

A █████
B █████
C █████
```

---

# 11. Why Not Just Round-Robin Everything?

Pure round-robin is simple:

```text
A → B → C → A → B → C
```

But Linux needs to handle:

```text
Different priorities
Different weights
Interactive tasks
CPU-bound tasks
Real-time tasks
Multiple CPUs
```

Therefore normal Linux scheduling is more sophisticated than simple round-robin.

---

# 12. Nice Value

Normal tasks have a nice value.

Typical range:

```text
-20 ... +19
```

Conceptually:

```text
Lower nice value
      ↓
Higher scheduling weight

Higher nice value
      ↓
Lower scheduling weight
```

Example:

```bash
nice -n 10 ./program
```

A process with a higher nice value generally receives less CPU share relative to a task with the default nice value.

---

# 13. Nice Is Not Real-Time Priority

This is a common interview trap.

```text
Nice value
    ↓
Normal/fair scheduling

Real-time priority
    ↓
Real-time scheduling policies
```

They are different mechanisms.

---

# 14. Preemption

Preemption means the currently running task can be stopped so another task can run.

Example:

```text
Task A
  |
  | running
  v
Higher-priority task becomes runnable
  |
  v
Preemption
  |
  v
Task B runs
```

Preemption improves responsiveness.

---

# 15. Voluntary Scheduling

A task may voluntarily stop running because it needs to wait.

Example:

```text
Task
 |
 +-- read()
 |
 +-- waiting for disk
 |
 v
Sleep
```

The task leaves the CPU and another runnable task can execute.

---

# 16. Involuntary Preemption

The scheduler can also stop a runnable task even when it did not voluntarily block.

Example:

```text
Task A running
      |
      v
Scheduler decision
      |
      v
Task A preempted
      |
      v
Task B running
```

This is important for fairness and responsiveness.

---

# 17. Context Switch

When switching from one task to another:

```text
Task A
   |
   | save execution state
   v
Scheduler
   |
   | select Task B
   v
Task B
   |
   | restore execution state
   v
Continue
```

The CPU state includes things such as:

```text
Registers
Instruction pointer
Stack pointer
Architecture-specific state
```

---

# 18. Context Switch Cost

Context switching is not free.

Potential costs include:

```text
Saving/restoring CPU state
Scheduler overhead
Cache disruption
TLB/address-space effects
Branch predictor effects
CPU migration
```

Therefore:

> More context switches do not automatically mean better performance.

---

# 19. Process vs Thread Scheduling

Linux schedules tasks.

A thread is also represented as a schedulable task.

For example:

```text
Process
 |
 +-- Thread A
 +-- Thread B
 +-- Thread C
```

Each thread can be scheduled independently.

However, threads may share:

```text
Address space
Files
Other process resources
```

depending on how they were created.

---

# 20. CPU Affinity

CPU affinity specifies which CPUs a task may execute on.

Example:

```text
Task A
 |
 +-- CPU 0
 +-- CPU 1
```

Affinity can restrict it to:

```text
CPU 0 only
```

Linux provides:

```c
sched_setaffinity()
```

and the command:

```bash
taskset
```

---

# 21. Why CPU Affinity Matters

Affinity can improve:

```text
Cache locality
Performance predictability
NUMA locality
Real-time behavior
CPU isolation
```

But overly restrictive affinity can reduce load-balancing opportunities.

---

# 22. SMP Scheduling

Modern systems contain multiple CPUs/cores.

```text
CPU 0
CPU 1
CPU 2
CPU 3
```

Tasks can execute simultaneously:

```text
CPU 0 → Task A
CPU 1 → Task B
CPU 2 → Task C
CPU 3 → Task D
```

This creates true concurrency.

Therefore scheduling and synchronization are closely related.

---

# 23. Per-CPU Runqueues

Conceptually, each CPU has scheduler state containing runnable tasks.

```text
CPU 0
 |
 +-- runnable tasks

CPU 1
 |
 +-- runnable tasks

CPU 2
 |
 +-- runnable tasks
```

Modern Linux scheduler internals are more complex than this simplified model, but this is a useful mental model.

---

# 24. Load Balancing

Suppose:

```text
CPU 0:
A
B
C
D

CPU 1:
E
```

The scheduler may move work:

```text
CPU 0:
A
B

CPU 1:
C
D
E
```

This improves CPU utilization.

---

# 25. Task Migration

Moving a task between CPUs is called migration.

```text
CPU 0
 |
 +-- Task A
       |
       | migration
       v
CPU 1
 |
 +-- Task A
```

Migration can improve load balance but may hurt cache locality.

---

# 26. Cache Locality

Suppose Task A frequently accesses data cached on CPU 0:

```text
CPU 0 cache
    |
    +-- Task A's hot data
```

If Task A moves to CPU 1:

```text
CPU 1
    |
    +-- Task A
```

CPU 1 may need to fetch the data into its cache.

Therefore:

```text
Load balancing
       vs
Cache locality
```

must be balanced.

---

# 27. NUMA Scheduling

Large systems may have multiple NUMA nodes.

```text
NUMA Node 0
   |
 CPU 0
 CPU 1
 Memory 0

NUMA Node 1
   |
 CPU 2
 CPU 3
 Memory 1
```

Accessing local memory is generally cheaper than remote memory.

Therefore scheduler decisions can consider CPU and memory locality.

---

# 28. Real-Time Scheduling

Linux provides real-time scheduling policies such as:

```text
SCHED_FIFO
SCHED_RR
```

Real-time scheduling focuses on predictable priority-based execution rather than ordinary fairness.

---

# 29. `SCHED_FIFO`

FIFO means:

```text
First In, First Out
```

A real-time task can continue running until it:

```text
Blocks
Terminates
Yields
```

or is preempted by a higher-priority real-time task.

Example:

```text
Priority 90 → Task A
Priority 50 → Task B
```

If both are runnable:

```text
Task A
  ↓
runs first
```

---

# 30. `SCHED_RR`

Round-robin scheduling is used among real-time tasks of the same priority.

Example:

```text
Priority 80

Task A
Task B
Task C
```

Execution can rotate:

```text
A → B → C → A → B → C
```

according to the applicable time slice.

---

# 31. Real-Time Priority

Real-time scheduling uses a priority range separate from the normal nice-based scheduling model.

Conceptually:

```text
Normal tasks
    |
    v
Fair scheduling

Real-time tasks
    |
    v
RT priority
```

Do not confuse:

```text
nice
```

with:

```text
real-time priority
```

---

# 32. Priority Inversion

Consider:

```text
Low-priority task L
       |
       +-- holds lock

High-priority task H
       |
       +-- needs same lock
```

H blocks.

Now:

```text
Medium-priority task M
```

runs continuously.

Result:

```text
H → waiting
M → running
L → waiting to run
```

The high-priority task is indirectly delayed by a low-priority task.

This is:

```text
Priority Inversion
```

---

# 33. Priority Inheritance

One solution is priority inheritance.

```text
High-priority H
       |
       | waiting for lock
       v
Low-priority L
       |
       | temporarily inherits H's priority
       v
Runs
       |
       v
Releases lock
       |
       v
H runs
```

This reduces priority inversion.

---

# 34. Scheduling and Locks

Scheduling and synchronization interact heavily.

Example:

```text
Task A
 |
 +-- holds mutex
 |
 +-- gets preempted

Task B
 |
 +-- waits for mutex
```

If the critical section is long, B may experience significant latency.

Therefore:

> Lock design is also scheduling design.

---

# 35. Blocking vs Busy Waiting

### Busy waiting

```c
while (!ready)
    ;
```

CPU remains occupied.

### Blocking

```text
wait_event()
```

Task sleeps.

```text
Condition false
      |
      v
    Sleep
      |
      v
Another task runs
      |
      v
Event occurs
      |
      v
Wake task
```

Blocking is generally preferable for long waits.

---

# 36. Scheduler Wakeup

Suppose a sleeping task waits for I/O.

```text
Task A
 |
 v
Sleep
```

I/O completes:

```text
Device
  |
  v
Interrupt
  |
  v
Driver
  |
  v
Wake Task A
```

Task A becomes runnable.

The scheduler can then decide when it should execute.

---

# 37. Interrupts and Scheduling

An interrupt handler can cause a task to become runnable.

Example:

```text
Hardware
   |
   v
IRQ
   |
   v
Interrupt handler
   |
   v
Wake waiting task
   |
   v
Scheduler
```

This is very common in device drivers.

---

# 38. Kernel Threads

Kernel threads are also schedulable tasks.

```text
Kernel
 |
 +-- kthread A
 +-- kthread B
 +-- kthread C
```

They can be scheduled on CPUs just like other tasks, subject to their scheduling policy and constraints.

---

# 39. Workqueues and Scheduling

A workqueue defers kernel work to worker threads.

Typical driver flow:

```text
IRQ
 |
 | minimal processing
 v
Queue work
 |
 v
Kernel worker
 |
 v
Process deferred work
```

The worker itself is scheduled like a kernel task.

---

# 40. Scheduler Latency

Latency is the time between:

```text
Task becomes runnable
```

and:

```text
Task actually starts executing
```

Conceptually:

```text
Wakeup
  |
  | <--- scheduling latency --->
  |
  v
Task starts
```

Low latency is particularly important for:

```text
Real-time
Audio
Networking
Control systems
Interactive workloads
```

---

# 41. Throughput vs Latency

These can conflict.

### Throughput

Amount of useful work completed over time.

### Latency

How quickly a particular task gets service.

Example:

```text
Batch workload
    ↓
High throughput

Interactive workload
    ↓
Low latency
```

A scheduler must balance both.

---

# 42. CPU Utilization

A scheduler wants CPUs to remain productive.

Bad situation:

```text
CPU 0 → overloaded
CPU 1 → mostly idle
```

Better:

```text
CPU 0 → balanced
CPU 1 → balanced
```

But blindly moving tasks can hurt cache and NUMA locality.

---

# 43. Scheduler Tick

Older descriptions often say:

> Scheduler runs every timer tick.

This is an oversimplification.

Linux supports tickless operation and scheduler decisions can happen for many reasons:

```text
Timer events
Task wakeups
Blocking
Preemption
Load balancing
Priority changes
Explicit scheduling operations
```

For senior interviews, avoid saying scheduling happens only on timer ticks.

---

# 44. Tickless Linux

Linux supports tickless operation:

```text
NO_HZ
```

The kernel can reduce periodic timer interrupts when appropriate.

Benefits can include:

```text
Lower overhead
Lower power consumption
Better efficiency for idle CPUs
```

Exact behavior depends on kernel configuration and workload.

---

# 45. Scheduler and Power Management

Scheduling also interacts with power management.

The system may prefer:

```text
Fewer active CPUs
```

when workload is low.

At higher load:

```text
More CPUs become active
```

This can reduce energy consumption.

---

# 46. CPU Isolation

For latency-sensitive applications, some CPUs can be isolated from ordinary workloads.

Conceptually:

```text
CPU 0
 |
 +-- Normal workload

CPU 1
 |
 +-- Normal workload

CPU 2
 |
 +-- Real-time workload

CPU 3
 |
 +-- Real-time workload
```

The exact configuration depends on the Linux version and deployment.

---

# 47. Scheduling in High-Performance Networking

Networking workloads may use CPU affinity to improve locality.

Example:

```text
NIC Queue 0 → CPU 0
NIC Queue 1 → CPU 1
NIC Queue 2 → CPU 2
NIC Queue 3 → CPU 3
```

This can improve:

```text
Cache locality
Packet processing scalability
Predictability
```

This is highly relevant for networking/system roles.

---

# 48. Scheduling in Device Drivers

A driver may involve:

```text
IRQ
 |
 v
Interrupt handler
 |
 +-- Wake task
 |
 +-- Schedule work
       |
       v
Workqueue
       |
       v
CPU
```

Understanding this flow is more valuable than memorizing scheduler source code.

---

# 49. Scheduling Scenario

### Question

A task wakes up but takes several milliseconds to run. What would you investigate?

### Answer

Check:

```text
1. CPU affinity
2. CPU load
3. Higher-priority runnable tasks
4. Real-time tasks
5. Interrupt load
6. Lock contention
7. CPU isolation
8. Scheduler policy
9. NUMA placement
10. CPU frequency/power state
11. Long non-preemptible sections
12. Kernel configuration
```

---

# 50. Scheduling Scenario – CPU at 100%

### Question

One CPU is at 100% while another CPU is mostly idle.

Possible causes:

```text
CPU affinity restriction
Non-migratable work
Per-CPU workload
IRQ affinity
Task pinned to CPU
Uneven workload
```

Do not immediately conclude:

> Scheduler is broken.

Investigate affinity and workload distribution first.

---

# 51. Scheduling Scenario – High Context Switches

If context switches are extremely high, investigate:

```text
Too many runnable tasks
Very short time slices
Excessive thread creation
Frequent blocking/wakeup
Lock contention
Busy application design
```

High context switching can hurt performance through:

```text
Scheduler overhead
Cache disruption
TLB effects
```

---

# 52. Scheduling Scenario – Real-Time Latency

For a real-time task:

```text
Task wakes
   |
   v
Expected immediate execution
```

but latency is high.

Investigate:

```text
Higher-priority RT tasks
Interrupt storms
IRQ affinity
Lock contention
Priority inversion
CPU isolation
Preemption behavior
Long non-preemptible regions
```

---

# 53. Important Commands

### View processes

```bash
ps -eLf
```

### Interactive process view

```bash
top
```

or:

```bash
htop
```

### CPU affinity

```bash
taskset -p <pid>
```

### Set CPU affinity

```bash
taskset -cp 0 <pid>
```

### Process scheduling information

```bash
chrt -p <pid>
```

### Run with real-time policy

```bash
chrt
```

Use real-time policies carefully because an incorrectly configured real-time task can starve normal workloads.

---

# 54. `/proc` Scheduling Information

Useful information can be found under:

```text
/proc/<pid>/
```

For example:

```bash
cat /proc/<pid>/status
```

and:

```bash
cat /proc/<pid>/sched
```

These can help investigate scheduler behavior.

---

# 55. `sched_switch` Tracepoint

Linux tracing can expose scheduler switches.

Conceptually:

```text
Task A
   |
   | sched_switch
   v
Task B
```

Tracing scheduler events can help answer:

```text
Why did my task stop?
Which task ran instead?
How long was it waiting?
How often was it scheduled?
```

---

# 56. `perf sched`

Linux `perf` provides scheduler analysis facilities.

Useful concepts include:

```text
perf sched
```

which can help analyze:

```text
Scheduling latency
Task migrations
Context switches
Wakeups
```

---

# 57. Scheduler Debugging Method

For a scheduling problem:

```text
Problem
  |
  v
Is task runnable?
  |
  +-- NO → Why is it sleeping?
  |
  +-- YES
        |
        v
Which CPU?
        |
        v
CPU overloaded?
        |
        v
Higher-priority task?
        |
        v
Affinity restriction?
        |
        v
Lock contention?
        |
        v
Interrupt/IRQ load?
        |
        v
NUMA/cache issue?
```

This is a strong senior-level debugging approach.

---

# 58. Scheduler vs Synchronization

These topics are closely related but different.

### Scheduler

Answers:

```text
Who runs?
When?
On which CPU?
```

### Synchronization

Answers:

```text
Who can access shared data?
When can they access it?
How do we prevent races?
```

Together:

```text
             Linux
               |
       +-------+-------+
       |               |
       v               v
   Scheduler      Synchronization
       |               |
       v               v
   CPU usage       Shared state
```

---

# 59. Most Important Interview Questions

### Q1. What does the Linux scheduler do?

Selects runnable tasks for execution on CPUs according to scheduling policies and priorities.

---

### Q2. What is a context switch?

Switching CPU execution from one task to another by saving/restoring execution state and performing required scheduler/address-space work.

---

### Q3. What is preemption?

Stopping a currently running task so another task can execute.

---

### Q4. What is CPU affinity?

A restriction specifying which CPUs a task may run on.

---

### Q5. Why can task migration hurt performance?

Because the task may lose cache locality and, on NUMA systems, may execute farther from its memory.

---

### Q6. Difference between `SCHED_FIFO` and `SCHED_RR`?

```text
SCHED_FIFO
    Priority-based
    Runs until blocking/yield/preemption by higher-priority RT task

SCHED_RR
    Similar RT priority semantics
    Adds time slicing among equal-priority RT tasks
```

---

### Q7. What is priority inversion?

A high-priority task is indirectly delayed because a lower-priority task holds a resource it needs.

---

### Q8. How can priority inversion be reduced?

Priority inheritance or appropriate real-time synchronization mechanisms.

---

### Q9. What happens when a task blocks?

It becomes non-runnable and the scheduler can select another runnable task.

---

### Q10. What happens when an interrupt wakes a task?

Conceptually:

```text
IRQ
 ↓
Driver
 ↓
Wake task
 ↓
Task becomes runnable
 ↓
Scheduler decides when it runs
```

---

# 60. Senior Interview Mental Model

Memorize this:

```text
                    TASK
                      |
                      v
               Task State
                      |
              +-------+-------+
              |               |
           Runnable        Sleeping
              |               |
              v               |
          Scheduler           |
              |               |
      +-------+-------+       |
      |       |       |       |
      v       v       v       |
    CPU 0   CPU 1   CPU 2     |
      |                     |
      +------ Wakeup <-------+
              |
              v
           Runnable
```

And:

```text
Scheduler
   |
   +-- Scheduling class
   |
   +-- Priority / weight
   |
   +-- CPU affinity
   |
   +-- Load balancing
   |
   +-- Wakeups
   |
   +-- Preemption
   |
   +-- Migration
```

---

# 61. Final Summary

The Linux scheduling chain is:

```text
Task
  ↓
Task State
  ↓
Runnable?
  ↓
Scheduling Policy/Class
  ↓
Priority / Weight
  ↓
CPU Affinity
  ↓
Scheduler
  ↓
CPU Selection
  ↓
Context Switch
  ↓
Running
```

When the task blocks:

```text
Running
   ↓
Sleeping
   ↓
Event/I/O
   ↓
Wakeup
   ↓
Runnable
   ↓
Scheduler
   ↓
Running
```

For senior Linux interviews, master the interaction between:

```text
Scheduling
    +
Preemption
    +
Context switching
    +
CPU affinity
    +
SMP/load balancing
    +
Real-time scheduling
    +
Priority inversion
    +
Interrupts
    +
Synchronization
    +
NUMA/cache locality
```

These concepts are substantially more important than memorizing individual scheduler source-code functions.
