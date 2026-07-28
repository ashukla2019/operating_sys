# Operating System - Process Management Handbook

> Complete interview notes covering processes, scheduling, IPC, synchronization, execution models, and CPU scheduling.

---

# Table of Contents

1. What is a Process?
2. Program vs Process
3. Components of a Process
4. Process Memory Layout
5. Process Control Block (PCB)
6. Process States
7. State Transition Diagram
8. Process Scheduling
9. Types of Schedulers
10. Scheduling Queues
11. Context Switching
12. Types of Processes
13. Inter Process Communication (IPC)
14. Process Synchronization
15. Process vs Thread
16. Deadlock
17. CPU Scheduling Algorithms
18. Advantages of Process Management
19. Process Classification
20. Process Execution Models
21. Concurrency vs Parallelism
22. Interview Questions

---

# 1. What is a Process?

A **process** is a **program in execution**.

A process is the basic unit of:

- CPU scheduling
- Resource allocation
- Process management

Unlike a program, a process has:

- Program Counter
- CPU Registers
- Stack
- Heap
- Open Files
- Process State
- Memory

---

## Example

Program on disk

```
calculator.exe
```

When executed

```
calculator.exe
        ↓
    Running Process
```

The operating system creates a process for it.

---

# 2. Program vs Process

| Program | Process |
|----------|----------|
| Passive entity | Active entity |
| Stored on disk | Exists in memory |
| Collection of instructions | Instructions currently executing |
| No execution state | Has execution state |
| Doesn't consume CPU | Uses CPU |
| No PCB | Has PCB |

---

# 3. Components of a Process

Every process contains several sections.

```
+----------------------+
| Text (Code)          |
+----------------------+
| Data                 |
+----------------------+
| Heap                 |
| grows upward         |
+----------------------+
|                      |
| Free Space           |
|                      |
+----------------------+
| Stack                |
| grows downward       |
+----------------------+
```

---

## 1. Text Section

Contains

- Machine instructions
- Executable code

Example

```
main()
{
   printf("Hello");
}
```

Stored here.

---

## 2. Data Section

Contains

- Global variables
- Static variables

Example

```cpp
int count = 10;
static int x = 5;
```

---

## 3. Heap

Dynamic memory allocated during runtime.

Example

```cpp
new int;
malloc();
```

Heap grows upward.

---

## 4. Stack

Stores

- Function calls
- Local variables
- Parameters
- Return address

Example

```cpp
void fun()
{
    int x;
}
```

"x" is stored on stack.

Stack grows downward.

---

## 5. Program Counter (PC)

Stores

```
Address of next instruction to execute.
```

After every instruction,

PC updates automatically.

---

# 4. Process Memory Layout

```
High Address
-----------------------
Stack
Local Variables
Return Address
-----------------------
Free Memory
-----------------------
Heap
Dynamic Allocation
-----------------------
Data
Global Variables
-----------------------
Text
Machine Instructions
-----------------------
Low Address
```

---

# 5. Process Control Block (PCB)

Every process has a PCB.

PCB is maintained by the operating system.

It stores everything needed to resume a process.

---

## PCB Contents

### Process ID (PID)

Unique identifier.

Example

```
PID = 2345
```

---

### Process State

Current state

- Running
- Ready
- Waiting

---

### Program Counter

Address of next instruction.

---

### CPU Registers

Stores

- General Registers
- Stack Pointer
- Instruction Pointer

during context switching.

---

### Scheduling Information

Contains

- Priority
- Scheduling Queue
- Time Slice

---

### Memory Information

Contains

- Base Register
- Limit Register
- Page Table
- Segment Table

---

### I/O Status

Contains

- Open Files
- Devices
- Pending I/O

---

## PCB Diagram

```
+-------------------------+
| Process ID              |
+-------------------------+
| Process State           |
+-------------------------+
| Program Counter         |
+-------------------------+
| CPU Registers           |
+-------------------------+
| Scheduling Info         |
+-------------------------+
| Memory Info             |
+-------------------------+
| Open Files              |
+-------------------------+
| I/O Information         |
+-------------------------+
```

PCB acts like the **identity card** of a process.

# Process Creation to Execution Flow in Linux

This chapter explains what happens internally in Linux from the moment a process is created until it starts executing on the CPU.

---
# 1. Process Creation (`fork()`)

Suppose a process executes:

```c
fork();
```

This is a **system call**, so execution switches from **user mode** to **kernel mode**.

The kernel performs the following operations:
```
User
 │
 │ types "ls"
 ▼
Shell (running process)
 │
 │ fork()
 ▼
Kernel
 │
 ├── Creates child PCB
 ├── Assigns PID
 ├── Sets up Copy-On-Write memory
 │
 ▼
Child process
 │
 │ exec("/bin/ls")
 ▼
Kernel ELF Loader
 │
 ├── Reads ELF executable
 ├── Creates new address space
 ├── Maps .text/.data/.bss
 ├── Creates heap and stack
 └── Initializes CPU registers
 │
 ▼
Ready Queue
 │
 ▼
Scheduler selects the process
 │
 ▼
Context Switch
 │
 ├── Save current process registers
 ├── Load selected process registers
 ├── Switch page tables
 └── Return to user mode
 │
 ▼
CPU executes the selected process
```
---

# 6. Process States

A process changes states during execution.

```
New
↓

Ready

↓

Running

↓

Waiting

↓

Ready

↓

Running

↓

Terminated
```

---

## New

Process is being created.

---

## Ready

Loaded into memory.

Waiting for CPU.

---

## Running

CPU executing instructions.

---

## Waiting (Blocked)

Waiting for

- Disk I/O
- Keyboard
- Network
- Event

CPU executes another process.

---

## Terminated

Execution completed.

Resources released.

---

## Suspended States

Some operating systems add

- Ready Suspended
- Blocked Suspended

Used when memory is insufficient.

---

# 7. State Transition Diagram

```
            Admit
      +--------------+
      |              |
      v              |
     New ---------> Ready
                      |
                  Dispatch
                      |
                      v
                  Running
                  /     \
                 /       \
           I/O Wait      Exit
               |          |
               v          v
           Waiting   Terminated
               |
          I/O Complete
               |
               v
             Ready
```

---

# 8. Process Scheduling

CPU is limited.

Many processes compete for CPU.

Scheduler decides

```
Who gets CPU next?
```

Goal

- Fairness
- Efficiency
- High CPU utilization
- Low waiting time

---

# 9. Types of Schedulers

## Long-Term Scheduler

Also called

```
Job Scheduler
```

Responsible for

- Selecting jobs
- Loading into memory

Controls

```
Degree of Multiprogramming
```

Runs rarely.

---

## Medium-Term Scheduler

Responsible for

- Suspend process
- Resume process

Used to reduce memory load.

---

## Short-Term Scheduler

Also called

```
CPU Scheduler
```

Chooses

```
Ready Process
        ↓
      Running
```

Runs every few milliseconds.

Very fast.

---

# 10. Scheduling Queues

Processes move through queues.

---

## Job Queue

Contains

All processes in system.

---

## Ready Queue

Contains

Processes waiting for CPU.

```
CPU
 ↑
 |
Ready Queue
```

---

## Device Queue

Processes waiting for

- Printer
- Disk
- Keyboard
- Network

---

# 11. Context Switching

CPU switches from one process to another.

Steps

```
Running Process
      ↓

Save Registers

↓

Save PCB

↓

Load Next PCB

↓

Restore Registers

↓

Run Next Process
```

---

## Why Needed?

Single CPU cannot execute all processes simultaneously.

Context switching enables multitasking.

---

## Cost

Context switching performs no useful computation.

It is pure overhead.

Therefore,

Lower context switching = Better performance.

---

# 12. Types of Processes

## Independent Process

- Doesn't share data
- Doesn't depend on others

Example

Calculator

---

## Cooperating Process

Shares data.

Communicates with other processes.

Example

Web Server

Database

Browser

Need IPC.

---

# 13. Inter Process Communication (IPC)

Processes exchange data.

Two major methods.

---

## Shared Memory

Processes share one memory region.

```
Process A

     |

Shared Memory

     |

Process B
```

Advantages

- Very Fast
- No kernel copy after setup

Disadvantages

- Synchronization required

---

## Message Passing

Processes exchange messages.

```
Process A

Send()

↓

Kernel

↓

Receive()

↓

Process B
```

Advantages

- Safer
- Simpler

Disadvantages

- Slower

---

## Common IPC Mechanisms

- Pipes
- Named Pipes (FIFO)
- Shared Memory
- Message Queue
- Socket
- Signals
- Semaphore

---

# 14. Process Synchronization

When multiple processes access shared data.

Need synchronization.

---

## Critical Section

Code accessing shared resource.

Example

```
balance = balance + 100;
```

If two processes execute simultaneously

Incorrect result may occur.

---

## Synchronization Tools

### Mutex

Only one process/thread enters critical section.

---

### Semaphore

Counter-based synchronization.

Supports multiple resources.

---

### Monitor

High-level synchronization abstraction.

Provides mutual exclusion automatically.

---

### Spinlock

Busy waiting.

Used inside kernel.

Good for very short waiting.

---

# 15. Process vs Thread

| Process | Thread |
|----------|---------|
| Heavyweight | Lightweight |
| Own memory | Shared memory |
| Own PCB | Shares PCB resources |
| Slow creation | Fast creation |
| Expensive switching | Cheap switching |
| IPC required | Shared memory directly |

---

## Example

Browser Process

```
Browser Process

├── UI Thread

├── Network Thread

├── Rendering Thread

└── JavaScript Thread
```

---

# 16. Deadlock

Deadlock occurs when

Processes wait forever.

None can proceed.

---

## Example

```
P1

holds Lock A

waiting Lock B

-------------------

P2

holds Lock B

waiting Lock A
```

Both wait forever.

---

## Coffman's Conditions

All four must exist.

### 1. Mutual Exclusion

Resource cannot be shared.

---

### 2. Hold and Wait

Holding one resource.

Waiting for another.

---

### 3. No Preemption

OS cannot forcibly remove resource.

---

### 4. Circular Wait

Circular dependency exists.

```
P1 → P2 → P3 → P1
```

---

# 17. CPU Scheduling Algorithms

---

## FCFS

First Come First Serve

Characteristics

- Non-preemptive
- Simple
- Poor response time

---

## SJF

Shortest Job First

Runs shortest job first.

Advantages

- Minimum average waiting time

Disadvantages

- Hard to predict burst time
- Starvation possible

---

## Priority Scheduling

Higher priority runs first.

Problem

Low priority starvation.

Solution

Aging.

---

## Round Robin

Each process receives

```
Time Quantum
```

Example

```
P1

↓

P2

↓

P3

↓

P1

↓

P2
```

Advantages

- Fair
- Interactive systems

---

## Multilevel Queue

Separate queues

Example

```
Foreground Queue

Background Queue
```

Each queue has its own scheduling.

---

## Multilevel Feedback Queue

Most advanced scheduler.

Processes move between queues.

Interactive processes receive higher priority.

---

# 18. Advantages of Process Management

- Better CPU utilization
- Supports multitasking
- Supports multiprogramming
- Resource sharing
- Process isolation
- Protection
- Improved responsiveness
- Concurrency support
- Efficient scheduling

---

# 19. Process Classification

## Based on Execution

### Foreground Process

Runs with user interaction.

Examples

- Browser
- Terminal
- Editor

---

### Background Process

Runs without user interaction.

Examples

- Daemons
- Services
- Cron jobs

---

## Based on Function

### System Process

Created by operating system.

Examples

- systemd
- init
- scheduler

---

### User Process

Created by users.

Examples

- Chrome
- VS Code
- GCC

---

## Based on Behavior

### CPU Bound

Mostly CPU computation.

Example

Image processing.

---

### I/O Bound

Mostly waits for I/O.

Example

Web server.

---

## Based on Creation

### Parent Process

Creates child processes.

Example

Using

```cpp
fork()
```

---

### Child Process

Created by parent.

---

## Based on Communication

### Independent

No interaction.

---

### Cooperating

Uses IPC.

---

## Based on Threading

### Single Threaded

One thread.

---

### Multi Threaded

Multiple threads.

Shared memory.

---

# 20. Process Execution Models

---

## Multiprogramming

Multiple programs loaded into memory.

CPU switches when one waits.

Goal

```
Maximum CPU Utilization
```

---

## Multitasking

Rapid switching between tasks.

Appears simultaneous.

Example

- Music
- Browser
- IDE

running together.

---

## Multiprocessing

Multiple CPUs or cores.

True parallel execution.

Example

Quad-core processor.

---

## Multithreading

One process.

Many threads.

Shared memory.

Example

Browser

- UI
- Rendering
- Network

---

## Distributed Processing

Multiple computers.

One problem.

Examples

- Hadoop
- Kubernetes
- Cloud

---

## Time Sharing

CPU gives each process

```
Time Slice
```

Ensures fairness.

---

## Real Time Processing

Deadline must be met.

Examples

- Airbag
- Pacemaker
- Flight control

### Hard Real-Time

Missing deadline

= System failure.

### Soft Real-Time

Occasional deadline miss acceptable.

---

## Concurrency

Managing multiple tasks together.

May execute on

Single CPU.

Tasks overlap.

---

## Parallelism

Executing multiple tasks simultaneously.

Requires

Multiple cores or CPUs.

---

# 21. Concurrency vs Parallelism

| Concurrency | Parallelism |
|--------------|-------------|
| Multiple tasks in progress | Multiple tasks executing simultaneously |
| Can use one CPU | Requires multiple cores |
| Focuses on structure | Focuses on speed |
| Achieved using scheduling | Achieved using hardware |

---

## Relationship

```
Parallelism

        ⊂

Concurrency
```

Every parallel program is concurrent.

Every concurrent program is **not** parallel.

---

# 22. Interview Questions

## Basic

- What is a process?
- Difference between process and program?
- What is PCB?
- Explain process states.
- What is context switching?
- Why is context switching expensive?
- Explain scheduler types.
- Difference between long-term and short-term scheduler?
- What are scheduling queues?

---

## Intermediate

- Explain IPC.
- Shared memory vs message passing.
- Process vs thread.
- CPU-bound vs I/O-bound process.
- Parent vs child process.
- Explain synchronization.
- Mutex vs semaphore.
- Critical section problem.

---

## Advanced

- Explain FCFS, SJF, RR.
- Difference between preemptive and non-preemptive scheduling.
- Explain multilevel feedback queue.
- Deadlock conditions.
- Deadlock prevention vs avoidance.
- Explain multiprogramming vs multitasking.
- Concurrency vs parallelism.
- Multiprocessing vs multithreading.
- Real-time operating systems.
- How Linux schedules processes?
- What happens during context switching?
- What information is saved inside PCB?

--------------------------
# Linux Process Control Block (`task_struct`) ⭐⭐⭐⭐⭐

> **Interview Importance:** Extremely High (Qualcomm, NVIDIA, AMD, Broadcom)

In Linux, every process and thread is represented by a kernel data structure called **`task_struct`**.

A classical Operating Systems textbook refers to this as the **Process Control Block (PCB)**, whereas Linux implements it using **`task_struct`**.

```
                Process
                    │
                    ▼
           +----------------+
           |  task_struct   |
           +----------------+
           | PID            |
           | State          |
           | Priority       |
           | Registers      |
           | Memory Info    |
           | Open Files     |
           | Signals        |
           | Parent         |
           | Children       |
           | Scheduling     |
           +----------------+
```

### Important Fields

| Field | Description |
|--------|-------------|
| pid | Unique Process ID |
| tgid | Thread Group ID |
| state | Current process state |
| parent | Pointer to parent process |
| children | List of child processes |
| mm | Memory descriptor (`mm_struct`) |
| files | Open file descriptor table |
| signal | Pending signal information |
| sched_class | Scheduling class |
| prio | Dynamic process priority |

> **Interview Tip**
>
> You are **not expected to memorize every member** of `task_struct`. Interviewers expect you to know **what information it stores** and why the kernel needs it.

---

# Process Creation in Linux

Linux creates processes primarily using:

- `fork()`
- `vfork()`
- `clone()`

```
             Parent Process
                    │
                 fork()
                    │
        ┌───────────┴───────────┐
        │                       │
 Parent Process           Child Process
```

Initially, the parent and child **share the same physical memory pages** using **Copy-on-Write (CoW)**.

Only when either process modifies a shared page does Linux allocate a new physical page.

### Advantages

- Fast process creation
- Reduced memory usage
- Efficient `fork()` followed by `exec()`

---

# fork() vs vfork() vs clone()

| Feature | fork() | vfork() | clone() |
|----------|---------|----------|----------|
| Address Space | Copy-on-Write | Shared temporarily | Configurable |
| Parent Blocks | No | Yes | Depends on flags |
| Child Memory | Separate after CoW | Shared until exec()/exit() | Shared or Separate |
| Typical Use | General process creation | Optimize fork()+exec() | Threads, Containers |

### Interview Tip

Linux threads are created using **`clone()`**, not `fork()`.

---

# exec() Family ⭐⭐⭐⭐⭐

The `exec()` family **replaces the current process image** with a new program.

```
Parent
   │
fork()
   │
Child
   │
exec()
   │
New Program Starts
```

Common functions

- `execl()`
- `execv()`
- `execvp()`
- `execve()`

### After a successful `exec()`

- PID remains unchanged.
- Address space is replaced.
- Execution starts from the new program's entry point (`main()`).
- Open file descriptors remain open unless marked with `FD_CLOEXEC`.

---

# wait() and waitpid()

When a child process exits, its exit status remains available until the parent collects it.

```
Parent
   │
wait()
   │
Child Exits
   │
Resources Released
```

If the parent never calls `wait()` or `waitpid()`, the child becomes a **Zombie Process**.

---

# Zombie and Orphan Processes ⭐⭐⭐⭐⭐

## Zombie Process

A Zombie Process has finished execution, but its parent has **not yet collected** its exit status.

```
Child Exits
      │
   Zombie
      │
 wait()/waitpid()
      │
 Removed
```

### Characteristics

- Uses no CPU
- Does not execute
- Occupies a PID entry
- Exists until the parent collects its status

---

## Orphan Process

An Orphan Process is still running, but its parent has terminated.

```
Parent Terminates
        │
 Child Continues
        │
 Adopted by systemd/init
```

Modern Linux systems automatically re-parent orphan processes to **systemd (PID 1)**.

---

## Zombie vs Orphan

| Zombie | Orphan |
|----------|----------|
| Already exited | Still running |
| Waiting for parent | Parent terminated |
| Uses PID entry | Continues execution |
| Removed by wait() | Adopted by systemd |

---

# Linux Completely Fair Scheduler (CFS)

Linux uses the **Completely Fair Scheduler (CFS)** for normal processes.

Instead of maintaining fixed-priority queues, CFS attempts to distribute CPU time fairly among runnable tasks.

```
Runnable Tasks
       │
       ▼
 Red-Black Tree
       │
Smallest vruntime
       │
       ▼
      CPU
```

### Important Concepts

- `vruntime`
- Run Queue
- Red-Black Tree
- Fair CPU allocation

### Advantages

- Prevents starvation
- Good interactive performance
- Scales efficiently with many runnable processes

---

# Real-Time Scheduling Policies

Linux supports the following scheduling policies.

| Policy | Description |
|----------|-------------|
| SCHED_OTHER | Default Completely Fair Scheduler |
| SCHED_FIFO | Real-time First-In First-Out |
| SCHED_RR | Real-time Round Robin |

Real-time processes always have higher priority than normal CFS tasks.

---

# CPU Affinity

CPU Affinity binds a process to one or more CPUs.

```
CPU0  ←  Process A

CPU1  ←  Process B
```

### Advantages

- Better cache locality
- Fewer CPU migrations
- Reduced context-switch overhead
- Predictable execution

Useful commands

```bash
taskset
sched_setaffinity()
```

---

# Signals Overview

Signals provide asynchronous communication with processes.

### Common Signals

| Signal | Purpose |
|----------|----------|
| SIGINT | Interrupt (Ctrl+C) |
| SIGTERM | Graceful termination |
| SIGKILL | Immediate termination |
| SIGSTOP | Suspend process |
| SIGCONT | Resume process |
| SIGCHLD | Child process terminated |

---

# Context Switch Internals

A context switch saves the CPU state of the currently running process and restores the state of another process.

```
Running Process
       │
Save Registers
       │
Save Program Counter
       │
Save Stack Pointer
       │
Load Next Process
       │
Restore Registers
       │
Resume Execution
```

### Why Context Switching Is Expensive

- Saving CPU registers
- Restoring CPU registers
- Updating memory-management information
- Scheduler overhead
- Cache pollution
- Possible reduction in TLB efficiency

> **Interview Tip**
>
> Modern CPUs may preserve TLB entries using features such as ASIDs or PCIDs, so a context switch does **not always flush the entire TLB**. However, context switches can still reduce cache and TLB efficiency.

---

# Process Debugging Commands

| Command | Purpose |
|----------|----------|
| `ps` | List processes |
| `top` | Monitor running processes |
| `htop` | Interactive process monitor |
| `pstree` | Display process hierarchy |
| `pgrep` | Find process by name |
| `pidof` | Find PID |
| `strace` | Trace system calls |
| `ltrace` | Trace library calls |
| `lsof` | List open files |
| `taskset` | Display or set CPU affinity |
| `pmap` | Show process memory map |

---

# Production Scenarios ⭐⭐⭐⭐⭐

## Scenario 1 – Zombie Processes Increasing

### Symptoms

- Large number of `<defunct>` processes
- PID exhaustion

### Debugging

```bash
ps -el | grep Z
```

### Root Cause

Parent process never calls `wait()` or `waitpid()`.

### Solution

- Handle `SIGCHLD`
- Call `wait()` or `waitpid()`

---

## Scenario 2 – High Context Switch Rate

### Symptoms

- High CPU utilization
- Low throughput
- Increased latency

### Debugging

```bash
vmstat 1
pidstat -w
```

### Possible Causes

- Excessive threads
- Lock contention
- Frequent wake-ups
- CPU oversubscription

---

## Scenario 3 – fork() Fails

### Possible Reasons

- `ENOMEM` (Insufficient memory)
- `EAGAIN` (Process limit reached)
- PID exhaustion

---

## Scenario 4 – Process Stuck in D State

### Symptoms

The process cannot be terminated, even using `SIGKILL`.

### Common Causes

- Waiting for disk I/O
- NFS or network storage delays
- Driver or hardware issues

### Debugging

```bash
ps -eo pid,state,comm
```

---

# Senior Interview Questions

1. Why is `fork()` fast in Linux?
2. Explain Copy-on-Write.
3. Difference between `fork()`, `vfork()`, and `clone()`.
4. What happens during `exec()`?
5. Explain Zombie and Orphan processes.
6. What information is stored in `task_struct`?
7. How does the Linux Completely Fair Scheduler (CFS) work?
8. What is `vruntime`?
9. Why are context switches expensive?
10. What is CPU affinity, and when should it be used?
11. Explain `SCHED_FIFO` and `SCHED_RR`.
12. How would you debug hundreds of Zombie processes?
13. What does a process in **D (Uninterruptible Sleep)** state indicate?
14. How do Linux threads differ from processes?
15. How would you investigate high context-switch rates?--------------------------------
# Answers to Senior Interview Questions

---

# 1. Why is `fork()` fast in Linux?

`fork()` creates a new process by duplicating the parent's process descriptor (`task_struct`) and page tables.

However, Linux **does not immediately copy all memory pages**.

Instead, Linux uses **Copy-on-Write (CoW)**.

Initially, the parent and child share the same physical memory pages.

If either process modifies a page, only that page is copied.

```
Parent
    │
 fork()
    │
 ┌──┴──┐
 │     │
Parent Child
   │
Shared Memory Pages
   │
Write?
   │
Copy New Page
```

### Advantages

- Fast process creation
- Low memory overhead
- Efficient for `fork()` followed by `exec()`

---

# 2. Explain Copy-on-Write (CoW).

Copy-on-Write is an optimization technique used during `fork()`.

Instead of copying all memory immediately, Linux marks shared pages as **read-only**.

Both parent and child initially share the same physical pages.

When one process writes to a page:

1. Page Fault occurs.
2. Kernel allocates a new page.
3. Data is copied.
4. Writing process gets the new page.

```
fork()

↓

Shared Pages

↓

Write Attempt

↓

Page Fault

↓

Allocate New Page

↓

Continue Execution
```

Advantages

- Saves memory
- Faster process creation
- Avoids unnecessary copying

---

# 3. Difference between `fork()`, `vfork()`, and `clone()`.

| Feature | fork() | vfork() | clone() |
|----------|---------|----------|----------|
| Address Space | Copy-on-Write | Shared temporarily | Configurable |
| Parent Blocks | No | Yes | Depends |
| Child Memory | Separate | Shared | Shared or Separate |
| Typical Use | New Process | fork()+exec() optimization | Threads, Containers |

### Interview Tip

Linux threads are implemented using **`clone()`**.

---

# 4. What happens during `exec()`?

The `exec()` family replaces the current process image with a new program.

The process itself continues to exist.

Only its program image changes.

```
fork()

↓

Child

↓

exec()

↓

Old Program Removed

↓

New Program Loaded

↓

main()
```

### After successful `exec()`

- PID remains the same.
- Address space changes.
- Program starts from `main()`.
- File descriptors remain open unless marked `FD_CLOEXEC`.

---

# 5. Explain Zombie and Orphan Processes.

## Zombie Process

A Zombie process has completed execution but still occupies an entry in the process table because the parent has not collected its exit status.

```
Child Exits

↓

Zombie

↓

wait()

↓

Removed
```

Characteristics

- No CPU usage
- No executable code
- Occupies PID
- Removed by `wait()` or `waitpid()`

---

## Orphan Process

An Orphan process is still running after its parent terminates.

Linux automatically assigns it to **systemd/init (PID 1)**.

```
Parent Dies

↓

Child Running

↓

systemd adopts child
```

---

# 6. What information is stored in `task_struct`?

`task_struct` is the Linux kernel's process descriptor.

Important information stored includes:

- Process ID (PID)
- Thread Group ID (TGID)
- Process State
- Scheduling Information
- CPU Registers
- Parent Process
- Child Processes
- Memory Descriptor (`mm_struct`)
- Open File Table
- Signal Information
- Credentials

Every process and thread has its own `task_struct`.

---

# 7. How does the Linux Completely Fair Scheduler (CFS) work?

The Completely Fair Scheduler (CFS) attempts to give every runnable process a fair share of CPU time.

It maintains all runnable tasks in a **Red-Black Tree** ordered by **Virtual Runtime (`vruntime`)**.

```
Runnable Processes

↓

Red-Black Tree

↓

Smallest vruntime

↓

CPU
```

The process with the **smallest `vruntime`** runs next.

Advantages

- Fair scheduling
- Prevents starvation
- Excellent interactive performance

---

# 8. What is `vruntime`?

`vruntime` (Virtual Runtime) is the amount of CPU time a process has effectively consumed.

Instead of using actual execution time, CFS tracks **weighted runtime**.

```
Smaller vruntime

↓

Higher chance of running
```

Processes with lower priority (higher nice value) accumulate `vruntime` faster, causing them to receive less CPU time.

---

# 9. Why are context switches expensive?

During a context switch, Linux must:

- Save CPU registers
- Save Program Counter
- Save Stack Pointer
- Load next process state
- Switch memory mapping if required
- Invoke scheduler logic

Additional costs include:

- Cache pollution
- Reduced TLB efficiency
- Scheduler overhead

Frequent context switches reduce overall system performance.

---

# 10. What is CPU Affinity, and when should it be used?

CPU Affinity binds a process or thread to a specific CPU core.

```
CPU0 ← Process A

CPU1 ← Process B
```

Advantages

- Better cache locality
- Reduced CPU migration
- Lower scheduling overhead
- Predictable execution

Useful in:

- Real-time systems
- High-performance networking
- Embedded systems

Commands

```bash
taskset
sched_setaffinity()
```

---

# 11. Explain `SCHED_FIFO` and `SCHED_RR`.

These are Linux real-time scheduling policies.

### SCHED_FIFO

- First-In First-Out
- Highest-priority task runs until:
  - Blocks
  - Terminates
  - Voluntarily yields
- No time slicing

Suitable for deterministic real-time applications.

---

### SCHED_RR

Round Robin scheduling for real-time tasks.

Processes of equal priority receive fixed time slices.

```
P1

↓

P2

↓

P3

↓

P1
```

Provides fairness among equal-priority real-time tasks.

---

# 12. How would you debug hundreds of Zombie processes?

### Symptoms

```
<defunct>
```

appears in process listings.

### Debugging

```bash
ps -el | grep Z

pstree

strace -p <parent_pid>
```

### Root Cause

Parent process is not calling:

- `wait()`
- `waitpid()`

### Solution

- Handle `SIGCHLD`
- Call `wait()` or `waitpid()`

---

# 13. What does a process in **D (Uninterruptible Sleep)** state indicate?

A process in **D state** is waiting for an operation that **cannot be interrupted by signals**, typically I/O.

Common causes

- Disk I/O
- NFS delays
- Storage failures
- Driver issues

Debugging

```bash
ps -eo pid,state,comm

cat /proc/<pid>/stack

dmesg
```

Even `SIGKILL` cannot terminate a process while it remains in this state.

---

# 14. How do Linux threads differ from processes?

| Process | Thread |
|----------|---------|
| Independent execution unit | Lightweight execution unit |
| Separate virtual address space | Shares process address space |
| Separate file descriptor table (unless shared explicitly) | Typically shares process resources |
| Higher creation overhead | Lower creation overhead |
| IPC required for communication | Shared memory communication |

Linux implements threads using the **`clone()`** system call.

---

# 15. How would you investigate high context-switch rates?

### Step 1 – Measure Context Switches

```bash
vmstat 1

pidstat -w

sar -w
```

### Step 2 – Identify Busy Processes

```bash
top

htop
```

### Step 3 – Check Thread Count

```bash
ps -eLf
```

### Step 4 – Look for Lock Contention

Use:

```bash
perf

strace
```

### Common Causes

- Excessive threads
- Lock contention
- Frequent wake-ups
- Short CPU bursts
- CPU oversubscription
- Improper scheduling policy

### Solutions

- Reduce unnecessary threads.
- Increase task granularity.
- Minimize lock contention.
- Use appropriate scheduling policies.
- Pin critical threads using CPU affinity if beneficial.
- Profile before optimizing to identify the real bottleneck.
