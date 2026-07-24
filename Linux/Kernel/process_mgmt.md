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