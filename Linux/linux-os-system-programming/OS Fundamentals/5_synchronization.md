# Chapter 4 — CPU Scheduling

> **Three-layer approach**
>
> This chapter covers CPU Scheduling from:
> 1. **[OS] Operating System concepts**
> 2. **[LSP] Linux System Programming**
> 3. **[KERNEL] Linux Kernel Internals**
>
> It also includes working/flows, C code, Linux commands, practical experiments, performance concepts, and senior-level interview questions.

---

# 1. What Is CPU Scheduling? [OS]

CPU scheduling is the mechanism used by an operating system to decide:

> **Which runnable execution unit should get the CPU next?**

In a multitasking system:

```text
Runnable tasks
     |
     +-- Task A
     +-- Task B
     +-- Task C
     +-- Task D
            |
            v
        Scheduler
            |
            v
           CPU
```

The scheduler tries to use CPU resources efficiently while providing appropriate responsiveness, fairness, throughput, and priority behavior.

---

# 2. Why Do We Need Scheduling? [OS]

A CPU can execute only a limited number of instructions simultaneously.

Suppose:

```text
CPU cores = 2

Runnable tasks:
A
B
C
D
E
```

The scheduler decides which tasks run on the available CPUs.

```text
             Scheduler
                 |
       +---------+---------+
       |                   |
       v                   v
     CPU 0               CPU 1
       |                   |
       v                   v
     Task A              Task B

Task C/D/E wait for CPU time
```

On a single CPU, only one task executes at an instant.

On multiple CPUs/cores, multiple tasks can execute simultaneously.

---

# 3. Program vs Process vs Thread [OS]

Scheduling happens at the level of executable entities.

```text
Program
   |
   v
Process
   |
   +-- Thread 1
   +-- Thread 2
   +-- Thread 3
```

Modern Linux schedules individual tasks/threads rather than treating a multithreaded process as one indivisible CPU execution unit.

Therefore:

```text
Process
  |
  +-- Thread A ----> schedulable
  +-- Thread B ----> schedulable
  +-- Thread C ----> schedulable
```

---

# 4. Basic CPU States [OS]

A simplified model:

```text
          +---------+
          |  Ready  |
          +---------+
               |
               | dispatch
               v
          +---------+
          | Running |
          +---------+
           /   |   \
          /    |    \
       I/O   preempt  exit
        |      |       |
        v      |       v
    +---------+ |  Terminated
    | Blocked | |
    +---------+ |
        |       |
        | event |
        +-------+
          Ready
```

Typical states:

```text
Ready
Running
Blocked/Waiting
Terminated
```

Exact kernel state representation is more detailed.

---

# 5. Ready vs Running [OS]

## Ready

The task can run but is waiting for CPU time.

```text
Task A -> Ready
```

## Running

The task is currently executing on a CPU.

```text
Task A -> Running
```

A preemptive scheduler can move:

```text
Running -> Ready
```

when another task should run.

---

# 6. CPU Burst and I/O Burst [OS]

A process/thread often alternates between CPU work and I/O waiting.

```text
CPU burst
    |
    v
I/O request
    |
    v
I/O wait
    |
    v
CPU burst
    |
    v
I/O request
```

Example:

```text
Read file
   |
   v
Process data
   |
   v
Read network data
   |
   v
Process data
```

This distinction is important for scheduling.

---

# 7. CPU-Bound vs I/O-Bound Tasks [OS]

## CPU-bound

Most time is spent using the CPU.

Examples:

```text
Compression
Encryption
Scientific computation
Image processing
```

Pattern:

```text
CPU ======== CPU ======== CPU
```

## I/O-bound

Frequently waits for I/O.

Examples:

```text
Network server
Database client
File processing
```

Pattern:

```text
CPU == I/O wait == CPU == I/O wait
```

A good scheduler should keep the CPU busy while also maintaining responsiveness.

---

# 8. Scheduling Goals [OS]

Important scheduling metrics:

```text
CPU utilization
Throughput
Turnaround time
Waiting time
Response time
Fairness
Latency
Priority guarantees
```

---

# 9. CPU Utilization [OS]

CPU utilization:

> Percentage of time the CPU is doing useful work.

Conceptually:

```text
CPU utilization =
busy time / total time
```

High utilization is generally desirable, but:

> 100% CPU utilization is not automatically a sign of a healthy system.

A system can have 100% CPU while suffering from poor latency, excessive contention, or inefficient work.

---

# 10. Throughput [OS]

Throughput:

> Number of tasks completed per unit time.

Example:

```text
100 requests / second
```

Higher throughput is generally desirable for batch/server workloads.

---

# 11. Turnaround Time [OS]

Turnaround time:

```text
Completion time - Arrival time
```

Example:

```text
Arrival    = 2
Completion = 10

Turnaround = 10 - 2 = 8
```

It measures total time from submission/arrival until completion.

---

# 12. Waiting Time [OS]

Waiting time is the time a task spends waiting in the ready queue for CPU service.

A common relationship:

```text
Waiting Time =
Turnaround Time - CPU execution time
```

Depending on the model, I/O waiting is not counted as ready-queue waiting.

---

# 13. Response Time [OS]

Response time:

```text
First CPU service/start time - Arrival time
```

Example:

```text
Arrival = 0
First execution = 3

Response time = 3
```

This is especially important for interactive systems.

---

# 14. Example of the Three Main Timing Metrics

Suppose:

```text
Arrival time = 2
First execution = 5
Completion = 12
CPU burst = 4
```

Then:

```text
Response time
= 5 - 2
= 3

Turnaround time
= 12 - 2
= 10

Waiting time
= 10 - 4
= 6
```

---

# 15. Scheduling: Preemptive vs Non-Preemptive [OS]

## Non-preemptive

Once a task gets the CPU, it keeps it until:

```text
terminates
or
blocks/waits
```

Example:

```text
Task A running
     |
     +-- finishes/block
     |
     v
Task B runs
```

## Preemptive

The OS can interrupt a running task and schedule another runnable task.

```text
Task A running
     |
     | timer/priority event
     v
Task A -> Ready
Task B -> Running
```

Modern general-purpose operating systems are preemptive.

---

# 16. Dispatcher [OS]

The **dispatcher** is the mechanism that gives the CPU to the task selected by the scheduler.

Conceptually:

```text
Scheduler
   |
   | selects Task B
   v
Dispatcher
   |
   | context switch
   v
Task B runs
```

The dispatcher may perform:

```text
Context switch
Switch to appropriate execution context
Transfer CPU control
```

---

# 17. Context Switch [OS]

A context switch occurs when execution changes from one task to another.

```text
Task A
  |
  | save execution state
  v
Scheduler
  |
  | select Task B
  v
restore Task B state
  |
  v
Task B
```

State can include architecture-dependent:

```text
Registers
Program counter
Stack pointer
Processor state
Scheduling state
Memory-management context where applicable
```

Context switching has overhead.

---

# 18. Why Context Switching Costs Time [OS]

The CPU is not doing application work during portions of a context switch.

There may be:

```text
Register save/restore
Scheduler work
Cache disruption
TLB/address-space effects
Kernel bookkeeping
```

Modern CPUs and kernels optimize these operations heavily.

Therefore:

```text
Too many switches
       |
       v
More overhead
       |
       v
Less useful application work
```

---

# 19. Dispatch Latency [OS]

Dispatch latency is the time required to stop one task and start another.

For interactive/real-time workloads, low dispatch latency can be important.

Conceptually:

```text
Task A stops
    |
    | dispatch latency
    v
Task B starts
```

---

# 20. FCFS — First Come First Served [OS]

FCFS schedules tasks in arrival order.

Example:

```text
A -> B -> C
```

If:

```text
A = 8 ms
B = 2 ms
C = 1 ms
```

Then:

```text
|---- A ----|-- B --|-C-|
0           8       10  11
```

Advantages:

```text
Simple
Easy to implement
Predictable ordering
```

Problem:

> Convoy effect.

---

# 21. Convoy Effect [OS]

Suppose:

```text
A = long CPU task
B = short task
C = short task
D = short task
```

FCFS:

```text
A--------------------|B|C|D|
```

B/C/D wait behind A.

This can produce poor response time for short tasks.

This is the **convoy effect**.

---

# 22. SJF — Shortest Job First [OS]

SJF chooses the task with the shortest CPU burst.

Example:

```text
A = 8
B = 2
C = 1
```

Order:

```text
C -> B -> A
```

Advantages:

```text
Good average waiting time
Good average turnaround time
```

Important theoretical result:

> If CPU burst lengths are known accurately, non-preemptive SJF minimizes average waiting time for a set of jobs.

Practical problem:

> The OS generally does not know the future CPU burst exactly.

---

# 23. SRTF — Shortest Remaining Time First [OS]

SRTF is the preemptive form of shortest-job scheduling.

The scheduler chooses the task with the shortest remaining CPU time.

Example:

```text
A running
 |
 | B arrives with shorter remaining time
 v
A -> Ready
B -> Running
```

Advantages:

```text
Good response for short jobs
Can reduce average waiting time
```

Problems:

```text
More preemption
Need burst estimates
Long jobs may starve
```

---

# 24. Priority Scheduling [OS]

Each task gets a priority.

```text
Task A -> priority 5
Task B -> priority 1
Task C -> priority 10
```

Depending on the system:

```text
higher number = higher priority
```

or the opposite.

Always check the API/system semantics.

High-priority tasks are favored.

---

# 25. Starvation [OS]

Starvation occurs when a task waits indefinitely or for an excessively long time because other tasks continually receive service first.

Example:

```text
High priority
High priority
High priority
High priority
...
Low priority -> keeps waiting
```

---

# 26. Aging [OS]

Aging gradually increases the priority of a waiting task.

```text
Low priority
    |
    | waits
    v
priority increases
    |
    v
eventually scheduled
```

Aging is one way to reduce starvation.

---

# 27. Round Robin [OS]

Round Robin gives each runnable task a time quantum.

Example:

```text
A -> B -> C -> A -> B -> C
```

Suppose:

```text
Quantum = 10 ms
```

Timeline:

```text
| A | B | C | A | B | C |
```

Advantages:

```text
Good responsiveness
Simple
Fair time sharing
```

---

# 28. Choosing the Round-Robin Quantum [OS]

If quantum is too small:

```text
Many context switches
High overhead
```

If quantum is too large:

```text
RR approaches FCFS
Poor responsiveness
```

Therefore:

```text
small quantum
    |
    +-- responsiveness
    +-- more overhead

large quantum
    |
    +-- less switching
    +-- worse interactive response
```

---

# 29. Multilevel Queue [OS]

Tasks are divided into queues.

Example:

```text
+----------------------+
| System / high        |
+----------------------+
| Interactive          |
+----------------------+
| Batch                |
+----------------------+
```

Each queue may have its own scheduling policy.

---

# 30. Multilevel Feedback Queue — MLFQ [OS]

MLFQ allows tasks to move between queues based on behavior.

Conceptually:

```text
High priority
     |
     v
+---------+
| Queue 1 |
+---------+
     |
     | CPU-heavy behavior
     v
+---------+
| Queue 2 |
+---------+
     |
     v
+---------+
| Queue 3 |
+---------+
```

Interactive tasks may remain favored while CPU-heavy tasks may move lower.

MLFQ attempts to balance:

```text
response
fairness
throughput
```

---

# 31. Scheduling Algorithm Comparison [OS]

| Algorithm | Preemptive? | Main Advantage | Main Problem |
|---|---:|---|---|
| FCFS | No | Simple | Convoy effect |
| SJF | No | Low average waiting | Burst prediction |
| SRTF | Yes | Good short-job response | Starvation/preemption |
| Priority | Either | Priority control | Starvation |
| Round Robin | Yes | Responsiveness | Quantum tuning |
| MLFQ | Usually | Adaptive behavior | Complexity |

---

# 32. Important Interview Distinction: Fairness vs Priority [OS]

Fairness does not always mean:

```text
Everyone gets exactly equal CPU time.
```

A scheduler may intentionally give different service based on:

```text
Priority
Policy
Nice value
Real-time requirements
Scheduling class
CPU affinity
```

A good answer should distinguish:

```text
fairness
vs
priority
vs
latency
```

---

# 33. Linux Scheduling — Big Picture [KERNEL]

Linux scheduling can be visualized as:

```text
                 Runnable Tasks
                      |
          +-----------+-----------+
          |                       |
          v                       v
     Scheduling classes      CPU affinity
          |
          v
       Scheduler
          |
          v
      Run queue(s)
          |
          v
       CPU core
```

The exact implementation is kernel-version dependent.

For interview preparation, understand the concepts rather than memorizing old kernel internals as if they were permanent.

---

# 34. Linux `task_struct` and Scheduling [KERNEL]

Linux represents a schedulable task with:

```c
struct task_struct
```

It contains or references scheduling-related state.

Conceptually:

```text
task_struct
 |
 +-- state
 +-- scheduling information
 +-- priority-related fields
 +-- CPU/scheduling information
 +-- relationships
 +-- memory/resource references
```

Exact fields change across Linux versions.

---

# 35. Runnable Task [KERNEL]

A runnable task is a task that can execute when selected by the scheduler.

Conceptually:

```text
Runnable
   |
   v
Scheduler
   |
   v
CPU
```

A task blocked on I/O is not competing for CPU in the same way as a runnable task.

---

# 36. Run Queue [KERNEL]

A run queue is the scheduler's structure for tracking runnable tasks associated with a CPU/scheduling context.

Conceptually:

```text
CPU 0
 |
 +-- Run queue
 |     +-- Task A
 |     +-- Task B
 |     +-- Task C
 |
 +-- Scheduler
```

Modern Linux has per-CPU scheduling structures and more complex scheduling-class-specific data structures.

---

# 37. Per-CPU Scheduling [KERNEL]

A multicore machine may conceptually look like:

```text
CPU 0                  CPU 1
 |                      |
Run queue               Run queue
 |                      |
A B C                    D E F
```

Tasks can migrate between CPUs.

Why?

```text
Load balancing
CPU affinity
Cache locality
Scheduling policy
```

---

# 38. CPU Affinity [KERNEL/LSP]

CPU affinity specifies which CPUs a task is allowed to run on.

Example:

```text
Task A
 |
 +-- allowed CPUs: {0,1}
```

It cannot run on:

```text
CPU 2
CPU 3
```

if they are outside its allowed mask.

Linux commands:

```bash
taskset -p <pid>
```

Set affinity:

```bash
taskset -cp 0 <pid>
```

This is useful for experiments and performance debugging.

---

# 39. Linux Scheduling Classes [KERNEL]

Linux supports different scheduling classes.

Important conceptual classes include:

```text
Stop
Deadline
Real-time
Fair
Idle
```

The exact ordering and implementation details are kernel-version dependent.

For general interview preparation, know:

```text
SCHED_DEADLINE
SCHED_FIFO
SCHED_RR
SCHED_OTHER
SCHED_IDLE
```

and understand that these policies have different semantics.

---

# 40. Normal/Fair Scheduling [KERNEL]

Normal tasks commonly use:

```text
SCHED_OTHER
```

Historically, Linux used the **Completely Fair Scheduler (CFS)** for normal scheduling.

Important interview caveat:

> Modern Linux kernels have evolved beyond a simple "Linux always uses CFS" description. Recent kernels use newer fair-scheduling implementations, including EEVDF-based scheduling in relevant mainline versions. Therefore, describe CFS as the historical/foundational model and understand the modern fair-scheduling direction as well.

---

# 41. CFS — Historical/Fundamental Model [KERNEL]

CFS was designed around the idea of approximating fair CPU sharing.

A simplified model:

```text
Runnable tasks
      |
      v
virtual runtime
      |
      v
choose task with smallest vruntime
      |
      v
CPU
```

The key idea:

> A task that has received less fair CPU service should become more eligible to run.

Do not treat the old implementation details as universal across modern kernels.

---

# 42. `vruntime` [KERNEL]

In the traditional CFS model, each runnable task has a virtual runtime.

Conceptually:

```text
Task A -> vruntime = 10
Task B -> vruntime = 20
Task C -> vruntime = 15
```

The task with the smallest virtual runtime is favored.

Priority/nice affects how quickly virtual runtime accumulates.

This is a conceptual explanation of CFS.

---

# 43. EEVDF — Modern Fair Scheduling Direction [KERNEL]

Modern Linux scheduling has evolved toward **EEVDF**:

```text
Earliest Eligible Virtual Deadline First
```

The scheduler considers concepts such as:

```text
virtual deadline
eligibility
lag
runtime
```

For interview purposes:

```text
Old/foundational:
CFS + vruntime

Modern:
EEVDF-based fair scheduling
```

Do not answer:

> "Current Linux scheduler is simply CFS"

without qualification.

---

# 44. Nice Value [LSP/OS]

Nice value influences the scheduling weight of normal tasks.

Typical range on Linux:

```text
-20 ... +19
```

Lower nice:

```text
higher scheduling preference
```

Higher nice:

```text
lower scheduling preference
```

Example:

```bash
nice -n 10 ./program
```

---

# 45. `nice()` [LSP]

C example:

```c
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(void)
{
    errno = 0;

    int old = nice(0);

    if (old == -1 && errno != 0)
    {
        perror("nice");
        return 1;
    }

    printf("Current nice value = %d\n", old);

    return 0;
}
```

To change nice value:

```c
nice(5);
```

Increasing niceness generally reduces scheduling preference for normal tasks.

Permissions may be required when moving toward higher priority/lower nice values.

---

# 46. `getpriority()` and `setpriority()` [LSP]

Linux/POSIX interfaces include:

```c
getpriority()
setpriority()
```

Example:

```c
#include <stdio.h>
#include <sys/resource.h>

int main(void)
{
    int p = getpriority(PRIO_PROCESS, 0);

    printf("Priority = %d\n", p);

    return 0;
}
```

Note:

`getpriority()` has special return/error semantics; robust code should check `errno` appropriately.

---

# 47. `sched_yield()` [LSP]

```c
sched_yield();
```

It voluntarily gives up the processor.

Conceptually:

```text
Thread running
      |
      | sched_yield()
      v
Scheduler
      |
      v
another runnable task may run
```

Important:

> `sched_yield()` does not guarantee that a specific other thread will run next.

It should not be used as a general synchronization mechanism.

---

# 48. `sched_getscheduler()` [LSP]

You can inspect the scheduling policy:

```c
#include <stdio.h>
#include <sched.h>
#include <unistd.h>

int main(void)
{
    int policy = sched_getscheduler(0);

    printf("Policy = %d\n", policy);

    return 0;
}
```

Common policy constants:

```text
SCHED_OTHER
SCHED_FIFO
SCHED_RR
SCHED_BATCH
SCHED_IDLE
SCHED_DEADLINE
```

Not every policy is available through every interface in the same way.

---

# 49. Real-Time Scheduling [OS/KERNEL]

Linux provides real-time scheduling policies such as:

```text
SCHED_FIFO
SCHED_RR
SCHED_DEADLINE
```

These are intended for workloads with stronger latency/timing requirements.

They must be used carefully because a high-priority real-time task can starve ordinary tasks.

---

# 50. `SCHED_FIFO` [OS/KERNEL]

A simplified model:

```text
High-priority FIFO task
        |
        v
runs until:
    - blocks
    - yields
    - exits
    - is preempted by higher-priority RT task
```

A running FIFO task is not normally time-sliced with another equal-priority FIFO task in the same way as Round Robin.

---

# 51. `SCHED_RR` [OS/KERNEL]

`SCHED_RR` provides round-robin behavior among equal-priority real-time tasks.

```text
RT Task A
    |
    | quantum
    v
RT Task B
    |
    | quantum
    v
RT Task C
```

Higher-priority real-time tasks can preempt lower-priority ones.

---

# 52. `SCHED_DEADLINE` [KERNEL]

`SCHED_DEADLINE` is designed for deadline-oriented real-time workloads.

Conceptually:

```text
Task
 |
 +-- runtime
 +-- period
 +-- deadline
```

The scheduler tries to meet timing constraints under the admission/control rules.

For interviews, know the concept and the existence of the policy rather than memorizing kernel implementation details.

---

# 53. `chrt` Command [LSP]

Inspect/change real-time scheduling policy:

```bash
chrt -p <pid>
```

Example:

```bash
chrt -r 10 ./program
```

This requests:

```text
SCHED_RR
priority 10
```

Use real-time policies carefully.

---

# 54. `ps` Scheduling Information [LSP]

Useful:

```bash
ps -eo pid,tid,cls,rtprio,ni,pri,psr,stat,comm
```

This can help inspect:

```text
PID
TID
scheduling class
real-time priority
nice value
priority
processor
state
command
```

Field availability can vary by system.

---

# 55. `top` Scheduling Information [LSP]

Run:

```bash
top
```

Useful columns include:

```text
PR
NI
S
%CPU
P
```

Meanings can include:

```text
PR -> priority representation
NI -> nice value
S  -> process state
%CPU -> CPU utilization
P  -> last-used CPU
```

Exact display depends on `top` version/configuration.

---

# 56. `htop`

If installed:

```bash
htop
```

It provides an interactive view of:

```text
CPU usage
Threads
Priorities
CPU assignment
Process state
```

Useful for practical scheduler observation.

---

# 57. CPU Migration [KERNEL]

Suppose:

```text
CPU 0 -> overloaded
CPU 1 -> lightly loaded
```

The scheduler may move a task:

```text
CPU 0 run queue
      |
      | migration/load balancing
      v
CPU 1 run queue
```

Migration has costs:

```text
Cache locality loss
Migration overhead
NUMA effects
```

Therefore schedulers balance load while trying to preserve locality.

---

# 58. CPU Affinity vs Load Balancing [KERNEL]

Affinity restricts where a task may execute.

Load balancing tries to distribute work.

Example:

```text
Task A affinity = CPU 0 only
```

Then:

```text
CPU 1 cannot take Task A
```

even if CPU 1 is idle.

This can intentionally or accidentally create imbalance.

---

# 59. NUMA and Scheduling [KERNEL]

On NUMA systems:

```text
CPU 0 ---- Memory Node 0
CPU 1 ---- Memory Node 0

CPU 2 ---- Memory Node 1
CPU 3 ---- Memory Node 1
```

Running a thread close to the memory it accesses can improve performance.

Therefore advanced scheduling involves:

```text
CPU affinity
Cache locality
NUMA locality
Memory placement
```

---

# 60. Preemption [KERNEL]

A running task may be preempted when the scheduler determines another task should run.

Simplified:

```text
Task A running
     |
     | preemption event
     v
scheduler
     |
     v
Task B running
```

Possible triggers include:

```text
Timer/scheduling event
Higher-priority task becomes runnable
Blocking
Wakeup
Explicit yield
Other kernel scheduling events
```

The exact path is architecture and kernel-version dependent.

---

# 61. Wakeup Path [KERNEL]

Suppose a task is blocked waiting for I/O:

```text
Task A
 |
 v
sleep/wait
 |
 v
I/O completes
 |
 v
wake up Task A
 |
 v
Task becomes runnable
 |
 v
scheduler considers it
```

The scheduler may choose it immediately or another task depending on policy and state.

---

# 62. Sleeping Does Not Mean CPU Consumption [OS]

If a thread is waiting for I/O:

```text
Thread
  |
  v
Blocked
```

it generally does not continuously consume CPU while sleeping.

This is why I/O-bound workloads can have many waiting threads without every thread using a full CPU.

---

# 63. Voluntary vs Involuntary Context Switch [LSP]

Linux tools may report:

```text
voluntary context switches
involuntary context switches
```

Conceptually:

### Voluntary

Task gives up CPU because it blocks or waits.

```text
read()
   |
   v
wait for I/O
```

### Involuntary

Task is preempted by the scheduler.

```text
Task A running
   |
   v
preempted
```

Inspect:

```bash
cat /proc/<pid>/status
```

Look for context-switch counters where available.

---

# 64. `/proc/<pid>/sched` [LSP/KERNEL]

Linux exposes scheduler-related information through:

```bash
cat /proc/<pid>/sched
```

This is useful for studying:

```text
scheduler statistics
runtime
switch counts
policy-related information
```

Exact fields vary by kernel version/configuration.

---

# 65. `/proc/schedstat` [KERNEL]

Linux may expose scheduler statistics through:

```bash
cat /proc/schedstat
```

Availability depends on kernel configuration/version.

This is useful when studying scheduler behavior at system level.

---

# 66. Measuring Context Switches with `pidstat`

If `sysstat` is installed:

```bash
pidstat -w -p <pid> 1
```

Useful fields include:

```text
cswch/s
nvcswch/s
```

Conceptually:

```text
cswch/s  -> voluntary context switches
nvcswch/s -> involuntary context switches
```

---

# 67. Performance Tool: `perf`

A useful command:

```bash
perf stat ./program
```

It can provide performance counters/statistics.

For context-switch-related measurements, depending on permissions/kernel:

```bash
perf stat -e context-switches,cpu-migrations,task-clock ./program
```

This is an important senior Linux performance tool.

---

# 68. Scheduling Experiment — CPU-Bound Threads

Create several CPU-bound threads:

```c
#include <pthread.h>
#include <stdio.h>

void *worker(void *arg)
{
    volatile unsigned long long x = 0;

    for (;;)
        x++;

    return NULL;
}

int main(void)
{
    pthread_t t[8];

    for (int i = 0; i < 8; ++i)
        pthread_create(&t[i], NULL, worker, NULL);

    for (int i = 0; i < 8; ++i)
        pthread_join(t[i], NULL);

    return 0;
}
```

Run:

```bash
gcc cpu_threads.c -pthread -O2 -o cpu_threads
./cpu_threads
```

Observe:

```bash
top -H -p <pid>
```

Then compare with:

```bash
nproc
```

You can study what happens when runnable threads exceed available CPUs.

---

# 69. Scheduling Experiment — Affinity

Run:

```bash
taskset -c 0 ./cpu_threads
```

This restricts the process to CPU 0.

Observe:

```bash
top
```

or:

```bash
ps -o pid,psr,pcpu,comm -p <pid>
```

Then compare with:

```bash
taskset -c 0-3 ./cpu_threads
```

This demonstrates the impact of CPU affinity.

---

# 70. Scheduling Experiment — Nice

Run:

```bash
nice -n 10 ./cpu_threads
```

Compare against:

```bash
./cpu_threads
```

Use:

```bash
ps -o pid,ni,pri,pcpu,comm -p <pid>
```

Remember:

> Nice affects normal scheduling preference; it is not equivalent to real-time priority.

---

# 71. Scheduling Experiment — Yield

Example:

```c
#include <stdio.h>
#include <sched.h>

int main(void)
{
    for (int i = 0; i < 1000000; ++i)
    {
        /* Work */

        sched_yield();
    }

    return 0;
}
```

Do not assume this improves performance.

In many workloads, excessive yielding can increase scheduling overhead.

---

# 72. Why `sleep()` Is Not a Scheduling Primitive

This is a common mistake:

```c
sleep(1);
```

does not mean:

> "Give exactly one second of CPU time to another thread."

It means the calling thread requests to sleep for approximately that duration.

The scheduler remains in control.

Likewise:

```c
sched_yield();
```

does not guarantee which thread runs next.

---

# 73. Scheduler and Synchronization [OS/KERNEL]

Scheduling and synchronization are closely related.

Example:

```text
Thread A
   |
   | lock(mutex)
   v
critical section

Thread B
   |
   | lock(mutex)
   v
blocked
```

The scheduler runs another runnable task while B waits.

Chapter 5 will connect:

```text
Scheduler
   +
Mutex
   +
Condition variable
   +
Semaphore
```

---

# 74. Priority Inversion [OS/KERNEL]

Suppose:

```text
High-priority Thread H
Low-priority Thread L
Medium-priority Thread M
```

L holds a lock needed by H:

```text
L -> holds mutex
H -> waits for mutex
M -> keeps running
```

Then:

```text
H cannot run
because L cannot run
because M keeps getting CPU
```

This is **priority inversion**.

A common mitigation is **priority inheritance**.

This topic becomes important in real-time systems.

---

# 75. Priority Inheritance [OS/KERNEL]

Conceptually:

```text
Low-priority L owns lock
High-priority H waits for lock

L temporarily inherits H's priority
          |
          v
L runs and releases lock
          |
          v
H acquires lock
```

This reduces priority inversion.

---

# 76. Scheduler and Mutex Contention

Suppose 32 threads all fight for one mutex:

```text
T1 \
T2  \
T3   \
...   +--> Mutex
T32  /
```

Only one can enter the critical section.

More threads do not necessarily improve performance.

You may get:

```text
Lock contention
Context switches
Cache-line bouncing
Scheduler overhead
```

This is a common performance issue in multithreaded applications.

---

# 77. Amdahl's Law and Threads [OS]

Suppose:

```text
90% parallel
10% serial
```

Even with many CPUs, the serial portion limits speedup.

Amdahl's Law:

```text
Speedup = 1 / (S + P/N)
```

where:

```text
S = serial fraction
P = parallel fraction
N = processors
```

For:

```text
S = 0.1
P = 0.9
N -> infinity
```

maximum theoretical speedup:

```text
1 / 0.1 = 10
```

This explains why "more threads" does not imply unlimited performance.

---

# 78. Oversubscription [OS]

Oversubscription occurs when there are significantly more CPU-bound runnable threads than available logical CPUs.

Example:

```text
CPU = 8
Runnable CPU-bound threads = 100
```

Potential effects:

```text
More context switches
Cache disruption
Scheduler overhead
Lower throughput
Higher latency
```

For I/O-bound workloads, having more threads can sometimes be useful because many threads may be blocked.

---

# 79. False Sharing [OS/Performance]

Two threads may update different variables that happen to share the same CPU cache line.

Conceptually:

```text
Cache line
+-----------------------------+
| counterA | counterB         |
+-----------------------------+
     ^           ^
   CPU 0       CPU 1
```

Even though variables are logically independent, cache coherence traffic can hurt performance.

This is not primarily a scheduler algorithm problem, but it is important in multithreaded performance analysis.

---

# 80. Scheduler Interview Question — What Happens When a Thread Blocks?

Strong answer:

```text
Thread performs blocking operation
        |
        v
Thread cannot continue
        |
        v
Kernel puts task into appropriate wait state
        |
        v
Scheduler selects another runnable task
        |
        v
CPU executes another task
        |
        v
Event/I/O completes
        |
        v
blocked task becomes runnable
        |
        v
scheduler can consider it again
```

---

# 81. Scheduler Interview Question — What Happens When a Higher-Priority Task Wakes Up?

Conceptually:

```text
Task A running
      |
      | higher-priority Task B wakes
      v
Scheduler evaluates priorities/policy
      |
      v
Task B may preempt Task A
```

The exact preemption behavior depends on the scheduling class and kernel state.

---

# 82. Scheduler Interview Question — Why Is Context Switching Expensive?

Strong answer:

> A context switch requires changing execution context and performing kernel bookkeeping. Depending on the switch, this can involve register state changes, scheduler work, cache disruption, and possibly address-space/TLB-related effects. Modern hardware and Linux optimize these operations, but excessive switching can still reduce useful application throughput.

---

# 83. Scheduler Interview Question — Process vs Thread Scheduling

A good senior answer:

> Linux schedules tasks, and threads are represented as schedulable tasks. Threads in the same process can therefore be scheduled independently and can execute on different CPUs. They may share the same memory-management context, which can make a switch between sibling threads different from a switch between tasks using different address spaces.

---

# 84. Scheduler Interview Question — CFS vs EEVDF

A strong current answer:

> CFS is the foundational Linux fair-scheduling model and introduced concepts such as virtual runtime. Modern Linux has evolved toward EEVDF, which uses eligibility and virtual deadlines to make fair-scheduling decisions. So for interviews I would know both: CFS for understanding the historical Linux scheduler model and EEVDF for current scheduler direction.

---

# 85. Scheduler Interview Question — What Is `nice`?

A good answer:

> Nice is a user-visible parameter that influences the relative scheduling weight of normal tasks. On Linux, the usual range is -20 to +19, with lower values representing greater scheduling preference. It is not the same thing as real-time priority.

---

# 86. Scheduler Interview Question — What Is CPU Affinity?

A good answer:

> CPU affinity restricts a task to a set of allowed CPUs. It can be used for performance tuning, cache locality, isolation, and real-time workloads, but overly restrictive affinity can also cause load imbalance.

---

# 87. Scheduler Interview Question — What Is Starvation?

> Starvation occurs when a runnable task receives insufficient CPU service for an unacceptably long time because other tasks continually receive preference. Priority scheduling can cause starvation; techniques such as aging can reduce it.

---

# 88. Scheduler Interview Question — What Is Priority Inversion?

> Priority inversion occurs when a high-priority task is blocked by a lower-priority task holding a resource, while medium-priority work prevents the lower-priority task from running and releasing the resource. Priority inheritance is one common mitigation.

---

# 89. Scheduler Interview Question — Does 100 Threads Mean 100-Way Parallelism?

No.

If:

```text
logical CPUs = 8
runnable CPU-bound threads = 100
```

only a limited number can execute simultaneously.

The remaining threads compete for CPU time.

Therefore:

```text
Threads != CPUs
Threads != parallelism
```

---

# 90. Scheduler Interview Question — Why Can More Threads Make a Program Slower?

Because of:

```text
Context switching
Lock contention
Cache misses
False sharing
Scheduler overhead
Memory overhead
CPU migration
NUMA effects
Oversubscription
```

A well-designed system chooses concurrency based on workload and hardware.

---

# 91. Three-Layer Summary

## [OS]

Know:

```text
CPU scheduling
Ready/running/blocked
CPU burst
I/O burst
Preemption
Dispatcher
Context switch
FCFS
SJF
SRTF
Priority
Round Robin
MLFQ
Starvation
Aging
Throughput
Waiting time
Turnaround time
Response time
```

## [LSP]

Know:

```text
nice()
getpriority()
setpriority()
sched_yield()
sched_getscheduler()
sched_getparam()
sched_setparam()
sched_setscheduler()
sched_getaffinity()
sched_setaffinity()

taskset
chrt
ps
top
htop
pidstat
perf
/proc/<pid>/sched
```

## [KERNEL]

Know:

```text
task_struct
runnable task
scheduler
run queue
per-CPU scheduling
scheduling classes
SCHED_OTHER
SCHED_FIFO
SCHED_RR
SCHED_DEADLINE
nice/weight
CFS
vruntime
EEVDF
preemption
wakeup
task migration
CPU affinity
NUMA
priority inversion
priority inheritance
context switch
```

---

# 92. Important Commands Cheat Sheet

```bash
# CPU count
nproc

# Show process scheduling information
ps -eo pid,tid,cls,rtprio,ni,pri,psr,stat,comm

# Show threads
ps -T -p <pid>

# Show all threads
ps -eLf

# Interactive process view
top

# Thread view
top -H -p <pid>

# Interactive view
htop

# Inspect scheduling information
cat /proc/<pid>/sched

# Inspect thread directories
ls /proc/<pid>/task

# Set process affinity
taskset -cp 0 <pid>

# Start process on selected CPU
taskset -c 0 ./program

# Inspect scheduling policy
chrt -p <pid>

# Run with Round Robin
chrt -r 10 ./program

# Run with a nice value
nice -n 10 ./program

# Change priority of an existing process
renice 10 -p <pid>

# Scheduler statistics
pidstat -w -p <pid> 1

# Performance statistics
perf stat ./program

# Context switch/migration counters
perf stat -e context-switches,cpu-migrations,task-clock ./program
```

---

# 93. Practice Problems

## OS Theory

1. Calculate waiting time for FCFS.
2. Calculate turnaround time for SJF.
3. Compare SJF and SRTF.
4. Calculate Round Robin waiting time for different quanta.
5. Explain convoy effect.
6. Explain starvation.
7. Explain aging.
8. Compare preemptive and non-preemptive scheduling.
9. Explain response vs turnaround time.
10. Explain priority inversion.

## Linux Programming

11. Print current scheduling policy.
12. Change nice value.
13. Print current nice value.
14. Call `sched_yield()`.
15. Inspect process CPU affinity.
16. Set CPU affinity from C.
17. Create CPU-bound threads.
18. Measure context switches using `pidstat`.
19. Compare affinity settings.
20. Compare normal and nice-adjusted workloads.

## Kernel Internals

21. Explain `task_struct`.
22. Explain runnable task.
23. Explain run queue.
24. Explain per-CPU scheduling.
25. Explain task migration.
26. Explain preemption.
27. Explain wakeup.
28. Explain CFS/vruntime.
29. Explain EEVDF at a high level.
30. Explain real-time scheduling classes.

---

# 94. Practical Senior-Level Exercise

Build a program that:

```text
1. Creates 4 CPU-bound threads
2. Prints PID/TID
3. Prints CPU number using sched_getcpu()
4. Measures execution time
5. Runs once normally
6. Runs once with CPU affinity
7. Runs once with a changed nice value
8. Observes it using top -H
9. Measures context switches using pidstat
10. Measures performance using perf
```

Then answer:

```text
Why did CPU utilization change?
Why did context switches change?
Why did affinity change performance?
Why can restricting CPUs hurt?
Why doesn't increasing thread count always help?
```

This is much more valuable for a senior Linux interview than memorizing scheduling algorithms alone.

---

# 95. Final Mental Model

```text
                         RUNNABLE TASKS
                              |
                +-------------+-------------+
                |             |             |
              Task A        Task B        Task C
                |             |             |
                +-------------+-------------+
                              |
                              v
                         SCHEDULER
                              |
          +-------------------+-------------------+
          |                   |                   |
          v                   v                   v
       CPU 0                CPU 1               CPU 2
          |                   |                   |
       Task A               Task B               Task C
          |
          v
    Context switch /
    preemption /
    wakeup /
    migration
          |
          v
       scheduler
```

Linux mental model:

```text
                  task_struct
                       |
                       v
                 schedulable task
                       |
              +--------+--------+
              |                 |
              v                 v
         Runnable            Blocked
              |                 |
              v                 |
        Scheduling class        |
              |                 |
              v                 |
        Run queue / CPU         |
              |                 |
              +--------+--------+
                       |
                       v
                    CPU
                       |
                       v
                  execution
```

---

# Chapter 4 — Key Takeaways

1. CPU scheduling chooses which runnable task executes.
2. Linux schedules individual tasks/threads.
3. Ready means runnable but waiting for CPU.
4. Running means currently executing.
5. Blocked means waiting for an event/resource.
6. CPU-bound and I/O-bound workloads behave differently.
7. Important metrics are utilization, throughput, waiting time, turnaround time, and response time.
8. Preemptive scheduling allows the OS to interrupt a running task.
9. Context switching has overhead.
10. FCFS is simple but can suffer from convoy effect.
11. SJF minimizes average waiting time under ideal known burst lengths.
12. SRTF is the preemptive shortest-job strategy.
13. Priority scheduling can cause starvation.
14. Aging can reduce starvation.
15. Round Robin uses a time quantum.
16. A very small quantum increases switching overhead.
17. A very large quantum makes RR approach FCFS.
18. Linux uses scheduling classes.
19. `SCHED_OTHER` is the common normal scheduling policy.
20. `SCHED_FIFO` and `SCHED_RR` are real-time policies.
21. `SCHED_DEADLINE` is designed for deadline-oriented workloads.
22. Nice values influence normal scheduling preference.
23. CPU affinity controls where a task may execute.
24. Run queues track runnable work in scheduler structures.
25. Tasks can migrate between CPUs.
26. NUMA and cache locality influence scheduling performance.
27. CFS is the foundational Linux fair-scheduling model.
28. Modern Linux has evolved toward EEVDF-based fair scheduling.
29. Priority inversion can be mitigated with priority inheritance.
30. More threads do not automatically mean better performance.
31. Oversubscription can increase scheduling overhead.
32. `ps`, `top`, `pidstat`, `perf`, `taskset`, `chrt`, and `/proc` are important practical tools.
33. For senior interviews, connect:

```text
Application
    ↓
pthread / process execution
    ↓
Linux task
    ↓
task_struct
    ↓
Runnable / blocked state
    ↓
Scheduling class
    ↓
Run queue / CPU
    ↓
Scheduler
    ↓
Context switch / preemption / wakeup
    ↓
CPU
```

---

# Next Chapter

## Chapter 5 — Synchronization

Planned three-layer coverage:

```text
[OS]
- Race conditions
- Critical sections
- Mutual exclusion
- Atomicity
- Mutex
- Semaphore
- Condition variables
- Spinlocks
- Reader-writer synchronization
- Barriers
- Deadlock relationship
- Memory ordering
- Lock contention

[LSP]
- pthread_mutex_*
- pthread_cond_*
- sem_*
- pthread_rwlock_*
- pthread_spin_*
- C/C++ atomics
- Producer-consumer code
- Reader-writer code
- Race-condition demonstrations
- Practical debugging

[KERNEL]
- Kernel locking concepts
- spinlock
- mutex
- rwlock
- atomic operations
- wait queues
- scheduler interaction
- interrupt context
- process context
- softirq considerations
- memory barriers
- lock contention/debugging
- priority inheritance

[INTERVIEW]
- Senior synchronization questions
- Race-condition debugging
- Deadlock scenarios
- Lock ordering
- Performance trade-offs
- User-space vs kernel-space synchronization
