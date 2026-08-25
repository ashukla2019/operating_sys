## 1 **High Level Linux Architecture**
Core component of the Kernel:
1) Process management
2) Thread scheduling
3) Virtual memory
4) Device drivers
5) File systems
6) Networking
7) Security
8) Inter-process communication
9) Interrupt handling


---


## **User Space vs Kernel Space**

**User space** = applications run with limited privileges.
**Kernel space** = OS with full privileges. CPU modes enforce this separation for protection and stability.


---


## **CPU Modes**
**User mode** runs applications with restricted privileges, while **kernel mode** runs the OS with full privileges.
**Virtual memory + page tables** map each process's virtual addresses to physical memory and define permissions like read/write/execute. These permissions are stored in Page tables:

```
Virtual Page    Physical Frame    Permissions

---

0x1000          0x5000            User, Read/Write
0x2000          0x6000            User, Read/Execute
0x3000          0x7000            Kernel, Read/Write
```

The **CPU/MMU** checks these permissions and blocks unauthorised access, such as user programs accessing kernel memory or another process's memory.
**System calls** provide a controlled way for user programs to request privileged operations from the kernel.

```
1. SOURCE CODE
      │
      │ You write C/C++ code.
      │ Example: int x = 10;
      ↓
2. COMPILER
      │
      │ Converts source code → machine/assembly code.
      │
      ↓
3. LINKER
      │
      │ Combines your code + libraries and creates
      │ the final executable file.
      │
      ↓
4. EXECUTABLE
      │
      │ Stored on SSD/HDD.
      │ It contains code, data, etc.
      │
      ↓
5. YOU RUN THE PROGRAM
      │
      │ OS receives a request to execute the file.
      │
      ↓
6. OS KERNEL CREATES A PROCESS
      │
      │ Kernel creates process information:
      │ - process ID
      │ - CPU state
      │ - virtual address space
      │ - page tables
      │
      ↓
7. VIRTUAL ADDRESS SPACE
      │
      │ Kernel gives the process its own virtual
      │ address space.
      │
      │ Example:
      │ Code → 0x00400000
      │ Data → 0x00600000
      │ Stack → high address
      │
      ↓
8. KERNEL CREATES/SETS PAGE TABLES
      │
      │ Page tables tell the MMU:
      │
      │ "Virtual page X → Physical frame Y"
      │
      │ and also permissions:
      │ Read / Write / Execute / User / Kernel
      │
      ↓
9. PROCESS IS READY TO RUN
      │
      │ OS scheduler gives the process CPU time.
      │ CPU starts at the program's entry point.
      │
      ↓
10. CPU FETCHES AN INSTRUCTION
      │
      │ CPU's instruction pointer contains a
      │ VIRTUAL address.
      │
      │ Example:
      │ RIP = 0x00401000
      │
      ↓
11. MMU RECEIVES VIRTUAL ADDRESS
      │
      │ MMU asks:
      │ "Where is virtual page 0x00401
      │  physically located?"
      │
      ↓
12. MMU LOOKS AT PAGE TABLE
      │
      │ Page table says:
      │
      │ Virtual page 0x00401
      │          ↓
      │ Physical frame 0x8A321
      │
      ↓
13. IS THE PAGE PRESENT IN RAM?
      │
      ├────────────── YES ──────────────┐
      │                                  │
      │                                  ↓
      │                         14. PHYSICAL ADDRESS
      │                                  │
      │                         Virtual address
      │                                  ↓
      │                              MMU translation
      │                                  ↓
      │                         Physical address
      │                                  │
      │                                  ↓
      │                            15. RAM
      │                                  │
      │                         Instruction is read
      │                                  │
      │                                  ↓
      │                            16. EXECUTE
      │
      │
      └────────────── NO ───────────────┐
                                         │
                                         ↓
                                14. PAGE FAULT
                                         │
                                         │ CPU tells kernel:
                                         │ "I need this page."
                                         ↓
                                  15. KERNEL
                                         │
                                         │ Finds the required data
                                         │ in executable/storage.
                                         ↓
                                  16. LOAD PAGE
                                         │
                                         │ Storage → RAM
                                         ↓
                                  17. UPDATE PAGE TABLE
                                         │
                                         │ Virtual page
                                         │       ↓
                                         │ Physical frame
                                         ↓
                                  18. CPU RETRIES
                                         │
                                         ↓
                                  19. MMU TRANSLATES
                                         │
                                         ↓
                                  20. PHYSICAL RAM
                                         │
                                         ↓
                                  21. EXECUTE
```

---


## **What is a System Call?**
A system call is a controlled interface provided by the operating system kernel through which a user-space program requests previleged kernel space services.

```
open()       → open a file
read()       → read data
write()      → write data
fork()       → create a process
execve()     → execute a program
mmap()       → map memory
ioctl()      → device-specific control operation
socket()     → create a socket
```text

## General System Call Flow

```text
APPLICATION
    |
    | calls
    v
LIBRARY FUNCTION
(e.g., read(), open(), write())
    |
    | calls
    v
SYSTEM-CALL WRAPPER
    |
    | prepares system-call number + arguments
    v
CPU REGISTERS
    |
    | wrapper executes
    v
SYSTEM-CALL INSTRUCTION
(e.g., syscall)
    |
    | causes
    v
CPU SWITCHES
USER MODE -> KERNEL MODE
    |
    | enters
    v
SYSTEM-CALL HANDLER / SYSTEM-CALL INTERFACE
    |
    | uses system-call number to look up
    v
SYSTEM-CALL TABLE
    |
    | selects
    v
CORRESPONDING KERNEL SYSTEM-CALL ROUTINE
(e.g., sys_read(), sys_open(), sys_write())
    |
    | may call
    v
KERNEL SUBSYSTEM
    |
    | if hardware access is required
    v
DEVICE DRIVER
    |
    | communicates with
    v
DEVICE CONTROLLER
    |
    | controls
    v
PHYSICAL DEVICE
```

The application never writes directly to the display hardware.

## **Why Use Libraries?**

Instead of invoking system calls manually, applications use libraries.

Benefits: easier programming, portable API, optimized implementations.

## **Kernel Components**

The Linux kernel consists of many subsystems:

Each subsystem performs a specialized task.


---


## Types of kernel:

## **Monolithic Kernel**

Linux uses a **Monolithic Kernel** architecture — all major services run inside kernel space.

**Advantages:** very fast, direct function calls, high performance, low overhead. **Disadvantages:** a buggy driver can crash the kernel; large code base.

## **Microkernel**

A Microkernel keeps only minimal functionality inside the kernel; everything else runs in user space.

**Advantages:** better isolation, better reliability, easier debugging. **Disadvantages:** more IPC, slower than monolithic kernels.


---

---

## 2 IPC — Inter-Process Communication

Processes have **separate address spaces**, so they cannot directly access each other's memory. The Operating System provides **IPC (Inter-Process Communication)** mechanisms for safe communication.

## Why IPC?

Processes may need to:

- Exchange data
- Synchronize execution
- Share resources
- Notify events
- Coordinate tasks

Examples:

- Browser ↔ Renderer
- Database ↔ Application
- Shell ↔ Child Process
- Producer ↔ Consumer

---

**1. Unnamed Pipe**
Simple one-way IPC, mainly between related processes (parent ↔ child). Exists only while processes are running.

int fd[2];
pipe(fd);

fd[0] → read
fd[1] → write

Parent → pipe → Child

Use: Shell pipelines (ls | grep.cpp), parent-child communication.
Avoid: Unrelated processes, persistent or bidirectional communication.


---

**2. Named Pipe (FIFO)**
Like a pipe, but has a filesystem name, allowing unrelated processes to communicate.

mkfifo("myfifo", 0666);

Process A → FIFO → Process B

Pros: Simple, works between unrelated processes.
Cons: Byte stream, one-way by default, slower than shared memory.
Use: Independent local programs, simple producer-consumer systems.

---

**3. Shared Memory**
Fastest IPC. Multiple processes map the same physical memory into their virtual address spaces, so data doesn't need to be copied through the kernel.

Process A ──┐
            ↓
       Shared Memory
            ↑
Process B ──┘


POSIX: shm_open(), mmap()
System V: shmget(), shmat(), shmdt(), shmctl()

Pros: Very fast, direct memory access.
Cons: Requires synchronization (mutexes, semaphores, etc.).
Use: Databases, video processing, high-speed producer-consumer systems.


---


**4. Message Queue**
Processes exchange discrete messages through a kernel-managed queue.

Process A
   │ mq_send()
   ↓
[ Message Queue ]
   │ mq_receive()
   ↓
Process B

POSIX: mq_send(), mq_receive()
System V: msgget(), msgsnd(), msgrcv(), msgctl()

Pros: Structured messages, simpler synchronization.
Use: Producer-consumer systems, task/event communication.


---


**5. Socket**
Provides bidirectional communication between processes and can work across machines.

Client → Socket → Network/Unix socket → Socket → Server


Common calls:

socket()
bind()
listen()
accept()
connect()
send()
recv()
close()


Pros: Bidirectional, network-capable, client-server model.
Cons: More protocol/communication overhead than shared memory.
Use: Web servers, chat apps, REST APIs, microservices, distributed systems.


---


**6. Memory-Mapped File (mmap)**
Maps a file into a process's virtual memory, allowing it to access the file like normal memory. Multiple processes can map the same file.

int fd = open("data.bin", O_RDWR);

void *ptr = mmap(
    NULL, size,
    PROT_READ | PROT_WRITE,
    MAP_SHARED, fd, 0
);

Pros: Fast, persistent, good for large files.
Cons: File/storage overhead; synchronization may be needed.
Use: Database files/caches, large-file processing, shared persistent data.

Quick Selection
Requirement	Use
Parent ↔ Child	Unnamed Pipe
Unrelated local processes	FIFO
Maximum speed	Shared Memory
Structured messages	Message Queue
Client ↔ Server	Socket
Cross-machine communication	Socket
Shared data + persistence	mmap()


One-line mental model
Pipe/FIFO       → stream of bytes
Message Queue   → messages
Shared Memory   → shared RAM
Socket          → communication endpoint
mmap()          → file/shared memory mapped into address space


---

---

## 3 Process Management

## 1. What is a Process?

A **process is a program in execution**.

A process is the basic unit of:

- CPU scheduling
- Resource allocation
- Process management
- Protection and isolation

Unlike a program stored on disk, a process has execution state and resources such as:

- Program Counter (PC)
- CPU registers
- Stack
- Heap
- Open files
- Process state
- Virtual address space
- Process Control Block (PCB)

```text
Program on disk
      │
      │ Execute
      ↓
Operating System
      │
      ↓
Running Process
```

---

## 2. Program vs Process

| Program | Process |
|---|---|
| Passive entity | Active entity |
| Stored on disk | Running/executing entity |
| Collection of instructions | Instructions + execution state |
| No execution state | Has execution state |
| Does not directly consume CPU | Uses CPU |
| No PCB | Has PCB |

---

## 3. Process Memory Layout

A process has its own virtual address space.

```text
High Address
┌──────────────────┐
│      Stack       │
│        ↓         │
├──────────────────┤
│                  │
│   Memory gap     │
│                  │
├──────────────────┤
│        ↑         │
│       Heap       │
├──────────────────┤
│       Data       │
├──────────────────┤
│       Text       │
│   Machine Code   │
└──────────────────┘
Low Address
```

### Text Section
Contains executable machine instructions.

```c
int main() {
    printf("Hello");
}
```

### Data Section
Contains initialized global and static variables.

```c
int count = 10;
static int x = 5;
```

### BSS
Contains uninitialized or zero-initialized global/static variables.

### Heap
Used for dynamic memory allocation.

- `malloc()`
- `new`

Typically grows upward.

### Stack
Stores:

- Function call information
- Local variables
- Parameters
- Return addresses

Typically grows downward.

### Program Counter
Stores the address of the next instruction to execute.

---

## 4. Process Control Block (PCB)

The PCB is maintained by the OS and contains the information required to manage and resume a process.

In Linux, the main process descriptor is:

```
task_struct
```

A classical OS textbook calls this information the PCB.

### Important PCB Information

| Information | Purpose |
|---|---|
| PID | Process identifier |
| TGID | Thread group ID |
| Process state | Running, ready, waiting, etc. |
| Program Counter | Next instruction |
| CPU registers | Saved/restored during context switching |
| Scheduling information | Priority, scheduling class, queues |
| Memory information | Address space, page tables, mm_struct |
| Parent/children | Process hierarchy |
| Open files | File descriptor information |
| Signals | Signal information |
| Credentials | User/group/security information |

> **Interview Tip:** You do not need to memorize every `task_struct` field. Know what information the kernel needs to manage and resume a process.

---

## 5. Process States

A process changes state during its lifetime.

```text
             ┌─────────┐
             │   New   │
             └────┬────┘
                  │ Admit
                  ↓
             ┌─────────┐
        ┌───►│  Ready  │◄──────────┐
        │    └────┬────┘           │
        │         │ Dispatch       │ I/O complete
        │         ↓                │
        │    ┌──────────┐          │
        │    │ Running  │──────────┘
        │    └────┬─────┘
        │         │
        │    I/O wait
        │         ↓
        │    ┌──────────┐
        └────│ Waiting  │
             └──────────┘

Running ──Exit──► Terminated
```

### New
Process is being created.

### Ready
Process is ready to execute but waiting for CPU time.

### Running
CPU is currently executing the process.

### Waiting / Blocked
Process is waiting for an event, commonly:

- Disk I/O
- Network I/O
- Keyboard input
- Synchronization event

### Terminated
Process has finished execution and its resources are released.

### Suspended States
Some systems also use:

- Ready Suspended
- Blocked Suspended

These allow processes to be temporarily removed from active memory to reduce memory pressure.

---

## 6. Process Scheduling

The CPU is limited while many processes may be runnable.

The scheduler decides: **Which process should run next?**

Goals include:

- Fairness
- High CPU utilization
- Low waiting time
- Good responsiveness
- Efficient resource usage

### Types of Schedulers

#### Long-Term Scheduler
Also called the Job Scheduler.

Responsible for:
- Selecting jobs/processes
- Admitting them into the system

Controls the degree of multiprogramming. Runs relatively infrequently.

#### Medium-Term Scheduler
Responsible for:
- Suspending processes
- Resuming processes

Used to reduce memory pressure.

#### Short-Term Scheduler
Also called the CPU Scheduler.

```text
Ready Queue
     │
     ↓
Scheduler
     │
     ↓
Running Process
```

Runs very frequently, so it must be fast.

---

## 7. Scheduling Queues

### Job Queue
Contains processes/jobs known to the system.

### Ready Queue
Contains processes waiting for CPU time.

```
Ready Queue → CPU
```

### Device Queue
Contains processes waiting for a particular I/O device or event.

Examples:
- Disk
- Keyboard
- Network
- Printer

---

## 8. CPU Scheduling Algorithms

### FCFS — First Come First Serve
Runs processes in arrival order.

- Non-preemptive
- Simple
- Can have poor response time

### SJF — Shortest Job First
Runs the process with the shortest CPU burst.

**Advantage:**
- Minimum average waiting time under ideal assumptions

**Disadvantages:**
- Burst time is difficult to predict
- Starvation is possible

### Priority Scheduling
Higher-priority processes run first.

**Problem:** Low-priority starvation
**Solution:** Aging

### Round Robin
Each process receives a fixed time quantum.

```
P1 → P2 → P3 → P1 → P2 → ...
```

**Advantages:**
- Fair
- Good for interactive systems

### Multilevel Queue
Processes are divided into separate queues.

Example:
- Foreground Queue
- Background Queue

Each queue can use a different scheduling policy.

### Multilevel Feedback Queue
Processes can move between queues based on their behavior. Interactive processes can receive higher priority.

---

## 9. Context Switching

A context switch occurs when the CPU stops executing one process/thread and starts another.

```text
Process A Running
      │
      ↓
Save CPU state
      │
      ↓
Scheduler selects Process B
      │
      ↓
Restore Process B state
      │
      ↓
Process B Running
```

The saved state includes information such as:
- CPU registers
- Program Counter
- Stack Pointer
- Memory-management context

### Why Context Switching Is Expensive
It performs no application work and introduces overhead from:

- Saving/restoring registers
- Scheduler execution
- Memory-management changes when required
- Cache pollution
- Possible TLB efficiency loss

> Modern CPUs may use mechanisms such as ASIDs/PCIDs, so a context switch does not necessarily flush the entire TLB.

**Goal:** Avoid unnecessary context switches, but do not optimize them blindly—measure first.

---

## 10. Independent vs Cooperating Processes

### Independent Process
Does not share data or depend on other processes.

**Example:** Calculator

### Cooperating Process
Shares data or communicates with other processes.

**Examples:**
- Browser ↔ Renderer
- Producer ↔ Consumer
- Application ↔ Database

Cooperating processes commonly use IPC (Inter-Process Communication).

---

## 11. Process vs Thread

| Process | Thread |
|---|---|
| Independent execution unit | Lightweight execution unit |
| Separate virtual address space | Shares process address space |
| Own process resources | Shares many process resources |
| Higher creation overhead | Lower creation overhead |
| More expensive switching | Usually cheaper switching |
| IPC often needed | Shared memory can be used directly |

> Linux implements threads using the `clone()` mechanism.

---

## 12. Types of Processes

### Based on Execution

**Foreground** — Interactive with the user.
Examples: Browser, Terminal, Editor

**Background** — Runs without direct user interaction.
Examples: Daemons, Services, Cron jobs

### Based on Function

**System Process** — Created/managed by the operating system.
Examples: systemd, Kernel/system services

**User Process** — Created by users/applications.
Examples: Chrome, VS Code, GCC

### Based on Behavior

**CPU-Bound** — Spends most of its time computing.
Example: Image processing

**I/O-Bound** — Spends much of its time waiting for I/O.
Example: Web server

### Based on Communication

**Independent** — No interaction with other processes.

**Cooperating** — Communicates using IPC.

### Based on Threading

**Single-Threaded** — Contains one thread.

**Multi-Threaded** — Contains multiple threads sharing the process's resources/address space.

---

## 13. Process Creation in Linux

Linux primarily uses:
- `fork()`
- `vfork()`
- `clone()`

### fork()
Creates a child process. Linux uses Copy-on-Write (CoW), so the parent and child initially share physical memory pages.

```text
Parent ─────┐
            ├── Shared physical pages
Child  ─────┘
```

If either process writes to a shared page, Linux creates a private copy.

### Why fork() is Fast
Linux does not immediately copy all process memory. It mainly creates process metadata and page-table structures while using CoW for memory.

---

## 14. Copy-on-Write (CoW)

During `fork()`, shared pages are initially marked so that a write causes a page fault.

When a process writes:

```text
1. Write attempt
      ↓
2. Page fault
      ↓
3. Kernel allocates new physical page
      ↓
4. Data is copied
      ↓
5. Page table is updated
      ↓
6. Process writes to its private copy
```

### Advantages
- Faster process creation
- Lower memory usage
- Avoids unnecessary copying

---

## 15. fork() vs vfork() vs clone()

| Feature | fork() | vfork() | clone() |
|---|---|---|---|
| Address Space | CoW | Temporarily shared | Configurable |
| Parent | Continues | Blocks until child exec()/exit | Depends on flags |
| Child Memory | Separate after CoW | Shared temporarily | Shared or separate |
| Typical Use | General process creation | fork() + exec() optimization | Threads, containers |

---

## 16. exec()

The `exec()` family replaces the current process image with a new program.

Common functions:
- `execl()`
- `execv()`
- `execvp()`
- `execve()`

Typical pattern:

```text
fork()
  ↓
Child
  ↓
exec()
  ↓
New Program
```

After successful `exec()`:
- PID remains unchanged
- Address space is replaced
- New program starts execution
- Open file descriptors normally remain open unless marked `FD_CLOEXEC`

> `exec()` does not create a new process. It replaces the program image of the existing process.

---

## 17. wait() and waitpid()

When a child exits, its exit status remains available until the parent collects it.

```text
Child exits
    ↓
Exit status retained
    ↓
Parent calls wait()/waitpid()
    ↓
Child's process-table entry can be cleaned up
```

If the parent fails to collect the status, the child becomes a **Zombie**.

---

## 18. Zombie and Orphan Processes

### Zombie
A process that has already exited, but whose parent has not collected its exit status.

**Characteristics:**
- Does not execute
- Uses no CPU
- Occupies a process/PID table entry
- Removed after `wait()`/`waitpid()`

```text
Child exits
    ↓
Zombie
    ↓
Parent calls wait()
    ↓
Removed
```

### Orphan
A process that is still running, but whose parent has terminated.

Linux re-parents orphan processes to an appropriate reaper, typically PID 1 (`systemd` on modern systems).

| | Zombie | Orphan |
|---|---|---|
| Status | Already exited | Still running |
| Parent | Has not collected status | Has terminated |
| Resource | Occupies process-table entry | Continues execution |
| Resolution | Reaped using `wait()` | Re-parented |

---

## 19. Deadlock

A deadlock occurs when processes/threads wait forever for resources held by each other.

**Example:**
```
P1 → waits for P2
P2 → waits for P1
```

Deadlock requires all four conditions:

### 1. Mutual Exclusion
A resource cannot be simultaneously shared.

### 2. Hold and Wait
A process holds one resource while waiting for another.

### 3. No Preemption
Resources cannot be forcibly taken away.

### 4. Circular Wait
A circular dependency exists.

```
P1 → P2 → P3 → P1
```

---

## 20. Process Execution Models

| Model | Meaning |
|---|---|
| Multiprogramming | Multiple programs are in memory; CPU switches when one waits |
| Multitasking | CPU rapidly switches between tasks to provide responsive execution |
| Multiprocessing | Multiple CPU cores execute tasks in parallel |
| Multithreading | Multiple threads within a process share memory/resources |
| Distributed Processing | Multiple computers cooperate on a problem |
| Time Sharing | CPU gives tasks time slices for fairness/responsiveness |

---

## 21. Concurrency vs Parallelism

### Concurrency
Multiple tasks are in progress and their execution may overlap. Can occur on one CPU through scheduling.

### Parallelism
Multiple tasks execute simultaneously. Requires multiple CPU cores/CPUs.

| | Concurrency | Parallelism |
|---|---|---|
| Definition | Multiple tasks in progress | Multiple tasks executing simultaneously |
| Hardware | Can use one CPU | Requires multiple cores/CPUs |
| Focus | Managing tasks | Simultaneous execution |
| Achieved via | Scheduling | Hardware capable of parallel execution |

> **Parallelism ⊂ Concurrency**
> Every parallel program is concurrent, but not every concurrent program is parallel.

---

## 22. Real-Time Processing

Real-time systems must meet timing requirements.

**Examples:** Airbag systems, Flight control, Pacemakers

### Hard Real-Time
Missing a deadline can cause system failure.

### Soft Real-Time
Occasional deadline misses are acceptable.

---

## 23. Linux Scheduling

Linux uses different scheduling policies. For normal tasks, modern Linux uses the fair scheduling framework; historically this was called the **Completely Fair Scheduler (CFS)**.

Important CFS concepts include:
- `vruntime`
- Run queue
- Fair CPU allocation
- Red-black tree in the traditional CFS implementation

The task with the smallest effective `vruntime` was selected to run under the traditional CFS design.

### vruntime
Represents weighted CPU time consumed by a task. Tasks with lower effective `vruntime` have a stronger claim to CPU time.

---

## 24. Real-Time Scheduling Policies

| Policy | Description |
|---|---|
| SCHED_OTHER | Default normal scheduling policy |
| SCHED_FIFO | Real-time first-in-first-out |
| SCHED_RR | Real-time round robin |

### SCHED_FIFO
A real-time task runs until it:
- Blocks
- Terminates
- Voluntarily yields

### SCHED_RR
Equal-priority real-time tasks receive time slices.

```
P1 → P2 → P3 → P1
```

> Real-time scheduling policies can have higher priority than normal scheduling classes.

---

## 25. CPU Affinity

CPU affinity binds a process/thread to one or more CPUs.

```
CPU0 ← Process A
CPU1 ← Process B
```

**Potential benefits:**
- Better cache locality
- Fewer CPU migrations
- More predictable execution
- Useful for real-time/high-performance workloads

**Commands/APIs:**
- `taskset`
- `sched_setaffinity()`

---

## 26. Signals

Signals provide asynchronous notification to processes.

| Signal | Purpose |
|---|---|
| SIGINT | Interrupt, e.g. Ctrl+C |
| SIGTERM | Request graceful termination |
| SIGKILL | Force termination |
| SIGSTOP | Suspend process |
| SIGCONT | Resume process |
| SIGCHLD | Child state changed/terminated |

---

## 27. Linux Process Debugging Commands

| Command | Purpose |
|---|---|
| `ps` | List processes |
| `top` | Monitor processes |
| `htop` | Interactive process monitor |
| `pstree` | Process hierarchy |
| `pgrep` | Find process by name |
| `pidof` | Find PID |
| `strace` | Trace system calls |
| `ltrace` | Trace library calls |
| `lsof` | List open files |
| `taskset` | Display/set CPU affinity |
| `pmap` | Show process memory map |
| `vmstat` | System/process statistics |
| `pidstat` | Per-process statistics |
| `perf` | Performance analysis |

---

## 28. Common Production Scenarios

### Scenario 1 — Zombie Processes

**Symptoms:**
- Many `<defunct>` processes
- PID exhaustion

**Debug:**
```bash
ps -el | grep Z
pstree
```

**Cause:** Parent does not call `wait()` / `waitpid()`

**Solution:**
- Handle `SIGCHLD`
- Call `wait()`/`waitpid()`

---

### Scenario 2 — High Context-Switch Rate

**Symptoms:**
- High CPU usage
- Low throughput
- Increased latency

**Measure:**
```bash
vmstat 1
pidstat -w
```

**Possible Causes:**
- Excessive threads
- Lock contention
- Frequent wake-ups
- CPU oversubscription
- Very short CPU bursts
- Poor scheduling configuration

**Solution:** Measure first, then consider:
- Reducing unnecessary threads
- Reducing lock contention
- Improving task granularity
- Adjusting scheduling
- CPU affinity where appropriate

---

### Scenario 3 — fork() Fails

Possible errors:
- `ENOMEM` → Insufficient memory/resources
- `EAGAIN` → Process/resource limit reached

Other causes can include PID/resource exhaustion.

---

### Scenario 4 — Process Stuck in D State

`D` means uninterruptible sleep, commonly while waiting for I/O.

**Possible causes:**
- Disk I/O delays
- NFS/network storage problems
- Storage failures
- Driver problems
- Hardware issues

**Debug:**
```bash
ps -eo pid,state,comm
cat /proc/<pid>/stack
dmesg
```

> A process cannot normally respond to signals, including `SIGKILL`, while it remains stuck in an uninterruptible kernel wait.

---

## 30. Quick Revision

```text
PROGRAM
   │
   │ execute
   ↓
PROCESS
   │
   ├── Virtual Address Space
   ├── CPU Registers
   ├── Program Counter
   ├── Stack
   ├── Heap
   ├── Open Files
   └── PCB / task_struct
           │
           ↓
       SCHEDULER
           │
           ↓
       CPU EXECUTION
           │
      ┌────┴─────┐
      │          │
    Running    Waiting
      │          │
      │          │ I/O complete
      │          ↓
      └──────► Ready
```

### Process Creation
```text
fork()
  ↓
Child process
  ↓
Copy-on-Write
  ↓
exec()
  ↓
New program image
```

### Process Communication
```text
Separate address spaces
        ↓
       IPC
        ↓
Pipe / FIFO / Shared Memory /
Message Queue / Socket / mmap()
```

### Core Concepts

| Concept | Description |
|---|---|
| Process | Program in execution |
| PCB | Information needed to manage process |
| Scheduler | Chooses what runs |
| Context switch | Switches CPU from one task to another |
| fork() | Creates child |
| CoW | Delays memory copying until write |
| exec() | Replaces process image |
| wait() | Collects child exit status |
| Zombie | Exited child not yet reaped |
| Orphan | Running child whose parent exited |
| Thread | Lightweight execution unit sharing process resources |
| Deadlock | Processes wait forever for resources |
| Concurrency | Multiple tasks in progress |
| Parallelism | Multiple tasks executing simultaneously |


---

---

## 4 — File System (VFS)

# Linux VFS + Filesystem: mkfs → mount → open → read/write

These are persistent structures. They remain on disk across reboot.

**IMPORTANT:** Mount does NOT copy the entire filesystem from disk into RAM.

Linux creates/initializes in-memory runtime structures needed to manage the mounted filesystem.

text
/ → mnt → data → a.txt

**IMPORTANT RELATIONSHIP:**

**6. IMPORTANT VFS STRUCTURES**

- `struct super_block` → represents a mounted filesystem
- `struct mount` / `vfsmount` → represents mount information
- `struct dentry` → pathname component / name → inode
- `struct inode` → file/directory metadata and object
- `struct file` → one particular open instance
- `struct path` → mount + dentry
- `struct file_operations` → operations available through f_op
- `struct address_space` → file/inode ↔ page-cache mapping
- Page Cache → cached file contents in RAM
- FD table → fd → struct file

**7. DENTRY vs INODE vs STRUCT FILE**

- DENTRY → "What name/path is this?"
- INODE → "What is this file?"
- STRUCT FILE → "How is this particular open() using the file?"
- FD → "Integer handle used by the application"


---

---

# 5 — System Calls & Interrupts**

Interview notes covering system calls, interrupts, kernel mode transition, and their relationship.

# **1. System Call**

A **system call** is a mechanism through which a **user program requests a service from the Operating System kernel** . User programs cannot directly access hardware because of:

Security Protection Resource management Therefore, applications use system calls to request OS services.

# **2. Why System Calls Are Needed?**

Applications run in:

User Mode

User programs have restricted access. The Operating System runs in:

Kernel Mode

The kernel has complete access to: CPU Memory Hardware devices System resources System calls provide a controlled interface between user programs and the OS kernel.

# **3. Examples of System Calls**

## **File Operations**

Used for file handling. Examples:

open() read() write() close()

Example:

read(file, buffer, size);

The program requests the OS to read data from a file.

## **Process Control**

Used for creating and managing processes. Examples:

fork() exec() exit() wait()

fork();

Creates a new process.

## **Device Management**

Used to communicate with hardware devices. Examples:

Requesting keyboard input Sending data to printer Accessing disk devices Communicating with network devices

## **Memory Management**

Programs request memory from the OS. Examples:

brk() mmap()

Functions like: malloc()

internally use system calls to allocate memory.

# **4. Example: printf() and System Call**

When a program executes:

printf("Hello");

The application does not directly access the screen. The kernel handles communication with hardware.

# **6. User Mode vs Kernel Mode**

## **User Mode**

|Used by:|
|---|
|Applications|
|Browsers|
|Games|
|Editors|
|Restrictions:|
|Cannot access hardware directly|
|Cannot execute privileged instructions|
|Cannot directly modify kernel memory|

## **Kernel Mode**

|Used by:|
|---|
|Operating System kernel|
|Has access to:|
|Hardware|
|Memory management|
|CPU instructions|
|Devices|

## **Mode Switching**

A system call causes:

User Mode ↓ Kernel Mode ↓ User Mode

# **7. Types of System Calls**

## **1. Process Control**

Responsible for process management. Examples:

## **2. File Management**

Handles files. Examples:

## **3. Device Management**

Controls hardware devices. Examples:

ioctl() read() write()

## **4. Information Maintenance**

Provides system information. Examples:

getpid() time() uname()

## **5. Communication**

Supports communication between processes. Examples:

pipe() socket() shmget()

## **6. Memory Management**

Handles memory allocation. Examples:

# **8. Interrupt**

An **interrupt** is a signal sent to the CPU indicating that an event requires immediate attention. When an interrupt occurs:

1. CPU pauses current execution

2. Saves CPU state

3. Transfers control to Interrupt Service Routine (ISR)

4. ISR handles the event

5. CPU restores previous state

6. Execution resumes

# **10. Types of Interrupts**

## **1. Hardware Interrupt**

Generated by external hardware devices. Examples: Keyboard key press Mouse click Network packet arrival Disk operation completed Flow:

## **2. Software Interrupt**

Generated by software. Examples: System calls Divide by zero error Invalid memory access

## **3. Timer Interrupt**

Generated by the system clock. Used for: CPU scheduling Multitasking Time sharing Example:
# **11. Interrupt Service Routine (ISR)**

ISR is a special kernel function executed when an interrupt occurs. Responsibilities:

Handle interrupt Process event Notify operating system Resume execution Example:

# **12. Relationship Between System Calls and Interrupts**

Both system calls and interrupts can cause:

However, their purpose is different.

|**System Call**|**Interrupt**|
|---|---|
|Requested by program|Triggered by event|
|Intentional|Can happen unexpectedly|
|Requests OS service|Notifes CPU about an event|
|Example: read(), write()|Example: keyboard input|

# **13. System Calls Using Software Interrupts**

Historically, operating systems implemented system calls using software interrupts. Example (x86):

|int 0x80|
|---|
Modern processors use:

`syscall` instruction

Execution flow:

# **14. System Call vs Function Call**

|**Function Call**|**System Call**|
|---|---|
|Runs in user space|Runs in kernel space|

No mode switch Causes mode switch Faster Slower Application code OS service Example: strlen() Example: read()

# **15. System Call vs Interrupt vs Exception**

|**Feature**|**System Call**|**Interrupt**|**Exception**|
|---|---|---|---|
|Source|Program request|Hardware/software signal|CPU detected error|
|Type|Intentional|Usually external event|Internal event|
|Example|read(), write()|Keyboard input|Divide by zero|

# **16. Real World Example**

Opening a file:

The application never directly controls the disk.


---

---

## 6 — Memory Management

Complete interview notes covering memory hierarchy, allocation techniques, paging, segmentation, virtual memory, fragmentation, swapping, and modern OS memory management.

|1. Introduction to Memory Management|
|---|
|2. Why Memory Management is Needed|
|3. Memory Hierarchy|
|4. SRAM vs DRAM|
|5. Responsibilities of Memory Management|
|6. Memory Allocation Techniques|
|7. Contiguous Memory Allocation|
|8. Fixed Partitioning|
|9. Dynamic Partitioning|
|10. Memory Allocation Strategies|
|11. Non-Contiguous Memory Allocation|
|12. Paging|
|13. Address Translation in Paging|
|14. Segmentation|
|15. Paged Segmentation|
|16. Virtual Memory|
|17. Demand Paging|
|18. Fragmentation|
|19. Swapping|
|20. Memory Protection|
|21. Memory Management Unit (MMU)|
|22. Memory Management in Modern Operating Systems|
|23. Advantages of Memory Management|
|24. Interview Questions|

# **1. Introduction to Memory Management**

|Memory Management is one of the most important responsibilities of an Operating System.|
|---|
|It is responsible for:|
|Allocating memory to processes|
|Tracking memory usage|
|Protecting memory|
|Reclaiming memory<br>Maximizing memory utilization|
|Without memory management, multiple programs cannot execute safely and eficiently.|

# **2. What is Memory Management?**

Memory is a large collection of bytes (or words) where programs and data are temporarily stored during execution. Memory Management is the process of:

|Allocating memory|
|---|
|Tracking allocated memory|
|Protecting memory|
|Releasing memory|
|Goal:|
|Maximum memory utilization|
|Eficient execution|
|Fair resource sharing|
|Process isolation|

# **3. Memory Hierarchy**

The closer the memory is to the CPU, the faster and more expensive it becomes.

## **Registers**

Located inside CPU Fastest memory Very small capacity Holds operands and intermediate results Example

R1 = 20 R2 = 30

## **Cache Memory**

Stores frequently used instructions and data. Levels

L1 Cache L2 Cache L3 Cache Characteristics Very fast Built using SRAM Expensive Small capacity

## **Main Memory (RAM)**

Stores:

Running programs Process data Stack Heap Characteristics Volatile Built using DRAM Larger than cache Slower than cache

## **Secondary Storage**

Examples SSD HDD Characteristics Non-volatile Permanent storage Used by virtual memory

# **4. SRAM vs DRAM**

|**SRAM**|**DRAM**|
|---|---|
|Static RAM|Dynamic RAM|
|Stores data using fip-fops|Stores data using capacitors|
|No refresh required|Refresh required continuously|
|Faster|Slower|
|Expensive|Cheaper|
|Larger cell size|Smaller cell size|
|Less dense|More dense|
|Used in Cache|Used in Main Memory|

## **DRAM**

Stores every bit as an electrical charge inside a capacitor. Problem Charge leaks over time. Therefore,

Memory must be refreshed thousands of times every second. Advantages Cheap High capacity Used in Main Memory (RAM)

## **SRAM**

Stores data using flip-flops. Characteristics No refreshing Very fast Expensive Low capacity Used in L1 Cache L2 Cache L3 Cache

# **5. Responsibilities of Memory Management**

The Operating System performs several tasks.

## **Tracking**

Maintains information about Free memory Allocated memory Reserved memory

## **Allocation**

Allocates memory whenever a process requests it. Example

malloc() new

## **Protection**

Ensures Process A cannot access Process B’s memory.

## **Sharing**

Allows multiple processes to safely share memory when required. Example Shared Memory IPC.

## **Relocation**

Moves processes in memory when required. Useful during Compaction Swapping

## **Deallocation**

Releases memory after process termination.

# **6. Memory Allocation Techniques**

Two major approaches exist.

# **7. Contiguous Memory Allocation**

Each process occupies one continuous block of memory.

Simple but suffers from fragmentation.

# **8. Fixed Partitioning**

Static Partitioning

Memory is divided into fixed partitions during system startup. Each partition contains only one process.

Memory = 1 GB

|**Partition**<br>**Size**|
|---|
|P1<br>256 MB|
|P2<br>256 MB|
|P3<br>512 MB|
|Process|
|200 MB|
|Can ft into|
|P1 or P2|
|Process|
|400 MB|
|Must go into|
|P3|

## **Diagram**

Simple Fast allocation Low overhead

### **Internal Fragmentation**

Unused memory inside allocated partition. Example

Partition = 256 MB Process = 200 MB Unused = 56 MB

Memory wasted.

### **Limited Number of Processes**

Maximum processes = Number of partitions.

**Poor Memory Utilization**

Large partition assigned to a small process.

# **9. Dynamic Partitioning**

Also called Variable Partitioning

Memory is allocated according to process size. Partitions are created dynamically.

Total Memory

|1024 MB|
|---|
|Allocate|
|Process A = 200 MB|
|Remaining|
|824 MB|
|Allocate|
|Process B = 300 MB|
|Remaining<br>524 MB|
|Now|
|Process A fnishes.|
|Free Block = 200 MB|
|Memory becomes|
|200 MB Hole|
|524 MB Hole|
|New Process|
|250 MB|
|Cannot ft into 200 MB hole.|
|<br>This causes|
|External Fragmentation.|

Better utilization Flexible No internal fragmentation

|External fragmentation|
|---|
|Compaction required|
|Complex allocation algorithms|

# **10. Memory Allocation Strategies**

When multiple free blocks exist, OS chooses one.

## **First Fit**

Choose the first block large enough. Advantages Fast Disadvantages Leaves many small holes.

## **Best Fit**

Advantages Reduces wasted space. Disadvantages Slow search Creates many tiny holes

## **Worst Fit**

Choose the largest available block. Advantages Leaves large free blocks. Disadvantages May waste large memory regions.

# **11. Non-Contiguous Memory Allocation**

Processes need not occupy consecutive memory locations. Techniques

Paging Segmentation Paged Segmentation

# **12. Paging**

Paging eliminates the need for contiguous allocation. Memory is divided into fixed-size blocks. Logical Memory ↓ Pages Physical Memory ↓ Frames

Logical Memory +------+ |Page0 | +------+ |Page1 | +------+ |Page2 | +------+ ↓ Page Table ↓ Physical Memory +------+ |Frame3| +------+ |Frame0| +------+ |Frame5| +------+

Pages can be placed into any free frame.

Eliminates external fragmentation Easy allocation Efficient virtual memory

Small internal fragmentation Page table overhead

# **13. Address Translation**

- Logical Memory: 32 KB
- Page Size: 4 KB
- Number of Pages: 32 / 4 = 8 Pages
- Physical Memory: 16 KB
- Frames: 16 / 4 = 4 Frames

Suppose Page 0 → Frame 2.

- Logical Address: (Page 0, Offset 100)
- Physical Address: (Frame 2, Offset 100)

Translation performed using the Page Table.

# **14. Segmentation**

Memory divided according to logical units. Examples Function Array Stack Heap Data Each segment has variable size.

## **Segment Table**

Stores Base Address Limit Logical Address Segment Number + Offset

Logical organization Easier programming Better protection

**Disadvantages**

External fragmentation

# **15. Paged Segmentation**

Advantages: Better protection, Reduced fragmentation, Efficient allocation.

# **16. Virtual Memory**

Virtual Memory provides the illusion of larger memory. Uses Disk space RAM

Only required pages remain in RAM. Remaining pages stay on disk.

Execute large programs Better multitasking Efficient RAM utilization Process isolation

Page faults Disk access slower than RAM

# **17. Demand Paging**

Pages are loaded Only when required.

## **Page Fault**

Occurs when requested page is absent from RAM. OS

Finds free frame Loads page from disk Updates page table Restarts instruction

# **18. Fragmentation**

Memory fragmentation reduces memory utilization.

## **Internal Fragmentation**

Unused memory Inside allocated block. Example

Allocated 256 MB Used 220 MB Waste 36 MB

Occurs in Fixed partitioning Paging (last page)

## **External Fragmentation**

Free memory scattered into small holes. Example

100 MB Free + 50 MB Free + 75 MB Free

Total 225 MB Process needs 200 MB Cannot allocate because memory isn’t contiguous.

## **Solutions**

### **Compaction**

Move processes together. Combine small holes into one large hole. Disadvantage Slow.

Avoids external fragmentation.

### **Paging**

# **19. Swapping**

Swapping moves processes between RAM and disk.

Frees RAM Supports more processes Improves CPU utilization

Disk I/O overhead Slower execution

# **20. Memory Protection**

Memory protection prevents one process from accessing another process’s memory. Methods Base Register Limit Register MMU Page Protection Segment Protection Benefits Security Isolation Stability

# **21. Memory Management Unit (MMU)**

MMU is hardware that translates logical addresses into physical addresses.

Responsibilities: Address translation, Memory protection, Virtual memory support, Paging support.

# **22. Memory Management in Modern Operating Systems**

Modern operating systems (Linux, Windows, macOS) use multiple techniques together. They use

Paging

- Virtual Memory Demand Paging Multi-level Cache MMU Memory Protection Copy-on-Write (CoW) Page Replacement Algorithms

- This provides: Better performance Better security Efficient memory utilization Large virtual address space

# **23. Advantages of Memory Management**

- Efficient memory utilization Efficient CPU utilization Supports multitasking Enables virtual memory Provides memory protection Process isolation Better system performance Reduces memory wastage Supports larger applications Improves overall system stability


# **Additional Linux Memory Management Topics (Senior Linux Embedded Interviews)**

**These topics should be added after Chapter 22 (Memory Management in Modern Operating Systems) and before Interview Questions.**

They extend the existing notes with Linux-specific concepts commonly discussed in senior embedded interviews (Qualcomm, NVIDIA, AMD, Broadcom, Intel, etc.).

# **23. Linux Process Virtual Address Space ⭐⭐⭐⭐⭐**

Every Linux process has its own **Virtual Address Space** .

Although different processes may have identical virtual addresses, they map to different physical memory.

## **Memory Regions**

### **Text Segment**

Contains executable instructions. Characteristics Read-only Shared among processes Loaded from executable file

### **Data Segment**

Stores initialized global and static variables. Example

int count = 10;

### **BSS Segment**

Stores uninitialized global and static variables. Example

int count;

The operating system initializes BSS variables to zero.

### **Heap**

Used for dynamic memory allocation. Functions

malloc() calloc() realloc() free()

The heap grows upward.

### **Stack**

Stores Local variables Function parameters Return addresses The stack grows downward.

### **mmap Region**

Contains Shared libraries Memory mapped files Anonymous mappings Allocated using

# **24. Linux Memory Descriptor** **( mm_struct ) ⭐⭐⭐⭐⭐**

Every Linux process owns a structure called **mm_struct** . It describes the process’s entire virtual address space.

Important Information: Page Table Pointer, Virtual Memory Areas (VMAs), Code Segment, Data Segment, Heap, Stack, Memory Statistics.

**Interview Tip:**

Every process has one mm_struct. Threads belonging to the same process typically share the same mm_struct.

# **25. Virtual Memory Areas** **( vm_area_struct ) ⭐⭐⭐⭐⭐**

Linux divides a process’s virtual memory into regions called **Virtual Memory Areas (VMAs)** . Each region has its own permissions.

Each VMA contains: Start Address, End Address, Read Permission, Write Permission, Execute Permission, Backing File (optional).

# **⭐⭐⭐⭐⭐**

The **TLB** is a small hardware cache inside the CPU. It stores recently used page table translations.

### **TLB Hit**

Translation already exists. Very fast.

**TLB Miss**

Translation not found. CPU must walk the page table. This is slower.

Faster address translation Reduced memory access time

# **27. Multi-Level Page Tables ⭐⭐⭐⭐⭐**

Modern systems use multi-level page tables instead of a single large page table.

Advantages: Lower memory usage, Scalable for large address spaces.

# **28. Copy-on-Write (CoW) ⭐⭐⭐⭐⭐**

Linux uses Copy-on-Write during fork() . Initially, parent and child share the same physical pages.

Advantages: Faster process creation, Lower memory consumption.

# **29. mmap() ⭐⭐⭐⭐⭐**

mmap() maps files or anonymous memory into a process’s address space.

Types File-backed mapping

Anonymous mapping Shared mapping Private mapping Advantages Zero-copy access Efficient file I/O Shared memory support

# **30. Linux Page Cache ⭐⭐⭐⭐⭐**

The Page Cache stores recently accessed file data in RAM.

### **Read Hit**

Data already exists in cache. No disk access.

### **Read Miss**

Kernel loads data from disk into cache.

### **Dirty Page**

Modified page not yet written back to disk.

### **Writeback**

Dirty pages are eventually written to storage. Advantages Faster file access Reduced disk I/O

# **31. Major vs Minor Page Fault ⭐⭐⭐⭐⭐**

|**Minor Page Fault**|**Major Page Fault**|
|---|---|
|Page already in RAM|Page must be loaded from disk|
|No disk I/O|Requires disk I/O|
|Fast|Slow|
|Major page faults si|gnifcantly impact application performance.|

# **32. Buddy Memory Allocator ⭐⭐⭐⭐**

Linux allocates physical pages using the Buddy Allocator. Memory is divided into blocks whose sizes are powers of two.

Advantages: Fast allocation, Fast merging, Reduced fragmentation.

# **33. SLAB / SLUB Allocator ⭐⭐⭐⭐**

The Buddy Allocator allocates pages. Kernel objects are allocated using **SLAB** or **SLUB** . Examples

task_struct inode dentry file Advantages Reuses objects Faster allocation Less fragmentation

# **34. kmalloc() vs vmalloc() ⭐⭐⭐⭐**

|**kmalloc()**|**vmalloc()**|
|---|---|
|Physically contiguous memory|Virtually contiguous memory|
|Faster|Slightly slower|
|Used for DMA and drivers|Used for large allocations|
|Limited by contiguous physical memory|Easier to allocate large regions|

# **35. Linux Memory Zones ⭐⭐⭐**

Linux divides physical memory into zones. Common zones DMA DMA32 Normal HighMem (32-bit systems) Purpose Different hardware devices have different memory accessibility requirements.

# **36. Huge Pages ⭐⭐⭐**

Huge Pages use larger page sizes. Advantages Fewer page table entries Better TLB efficiency Improved performance for large memory workloads Linux also supports **Transparent Huge Pages (THP)** .

# 37. Out Of Memory (OOM) Killer

When the system cannot satisfy memory requests, Linux invokes the **OOM Killer** . Responsibilities

Select a victim process Free memory Prevent complete system failure Useful files /proc/<pid>/oom_score /proc/<pid>/oom_score_adj

# 38. Memory Debugging Commands

|**Command**|**Purpose**|
|---|---|
|free|Memory usage summary|
|vmstat|Virtual memory statistics|
|pmap|Process memory map|
|cat /proc//maps|Virtual memory layout|
|cat /proc//smaps|Detailed memory statistics|
|slabtop|SLAB allocator usage|
|top|Memory utilization|
|htop|Interactive monitoring|
|valgrind|Detect memory leaks|
|AddressSanitizer|Detect memory corruption|

# 39. Production Scenarios

**Scenario 1 – Memory Usage Continuously Increasing**

Possible Causes Memory leak Growing page cache Unreleased shared memory Debugging

top pmap cat /proc/<pid>/smaps

## **Scenario 2 – Cached Memory Is Very High**

Explanation Linux aggressively uses free RAM as **Page Cache** . This is normal.

The cache is reclaimed automatically when applications require memory.

## **Scenario 3 – OOM Killer Terminates Application**

dmesg cat /proc/<pid>/oom_score

Possible Causes Memory leak Excessive allocation Insufficient RAM

## **Scenario 4 – High Major Page Faults**

vmstat sar -B

#### Possible Causes

Working set larger than RAM Heavy swapping Slow storage

## **Scenario 5 – Slow fork()**

Possible Causes

Very large page tables Memory pressure Frequent page faults

Although Copy-on-Write makes fork() efficient, creating and copying page tables still has overhead.


---

---

## 7 — Interrupts (Deep Dive)

## **1. What Is an Interrupt?**

An interrupt is a mechanism by which hardware or software requests CPU attention. Without interrupts, the CPU would need to continuously check devices (Check NIC, Check Disk, Check UART, Check Timer, Check USB, Repeat…) — inefficient. With interrupts:

The CPU can perform other work until the device actually needs attention.

## **2. Why Are Interrupts Needed?**

Consider a NIC — without interrupts, the CPU must repeatedly ask “Is packet available?” (polling). With interrupts:

The CPU is notified only when necessary.

## **3. Basic Interrupt Flow**

The exact hardware details vary by architecture.

## **5. Interrupt Controller**

The CPU normally does not directly manage every device interrupt — an interrupt controller receives interrupt requests and routes them appropriately:

On modern systems there can be multiple interrupt-controller layers.

## **6. Interrupt Number**

Linux identifies interrupts using IRQ numbers, inspectable via cat /proc/interrupts :

CPU0       CPU1 40:       100         50   NIC 41:        20         30   NVMe

The exact output depends on the machine.

## **7. /proc/interrupts**

An extremely useful debugging interface ( cat /proc/interrupts ) showing IRQ number, interrupt count, per-CPU interrupt distribution, interrupt controller information, and device/driver association. This can help identify interrupt imbalance, interrupt storms, CPU affinity problems, and unexpected interrupt activity.

## **9. Interrupt Handler Responsibilities**

An interrupt handler should normally perform only urgent work: 1. Determine interrupt source 2. Acknowledge/clear interrupt 3. Read minimal device status 4. Capture necessary information 5. Schedule deferred processing 6. Return quickly Avoid doing large amounts of work directly in hard interrupt context.

## **10. Why Must Interrupt Handlers Be Fast?**

A long handler leaves the CPU unavailable for other work, which can delay networking, audio, storage, real-time workloads, and system responsiveness. So: do minimal work in the handler, defer expensive work.

## **11. Hard IRQ Context**

The immediate interrupt handler runs in interrupt context:

Important rule: code executing in hard interrupt context must not sleep.

## **12. Why Can’t IRQ Handlers Sleep?**

Sleeping means the current execution waits for something while the scheduler chooses another task. But an interrupt handler is not running as a normal schedulable process. Therefore, generally avoid mutex_lock() , kmalloc(..., GFP_KERNEL) , blocking I/O, and wait_event() in hard IRQ context.

## **14. Top Half**

Historically, interrupt processing was divided into Top Half and Bottom Half. The top half executes immediately when the interrupt occurs, typically: acknowledge interrupt, read status, save minimal information, schedule deferred work — then returns quickly.

## **16. Deferred Interrupt Processing**

Important mechanisms: Softirqs, Tasklets, Workqueues, Threaded IRQs. Understand the differences rather than memorizing old APIs.

## **21. Comparing Deferred Mechanisms**

|**Mechanism**|**Can Sleep?**|**Typical Use**|
|---|---|---|
|Hard IRQ|No|Immediate interrupt handling|
|Softirq|No|High-performance deferred kernel work|
|Tasklet|No|Legacy/simple deferred work|
|Workqueue|Yes|Deferred process-context work|
|Threaded IRQ|Yes in threaded part|Device interrupt processing|
|The exact kern|el execution conte|xt and rules matter more than memorizing the table.|

## **22. Interrupt Handler Example**

irqreturn_t my_irq_handler(int irq, void *data) { **struct** device_data *dev = data; _/* Read device status */_ status = readl(dev->base + STATUS); _/* Acknowledge interrupt */_ writel(status, dev->base + IRQ_ACK); _/* Defer expensive work */_ schedule_work(&dev->work); **return** IRQ_HANDLED; }

## **23. IRQ_RETURN Values**

An interrupt handler commonly returns IRQ_HANDLED when it handled the interrupt, or IRQ_NONE when the interrupt was not from that device:

This is particularly relevant for shared interrupts.

## **24. Shared Interrupts**

Multiple devices can sometimes share an interrupt line:

The handlers need to determine whether their device generated the interrupt (Handler A checks device A, etc.). If a handler did not handle the interrupt, IRQ_NONE can be returned.

## **25. Interrupt Storm**

An interrupt storm occurs when a device generates interrupts excessively — the CPU spends too much time handling interrupts. Symptoms: high CPU usage, poor application performance, high interrupt latency, system instability.

## **26. Causes of Interrupt Storms**

Possible causes: interrupt not acknowledged, interrupt status not cleared, hardware malfunction, driver bug, incorrect interrupt configuration, device repeatedly reporting the same event. Debug with cat /proc/interrupts and driver logs/tracing.

## **27. Interrupt Affinity**

On multicore systems, interrupts can be routed to particular CPUs, e.g. NIC IRQ --> CPU 2, or per-queue: RX queue 0 → CPU 0, RX queue 1 → CPU 1, etc. Important for high-performance networking and storage.

## **28. /proc/irq**

Linux exposes interrupt configuration through /proc/irq/<IRQ>/ — information can include affinity, interrupt controller information, and statistics, depending on kernel configuration.

## **29. SMP and Interrupts**

On a multicore system, the Interrupt Controller routes to CPU0/CPU1/CPU2/etc. — the kernel must coordinate interrupt processing across CPUs, which introduces concurrency issues.

## **30. Interrupts and Locking**

If a driver shares data between process context and interrupt context, a normal mutex may not be appropriate because the interrupt handler cannot sleep. The driver may need an IRQ-safe locking strategy:

spin_lock_irqsave(&lock, flags); ... spin_unlock_irqrestore(&lock, flags);

A common pattern when the same lock can be accessed from interrupt and process context.

## **31. Why spin_lock_irqsave()?**

If process context holds a lock and an IRQ arrives whose handler tries the same lock, the CPU can deadlock (the handler spins waiting for a lock held by the interrupted code). Disabling local interrupts while holding the lock prevents this specific re-entry scenario:

The exact locking strategy must match where the lock is used.

## **32. Spinlock in Interrupt Context**

A spinlock is appropriate when the critical section is short and code cannot sleep:

Do not perform long operations while holding a spinlock.

## **33. Interrupt Latency**

Interrupt latency is the time between the interrupt occurring and the handler starting: IRQ occurs --latency--> Handler begins. Low latency is important for real-time systems, audio, control systems, and high-performance devices.

## **34. Interrupt Processing Time**

Two separate concepts: **Interrupt latency** ( IRQ → handler starts ) and **Interrupt handling time** ( Handler starts → handler completes ). A system can have low latency but long handler execution, or the reverse.

## **35. Interrupt Coalescing**

High-speed devices can reduce interrupt frequency by combining multiple events. Without coalescing: each packet triggers an IRQ. With coalescing: multiple packets → one IRQ. Benefits: lower interrupt overhead, higher throughput. Trade-off: potentially higher latency. Widely used in NICs and other high-throughput devices.

## **37. MSI-X**

MSI-X supports multiple interrupt vectors — especially useful for devices with multiple queues, e.g. RX Queue 0 → IRQ 0, RX Queue 1 → IRQ 1, etc. These can be distributed across CPUs.

## **38. NIC Interrupt Flow**

A modern network receive path:

This connects interrupts with DMA, networking, and scheduling.

**39. NAPI**

## **41. Storage Interrupt Example**

Consider NVMe:

This is a very important senior Linux/storage flow.

## **42. Interrupt + DMA Relationship**

A common hardware pattern:

The CPU is not required to copy every byte.

## **45. Interrupt Safety Rules**

In hard interrupt context: - **DO:** keep handler short; use atomic/IRQ-safe synchronization; acknowledge interrupt; schedule deferred work; update protected state - **DON’T:** sleep; block; take a mutex that may sleep; perform long operations; perform unnecessary allocations

## **46. Common Interrupt Bugs**

2. **Sleeping in IRQ** — IRQ Handler --> Blocking operation → invalid context, can produce warnings or crashes.

3. **Race with shared state** — CPU 0 modifies state while the IRQ reads it concurrently; without proper synchronization, the interrupt may observe inconsistent data.

## **47. Debugging Interrupt Problems**

First check cat /proc/interrupts, looking for unexpectedly high interrupt counts, one CPU receiving all interrupts, interrupt count not increasing, or interrupt count increasing too rapidly. Then inspect dmesg, /sys, /proc/irq, ftrace, tracepoints, perf.

## **48. Interrupt Debugging Example**

Suppose CPU usage is 100%. cat /proc/interrupts shows IRQ 45: CPU0 = 50000000, CPU1 = 10 — suspicion: interrupt storm. Next investigate: which device owns IRQ 45? Is the interrupt being acknowledged? Is the device continuously generating events? Is IRQ affinity correct? Is the driver stuck?

## **49. Senior Interview Scenario**

**Question:** A device driver causes CPU usage to reach 100%. How would you debug it?

**Answer structure:** 1. Check /proc/interrupts 2. Identify rapidly increasing IRQ 3. Identify device/driver 4. Check whether interrupt is being acknowledged 5. Check driver logs 6. Check IRQ affinity 7. Check for interrupt storm 8. Inspect handler/deferred work 9. Trace interrupt activity if necessary 10. Check device/hardware state This is much stronger than simply saying “I would check the CPU.”

## **50. Interrupt Mental Model**

Memorize:

## **52. What You Must Master for Senior Interviews**

Priority order:

|★★★★★|Interrupt flow|
|---|---|
|★★★★★|Interrupt vs process context|
|★★★★★|Why IRQ handlers cannot sleep|
|★★★★★|Top half / deferred processing|
|★★★★★|Workqueues|
|★★★★★|Spinlocks and IRQ-safe locking|
|★★★★★|DMA + interrupt completion|
|★★★★★|MSI/MSI-X|
|★★★★★|Interrupt affinity|
|★★★★★|Interrupt storms|
|★★★★★|/proc/interrupts|
|★★★★☆|NAPI|
|★★★★☆|Wait queues|
|★★★★☆|Completions|
|★★★☆☆|Softirqs|
|★★★☆☆|Tasklets|

## **53. Final Connection**

Device drivers and interrupts should be understood together:

The most important senior-level idea: **a high-performance Linux driver normally configures hardware through MMIO, transfers bulk data through DMA, receives completion notifications through interrupts, performs only minimal work in hard IRQ context, and defers heavier processing to an appropriate context.**


---

---

# 8 — Networking Basics

# **1. Why Linux Networking Internals?**

For senior Linux, embedded, networking, infrastructure, and system roles, you should understand what happens after an application does:

send(); recv();

The important path is:

This is the core Linux networking mental model.

# **2. Linux Networking Stack**

A simplified Linux networking stack:

# **3. Socket**

A socket is the primary interface applications use to communicate through the networking stack. Example:

int fd = socket(AF_INET, SOCK_STREAM, 0);

The application receives a file descriptor.

This follows an important Linux principle: A socket is exposed to user space through a file descriptor.

# **4. Socket Types**

Common socket types:

# **5. TCP Socket Lifecycle**

Server:

Client:

# **6. What Happens During** **socket() ?**

Conceptually:

The returned FD refers to the kernel-managed socket object.

# **7. Socket and File Descriptor**

This connects networking internals to Linux VFS/file-descriptor concepts.

# **8.** **bind()**

A server typically binds a socket to:

IP address + Port

bind(fd, ...);

# **9.** **listen()**

For TCP servers:

listen(fd, backlog);

puts the socket into a listening state. Conceptually:

# **10.** **accept()**

When a TCP connection is established:

int client_fd = accept(server_fd, ...);

The listening socket remains available for additional connections. Conceptually:

#### This is important:

accept() creates/returns a connected socket for the client connection; it does not turn the listening socket into the connection.

# **11. TCP Send Path**

Suppose an application executes:

send(fd, data, len, 0);

#### Simplified path:

|---|

# **12. TCP Receive Path**

Incoming packet:

This path is extremely important for interviews.

# **13. NIC Driver**

The NIC driver connects Linux networking to hardware. Conceptually:

The driver handles things such as:

Transmit Receive DMA Interrupts Descriptor rings Device configuration Offloads

# **14. Network Device**

Linux represents network interfaces through structures associated with:

||**struct**net_device|
|---|---|
|Concept|ually:|
|net_devi<br>||ce|
|<br>+--|Interface name|
|+--<br>+--<br>+--<br>|MAC address<br>MTU<br>Device operations<br>|
|+--<br>+--|Statistics<br>Queue information|
|Example|interface:|
|eth0||
|or:<br>ens33||

# **15. Network Device Operations**

The driver provides operations that allow the networking subsystem to interact with the hardware. Conceptually:

NIC

The exact driver APIs evolve across kernel versions.

# **16.** **sk_buff**

One of the most important Linux networking structures is:

**struct** sk_buff

Often called:

skb It represents a network packet/buffer within the networking stack. Conceptually:

skb | +-- Packet data +-- Length +-- Protocol information +-- Network header +-- Transport header +-- Device information +-- Metadata

You should know sk_buff for senior Linux networking interviews.

# **17. Packet Flow Using** **sk_buff**

The packet is represented and manipulated through kernel networking buffers.

# **18. Receive Path – Detailed View**

A simplified receive path:

This is one of the most important diagrams in this chapter.

# **19. DMA**

DMA means:

Direct Memory Access

The NIC can transfer packet data to system memory without requiring the CPU to copy every byte itself.

This significantly improves networking performance.

# **20. Why DMA Is Important**

This consumes CPU cycles. With DMA:

The CPU primarily handles control and packet-processing work rather than copying every byte.

# **21. Descriptor Ring**

High-performance NICs commonly use descriptor rings. Conceptually:

+----+----+----+----+----+ | D0 | D1 | D2 | D3 | D4 | +----+----+----+----+----+ ^                   ^ |                   | Producer            Consumer

Descriptors describe buffers or packet ownership/state. There can be:

RX ring TX ring

# **22. RX Ring**

Receive path:

The NIC uses descriptors to determine where incoming packets should be placed. The driver processes completed descriptors.

# **23. TX Ring**

Transmit path:

The driver provides the NIC with buffers/descriptors describing packets to transmit.

# **24. Interrupts in Networking**

A basic receive model could be:

But doing too much packet processing directly in hard IRQ context would be inefficient. Linux therefore uses mechanisms such as:

NAPI

# **25. NAPI**

NAPI stands for:

New API

It combines interrupt notification with polling for packet processing. Basic idea:

This reduces interrupt overhead under high packet rates.

# **26. Why NAPI?**

Suppose 1 million packets arrive. Without efficient batching:

Huge interrupt overhead. With NAPI:

|---|---|
|+--|Packet 2|
|+--|Packet 3|
|+--|...|
|+--|Packet N|

Batch processing improves scalability.

# **27. Interrupt Mitigation**

NICs can also use interrupt moderation/coalescing. Instead of:

for every packet, the NIC may delay/coalesce notifications. Conceptually:

|---|
|Packet 2<br>|
|Packet 3<br>|

This reduces interrupt overhead but may increase latency. Therefore:

Latency vs Throughput

must be balanced.

# **28. TX Path**

Simplified transmit path:

# **29. Routing**

Before transmitting an IP packet, Linux needs to determine where it should go. Conceptually:

# **30. Routing Table**

Linux maintains routing information. Useful command:

ip route Example conceptually: default via 192.168.1.1 dev eth0 192.168.1.0/24 dev eth0 Meaning: Local subnet → eth0 Everything else → default gateway

# **31. ARP**

For IPv4, Linux may need to map:

IP address ↓ MAC address

This is ARP. Example:

# **32. Neighbor Table**

Useful command:

ip neigh

IP              MAC 192.168.1.1  →  AA:BB:CC:DD:EE:FF

For IPv6, neighbor discovery performs the corresponding neighbor-resolution functions.

# **33. Ethernet Frame**

At the link layer:

The IP packet is carried inside the Ethernet frame when Ethernet is used.

# **34. IP Packet**

Linux networking layers process the appropriate headers at each stage.

# **35. TCP Segment**

For TCP:

TCP provides:

Reliable delivery, Ordering, Retransmission, Flow control, Congestion control.

# **36. UDP Datagram**

UDP is simpler:

UDP does not itself provide TCP-like:

Reliable delivery

Ordering Retransmission

# **37. TCP Receive Path**

Incoming TCP packet:

# **38. Socket Lookup**

When a packet arrives, Linux must determine which socket should receive it. Conceptually:

This is essential for multiplexing network traffic among applications.

TCP maintains receive state and buffering. Conceptually:

If the application is slow:

TCP flow control helps prevent the sender from overwhelming the receiver.

Similarly:

send() returning successfully does not necessarily mean the remote application has received the data. It generally means the data was accepted according to the local socket’s send semantics.

# **41. TCP Three-Way Handshake**

Connection establishment:

Then:

TCP Connection Established

Linux maintains TCP connection state in kernel structures associated with the socket.

# **42. TCP State Machine**

Important states include:

CLOSED LISTEN SYN-SENT SYN-RECEIVED ESTABLISHED FIN-WAIT CLOSE-WAIT LAST-ACK TIME-WAIT

Senior interviews often ask about:

TIME_WAIT CLOSE_WAIT

# **43.** **TIME_WAIT**

After TCP connection termination, one side can enter:

TIME_WAIT

It helps ensure delayed packets from the old connection do not interfere with a new connection using the same connection identifiers.

It also supports correct handling of TCP connection termination.

# **44.** **CLOSE_WAIT**

#### CLOSE_WAIT means:

A large number of CLOSE_WAIT sockets often indicates an application that is not closing connections properly.

Linux includes packet filtering and networking hooks through:

##### Netfilter

use the kernel’s packet-filtering infrastructure.

# **46. Firewall Path**

A simplified incoming path:

The exact hook ordering depends on packet direction and networking configuration.

# **47. NAT**

Network Address Translation changes packet address/port information according to configured rules. Example:

Private: 10.0.0.10:5000 NAT

Public: 203.0.113.10:40000

Linux implements NAT using networking infrastructure including Netfilter/connection tracking.

# **48. Connection Tracking**

Connection tracking allows Linux to maintain state about flows. Conceptually:

For TCP, state can reflect the connection lifecycle. This is important for:

NAT Stateful firewalling Load balancing Containers

# **49. Network Namespaces**

Linux network namespaces provide isolated network stacks. Conceptually:

|---|
|<br>+-- Network Namespace A<br>|
||      ||
|<br>|      +-- eth0<br>|
||      +-- routes<br>|
||      +-- sockets<br>||
|<br>+-- Network Namespace B<br>||
|<br>+-- eth0<br>|
|+-- routes<br>|
|+-- sockets|
|Containers use network namespaces extensively.|

# **50. Virtual Ethernet Pair**

A common container networking mechanism is a veth pair.

Namespace A | veth0

Packets entering one side appear on the other side.

# **51. Linux Bridge**

A Linux bridge operates like a Layer-2 switch.

The bridge forwards Ethernet frames based on MAC addresses. This is common in container networking.

# **52. Container Networking Flow**

Simplified:

This is an important Linux networking internals concept for Docker/Kubernetes roles.

# **53.** **iptables vs** **nftables**

Historically: iptables

was widely used for Linux packet filtering and NAT. Modern Linux systems increasingly use:

nftables

as the newer packet-filtering framework. For interviews:

|Netfilter<br>↓<br>Kernel packet-filtering infrastructure|
|---|
|nftables|
|↓<br>Modern user-facing framework|

# **54.** **tc**

Linux provides traffic control through:

|tc|
|---|

It can implement:

|Queuing<br>Shaping<br>Scheduling<br>Classification<br>Filtering|
|---|
|Conceptually:|

# **55. Qdisc**

A qdisc controls how packets are queued before transmission. Conceptually:

Examples of scheduling algorithms include:

FIFO Fair queuing variants Classful schedulers

The exact default depends on Linux configuration/version.

# **56. Offloading**

Modern NICs can offload some work from the CPU. Examples:

Checksum offload TSO GSO GRO RSS

The goal is to reduce CPU overhead and improve throughput.

# **57. TSO**

TCP Segmentation Offload allows the kernel to hand a larger TCP packet representation to the NIC, which can perform segmentation into smaller wire packets.

This reduces per-packet CPU work.

# **58. GSO**

Generic Segmentation Offload allows segmentation to be deferred within the networking stack/NIC path. Conceptually:

Large packet representation | v Segmentation later

# **59. GRO**

Generic Receive Offload combines packets received from the network where appropriate to reduce per-packet processing overhead.

# **60. RSS**

Receive Side Scaling distributes received packets across CPUs/queues.

# **61. RPS and RFS**

Linux also provides software mechanisms for distributing packet processing. Conceptually:

RPS ↓ Software packet processing distribution

RFS can consider the CPU where the receiving application is running to improve locality. These mechanisms can interact with:

RSS CPU affinity NUMA

# **62. Zero-Copy Networking**

Traditional path may involve copying:

Zero-copy techniques try to reduce unnecessary copies. Examples/concepts include:

sendfile() splice() mmap() io_uring-related networking paths AF_XDP

The exact zero-copy behavior depends on the API, device, protocol, and workload.

# **63.** **sendfile()**

sendfile() can transfer data between file and socket descriptors without requiring the application to explicitly copy the data through its own user-space buffer.

This can reduce user/kernel copying overhead.

# **64.** **epoll**

For scalable network servers:

##### epoll

allows an application to monitor many file descriptors. Conceptually:

The application waits for readiness events.

# **65. Event-Driven Server**

Typical architecture:

This avoids requiring one thread per connection.

# **66. Blocking vs Nonblocking Sockets**

Blocking:

for high-concurrency servers.

# **69. Networking + Interrupt + Scheduler**

This is a very important senior-level connection.

are interconnected.

# **70. Networking Performance Bottlenecks**

When networking performance is poor, investigate:

NIC speed CPU utilization IRQ distribution NAPI budget RX/TX ring sizes Packet drops Socket buffers TCP congestion control MTU GRO/GSO/TSO RSS/RPS CPU affinity NUMA locality qdisc Memory pressure

Do not immediately assume:

"Network is slow."

The bottleneck may actually be CPU, memory, scheduling, IRQ distribution, or application processing.

# **71. Useful Linux Commands**

### **Interfaces**

ip link

### **IP addresses**

ip addr

### **Routing**

ip route

### **Socket information**

ss -tulnp

### **Network statistics**

ip -s link

### **Interface statistics**

ethtool eth0

### **Driver information**

ethtool -i eth0

### **Interrupts**

cat /proc/interrupts

cat /proc/net/dev

# **86. What You Must Master**

For senior Qualcomm / AMD / NVIDIA / Intel / Linux networking interviews:

★★★★★ Linux socket architecture ★★★★★ TCP/UDP kernel path ★★★★★ sk_buff ★★★★★ RX/TX path ★★★★★ NIC driver ★★★★★ DMA ★★★★★ Descriptor rings ★★★★★ NAPI ★★★★★ Interrupts ★★★★★ Routing ★★★★★ Netfilter ★★★★★ Socket buffers ★★★★★ Network namespaces ★★★★★ veth ★★★★★ Linux bridge ★★★★★ epoll ★★★★☆ RSS/RPS/RFS ★★★★☆ GRO/GSO/TSO ★★★★☆ qdisc ★★★★☆ Zero-copy ★★★★☆ Connection tracking ★★★★☆ NUMA networking

# **87. Final Networking Mental Model**

The most important diagram in this chapter:

|USER SPACE<br>||
|---|

The senior-level chain to memorize is:

For receive:

If you understand these two paths deeply, you have the foundation needed to answer most **Linux networking internals** questions at the senior embedded/kernel level.


---

---

# 9 — Block I/O**

# **Objectives**

- After completing this chapter, you should understand: What block I/O is Block devices vs character devices Linux block layer BIO Requests Request queues I/O schedulers Buffered I/O Direct I/O Page cache interaction Read and write paths DMA Interrupt-driven I/O NVMe vs SATA at a high level I/O completion Important interview questions

# **1. What is Block I/O?**

Block I/O is the mechanism Linux uses to communicate with storage devices that operate on blocks of data. Examples:

HDD SSD NVMe SSD USB storage eMMC SD card These devices are generally accessed through the Linux block layer.

# **2. What is a Block Device?**

A block device provides storage that can be accessed in units of blocks/sectors. Examples:

/dev/sda /dev/sdb /dev/nvme0n1 /dev/mmcblk0 Conceptually:

# **3. Block Device vs Character Device**

This is an important interview question.

## **Block Device**

Designed for block-oriented storage. Examples:

HDD SSD NVMe eMMC

## **Character Device**

Provides a stream-oriented interface. Examples:

Serial port Terminal Some sensors Some device drivers

|Block Device|
|---|
|Data<br>+----+----+----+----+|
|| B0 | B1 | B2 | B3 |<br>+----+----+----+----+|
|Character device:|
|Data stream|

# **4. Why Do We Need the Block Layer?**

Different storage devices have different hardware interfaces. For example:

SATA NVMe USB Storage eMMC

Linux applications should not need to know these hardware details.

The block layer provides a common abstraction.

# **5. High-Level Storage Stack**

For direct I/O, the page-cache path can be bypassed.

Consider:

read(fd, buffer, 4096);

Simplified flow:

write(fd, buffer, 4096);

Application

The write does not necessarily reach the physical device immediately.

# **8. Direct I/O**

Applications can request direct I/O using mechanisms such as:

|O_DIRECT|
|---|
|Conceptually:|

The page cache is generally bypassed for the file data path. Direct I/O has alignment and filesystem-specific restrictions.

# **9. Why Use Direct I/O?**

Potential reasons include: Database workloads Applications with their own caching Avoiding double buffering Predictable I/O behavior in some workloads But direct I/O is not automatically faster. It increases application responsibility for: Alignment Buffer management Caching I/O behavior

# **10. What is a BIO?**

BIO is a kernel structure used to represent an I/O operation at the block layer. Conceptually:

|BIO<br>||
|---|---|
||||
|<br>+--<br>|Operation<br>|
|||||
||<br>|+-- READ<br>|
||<br>||+-- WRITE|
|<br>+--<br>|Sector information|
||<br>+--<br>|Data segments|
||||
|<br>+--|Completion information|

A BIO describes the data involved in an I/O operation.

# **11. BIO Mental Model**

Suppose the filesystem needs to read several sectors. Conceptually:

The block layer processes the I/O and eventually sends it toward the device driver.

# **12. BIO Is Not the Physical Device Request**

This is an important distinction.

A BIO represents an I/O operation at a particular layer.

The block layer may combine, split, transform, or schedule I/O before it reaches the hardware. Conceptually:

The exact internal path varies by kernel version and block architecture.

# **13. Request**

A block request represents work that the block layer sends toward a device queue. Conceptually:

Multiple BIOs may be associated with a request depending on the I/O path and whether they can be merged.

# **14. I/O Request Flow**

This is the core block-I/O mental model.

# **15. Request Queue**

The block layer manages I/O through queues associated with block devices. Conceptually:

The queue allows the kernel and driver to manage outstanding operations.

# **16. Why Queue I/O?**

Storage devices can process multiple operations. Instead of:

READ wait WRITE wait READ wait the system can maintain multiple outstanding requests.

READ WRITE READ WRITE READ

This allows better utilization of modern storage devices.

# **17. I/O Scheduling**

Linux can use I/O scheduling mechanisms to manage block requests. Goals may include:

Throughput Latency Fairness Request merging Device utilization

Historically Linux used schedulers such as:

CFQ Deadline NOOP

Modern Linux also uses: mq-deadline BFQ none

depending on kernel/device configuration.

# **18. Why Multiple I/O Schedulers?**

Different workloads have different requirements. For example:

Desktop Server Database Embedded system NVMe storage

may benefit from different scheduling behavior.

# **19. I/O Scheduler Example**

Suppose requests arrive:

READ sector 100 READ sector 101 READ sector 5000 READ sector 102

A scheduler may reorder or merge operations where appropriate. Conceptually:

Before:

100 101 5000

102

After:

100 101 102 5000

The exact behavior depends on the scheduler and device.

# **20. Request Merging**

Suppose:

Request A: READ sectors 100-103

Request B: READ sectors 104-107

These may be merged into:

READ sectors 100-107

Merging can reduce overhead.

# **21. Random vs Sequential I/O**

## **Sequential**

100 101 102 103 104

Data is accessed continuously.

## **Random**

100 5000 72 9000 301

Accesses are scattered.

Historically, HDDs benefited significantly from request ordering because of seek time. Modern SSD/NVMe devices have very different characteristics.

# **22. HDD vs SSD**

## **HDD**

Uses:

Mechanical head Rotating platters Seek Rotation

Random I/O can be expensive.

## **SSD**

Flash memory No mechanical seek

Much lower random-access latency.

# **23. NVMe**

NVMe is a protocol designed for high-performance non-volatile storage, especially PCIe-connected SSDs. Conceptually:

NVMe supports many queues and high concurrency.

# **24. SATA vs NVMe**

SATA:

NVMe:

NVMe is designed for much higher parallelism and lower protocol overhead.

# **25. DMA**

DMA stands for:

Direct Memory Access DMA allows a device to transfer data to/from memory without the CPU copying every byte. Conceptually: Without DMA:

# **26. Why DMA Is Important**

Suppose a network card receives:

|1 MB|
|---|
|Without DMA:|

CPU | +-- Configure DMA +-- Handle completion

This improves efficiency.

# **27. Storage + DMA**

For a storage read:

The device/controller transfers data directly into memory.

# **28. Interrupts and I/O Completion**

After an I/O operation completes, the device needs to notify the CPU. One mechanism is an interrupt. Conceptually:

# **29. Interrupt + DMA**

A common high-level flow:

# **30. Polling vs Interrupts**

Devices can sometimes be handled using polling rather than interrupts.

## **Interrupt**

CPU does not continuously check the device.

**Polling**

Polling can be useful for very high event rates because interrupt overhead can become expensive.

# **34. Direct I/O Path**

With direct I/O:

The normal file-data page-cache path is bypassed.

# **35. I/O Completion**

A simplified completion model:

Suppose a process executes:

and data is unavailable. The process may sleep:

SLEEPING

When the I/O completes:

# **37. Nonblocking I/O**

A file descriptor may be configured for nonblocking operation. Example:

O_NONBLOCK

Instead of waiting indefinitely:

The exact return/error behavior depends on the object and operation.

# **38. Synchronous vs Asynchronous I/O**

## **Synchronous**

The caller waits for the operation to complete.

## **Asynchronous**

The application can continue while the I/O progresses. Conceptually:

text
Application
 ├── submit I/O
 ├── continue work
 └── receive completion

Linux provides several mechanisms for asynchronous I/O.

# **39. Important Distinction**

Do not confuse:

Nonblocking I/O with: Asynchronous I/O Nonblocking means: Do not wait if the operation cannot proceed immediately. Asynchronous I/O means: Submit the operation and receive completion separately. They are related but not identical concepts.

# **40. Block Layer and Filesystem**

The filesystem determines what storage operations are needed.

The block layer does not understand the full meaning of the file. It primarily handles block-device I/O.

# **41. Storage Stack Mental Model**

For a page-cache hit, the lower part may not be needed.

# **42. Example: Reading a File**

file.txt is stored on an NVMe SSD. Application:

Flow:

v NVMe Controller | v Flash

Completion:

# **53. Senior Interview Whiteboard Flow**

You should be able to draw:

And explain the return path:

# **54. What You Must Remember**

### **Block device**

Storage device accessed through block I/O.

### **Block layer**

Generic kernel infrastructure between filesystem and block-device driver.

### **BIO**

Represents an I/O operation and its data segments.

### **Request**

Block-layer work sent toward a device queue.

### **I/O scheduler**

Manages/schedules block I/O.

### **DMA**

Device ↔ RAM transfer without CPU copying every byte.

### **Page cache**

Caches filesystem data in RAM.

# **55. Final Mental Model**

The complete storage path to remember is:

# **Chapter Summary**

Linux uses the block layer to provide a common abstraction for block storage devices. The important concepts are:

The most important end-to-end flow is:

For senior Linux Systems, Storage, Embedded, and Infrastructure interviews, you should be able to explain this flow and clearly distinguish:

BIO Request Block Layer I/O Scheduler DMA Interrupt Page Cache Buffered I/O Direct I/O

without memorizing kernel source code.


---

---

# 10 — Kernel Locking, Synchronization & RCU

After completing this chapter, you should understand: - Why the kernel needs synchronization primitives beyond simple mutexes - Spinlocks, mutexes, semaphores, and when each is legal to use - Atomic operations and per-CPU variables - Seqlocks - RCU (ReadCopy-Update) — the mechanism senior/staff Linux interviews lean on hardest - lockdep, KASAN, and how real kernel concurrency

bugs are found - A decision table for “which lock do I use here?”

# **1. Why Kernel Locking Is Different From User-Space Locking**

In user space, a thread that blocks on a mutex is simply rescheduled — the OS handles it. Inside the kernel, the code holding the lock **might itself be** :

So the kernel needs a _family_ of primitives, each legal in a different context. Picking the wrong one is one of the most common senior-level interview traps (and real production bugs).

# **2. Spinlock **

A spinlock busy-waits — the CPU spins in a loop until the lock is free. It never sleeps.

spin_lock(&lock); _/* critical section */_ spin_unlock(&lock);

**Rules** - Never sleep while holding a spinlock (no kmalloc(GFP_KERNEL) , no mutex_lock() , no blocking I/O). - Safe to use in interrupt context — _if_ you use the IRQ-safe variant. - Held for a very short time only; spinning wastes CPU.

## **2.1 spin_lock vs spin_lock_irq vs spin_lock_irqsave**

|**Variant**|**Disables local IRQs?**|**Saves/restores IRQ state?**|**When to use**|
|---|---|---|---|
|spin_lock()|No|No|Data never touched from interrupt<br>context|
|spin_lock_irq()|Yes|No (assumes IRQs were enabled)|Data touched from process context<br>and interrupts, and you know IRQs<br>were on|
|spin_lock_irqsave()|Yes|Yes|Data touched from interrupt<br>context and you don’t know the<br>caller’s IRQ state — the safe default|

unsigned long flags; spin_lock_irqsave(&lock, flags); _/* critical section, safe against this CPU's interrupts too */_ spin_unlock_irqrestore(&lock, flags);

**Why this matters:** if a process holds a plain spinlock and an interrupt fires on the _same CPU_ whose handler tries to take the same lock, that CPU deadlocks against itself — the interrupt handler spins forever waiting for a lock held by code that can’t run until the interrupt returns. spin_lock_irqsave() prevents this by disabling interrupts on the local CPU for the duration of the critical section.

## **2.2 Spinlock on Uniprocessor vs SMP**

On SMP: real spinning happens (another CPU may hold the lock). On UP (or with preemption considerations): spin_lock() effectively becomes “disable preemption,” since there’s no other CPU to be spinning against.

# **3. Mutex **

A kernel mutex puts the waiting task to sleep instead of spinning.

mutex_lock(&mtx); _/* critical section - can sleep, can call kmalloc(GFP_KERNEL), can block on I/O */_ mutex_unlock(&mtx);

**Rules** - Only usable in process context (never in interrupt/softirq context). - Only the task that locked it may unlock it (unlike a semaphore). - Cannot be held across a context that might not resume it (careful with cross-CPU handoff patterns).

## **Spinlock vs Mutex**

||**Spinlock**|**Mutex**|
|---|---|---|
|Waiting behavior|Busy-wait (spin)|Sleep|
|Usable in interrupt context|Yes (irqsave variant)|No|
|Hold duration|Very short|Can be longer|
|CPU cost while waiting|Wastes CPU cycles|Frees CPU for other tasks|
|Typical use|Protecting small, fast-access data (a counter, a<br>list pointer)|Protecting a section that may sleep or take a<br>while|

**Interview one-liner:** _“Spin if the critical section is short and you can’t sleep; sleep (mutex) if the critical section might block or take a while.”_

# **4. Semaphore **

A counting synchronization primitive — allows N holders instead of just one.

**struct** semaphore sem;

sema_init(&sem, N); down(&sem); _/* acquire (may sleep) */ /* critical section */_ up(&sem); _/* release */_

- Binary semaphore (count = 1) behaves similarly to a mutex but **without ownership tracking** — any task can call up() , not just the one that called down() .

Largely superseded by mutexes in modern kernel code where mutual exclusion (not counting) is the goal. Still used where a genuine _counting_ resource limit is needed (e.g., limiting concurrent access to N identical resources).

# **5. Atomic Operations **

For simple counters, full locking is overkill. The kernel provides atomic types and operations implemented with CPU-level atomic instructions (e.g., LOCK prefix on x86, LDXR/STXR on ARM).

atomic_t counter = ATOMIC_INIT(0); atomic_inc(&counter); atomic_dec(&counter); atomic_add(5, &counter); int val = atomic_read(&counter); **if** (atomic_dec_and_test(&counter)) { _/* counter reached zero */_ }

**Why atomics matter:** they avoid the overhead of a full lock (no spinning, no context switch, no scheduler involvement) for operations that hardware can do atomically in a single instruction.

**Common interview question:** _“Why not just use_ _i++ on a shared integer?”_ → i++ is read-modify-write across multiple instructions; two CPUs can interleave and lose an update. atomic_inc() is a single indivisible hardware operation.

# **6. Per-CPU Variables **

Instead of locking a single shared counter, give every CPU its own private copy.

DEFINE_PER_CPU(int, my_counter);

this_cpu_inc(my_counter); _/* no locking needed */_ int val = per_cpu(my_counter, cpu);

**Advantages** - Zero lock contention — each CPU only touches its own copy. - Excellent cache locality (no cache-line bouncing between CPUs).

**Used heavily in:** networking statistics, scheduler run-queue data, per-CPU memory allocator caches (SLAB per-CPU caches). **Caveat:** code accessing a per-CPU variable must not be preempted and migrated to another CPU mid-access — the kernel provides get_cpu()/put_cpu() or this_cpu _ *() helpers that handle this safely.

# **7. Seqlock (Sequence Lock) **

Optimized for **read-mostly, write-rare** data, where readers should never block writers.

seqlock_t sl = SEQLOCK_UNLOCKED; _/* Writer */_ write_seqlock(&sl); _/* update data */_ write_sequnlock(&sl); _/* Reader */_ unsigned seq; **do** { seq = read_seqbegin(&sl); _/* read data */_ } **while** (read_seqretry(&sl, seq));

**How it works:** a sequence counter is incremented before and after every write. A reader records the counter, reads the data, then checks whether the counter changed (or is odd, meaning a write is in progress). If it changed, the reader retries. **Key property:** writers are never blocked by readers, and readers never block each other — but readers may have to retry. Used for data like jiffies / timekeeping where writes are rare and reads are extremely frequent.

**Not safe for:** data containing pointers that a concurrent writer might free — a reader could dereference a stale pointer mid-read (this is one motivation for RCU, below, when the read side involves pointers/lists).

# **8. RCU – Read-Copy-Update **

**This is the single most common gap in mid-level notes, and one of the most-asked topics in senior/staff Linux kernel interviews.**

## **8.1 The Problem RCU Solves**

Imagine a linked list read very frequently (e.g., on every packet, every syscall) and updated rarely. Using a spinlock or rwlock for every read would: - Add overhead to a hot read path - Create cache-line contention across many CPUs reading “at the same time” RCU allows **readers to proceed with zero locking overhead** , even while a writer is concurrently updating the structure.

## **8.2 Core Idea**

- Readers:  rcu_read_lock() → read pointer → rcu_read_unlock() (no blocking, no atomic instructions, nearly free) Writers:  1. Create a new copy of the data 2. Update the pointer to point to the new copy (atomic pointer write) 3. Wait for a "grace period" (all pre-existing readers to finish) 4. Free the old copy

## **8.3 Reader Side**

rcu_read_lock(); **struct** foo *p = rcu_dereference(shared_ptr); **if** (p) use(p->field); rcu_read_unlock();

- rcu_read_lock() / rcu_read_unlock() are extremely cheap — on most architectures they just disable preemption; they are **not** a real lock and never block.

- rcu_dereference() ensures correct memory ordering when reading the pointer (the reader must never see a partiallyconstructed new object).

## **8.4 Writer Side**

**struct** foo *new_foo = kmalloc( **sizeof** (*new_foo), GFP_KERNEL); *new_foo = *old_foo; new_foo->field = updated_value; rcu_assign_pointer(shared_ptr, new_foo); _/* publish new version */_ synchronize_rcu(); _/* block until all current readers finish */ /* or: call_rcu(&old_foo->rcu, free_callback);  -- async version */_ kfree(old_foo);

- rcu_assign_pointer() performs the pointer update with the correct memory barrier so readers never observe a half-initialized object.

- synchronize_rcu() blocks the writer (can sleep) until a **grace period** has elapsed — i.e., until every CPU has passed through at least one point where it’s guaranteed not to be holding a reference from before the update.

- call_rcu() is the non-blocking alternative: register a callback to run after the grace period, and continue immediately. Very common in interrupt-adjacent or performance-sensitive writer paths.

## **8.5 What Is a “Grace Period”?**

A grace period is the time the kernel waits to guarantee that **no CPU is still executing inside an RCU read-side critical section that began before the update** . Once the grace period ends, it is safe to free the old data — every reader that could have seen the old pointer has finished with it.

CPU0: [rcu_read_lock.... rcu_read_unlock]   ← reader in progress CPU1:                     writer updates pointer, calls synchronize_rcu() CPU1: [[[[[[[[[[[[[[[[[[[ blocked/waiting ]]]]]]]]]]]]]]]]]]] CPU0:                                          [unlock happens here] CPU1: <-- grace period ends, synchronize_rcu() returns, old data can be freed

## **8.6 RCU vs rwlock — Why RCU Wins for Read-Heavy Data**

||**rwlock**|**RCU**|
|---|---|---|
|Reader cost|Atomic operation, cache-line contention across<br>CPUs|Near-zero, no atomic instruction needed on the<br>fast path|
|Readers block writers?|Yes|No — writer proceeds immediately, old data just<br>isn’t freed yet|
|Writers block readers?|Yes|No — readers may briefy see the old version,<br>never a corrupt one|
|Scales with CPU count|Poor (readers contend on the lock’s cache line)|Excellent|
|Complexity|Simple|Higher — requires understanding grace periods,<br>careful use of<br>rcu_dereference/<br>rcu_assign_pointer|

**The core trade RCU makes:** readers get near-zero cost and never block, in exchange for delayed reclamation (freeing memory isn’t immediate) and the requirement that updates use copy-and-replace rather than in-place mutation of anything a reader might be looking at.

## **8.7 Where RCU Is Used in Linux**

Routing tables and networking data structures (very read-hot, e.g., fib lookups)

dentry / dcache lookups in the VFS (pathname resolution is one of the hottest read paths in the kernel) Module lists, list of loaded netfilter rules

Many “list of things looked up on every packet/syscall, rarely modified” structures

## **8.8 RCU Interview Traps**

- **“Can rcu_read_lock() sleep?”** No — RCU read-side critical sections must not sleep (in the classic/non-preemptible RCU flavor commonly discussed). This is why RCU works well for hot paths but can’t replace a mutex-protected section that needs to block.

- **“Does the reader see the old or new data?”** Either is valid — a reader that started before the update may still see the old, fully-consistent version; a reader that starts after sees the new one. What RCU guarantees is that no reader ever sees a _torn_ or partially-updated object.

- **“When is the old object actually freed?”** Only after the grace period completes — not immediately at rcu_assign_pointer() time.

# **9. Decision Table — Which Primitive Do I Use?**

|**Scenario**|**Use**|
|---|---|
|Very short critical section, might be touched from interrupt context|Spinlock (<br>spin_lock_irqsave )|
|Critical section might sleep / call blocking allocation / take a while|Mutex|
|Need to allow N concurrent holders of a resource|Semaphore|
|Simple counter increment/decrement|Atomic operations|
|Per-CPU statistics/counters, no cross-CPU sharing needed|Per-CPU variables|
|Read-mostly small data (e.g., a timestamp), write rare, no pointers to free|Seqlock|
|Read-extremely-hot data structure (list/tree), write rare, readers must<br>never block|RCU|

# **10. Common Kernel Concurrency Bugs**

## **10.1 Deadlock via Lock Ordering**

CPU0: lock(A) → tries lock(B) CPU1: lock(B) → tries lock(A)

Both wait forever. **Fix:** always acquire locks in a globally consistent order.

## **10.2 Sleeping While Holding a Spinlock**

spin_lock(&lock); kmalloc(size, GFP_KERNEL); _/*_ **BUG** _: this can sleep */_ spin_unlock(&lock);

Produces a BUG: sleeping function called from invalid context kernel warning/oops.

## **10.3 Missing irqsave Variant**

A driver takes a plain spin_lock() in process context; the same lock is also taken inside its interrupt handler on the same CPU → self-deadlock the moment the interrupt fires while the lock is held.

## **10.4 Using RCU Incorrectly**

Forgetting rcu_read_lock() / unlock() around a dereference — no compile-time enforcement, only caught by tooling. Freeing an RCU-protected object with kfree() directly instead of call_rcu() / synchronize_rcu() — a concurrent reader can then dereference freed memory (use-after-free).

## **10.5 Priority Inversion**

A low-priority task holds a lock a high-priority task needs, and a medium-priority task preempts the low-priority one — the highpriority task is effectively blocked by the medium-priority one. Real-time kernels / PREEMPT_RT address this with priority inheritance mutexes.

# **11. Finding Concurrency Bugs — Tooling**

|**Tool**|**Purpose**|
|---|---|
|**lockdep**|Kernel’s built-in lock-ordering validator; detects potential deadlocks (even<br>ones that haven’t happened yet) by tracking every lock acquisition order<br>seen at runtime|
|**KASAN**|Kernel Address Sanitizer; catches use-after-free and out-of-bounds access<br>— very efective at catching RCU misuse (reading freed memory)|
|**KCSAN**|Kernel Concurrency Sanitizer; specifcally detects data races<br>(unsynchronized concurrent access)|
|**RCU stall warnings**|The kernel itself will print<br>rcu: INFO: rcu_sched detected stalls if a<br>grace period takes too long — usually means a CPU is stuck in an RCU<br>read-side section, or not passing through a quiescent state|
**Practical debugging flow:**

# **13. Summary**

The single idea to hold onto for interviews: **the right primitive is chosen by what context the critical section runs in (can it sleep?) and how read-heavy vs write-heavy the access pattern is.** RCU exists specifically to make the read-heavy, write-rare case nearly free for readers, at the cost of deferred reclamation and writer-side complexity.

# PART A.11 — Chapter 10: ARM & SoC Internals

After completing this chapter, you should understand: - ARM Exception Levels (EL0–EL3) and how they relate to x86 ring/userkernel mode - Device Tree — what it is, why ARM needs it, and how the kernel uses it - Cache coherency protocols (MESI/MOESI) and why they matter on SoCs - Linux power management on ARM: cpuidle, cpufreq, runtime PM - PCIe and interconnect basics relevant to SoC platforms - Why this material specifically matters for Qualcomm/ARM interviews

# **1. Why This Chapter Matters**

Everything in earlier chapters (scheduler, memory management, interrupts, drivers) is largely architecture-agnostic Linux kernel material. Qualcomm, ARM, and other SoC vendors additionally expect you to know **how that generic kernel code maps onto real ARM hardware** — exception levels instead of x86 rings, device tree instead of PCI/ACPI-style enumeration for most on-chip peripherals, and a heavier emphasis on power management because these are battery-powered, thermally-constrained platforms.

# **2. ARM Exception Levels (EL0–EL3) **

ARM’s privilege model (AArch64) has **four exception levels** , more granular than x86’s simple user/kernel mode split.

|**Level**|**Who runs here**|**Analogous to (x86)**|
|---|---|---|
|EL0|User applications|Ring 3 (user mode)|
|EL1|Linux kernel|Ring 0 (kernel mode)|
|EL2|Hypervisor (KVM)|VMX root mode|
|EL3|Secure Monitor / TrustZone firmware|System Management Mode (roughly)|

## **2.1 Why Four Levels Instead of Two?**

**EL0/EL1** — same idea as any OS: unprivileged apps vs. privileged kernel.

**EL2** — exists specifically to support virtualization. A hypervisor (like KVM) runs at EL2 and can host multiple guest kernels, each thinking it’s running at EL1.

**EL3** — exists for **TrustZone** : a hardware-enforced split between a “Normal World” (where Linux runs) and a “Secure World” (where trusted firmware, secure boot verification, DRM keys, or a secure OS runs). EL3 is the only level that can switch between Normal and Secure worlds.

## **2.2 Exception Level Transitions**

Moving to a **higher** EL happens via an explicit exception (syscall, interrupt, secure monitor call). Moving to a **lower** EL happens via an explicit return instruction ( ERET ) .

**Interview point:** a Linux kernel syscall on ARM64 is implemented with the SVC instruction (Supervisor Call), causing a transition EL0 → EL1 — conceptually the same role as syscall / int 0x80 on x86, just a different instruction and a formalized privilege-level model.

## **2.3 PSCI (Power State Coordination Interface)**

Since normal Linux code at EL1 can’t directly power off/reset a CPU core (that’s a secure/firmware-level operation), ARM systems standardize this through **PSCI** — a firmware interface invoked via SMC / HVC calls, used for CPU on/off, system reset, and CPU idle state entry. Linux’s cpuidle and SMP boot code call into PSCI rather than touching power-controller hardware registers directly on most modern SoCs.

# **3. Device Tree **

## **3.1 The Problem It Solves**

On x86/PC platforms, most hardware is discoverable — PCI devices announce themselves via PCI configuration space, ACPI tables describe the rest. Most ARM SoC peripherals (UART, I2C, GPIO, clock controllers, interrupt controllers, memory-mapped custom IP blocks) are **not self-describing** — there’s no bus protocol to ask “what are you and where are your registers?”

**Device Tree** is a data structure (and file format) that describes the hardware layout so the kernel doesn’t need hardcoded, boardspecific C code for every SoC variant.

Without Device Tree: Kernel source contains hardcoded board files, one per board — doesn't scale across hundreds of SoC variants.

With Device Tree: Same kernel image + different.dtb file → describes UART address, IRQ number, clock, GPIO for THIS board.

## **3.2 Device Tree Source (.dts) Example**

uart0: serial@ff000000 { compatible = "arm,pl011"; reg = <0xff000000 0x1000>; interrupts = <0 100 4>; clocks = <&uartclk>; status = "okay"; };

compatible — string(s) used to match this node to a kernel driver (the driver registers a matching compatible string via of_match_table ).

reg — base address and size of the device’s MMIO register region.

interrupts — which IRQ this device is wired to (interrupt controller-specific encoding). clocks — reference to the clock(s) this device needs enabled to function.

## **3.4.dts vs.dtb vs.dtsi**

**File Meaning** .dts Device Tree Source — human-readable, per-board.dtsi Device Tree Source _Include_ — shared SoC-level definitions reused across multiple boards using the same chip.dtb Device Tree Blob — compiled binary form the bootloader hands to the kernel

**Interview point:** a single SoC (e.g., a Qualcomm chip) typically has one.dtsi describing the chip itself, and multiple.dts files (one per board/reference design) that #include the.dtsi and add board-specific bits (which GPIOs are wired to which peripherals on _this particular board_ ).

## **3.5 Driver Matching to Device Tree**

static const **struct** of_device_id my_driver_of_match[] = { { .compatible = "vendor,my-device", }, { } }; MODULE_DEVICE_TABLE(of, my_driver_of_match); static **struct** platform_driver my_driver = { .probe = my_probe, .remove = my_remove, .driver = { .name = "my-device", .of_match_table = my_driver_of_match, }, };

When the kernel parses the device tree and finds a node whose compatible string matches, it calls the driver’s probe() with a platform_device carrying the resolved address/IRQ/clock info.

# **4. Cache Coherency **

## **4.1 The Problem**

On a multi-core SoC, each core typically has its own L1 (and often L2) cache. If Core A caches a value and Core B modifies the same memory location, Core A must not keep using its stale cached copy.

Core A: L1 cache has X = 5 Core B: writes X = 10 to main memory Core A: still thinks X = 5   ← INCOHERENT, must be fixed

Hardware cache coherency protocols solve this automatically, so software (mostly) doesn’t need to manually flush caches for normal shared-memory access between cores.

## **4.2 MESI Protocol**

Each cache line is tagged with one of four states:

|**State**<br>**Meaning**|
|---|
|**M**odifed<br>This cache has the only copy, and it’s been written (dirty) — memory is<br>stale|
|**E**xclusive<br>This cache has the only copy, and it matches memory (clean)|
|**S**hared<br>Multiple caches may have this line, all match memory|
|**I**nvalid<br>This cache line is not valid — must be fetched before use|

## **4.3 MOESI (adds “Owned”)**

Many real SoCs (including many ARM implementations) use **MOESI** , adding an **O** wned state:

|**State**|**Meaning**|
|---|---|
|**O**wned|This cache holds the only_dirty_copy but is sharing it directly with other<br>caches (which are in S state), avoiding a costly write-back to main memory<br>before sharing|

This lets a dirty cache line be shared cache-to-cache without first flushing to slow main memory — a meaningful performance win on SoCs with many cores.

## **4.4 Why This Matters for Kernel Work**

Explains **why** atomic operations and memory barriers are needed even though caches are “coherent” — coherency guarantees _eventual_ consistency and a defined protocol for cache-line state, but not _ordering_ of multiple different memory locations as observed by other cores. That ordering is what memory barriers ( smp_mb() , smp_wmb() , smp_rmb() ) control.

Explains **cache-line bouncing** : if multiple cores frequently write to variables sharing a cache line, the line ping-pongs between M/S/I states across cores — a real performance bug pattern (often called “false sharing”). This is exactly why per-CPU variables (Chapter 9) matter — they avoid this bouncing entirely.

- On **non-coherent** interconnects (some DMA-capable peripherals, or specific SoC memory regions), software must explicitly manage cache maintenance — dma_map_single() / dma_sync_single_for_cpu() and friends perform explicit cache invalidate/clean

operations precisely because the hardware doesn’t guarantee coherency between that device and the CPU caches.

# **5. Linux Power Management on ARM **

SoCs are battery/thermally constrained, so ARM-focused interviews (especially Qualcomm) lean heavily on this compared to server-class Intel/AMD interviews.

## **5.1 cpufreq — Dynamic Frequency/Voltage Scaling**

Controls **how fast** a CPU core runs.

Governor decides target frequency │ ▼ cpufreq driver │ ▼ Actual voltage/frequency change (via regulator + clock framework, or firmware call)

Common governors:

|**Governor**|**Behavior**|
|---|---|
|performance|Always run at max frequency|
|powersave|Always run at min frequency|
|ondemand|Scale up quickly under load, scale down when idle|
|schedutil|Frequency decisions driven directly by the CFS scheduler’s utilization<br>tracking — the modern default on most systems|

**Interview point:** schedutil is significant because it removes the old separate “sampling” governor logic and ties frequency scaling directly into the scheduler’s own view of how busy a CPU actually is, reacting faster and more accurately than periodic polling-based governors.

## **5.2 cpuidle — CPU Idle State Management**

Controls **what a CPU does when it has nothing to run** , trading wake-up latency for power savings.

CPU idle │ ▼ cpuidle governor picks a C-state (idle depth) │ ▼ C1: light sleep, fast wakeup, small power savings C2: deeper sleep, more savings, slower wakeup C3+: core power collapse, cluster power collapse — largest savings, slowest wakeup

Deeper idle states may power down cache, or the whole CPU cluster, requiring state save/restore on wake.

The governor (e.g., the menu governor) predicts how long the CPU will likely stay idle and picks the deepest state that still meets latency requirements (e.g., not violating a device’s requested QoS wakeup latency).

Entering deep idle states on ARM commonly goes through **PSCI CPU_SUSPEND** calls (see §2.3) — the actual power sequencing is handled by firmware below EL1.

## **5.3 Runtime PM (Power Management)**

Where cpufreq/cpuidle manage the _CPU_, **Runtime PM** manages individual **devices/peripherals** — powering down a peripheral (UART, camera, GPU, modem block) when it’s not in use, independent of whether the CPU itself is busy.

pm_runtime_get_sync(dev); _/* power on device, block until ready */ /* use device */_

pm_runtime_put(dev); _/* mark idle; framework may power it off after a delay */_

The runtime PM framework tracks usage counts per device and automatically calls the driver’s runtime_suspend / runtime_resume callbacks when a device becomes idle/needed, without every driver reinventing this bookkeeping.

## **5.4 Suspend/Resume (System Sleep)**

Distinct from per-device runtime PM: whole-system suspend (e.g., “suspend to RAM”).

# **6. Interconnect & PCIe on SoCs

Most SoC-internal peripherals (UART, I2C, GPIO, clock/power controllers) are **not** on PCIe — they’re on a memory-mapped internal bus (AMBA/AXI/AHB on ARM SoCs) and described via device tree, as covered above.

PCIe on an SoC is typically used for **external, discoverable** high-speed devices: NVMe SSDs, discrete GPUs, WiFi/cellular modem cards, or chip-to-chip links between an SoC and an external accelerator.

**Interview point:** know to distinguish “how does the kernel find out about this device” for the two cases — device tree (static, board-description-driven) for most on-chip peripherals, vs. PCI enumeration (dynamic, self-describing via configuration space) for PCIe-attached devices — and that a single modern SoC commonly uses **both** simultaneously.

# **8. Summary**

The throughline for SoC interviews: **generic Linux kernel concepts (scheduler, memory, drivers) still apply, but the platform layer beneath them — privilege levels, hardware description, coherency, and power — is ARM/SoC-specific, and interviewers expect you to connect the two.**

# PART A.12 — Chapter 11: Kernel Debugging & Crash Analysis

After completing this chapter, you should understand: - How to read a kernel oops / panic message - The difference between an oops, a panic, and a warning - kdump and the crash tool for postmortem analysis - ftrace and perf for live tracing/profiling - KASAN, KFENCE, lockdep, and how real concurrency/memory bugs are actually caught - A structured approach for “walk me through how you’d debug this” interview scenarios

Earlier chapters cover command lists ( vmstat, pmap, /proc/interrupts, etc.) for symptom-level triage. At the 15–20 year bar, interviewers expect you to go one level deeper: **given an actual kernel oops or a crash dump, can you read it and find the bug?** This chapter covers that.

# **2. Oops vs Panic vs Warning **

|**Event**|**Meaning**|**System survives?**|
|---|---|---|
|**WARN_ON / WARNING**|Kernel detected something unexpected but<br>recoverable; prints a stack trace and continues|Yes|
|**Oops**|Kernel hit an invalid operation (bad pointer<br>deref, etc.) in a context it can partially recover<br>from — the ofending process/thread is killed|Usually — rest of the system keeps running, but<br>state may be suspect|
|**Panic**|Kernel hit something it cannot safely continue<br>from (e.g., oops in interrupt context, oops while<br>holding a critical lock, or an explicit<br>panic()<br>call)|No — system halts/reboots|

**Interview point:** an oops that happens while the kernel is in interrupt context, holding a spinlock, or already handling another oops, is escalated to a panic — there’s no safe way to “kill the current task” and continue when the current context isn’t a killable task in the first place.

# **3. Reading an Oops Message **

A real (simplified) example:

- [  142.552931] BUG: kernel NULL pointer dereference, address: 0000000000000018

- [  142.552940] #PF: supervisor read access in kernel mode

- [  142.552944] #PF: error_code(0x0000) - not-present page [  142.552948] PGD 0 P4D 0

- [  142.552953] Oops: 0000 [#1] SMP PTI

[  142.552958] CPU: 2 PID: 1842 Comm: my_driver_wq Tainted: G  W  5.15.0 #1

- [  142.552965] RIP: 0010:my_driver_process+0x2c/0xb0 [my_driver]

- [  142.552974] Call Trace:

- [  142.552977]  process_work_item+0x94/0x1a0

- [  142.552981]  worker_thread+0x2f5/0x420

- [  142.552985]  kthread+0x127/0x150

- [  142.552988]  ret_from_fork+0x22/0x30

## **3.1 Line-by-Line**

|**Field**|**What it tells you**|
|---|---|
|BUG: kernel NULL pointer dereference, address: 0x18|The actual fault — dereferencing a pointer that was NULL plus a small<br>ofset (0x18), suggesting a struct member access on a NULL struct pointer|
|#PF: supervisor read access in kernel mode|This was a kernel-mode read page fault, not a user-space one|
|Oops: 0000 [#1]|This is the_frst_oops since boot(<br>#1); a rapidly incrementing counter<br>across multiple oopses suggests something is repeatedly hitting the same<br>bug|
|CPU: 2 PID: 1842 Comm: my_driver_wq|Which CPU and which task/thread was running —<br>my_driver_wq<br>immediately tells you this is a workqueue thread, i.e., deferred work<br>(Chapter 6), not a hard IRQ handler|
|Tainted: G  W|Kernel taint fags —<br>G= proprietary module loaded (not necessarily bad),<br>W= a previous warning already fred; taint fags narrow down whether a<br>third-party/out-of-tree module might be involved|
|RIP: 0010:my_driver_process+0x2c/0xb0 [my_driver]|**The single most important line**— exact function and byte ofset where<br>the fault happened, and which module it’s in|
|Call Trace:|The stack, innermost frame frst — read top to bottom to reconstruct how<br>execution got here|

## **3.2 Reconstructing the Bug From the Trace Above**

This tells a clear story: a workqueue worker thread (Chapter 6 — deferred interrupt work) called into my_driver_process() , which dereferenced a NULL pointer at offset 0x18 into some struct. **Next debugging step:** open my_driver.c at the +0x2c offset (via addr2line or by inspecting the disassembly with objdump -dS ) to find which struct member access that corresponds to, then trace backward to find what could leave that pointer NULL — a classic pattern is a race where the pointer is cleared by another path (e.g., device removal / remove() ) between when the work was scheduled and when it actually ran.

## **3.3 Turning an Address Into a Line of Source**

addr2line -e vmlinux -i my_driver_process+0x2c _# or, for a module:_ addr2line -e my_driver.ko 0x2c

Requires a kernel/module build with debug symbols ( CONFIG_DEBUG_INFO=y ) .

# **4. kdump and the** **crash Tool **

An oops message tells you a lot, but sometimes the system panics before you can even read the console (headless server, log not flushed, etc.). **kdump** solves this by capturing a full memory dump at the moment of panic, which you analyze afterward.

## **4.1 How kdump Works**

Normal kernel panics │ ▼ Reserved crash kernel (kexec) boots immediately │ ▼ Crash kernel dumps memory of the CRASHED kernel to disk/network │  (as /var/crash/.../vmcore) ▼ System reboots normally │ ▼ Engineer analyzes vmcore later, offline, with the `crash` tool

A small amount of memory is reserved at boot ( crashkernel= boot parameter) for the secondary “crash kernel.” On panic, kexec jumps directly into this reserved kernel **without going through firmware/BIOS reset** — fast, and critically,

it can read the crashed kernel’s memory image before anything is overwritten.

The dump ( vmcore ) plus the matching vmlinux (kernel image with debug symbols) is enough to fully reconstruct kernel state at the moment of the crash.

## **4.2 Using the** **crash Tool**

crash /usr/lib/debug/boot/vmlinux-5.15.0 /var/crash/127.0.0.1-2026-08-16-10:22:01/vmcore

#### Common commands inside crash :

|**Command**|**Purpose**|
|---|---|
|bt|Backtrace of the crashing task (same info as the oops call trace, but from<br>the actual dump)|
|bt -a|Backtrace of**all**CPUs — critical for concurrency bugs, since you can see<br>what every core was doing at the moment of panic|
|ps|Full process list as it existed at crash time|
|log|The kernel ring bufer (<br>dmesg ) as captured in the dump|
|struct task_struct <addr>|Dump the full contents of a specifc structure — e.g., inspect the crashing<br>task’s<br>task_struct felds directly|
|kmem -s|SLAB allocator state — useful for memory-corruption postmortems|
|mod|List loaded modules, useful for correlating with<br>Tainted: fags|

**Interview point:** bt -a is the key differentiator between a single-CPU bug (a straightforward NULL deref) and a genuine race condition — if another CPU’s backtrace shows it was in the middle of freeing or modifying the same structure at the same moment, that’s your race.

# **5. ftrace **

The kernel’s built-in, low-overhead tracing framework — useful for **live** systems where you need to see function call flow or timing, not a postmortem dump.

cd /sys/kernel/debug/tracing echo function > current_tracer echo my_driver_process > set_ftrace_filter echo 1 > tracing_on cat trace

Common tracers:

|**Tracer**|**Purpose**|
|---|---|
|function|Trace every call to a given function|
|function_graph|Trace calls**and**their nesting/duration — shows a call graph with timing|
|irqsoff|Records the longest interval interrupts were disabled — great for tracking<br>down latency spikes|
|preemptoff|Same idea, for preemption-disabled intervals|
|wakeup|Tracks scheduling wakeup latency|

#### **Practical example — tracking down a latency spike:**

echo irqsoff > current_tracer echo 1 > tracing_on _# reproduce the issue_ cat trace _# shows the exact code path that held IRQs disabled longest, and for how long_

# **6. perf **

Where ftrace is about _function-level tracing_, perf is about _statistical profiling and hardware performance counters_ — “where is the CPU time actually going?”

perf record -g -a sleep 10 _# sample the whole system for 10 seconds, with call graphs_ perf report _# view where time was spent, as a call-graph-annotated report_

#### Other common uses:

perf top _# live, continuously updating hotspot view_ perf stat./some_workload _# cache misses, branch mispredicts, IPC, context switches_ perf trace _# syscall-level tracing, like strace but lower overhead_

**Interview point:** perf stat exposing cache-miss and IPC (instructions-per-cycle) counters connects directly back to the cachecoherency material (Chapter 10) — a workload with unexpectedly high cache-miss rates and low IPC across multiple cores is a classic false-sharing symptom.

# **7. KASAN, KFENCE, and KCSAN **

These are **compile-time-instrumented sanitizers** for kernel builds — they don’t find bugs in production kernels, but are essential in debug/test builds and CI.

|**Tool**|**Detects**|**How**|
|---|---|---|
|||Instruments every memory access with “shadow|

|**KASAN**(Kernel Address Sanitizer)|Use-after-free, out-of-bounds reads/writes|memory” checks; a poisoned shadow byte means<br>the real access is invalid|
|---|---|---|
|**KFENCE**|Same class of bugs as KASAN|Much lower overhead — samples a small<br>fraction of allocations and places guard pages<br>around them, safe enough to run in production<br>with negligible cost|
|**KCSAN**(Kernel Concurrency Sanitizer)|Data races — unsynchronized concurrent access<br>to the same memory|Randomized, sampling-based instrumentation of<br>memory accesses to detect racing reads/writes<br>without a happens-before relationship|
|**lockdep**|Potential deadlocks from lock ordering|Tracks every lock acquisition order ever<br>observed at runtime; fags any ordering that<br>could theoretically deadlock, even if it never<br>actually has yet|

## **7.1 Example KASAN Report (Use-After-Free)**

BUG: KASAN: use-after-free in my_driver_process+0x5c/0xb0 Read of size 4 at addr ffff888012345678 by task my_driver_wq/1842

CPU: 2 PID: 1842 Comm: my_driver_wq Call Trace: my_driver_process+0x5c/0xb0... Allocated by task 1840: my_driver_alloc+0x30/0x50... Freed by task 1841: my_driver_remove+0x20/0x40...

This is exactly the RCU-misuse pattern from Chapter 9 — KASAN doesn’t just say “bad access,” it shows **which task allocated it and which task freed it** , immediately pointing at a race between remove() freeing a structure and a workqueue item still using it — precisely the bug call_rcu() / proper reference counting would have prevented.

# **8. RCU Stall Warnings **

rcu: INFO: rcu_sched detected stalls on CPUs/tasks: rcu:     2-...!: (1 GPs behind) idle=1c2/1/0x4000000000000000 rcu:     (detected by 0, t=6502 jiffies, g=4517, q=193)

Means a grace period (Chapter 9) has been unable to complete for an unusually long time — usually because some CPU is stuck (e.g., spinning with interrupts disabled, or stuck in an RCU read-side critical section that never exits). **First step:** bt -a (if you have a dump) or check dmesg around that CPU’s activity — the stalled CPU number is given directly in the message.

# **9. Structured Debugging Approach (Interview Framework) **

When asked “how would you debug X,” a strong senior answer follows a **narrowing funnel** , not a list of random tools:

1. Reproduce / characterize

- Is it deterministic or intermittent?

- Single CPU or does it correlate with core count / load?

2. Collect evidence

- dmesg / oops / panic message

- If system fully crashed: kdump vmcore

- If live and reproducible: ftrace / perf

3. Localize

- RIP / call trace → exact function + offset

- addr2line → exact source line

- bt -a (if crash dump) → what were OTHER CPUs doing (race check)

4. Classify the bug type

- NULL/invalid pointer → likely a lifecycle/ordering bug (freed too early, not yet initialized)

- Sleeping in atomic context → misused lock type (Chapter 9)

- Deadlock → lock ordering (lockdep output)

- Data race → KCSAN / missing synchronization

- Latency spike → ftrace irqsoff/preemptoff, or interrupt storm (Chapter 6)

5. Confirm hypothesis

- Re-run with the relevant sanitizer enabled (KASAN/KCSAN) if not already

- Add targeted trace points / WARN_ON if still not root-caused

6. Fix and prevent recurrence

- Correct the synchronization/lifecycle issue

- Consider whether a lockdep annotation, a WARN_ON, or a test case should be added to catch a regression

**Interview point:** interviewers evaluating 15–20 years of experience are often less interested in whether you know a specific command and more interested in whether you can narrate this funnel out loud, under time pressure, on an example you’ve never seen before.

# **11. Summary**

The throughline: **a real oops or crash dump is a story, told backward from the crashing instruction through the call stack to the root cause — and every debugging tool in this chapter exists to help you read that story faster and more completely.**

# PART A.13 — Chapter 12: Driver Skeleton & Real Kernel Code Walkthrough

After completing this chapter, you should understand: - A complete, working platform driver skeleton (probe/remove, not just theory) - How device tree (Chapter 10), interrupts (Chapter 6), and locking (Chapter 9) all come together in one real driver - A misc character device example (the other extremely common driver shape) - Annotated real-shape excerpts of core kernel code: task_struct, CFS pick_next_task, wait_event - How to read kernel source you’ve never seen before under interview pressure

Every previous chapter explained a _concept_ (interrupts, locking, device tree, scheduling) largely through diagrams and short snippets. At the 15–20 year bar, interviewers frequently ask you to **read or write actual driver code** , or to walk through a real kernel function. This chapter ties the previous 11 chapters together into code you could plausibly be asked to write or explain on a whiteboard.

# **2. Full Platform Driver Skeleton ⭐⭐⭐⭐⭐**

This example pulls together device tree matching (Ch. 10), interrupt handling (Ch. 6), and locking (Ch. 9) into one realistic driver.

## **2.1 Device Tree Node (what the platform gives us)**

mydev0: mydevice@ff010000 { compatible = "vendor,mydevice-v1"; reg = <0xff010000 0x1000>; interrupts = <0 45 4>; clocks = <&mydev_clk>; status = "okay"; };

## **2.2 Driver Structure**

#include **<linux/module.h>** #include **<linux/platform_device.h>** #include **<linux/of.h>** #include **<linux/interrupt.h>** #include **<linux/io.h>** #include **<linux/clk.h>** #include **<linux/spinlock.h>** #include **<linux/workqueue.h>** _/* Per-device private state — one instance per probed device */_ **struct** mydev_priv { void __iomem   *regs; _/* mapped MMIO register base   */_ int             irq; **struct** clk     *clk; spinlock_t      lock; _/* protects hw register access from IRQ + process ctx */_ **struct** work_struct work; _/* deferred processing (Chapter 6) */_ **struct** device  *dev; }; _/* Register offsets — driver-specific, matches the hardware datasheet */_

#define MYDEV_STATUS_REG   0x00 #define MYDEV_IRQ_ACK_REG  0x04 #define MYDEV_CTRL_REG     0x08 #define MYDEV_IRQ_PENDING  BIT(0)

_/* ---- Deferred work (bottom half, Chapter 6) ---- */_ static void mydev_work_handler( **struct** work_struct *work) { **struct** mydev_priv *priv = container_of(work, **struct** mydev_priv, work); unsigned long flags; u32 status; spin_lock_irqsave(&priv->lock, flags); status = readl(priv->regs + MYDEV_STATUS_REG); spin_unlock_irqrestore(&priv->lock, flags); _/* Longer processing goes here — this runs in process context, so it is safe to sleep, allocate with GFP_KERNEL, etc. */_ dev_dbg(priv->dev, "deferred processing, status=0x%x\n", status); } _/* ---- Hard IRQ handler (top half, Chapter 6) ---- */_ static irqreturn_t mydev_irq_handler(int irq, void *data) { **struct** mydev_priv *priv = data; u32 status;

spin_lock(&priv->lock); _/* no _irqsave needed: we're already in IRQ context */_ status = readl(priv->regs + MYDEV_STATUS_REG);

**if** (!(status & MYDEV_IRQ_PENDING)) { spin_unlock(&priv->lock); **return** IRQ_NONE; _/* not our interrupt (shared IRQ line, Chapter 6 §24) */_ }

_/* Acknowledge in hardware so it doesn't fire again immediately */_ writel(status, priv->regs + MYDEV_IRQ_ACK_REG); spin_unlock(&priv->lock);

_/* Do minimal work here; defer the rest */_ schedule_work(&priv->work);

**return** IRQ_HANDLED; } _/* ---- probe(): called when device tree node matches this driver ---- */_ static int mydev_probe( **struct** platform_device *pdev) {

**struct** mydev_priv *priv; **struct** resource *res; int ret;

priv = devm_kzalloc(&pdev->dev, **sizeof** (*priv), GFP_KERNEL); **if** (!priv) **return** -ENOMEM;

priv->dev = &pdev->dev; spin_lock_init(&priv->lock); INIT_WORK(&priv->work, mydev_work_handler);

_/* Map the MMIO region described by "reg" in device tree */_ res = platform_get_resource(pdev, IORESOURCE_MEM, 0); priv->regs = devm_ioremap_resource(&pdev->dev, res); **if** (IS_ERR(priv->regs)) **return** PTR_ERR(priv->regs);

_/* Get the IRQ number described by "interrupts" in device tree */_ priv->irq = platform_get_irq(pdev, 0); **if** (priv->irq < 0) **return** priv->irq;

_/* Get and enable the clock described by "clocks" in device tree */_ priv->clk = devm_clk_get(&pdev->dev, NULL); **if** (IS_ERR(priv->clk)) **return** PTR_ERR(priv->clk);

ret = clk_prepare_enable(priv->clk); **if** (ret) **return** ret;

ret = devm_request_irq(&pdev->dev, priv->irq, mydev_irq_handler, IRQF_SHARED, "mydev", priv); **if** (ret) { clk_disable_unprepare(priv->clk); **return** ret; } platform_set_drvdata(pdev, priv); dev_info(&pdev->dev, "mydevice probed, irq=%d\n", priv->irq); **return** 0; } _/* ---- remove(): called on unbind / module unload ---- */_ static int mydev_remove( **struct** platform_device *pdev) { **struct** mydev_priv *priv = platform_get_drvdata(pdev); _/* devm_* resources (regs, irq, kzalloc) are freed automatically, but anything NOT devm-managed must be cleaned up explicitly: */_ cancel_work_sync(&priv->work); _/* wait for any in-flight deferred work to finish BEFORE the hardware/memory it touches goes away */_ clk_disable_unprepare(priv->clk); **return** 0; } static const **struct** of_device_id mydev_of_match[] = { { .compatible = "vendor,mydevice-v1", }, { } }; MODULE_DEVICE_TABLE(of, mydev_of_match);

static **struct** platform_driver mydev_driver = { .probe  = mydev_probe, .remove = mydev_remove, .driver = { .name           = "mydevice", .of_match_table = mydev_of_match, }, }; module_platform_driver(mydev_driver); MODULE_LICENSE("GPL"); MODULE_DESCRIPTION("Example platform driver");

## **2.3 Why** **cancel_work_sync() in** **remove() Matters (Common Interview Trap)**

If remove() simply freed priv (or let devm_kzalloc free it) without first calling cancel_work_sync() , a **race** is possible:

CPU0: remove() runs, frees priv's memory (via devm cleanup) CPU1: workqueue worker finally gets scheduled, runs mydev_work_handler(), dereferences priv → USE AFTER FREE

This is exactly the class of bug KASAN (Chapter 11) is built to catch, and exactly the lifecycle problem RCU/reference-counting (Chapter 9) exists to prevent in more complex cases. cancel_work_sync() blocks until any already-scheduled work item has _finished_ running, guaranteeing it’s safe to then free the memory it used.

## **2.4 Why** **devm_* Functions Matter**

Every devm _ * call ( devm_kzalloc, devm_ioremap_resource, devm_request_irq, devm_clk_get ) ties the resource’s lifetime to the struct device. If probe() fails partway through, or remove() is called, the kernel automatically releases everything allocated with devm _ * — this is why the example above doesn’t need manual kfree() / iounmap() / free_irq() calls for those resources, only for the nondevm work ( cancel_work_sync, clk_disable_unprepare ).

# **3. Misc Character Device Skeleton ⭐⭐⭐⭐**

The other extremely common driver shape — for a simple device exposing a /dev/mydev node with open / read / write / ioctl, without needing a full device-tree-matched platform device.

#include **<linux/miscdevice.h>** #include **<linux/fs.h>** #include **<linux/uaccess.h>** #define MYDEV_BUF_SIZE 256 static char kbuf[MYDEV_BUF_SIZE]; static ssize_t mydev_read( **struct** file *filp, char __user *ubuf, size_t len, loff_t *off) { **if** (*off >= MYDEV_BUF_SIZE) **return** 0; len = min(len, (size_t)(MYDEV_BUF_SIZE - *off)); **if** (copy_to_user(ubuf, kbuf + *off, len)) _/* user pointer — never deref directly */_ **return** -EFAULT; *off += len; **return** len; } static ssize_t mydev_write( **struct** file *filp, const char __user *ubuf, size_t len, loff_t *off) { len = min(len, (size_t)MYDEV_BUF_SIZE); **if** (copy_from_user(kbuf, ubuf, len)) **return** -EFAULT; **return** len; } static const **struct** file_operations mydev_fops = { .owner = THIS_MODULE, .read  = mydev_read, .write = mydev_write, }; static **struct** miscdevice mydev_misc = { .minor = MISC_DYNAMIC_MINOR, .name  = "mydev", .fops  = &mydev_fops, }; static int __init mydev_init(void) { **return** misc_register(&mydev_misc); _/* creates /dev/mydev */_ } static void __exit mydev_exit(void) { misc_deregister(&mydev_misc); } module_init(mydev_init); module_exit(mydev_exit); MODULE_LICENSE("GPL");

**Interview point:** copy_to_user() / copy_from_user() are not optional politeness — a user-space pointer must never be dereferenced directly from kernel code. These functions validate the address range and safely fault-handle the copy, returning -EFAULT if the user pointer is invalid, instead of letting a malicious or buggy user-space program crash or corrupt the kernel.

# **4. Reading Real Kernel Code —** **task_struct (Selected Fields) ⭐⭐⭐⭐⭐**

You won’t be asked to recite the full task_struct (it has 100+ fields), but you should recognize the important groupings when shown a subset:

|**struct**task_struct{<br> volatile long <br> void <br> **struct**list_head|state; _/* TASK_RUNNING, TASK_INTERRUPTIBLE, ... */_<br> *stack;<br>tasks; _/* linked into the global process list */_|
|---|---|
|**struct**mm_struct<br> **struct**mm_struct|*mm; _/* address space (Chapter 5) — NULL for kernel threads */_<br> *active_mm;|
|pid_t<br>pid_t|pid;<br>tgid; _/* thread group ID — same for all threads in a process */_|
|**struct**task_struct<br> **struct**list_head|*parent;<br>children;|
|**struct**sched_entity<br> int <br> unsigned int <br>cpumask_t|se; _/* CFS scheduling data (Chapter 2) — includes vruntime */_<br>prio,static_prio,normal_prio;<br>policy; _/* SCHED_NORMAL, SCHED_FIFO, SCHED_RR, ... */_<br>cpus_allowed; _/* CPU affinity (Chapter 2) */_|
|**struct**files_struct<br> **struct**fs_struct|*files; _/* open file descriptor table */_<br> *fs; _/* filesystem context: cwd, root */_|
|**struct**signal_struct<br>sigset_t|*signal;<br>blocked,pending;|
|**struct**cred|*cred; _/* uid, gid, capabilities */_|
|_/* ... 100+ more field_<br>};|_s: cgroups, namespaces, RCU state, tracing, etc. */_|

# **5. Reading Real Kernel Code — CFS** **pick_next_task (Simplified/Annotated) ⭐⭐⭐⭐⭐**

The real function is more complex (handles multiple scheduling classes, load balancing hooks, etc.), but the **conceptual core** every interviewer wants you to recognize:

_/* Simplified/annotated shape of the real CFS pick logic */_ static **struct** task_struct *pick_next_task_fair( **struct** rq *rq) { **struct** cfs_rq *cfs_rq = &rq->cfs; **struct** sched_entity *se; **if** (!cfs_rq->nr_running) **return** NULL; _/* nothing runnable on this CPU's CFS runqueue */_ **do** { _/* Walks down the red-black tree to the leftmost node — the leftmost node is, by construction, the entity with the SMALLEST vruntime (Chapter 2) */_ se = pick_first_entity(cfs_rq); cfs_rq = group_cfs_rq(se); _/* handle nested task groups (cgroups CPU controller) */_ } **while** (cfs_rq); **return** task_of(se); _/* container_of: sched_entity -> task_struct */_ }

**What to say out loud in an interview reading this:** 1. “ cfs_rq is a per-CPU runqueue; each CPU picks independently.” 2. “The tasks are stored as sched_entity structs in a red-black tree keyed by vruntime — this is the same red-black tree from Chapter 2.” 3. “ pick_first_entity walks to the leftmost node — leftmost in a red-black tree keyed by vruntime means smallest vruntime, i.e., the task that has received the least CPU time so far relative to its weight — exactly the fairness invariant CFS is built around.” 4. “The do/while loop handling group_cfs_rq is because of the cgroup CPU controller — task groups can be nested, so picking a task might mean descending through a hierarchy of runqueues, not just one flat list.” 5. “ task_of(se) is a container_of() -style cast — sched_entity is embedded inside task_struct, so given a pointer to the embedded struct, the kernel can recover the pointer to the containing struct.” This container_of pattern is used constantly throughout the kernel — you already saw it in the driver skeleton above ( container_of(work, struct mydev_priv, work) ) .

# **6. Reading Real Kernel Code —** **wait_event /** **wake_up (Annotated) ⭐⭐⭐⭐**

Ties together Chapter 6 (interrupt + wait queue pattern) with actual code shape:

_/* Process context: block until condition becomes true */_ wait_event_interruptible(priv->waitq, priv->data_ready); _/* ... later, from the IRQ handler or workqueue (Chapter 6 §43) ... */_ priv->data_ready = **true** ; wake_up_interruptible(&priv->waitq);

**What** **wait_event_interruptible actually expands to (conceptually):**

**while** (!(priv->data_ready)) { prepare_to_wait(&priv->waitq, &wait, TASK_INTERRUPTIBLE); **if** (priv->data_ready)

**break** ; **if** (signal_pending(current)) **return** -ERESTARTSYS; schedule(); _/* actually yields the CPU — this is the sleep */_ } finish_wait(&priv->waitq, &wait);

**Interview point:** the condition ( priv->data_ready ) is checked in a **loop** , not a single if — this matters because wake_up() can have spurious wakeups, and multiple waiters can race to consume the same condition. Re-checking the condition after waking up is what makes this pattern correct; a driver that used a plain if here has a real, subtle bug.

# **7. A General Strategy for Reading Unfamiliar Kernel Code ⭐⭐⭐⭐⭐**

When handed a real kernel function you’ve never seen, on a whiteboard or in an interview:

1. Identify the context first

- What calls this? (probe? IRQ handler? syscall path? scheduler tick?)

- That tells you what's legal here (can it sleep? Chapter 9)

2. Identify the "shape" before the details

- Is this a linked-list/tree walk? A state machine? A lock/unlock pair?

- Most kernel functions are one of a small number of recognizable shapes

3. Find the data structure being manipulated

- task_struct? sched_entity? request? sk_buff? — you've now seen most of the common ones across Chapters 2, 5, 7, 8

4. Find the synchronization

- What lock (if any) is expected to already be held, or is taken here?

- Comments and lockdep annotations (lockdep_assert_held) are strong hints

5. Trace error paths

- What happens on the failure branches? Often where subtle bugs hide

- In probe(), check every error path unwinds cleanly (goto err_* chains are extremely common — a leaked resource on an error path is a classic bug)

6. Narrate a summary in one or two sentences

- Interviewers are grading whether you can compress the function into

- "this walks X to find Y, under lock Z, and does W" — not whether you can recite every line

# **9. Summary**

└── INIT_WORK/schedule_work     → bottom half (Chapter 6)

misc character device → simplest /dev/ node shape, open/read/write/ioctl

container_of()  → the pattern used everywhere to go from an embedded struct member back to its containing structure

The throughline for this chapter — and really for the whole set of notes: **a senior/staff-level interview isn’t testing whether you memorized definitions, it’s testing whether you can look at unfamiliar real code and immediately recognize the patterns (locking, deferred work, lifecycle management, scheduling structures) from the concepts in Chapters 1–11.**

# PART B.14 — Linux System Programming: Complete Study Guide

_Based on “Linux System Programming” (2nd Edition) by Robert Love, plus original supplementary code examples_

## **Contents**

**Part 1: Chapter-wise Study Notes** — Chapters 1–11 + Appendices **Part 2: Code Examples (Companion to the Study Notes)** — original programs illustrating book APIs (file I/O, fork/exec,

pipes, signals, pthreads, mmap, epoll, time)

- **Part 3: Interview-Prep Code Examples (Beyond the Book)** — condition variables, semaphores, shared memory, rwlocks, deadlock demo, zombies/orphans, file locking, custom memcpy, daemonizing

- **Part 4: Deep-Dive Patterns** — thread pools, barriers, recursive mutexes, TLS, named semaphores, sigwait/real-time signals, FIFOs, POSIX message queues, Unix domain sockets, multi-process fan-out/pipelines/process trees

## **Part 1: Chapter-wise Study Notes**

## **Chapter 1: Introduction and Essential Concepts**

**What system programming is** : writing low-level code that talks directly to the kernel and the core system libraries — as opposed to application-level programming that sits on top of frameworks and GUIs. It sits at the intersection of three things:

- **System calls** — the interface the kernel exposes to user space ( open() , read() , fork() , etc.). These are the _only_ way into the kernel; everything else in system programming is built on top of them.

- **The C library (glibc on Linux)** — wraps system calls, adds portable/higher-level functionality ( malloc() , printf() , threading primitives, stdio ), and implements the C standard library.

- **The C compiler (gcc)** — turns source into the actual system binary that talks to the kernel; understanding compiler behavior (optimization, inlining) matters for system code.

**APIs vs. ABIs** : an API is a source-level contract (function signatures, behavior); an ABI is the binary-level contract (calling convention, struct layout, system call numbers). Portable code targets a stable API; a given compiled binary depends on a specific ABI.

**Standards** : the chapter surveys POSIX and the Single UNIX Specification (SUS) as the standards that keep Unix-like systems interoperable, and the C language standards (K&R, C89/ANSI C, C99, C11) that govern the language itself. Linux mostly follows POSIX but has many of its own extensions (Linux-specific system calls not in POSIX), and this book focuses on Linux directly rather than being generically portable.

**Core Linux/Unix concepts introduced** (each expanded in later chapters): - **Files and the filesystem** — everything is accessed through a unified hierarchical namespace; a file descriptor is a small integer handle a process uses to refer to an open file. - **Processes** — a running instance of a program, identified by a PID, with its own address space, one or more threads, open file descriptors, and a place in the process hierarchy (parent/child). - **Users and groups** — every process runs with a real/effective/saved UID and GID that determine what it’s allowed to do. - **Permissions** — the read/write/execute bits (plus setuid/setgid/sticky) that gate access to files. - **Signals** — a primitive form of software interrupt/notification delivered to a process (e.g., SIGINT, SIGSEGV ). - **Interprocess communication (IPC)** — mechanisms (pipes, sockets, shared memory, etc.) that let independent processes exchange data. - **Headers and error handling** — system calls generally return -1 on failure and set the global errno to indicate the specific error; checking return values is not optional in system code.

This chapter is essentially the roadmap for the rest of the book — it defines vocabulary that every subsequent chapter assumes.

## **Chapter 2: File I/O**

The core of Unix philosophy: “everything is a file.” This chapter covers the fundamental system calls for unbuffered (direct) file I/O. **Opening files —** **open()**

int open(const char *name, int flags, ... _/* mode_t mode */_ );

- flags is a bitwise OR of one required access mode ( O_RDONLY, O_WRONLY, O_RDWR ) plus optional flags: O_CREAT (create if missing, requires a mode argument), O_EXCL (fail if file exists — combined with O_CREAT for atomic file creation), O_TRUNC, O_APPEND, O_NONBLOCK, O_SYNC, O_DIRECT, O_CLOEXEC, etc.

- New files are owned by the creating process’s effective UID/GID (with a BSD-style group-inheritance option), and the requested mode is masked by the process’s umask.

- creat(path, mode) is shorthand for open(path, O_WRONLY|O_CREAT|O_TRUNC, mode) .

- Returns a non-negative file descriptor on success, -1 and sets errno on failure ( ENOENT, EACCES, EEXIST, EMFILE, ENOSPC, …).

**Reading —** **read()**

ssize_t read(int fd, void *buf, size_t len);

Returns the number of bytes actually read, which can legitimately be _less_ than requested (a “short read” — not an error); returns 0 at end-of-file; returns -1 on error ( EINTR, EAGAIN for nonblocking fds, EIO ). Correct code loops until the requested amount is read or EOF is hit.

**Writing —** **write()**

ssize_t write(int fd, const void *buf, size_t count);

Same short-write caveat applies. O_APPEND makes writes atomically seek-to-end-then-write, important for multiple writers sharing a file (e.g., log files). Nonblocking writes can return EAGAIN if the underlying buffer is full.

**Synchronized I/O** — normal writes only land in the kernel’s page cache, not on disk, until the kernel decides to flush (“writeback”). To force durability: - fsync(fd) — flush data and all metadata to disk. - fdatasync(fd) — flush data and only the metadata needed to access it (skips things like mtime), cheaper than fsync() . - sync() — flush the entire system’s dirty buffers. - O_SYNC / O_DSYNC / O_RSYNC flags make every write synchronous automatically. - O_DIRECT bypasses the page cache entirely for large, well-aligned I/O (used by databases that manage their own caching).

**Closing —** **close(fd)** . Closing doesn’t guarantee data is on disk (see fsync above); it does release the descriptor. **Seeking —** **lseek()**

off_t lseek(int fd, off_t pos, int whence);

whence is SEEK_SET, SEEK_CUR, or SEEK_END. Seeking past the end of a file and then writing creates a **sparse file** (a “hole” that reads back as zeros but consumes no disk blocks). pread() / pwrite() do positional I/O without touching (or needing) the file offset — useful for thread-safe I/O on a shared descriptor.

**Truncating** — truncate() / ftruncate() set a file to an exact length, extending with a hole if it’s growing.

**Multiplexed I/O —** **select() and** **poll()** : both let a process block until one of several file descriptors becomes ready for I/O, which is the classic building block for single-threaded servers handling many connections. - select() uses fixed-size bitmasks ( fd_set ), has a compiled-in fd limit ( FD_SETSIZE ) , and its timeout parameter is mutated by Linux (elapsed time is subtracted). - poll() uses a dynamically sized array of struct pollfd, has no descriptor-count limit, and gives more precise per-fd event/revent flags. - Neither scales well to very large numbers of descriptors — that motivates epoll() in Chapter 4.

**Kernel internals note** : the chapter closes with a look at the Virtual Filesystem (VFS) layer that gives Linux a uniform interface across different filesystem types, and the page cache, which caches file data in RAM and is the reason normal I/O is fast (and why fsync() is needed for durability guarantees).

## **Chapter 3: Buffered I/O**

Raw read() / write() calls are system calls with real overhead, so making one per byte (or per small chunk) is expensive. The C library’s **standard I/O (** **stdio )** layer adds a _user-space_ buffer on top of file descriptors to batch data into fewer, larger system calls.

- A FILE * (a “stream”) wraps a file descriptor plus a buffer. Streams are opened with fopen() / fdopen() / freopen() using mode strings ( "r" , "w" , "a" , "r+" , "rb" , etc., mirroring open() ’ s flags) and closed with fclose() (or fcloseall() for every open stream).

- **Buffering modes** , tunable with setvbuf() / setbuf() : fully buffered (block-sized buffer, used for regular files), line buffered (flushed on \n, typical for interactive terminals), and unbuffered (every call is an immediate write, typical default for stderr ). **Reading** : fgetc() (one char), fgets() (a line, bounded), fread() (binary/structured data).

- **Writing** : fputc() , fputs() , fwrite() , plus the formatted-output family ( printf / fprintf / sprintf ) .

- **Seeking** : fseek() / ftell() / rewind() operate on the stream’s logical position, distinct from the kernel’s file offset until a flush occurs.

- **Flushing** : fflush() pushes the user-space buffer down to the kernel (via write() ) — it does _not_ guarantee an fsync() to disk. fileno(FILE *) recovers the underlying raw file descriptor when you need to fall back to a system call.

- **Thread safety** : stdio streams are internally locked by default ( flockfile() / funlockfile() ); _ unlocked variants ( getc_unlocked() , etc.) skip the lock for a speed gain when the caller already guarantees exclusivity.

- **Critiques of standard I/O** : the double-buffering (user-space stdio buffer _and_ kernel page cache) is a common criticism — copying data twice — along with the historical int - sized return types of some calls being awkward with modern large files, and stdio not always being the fastest path for high-performance I/O (raw read() / write() with well-tuned buffer sizes can win).

## **Chapter 4: Advanced File I/O**

**Scatter/gather I/O —** **readv() / writev()** : transfer data to/from multiple non-contiguous buffers in a single system call using an array of struct iovec {void *iov_base; size_t iov_len;} . Saves the overhead of many small read() / write() calls and can be more efficient than manually concatenating buffers.

**Event polling —** **epoll()** : Linux’s scalable replacement for select() / poll() when watching very large numbers of file descriptors. - epoll_create() makes an epoll instance (a kernel object referenced by its own fd). - epoll_ctl() adds/modifies/removes watched descriptors ( EPOLL_CTL_ADD/MOD/DEL ) and the events of interest ( EPOLLIN, EPOLLOUT, etc.). - epoll_wait() blocks and returns only the descriptors that are actually ready — unlike poll() , which re-scans everything you passed in every call, so epoll() ’ s cost scales with the number of _ready_ fds, not the number _watched_. - **Level-triggered vs. edge-triggered** ( EPOLLET ): level-triggered (default) keeps notifying as long as data is available; edge-triggered notifies only on the transition to ready, demanding that the caller drain the fd completely (usually in a loop until EAGAIN ) — faster but easier to get wrong.

**Memory-mapped I/O —** **mmap() /** **munmap()** : maps a file (or anonymous memory) directly into the process’s address space so file contents can be accessed as if they were an array in memory, with the kernel handling paging transparently. - Key parameters: desired address (usually NULL, let the kernel choose), length, protection ( PROT_READ/WRITE/EXEC ), flags ( MAP_SHARED — writes go back to the file and are visible to other mappers — vs. MAP_PRIVATE — copy-on-write, changes stay local), fd, and offset. - **Advantages** : avoids extra copies between kernel and user buffers, avoids a separate system call per access, and lets multiple processes trivially share memory via a shared mapping. - **Disadvantages** : mappings must be page-aligned, wasteful for small files (rounds up to a page), can complicate error handling (a SIGBUS if the backing file shrinks or I/O fails during an access, rather than a normal error return), and there are limits to the number/size of mappings. - msync() flushes a shared mapping’s changes back to the file (a _manual_ mmap analogue of fsync() ). mprotect() changes a mapping’s protection after the fact.

**I/O advice** : posix_fadvise() tells the kernel about expected access patterns for normal file I/O ( POSIX_FADV_SEQUENTIAL, _ RANDOM, _ WILLNEED, _ DONTNEED ) so it can tune readahead and caching; madvise() is the mmap() analogue. readahead() explicitly pre-populates the page cache for a file range.

**Synchronous vs. asynchronous I/O** : normal calls are synchronous (the caller blocks or at least issues the request and waits for completion status); Linux’s **AIO** ( aio_read() , aio_write() , aio_error() , aio_return() , and friends) lets a program submit I/O requests and be notified later (via polling, signal, or callback) rather than blocking — useful for I/O-heavy workloads, though the interface (and kernel support) has historically had limitations.

**I/O schedulers** : the kernel block layer reorders and merges pending disk I/O requests to reduce seek overhead (“elevator algorithms”). The chapter walks through disk addressing (why sequential access is cheap and random access is expensive on rotating media), and Linux’s available schedulers — e.g., the historically default **CFQ (Completely Fair Queuing)** , which timeslices disk access fairly among processes, versus deadline and noop schedulers better suited to SSDs or specific workloads. Perprocess I/O priority can be tuned via ioprio_set() .

## **Chapter 5: Process Management**

A **process** is a running program: an address space, one or more threads of execution, and kernel-tracked resources (open files, signal handlers, etc.). This is distinct from a _program_ (the on-disk binary) and a _thread_ (one flow of execution inside a process’s address space, covered fully in Chapter 7).

**PIDs** : allocated by the kernel, historically capped at 32,768 (tunable via /proc/sys/kernel/pid_max on 64-bit systems), reused only after wrapping. getpid() / getppid() return the process’s own ID and its parent’s. All processes form a tree rooted at init (PID 1).

**Creating processes** : - **fork()** duplicates the calling process, returning 0 in the child and the child’s PID in the parent ( -1 on failure). Modern Linux uses **copy-on-write** so the child’s address space isn’t physically duplicated until either process writes to a shared page — making fork() cheap despite conceptually copying everything. - **The** **exec family** ( execl() , execle() , execlp() , execv() , execve() , execvp() ) replaces the calling process’s image with a new program — the classic Unix pattern is fork() then exec() to run a new program in a child, which is how shells launch commands.

**Terminating a process** : normal exit is via exit() (flushes stdio buffers, runs atexit() / on_exit() handlers, then calls the low-level_exit() ) or return from main() ; _ exit() / _ Exit() terminates immediately without cleanup. A terminated child becomes a **zombie** — an entry retained by the kernel to hold its exit status — until the parent reaps it.

**Waiting for children** : wait() blocks for any child; waitpid() waits for a specific PID (or process group, with options like WNOHANG for non-blocking checks); the BSD-derived wait3() / wait4() add resource-usage reporting. SIGCHLD is delivered to the parent when a child changes state, letting a parent avoid polling. Unreaped zombies waste kernel resources; long-running daemons must always reap their children.

**Users and groups** : each process carries **real** , **effective** , and **saved** UID/GID. The real ID identifies who actually owns the process; the effective ID is what’s checked for permission decisions (and can temporarily change via setuid programs); the saved ID lets a privileged process drop and later reclaim elevated privileges safely. setuid() / setgid() , seteuid() / setegid() , and setreuid() / setregid() (BSD-style) manipulate these, with setresuid() / setresgid() as the modern, precise Linux way to control all three at once.

**Sessions and process groups** : a **process group** is a set of related processes (e.g., a pipeline) that can be signaled together; a **session** is a set of process groups typically tied to a controlling terminal — the basis for shell job control. setsid() creates a new session (detaching from any controlling terminal), central to daemonizing a process.

**Daemons** : the classic recipe — fork() and let the parent exit; call setsid() in the child to get a new session with no controlling terminal; chdir("/") so the daemon doesn’t pin any mount point busy; close (or redirect to /dev/null ) the standard file descriptors; then run the daemon’s real work loop.

## **Chapter 6: Advanced Process Management**

**Scheduling** : the kernel time-slices the CPU(s) among runnable processes. The chapter explains the trade-off between I/O-bound processes (want low latency, frequent short bursts) and processor-bound processes (want maximum throughput), and describes preemptive multitasking, where the kernel can interrupt a running process. Linux’s mainline scheduler for normal tasks is the **Completely Fair Scheduler (CFS)** , which approximates giving every runnable task an equal share of CPU time weighted by priority ( nice value), rather than using fixed timeslices.

- **Yielding** : sched_yield() voluntarily gives up the CPU — rarely the right tool; usually indicates a design that should use proper synchronization instead.

- **Priorities** : nice() and getpriority() / setpriority() adjust a process’s (or process group’s/user’s) scheduling weight (nice values conventionally range −20 to 19, lower is higher priority); ordinary users can only lower their own priority (raise the nice value).

- **I/O priority** : ioprio_set() / ioprio_get() similarly tune priority for disk I/O scheduling, independent of CPU priority.

- **Processor affinity** : sched_setaffinity() / sched_getaffinity() pin a process to a specific subset of CPUs — useful for cache locality or isolating latency-sensitive work.

**Real-time scheduling** : distinguishes **hard** real-time (a missed deadline is a system failure) from **soft** real-time (missed deadlines degrade quality but aren’t fatal) and defines latency, jitter, and deadlines as the vocabulary for reasoning about timing guarantees. Linux is not a hard real-time OS out of the box, but offers real-time scheduling policies — SCHED_FIFO (runs to completion or blocking, among equal-priority tasks) and SCHED_RR (round-robin with time slices) — set via

sched_setscheduler() / sched_getscheduler() and sched_setparam() / sched_getparam() , with sched_rr_get_interval() reporting the RR timeslice. Real-time processes require care (they can starve the rest of the system) and typically require elevated privileges.

**Resource limits** : getrlimit() / setrlimit() (and the convenience getrusage() for usage stats) read and cap per-process resource consumption — open file count ( RLIMIT_NOFILE ), max memory ( RLIMIT_AS ) , CPU time ( RLIMIT_CPU ) , core dump size, stack size, and more — each with a _soft_ limit (currently enforced, changeable up to the hard limit) and a _hard_ limit (a ceiling only a privileged process can raise). These limits are inherited across fork() / exec() , which is how shells implement ulimit.

## **Chapter 7: Threading**

**Threads vs. processes** : a thread is an independent flow of execution that shares its address space (and most other resources) with sibling threads in the same process, whereas processes each get their own address space. The chapter frames **multithreading** as one of several concurrency strategies alongside multiple processes, event-driven single-threaded designs, and hybrids.

- **Costs of multithreading** : synchronization complexity, harder debugging, and the ever-present risk of races and deadlocks — threading is a tool, not a default.

- **Threading models** : kernel-level (1:1 — each user thread maps to a kernel schedulable entity, Linux’s approach via clone() ), user-level (N:1 — a userspace library multiplexes many threads onto one kernel thread), and hybrid (M:N). Coroutines/fibers are cooperative, non-preemptive alternatives to full threads for structuring concurrent-looking code without real parallelism. **Threading patterns** : thread-per-connection (simple, but doesn’t scale to huge connection counts) vs. event-driven (a small thread/process pool multiplexing many connections via select() / poll() / epoll() , more scalable but more complex to write).

**Concurrency, parallelism, and races** : concurrency is about correctly managing multiple logically-simultaneous activities; parallelism is about actually running them at once on multiple cores. A **race condition** happens when correctness depends on the unguaranteed timing/interleaving of operations on shared state.

**Synchronization** : a **mutex** (mutual exclusion lock) ensures only one thread executes a critical section at a time. A **deadlock** occurs when threads each hold a resource the other needs (classically, inconsistent lock-ordering across two or more locks) — the chapter stresses always acquiring locks in a consistent global order to avoid it.

**Pthreads (POSIX threads)** — Linux’s threading API, implemented via NPTL (Native POSIX Thread Library) on top of the clone() system call: - pthread_create() spawns a thread running a given function; pthread_self() gets the caller’s own thread ID; threads are compared with pthread_equal() . - A thread ends by returning from its function, calling pthread_exit() , or being canceled. - pthread_join() blocks until a specific thread finishes and retrieves its return value (like waitpid() for threads); a thread can instead be pthread_detach() ed so its resources are reclaimed automatically on exit, at the cost of not being able to join it. - **Mutexes** : pthread_mutex_init() / destroy() , pthread_mutex_lock() / trylock() / unlock() guard critical sections; pthread_mutex_t can be statically initialized with PTHREAD_MUTEX_INITIALIZER. - Compiling/linking Pthread programs requires -pthread (or historically - lpthread ) . - The chapter closes pointing toward condition variables, read-write locks, and other primitives as further study beyond the basics covered.

## **Chapter 8: File and Directory Management**

**File metadata — the stat family** : stat() , fstat() , and lstat() (the last doesn’t follow symlinks) fill a struct stat with a file’s device, inode number, type, permission bits, link count, owning UID/GID, size, block count, and timestamps (access/modify/change time). This is how tools like ls -l get their information.

- **Permissions** : the classic rwx bits for owner/group/other, plus the special bits — **setuid** / **setgid** (run with the file

- owner’s/group’s privileges) and the **sticky bit** (on a directory, restricts deletion of files to their owner — e.g., /tmp ). chmod() / fchmod() change permissions; the process’s umask() masks bits off newly created files/directories.

- **Ownership** : chown() / fchown() / lchown() change owner/group; only privileged processes can generally give a file away to another user.

- **Extended attributes** : name/value pairs attached to a file beyond standard metadata ( getxattr() , setxattr() , listxattr() , removexattr() , and the namespaced variants) used for things like ACLs, SELinux labels, or capabilities.

**Directories** : getcwd() retrieves the current working directory; chdir() / fchdir() change it. mkdir() creates and rmdir() removes (empty) directories. Reading a directory’s contents uses opendir() / readdir() / closedir() over a DIR * stream, yielding struct dirent entries (name plus, on Linux, a d_type hint).

**Links** : a **hard link** ( link() ) is a second directory entry pointing at the same inode — indistinguishable from the “original,” removed via unlink() , and the underlying data isn’t freed until the link count hits zero _and_ no process still has it open. A **symbolic link** ( symlink() ) is a separate small file that just contains a path string, can cross filesystems (hard links can’t), and can dangle; read with readlink() .

**Copying and moving** : there’s no single “copy” system call — copying a file means opening the source, reading, writing to a new destination, and preserving metadata; rename() moves/renames atomically within the same filesystem (a cross-filesystem “move” degrades to copy+unlink).

**Device nodes** : special files ( /dev/sda, /dev/null, etc.) that represent hardware or kernel-provided interfaces rather than stored data, created with mknod() and identified by major/minor numbers. /dev/random and /dev/urandom are discussed as kernel-provided randomness sources, /dev/urandom being non-blocking and generally the right default for cryptographic-quality random bytes on Linux.

**Monitoring file events —** **inotify** : a Linux-specific API for watching files/directories for changes without polling. inotify_init() creates an instance (an fd you can read() or feed to select() / poll() / epoll() ) ; inotify_add_watch() registers a path and an event mask ( IN_MODIFY, IN_CREATE, IN_DELETE, etc.); reading the fd yields a stream of struct inotify_event records; inotify_rm_watch() removes a watch and close() tears down the whole instance. ioctl(fd, FIONREAD, ...) can report how many bytes of pending events are queued.

## **Chapter 9: Memory Management**

**The process address space** : a process’s virtual memory is organized into regions (“mappings”) — text (code), data (initialized globals), BSS (zeroed globals), heap (grows via brk() / sbrk() , historically what malloc() used), memory-mapped files/libraries, and the stack (grows downward, holds local variables and call frames). Memory is managed by the kernel in fixed-size **pages** (4 KB on most architectures), and the mapping from virtual to physical addresses is maintained transparently by the kernel and hardware MMU.

**Dynamic allocation** : malloc() / free() are the standard workhorses; calloc() allocates and zeroes an array (also guards against multiplication overflow, unlike hand-rolled malloc(n*size) ); realloc() resizes an existing allocation (may move it, preserving contents up to the smaller of the old/new size). glibc’s allocator internally uses sbrk() for smaller requests and mmap() directly for very large ones. posix_memalign() / aligned_alloc() / memalign() return allocations with a specific, larger-than-default alignment (needed for things like SIMD data).

**Advanced allocation tuning** : malloc_usable_size() reports the actual usable size of a block (often larger than requested, due to allocator bookkeeping/rounding); malloc_trim() asks the allocator to release free memory back to the OS; mallopt() / mallinfo() tune and inspect allocator behavior; glibc’s MALLOC_CHECK_environment variable (and tools like Valgrind / mtrace ) help debug heap corruption and leaks.

**Stack-based allocation** : alloca() allocates from the current stack frame, automatically freed on function return — fast, but dangerous for large or unbounded sizes (stack overflow, no error return) and non-portable in some contexts; strdupa() duplicates a string on the stack. C99 **variable-length arrays (VLAs)** are a language-level equivalent. The chapter gives guidance on choosing between the stack, the heap, and static/anonymous mappings depending on size, lifetime, and performance needs. **Anonymous memory mappings** : mmap() with MAP_ANONYMOUS (no backing file) is how large allocations and thread stacks are typically obtained — equivalent to mapping /dev/zero, which the chapter notes as the historical, more portable way to achieve the same thing.

**Manipulating raw memory** : the mem*() family — memset() (fill bytes), memcmp() (compare), memmove() / memcpy() (copy, with memmove() being safe for overlapping regions and memcpy() not), memchr() (search for a byte), and GNU extensions like memmem() / memfrob() (“frobnicating” — a trivial XOR-based obfuscation, not real encryption).

**Locking memory** : mlock() / munlock() (and whole-process mlockall() / munlockall() ) pin pages in physical RAM, preventing them from being swapped out — important for security-sensitive data (like cryptographic keys) or real-time code that can’t tolerate page-fault latency; subject to RLIMIT_MEMLOCK. mincore() checks whether specific pages are currently resident in physical memory. **Overcommit and OOM** : Linux by default allows processes to allocate (“commit”) more virtual memory than physically exists, betting that not all of it will actually be touched at once (“opportunistic allocation”); when physical memory genuinely runs out, the kernel’s **OOM killer** selects and kills a process to reclaim memory, rather than every allocation failing outright — a distinctive and sometimes surprising Linux behavior worth understanding when reasoning about malloc() failure semantics.

## **Chapter 10: Signals**

**Signal concepts** : a signal is an asynchronous notification delivered to a process, either from the kernel (e.g., SIGSEGV on an illegal memory access, SIGCHLD when a child changes state) or from another process. Each signal has a small integer identifier and a symbolic name ( SIGINT, SIGTERM, SIGKILL, SIGSTOP, etc.); some are catchable/blockable and some ( SIGKILL, SIGSTOP ) are not, by design.

**Basic management** : signal() is the classic, portable-but-limited way to install a handler; sigaction() is the modern, robust replacement that offers precise control over blocking, restart behavior, and additional handler info, and is generally preferred. A process can also just wait for the next signal with pause() . Handlers are inherited across fork() but reset to default on exec() (except ignored signals, which stay ignored). strsignal() / sys_siglist map signal numbers to human-readable strings.

**Sending signals** : kill(pid, sig) sends a signal to a process (or, with a negative pid, a whole process group) — permission requires matching UID (or privilege); raise(sig) sends a signal to the calling process/thread itself.

**Reentrancy** : because a handler can interrupt “normal” code at almost any point, it must only call **async-signal-safe** functions (a small, well-defined POSIX list) — most of the standard library, including malloc() and printf() , is _not_ safe to call from a handler, a common source of subtle bugs.

**Signal sets and blocking** : sigset_t plus sigemptyset() / sigfillset() / sigaddset() / sigdelset() / sigismember() build a set of signals; sigprocmask() (single-threaded) or pthread_sigmask() (threaded) blocks/unblocks sets of signals, temporarily deferring their delivery; sigpending() reports which blocked signals are currently pending; sigsuspend() atomically sets the block mask and waits,

#### avoiding races between checking and waiting.

**Advanced signal management** : sigaction() ’s siginfo_t structure carries extra context about _why_ a signal was raised (which process sent it, a faulting address for SIGSEGV, etc.), decoded via si_code. sigqueue() sends a signal along with a small integer or pointer payload (real-time signals, SIGRTMIN. . SIGRTMAX, additionally support queuing multiple pending instances rather than coalescing, unlike standard signals). The chapter notes this coalescing behavior of standard signals — if a signal is already pending, sending it again is a no-op until it’s delivered — as a real design wart inherited from classic Unix.

## **Chapter 11: Time**

**Representing time** : time_t (seconds since the Unix epoch, 1 Jan 1970 UTC) is the original, second-resolution representation; struct timeval adds microsecond precision; struct timespec adds nanosecond precision and is the modern preferred structure for new APIs. struct tm breaks a time value down into calendar fields (year, month, day, hour, etc.) via gmtime() / localtime() and reassembles via mktime() ; clock_t measures process CPU time (in clock ticks, convert via sysconf( _ SC_CLK_TCK) ).

**POSIX clocks** : clock_gettime() / clock_settime() / clock_getres() work against a named clock ID — CLOCK_REALTIME (wall-clock time, can jump if the system clock is adjusted), CLOCK_MONOTONIC (steadily increasing, unaffected by wall-clock changes — the right choice for measuring elapsed intervals), and others like CLOCK_PROCESS_CPUTIME_ID.

**Getting the current time** : time() (seconds only) → gettimeofday() (microseconds, the traditional “better interface”) → clock_gettime() (nanoseconds, the modern “advanced interface”). times() reports process/child CPU time usage.

**Setting the time** : stime() / settimeofday() / clock_settime() set the system clock (privileged operation); adjtime() / the kernel’s NTP-style tuning lets the clock be gradually slewed to a new value instead of jumping discontinuously, avoiding the problems a sudden jump causes for anything measuring intervals.

**Sleeping** : sleep() (seconds), usleep() (microseconds, obsolete), nanosleep() (nanoseconds, the modern POSIX call, and interruptible/resumable by tracking remaining time on EINTR ), and clock_nanosleep() (an “advanced” version that can sleep until an absolute time on a specified clock, avoiding drift from repeated relative sleeps). The chapter notes portable idioms for a “sleep that survives signals” and mentions that busy-waiting or misusing sleep for synchronization is generally an anti-pattern — blocking on the actual event (I/O, a condition variable, etc.) is preferable when possible.

**Timers** : alarm() is the simple, one-shot, second-resolution timer that delivers SIGALRM ; setitimer() / getitimer() provide repeating interval timers with microsecond resolution across a few clock types (real time, virtual/process time, profiling time); the modern POSIX **timer_create()/timer_settime()/timer_gettime()/timer_delete()** family offers per-process, high-resolution timers that can notify via signal or thread callback and support both one-shot and periodic firing.

## **Appendices (not detailed above)**

- **Appendix A — GCC Extensions to the C Language** : covers GNU C extensions used throughout the book’s examples (statement expressions, typeof, attributes like __ attribute __ ((packed)) , built-in functions, etc.) that go beyond standard C. **Appendix B — Bibliography** : a reading list of further Unix/Linux systems programming references (POSIX/SUS documentation, kernel internals books, and related titles).

_These notes summarize the structure and key ideas of each chapter for study purposes. For exact function signatures, error-code tables, and worked code examples, refer to the original book._ -e

## **—**

# 15 : Drivers, Real-Time, PCIe & Memory

> **Purpose:** Close the remaining gaps for 10–15+ year Linux Systems, Storage, Embedded, Device Driver, Infrastructure and GPU interviews. This section is intentionally interview-focused and complements the existing chapters rather than duplicating them.

## 1. Priority Inversion

Priority inversion occurs when a high-priority task is indirectly blocked by a lower-priority task holding a resource, while medium-priority work prevents the lower-priority task from running.

### Solution: Priority Inheritance

The low-priority owner temporarily inherits the higher priority of the blocked waiter.

### Interview points

- Priority inversion is different from a deadlock.
- Priority inheritance bounds the inversion in many cases; it does not make arbitrary locking problems disappear.
- Priority ceiling is another real-time synchronization strategy.
- In Linux/RT systems, understand mutex priority inheritance and the PREEMPT_RT implications.

## 2. PREEMPT_RT and Real-Time Linux

Know the distinction between normal Linux and a real-time configuration.

Important concepts:

### Why real-time kernels?

The goal is predictable scheduling latency rather than maximum average throughput.

- worst-case latency
- scheduling latency
- threaded interrupts
- priority inheritance
- real-time mutexes
- `SCHED_FIFO`
- `SCHED_RR`
- CPU affinity/isolation
- avoiding unbounded critical sections

### Interview question

**Why can a real-time system not simply use a normal mutex everywhere?**

Because blocking time, priority inversion, interrupt behavior and non-preemptible critical sections can create unacceptable latency. Real-time design focuses on bounded behavior.

## 3. Linux Device Model

A senior driver engineer should understand the relationship between the Linux device model objects.

### Important concepts

- `struct device`
- `struct device_driver`
- buses and driver matching
- `probe()` / `remove()`
- sysfs
- udev
- device lifetime/reference counting
- platform devices/drivers
- PCI devices/drivers

## 4. PCI / PCIe Driver Lifecycle

For a PCI device such as a discrete GPU or NIC:

Know these concepts:

- PCI configuration space
- vendor/device IDs
- BARs
- MMIO
- `pci_enable_device()`
- resource ownership
- DMA masks
- MSI/MSI-X
- suspend/resume
- reset/recovery
- `remove()`

**What is a BAR?**

A Base Address Register describes a PCI device's memory or I/O resource region. A driver uses the resource information to access device registers, commonly by mapping a memory BAR for MMIO.

## 5. Kernel Memory Allocation

Know the difference between the common kernel allocation mechanisms.

| Mechanism | Typical use | Key point |
|---|---|---|
| `kmalloc()` | small physically contiguous allocations | fast, physically contiguous memory |
| `kzalloc()` | zero-initialized kernel objects/buffers | equivalent to `kmalloc` + zeroing |
| `vmalloc()` | larger virtually contiguous regions | physical pages need not be contiguous |
| page allocator | page-level allocation | works with pages/order |
| slab/slub | frequently allocated kernel objects | object caching |

### GFP flags

**Critical interview point:**

`GFP_KERNEL` allocation may sleep. It must not be used in contexts where sleeping is illegal, such as hard interrupt context.

`GFP_ATOMIC` is intended for contexts where sleeping is not allowed, but it is not a general replacement for `GFP_KERNEL`.

## 6. Linux DMA API

A driver must not assume that a CPU virtual address, physical address and device DMA address are interchangeable.

Important APIs/concepts:

- DMA mask
- streaming vs coherent DMA
- cache coherency
- scatter-gather lists
- IOMMU/IOVA
- DMA mapping lifetime
- map/unmap ownership rules

**Why should a driver use the Linux DMA API instead of treating a physical address as the DMA address?**

Because the device's DMA-visible address space can differ from CPU physical addressing, especially with IOMMUs, and the architecture may require cache/coherency handling.

## 7. User/Kernel Memory Boundary

Never blindly dereference user-space pointers from kernel code.

Common interfaces:

Typical flow:

### Why?

- user pointers are untrusted
- address may be invalid
- page faults can occur
- access must follow kernel/user memory rules
- copying prevents the driver from trusting mutable user memory directly

**Why not simply dereference a pointer supplied through an ioctl?**

Because it is a user-space pointer and cannot be trusted as a normal kernel pointer. The kernel must validate/access it using the appropriate user-memory mechanisms.

## 8. Interrupt Context vs Process Context

This distinction should be automatic in a senior interview.

| Context | Can sleep? | Typical operations |
|---|---:|---|
| Process context | Yes, when allowed | mutex, blocking allocation, wait queues |
| Hard IRQ context | No | acknowledge device, minimal work, wake/defer work |
| Softirq/NAPI context | No sleeping | deferred networking/work |
| Threaded IRQ | Can sleep in thread context | longer interrupt processing |

### Typical interrupt flow

## 9. Kernel Locking Decision Table

Also know:

- raw vs normal spinlocks in RT configurations
- atomic operations
- per-CPU data
- lock ordering
- lockdep
- seqlocks
- wait queues
- completion objects

### Rule

Do not select a lock by memorized popularity. Select it based on **context, whether sleeping is legal, contention, reader/writer pattern, interrupt interaction and lifetime requirements**.

## 10. Wait Queues and Completions

Know the difference between protecting data and waiting for an event.

Common concepts:

- wait queues
- `wait_event*()` family
- `wake_up*()`
- completions
- condition/state must be checked under the appropriate synchronization

**Why should a wait be associated with a condition rather than just sleeping?**

Because the wakeup is a notification, while the condition/state determines whether the task can actually proceed. Spurious or unrelated wakeups must not cause incorrect progress.

## 11. Driver Error-Recovery Mental Model

For any driver failure, use this sequence:

---

# Interview Questions

_All interview / practice questions from the notes above, collected here in one place, grouped by chapter. Duplicate questions have been removed._

## 3 — Process Management

**Interview Questions**

**Fundamentals**
- What is a process?
- Program vs process?
- What is a PCB?
- What information is stored in a PCB?
- Explain process states.
- What is context switching?
- Why is context switching expensive?
- What are scheduling queues?
- Explain long-, medium-, and short-term schedulers.

**IPC and Processes**
- What is IPC?
- Shared memory vs message passing
- Independent vs cooperating processes
- Process vs thread
- CPU-bound vs I/O-bound process
- Parent vs child process
- Explain synchronization
- Mutex vs semaphore
- Critical-section problem

**Scheduling**
- Explain FCFS, SJF, Priority, and Round Robin.
- Preemptive vs non-preemptive scheduling
- Explain Multilevel Queue
- Explain Multilevel Feedback Queue
- Explain Linux scheduling
- What is vruntime?
- Explain SCHED_FIFO and SCHED_RR

**Process Lifecycle**
- Why is fork() fast?
- Explain Copy-on-Write.
- fork() vs vfork() vs clone()
- What happens during exec()?
- What are Zombie and Orphan processes?
- What does wait() do?
- How are Linux threads created?

**Advanced**
- Explain deadlock and its four conditions.
- Multiprogramming vs multitasking
- Concurrency vs parallelism
- Multiprocessing vs multithreading
- Hard vs soft real-time systems
- What is CPU affinity?
- What happens during a context switch?
- What information is saved in task_struct?
- How would you debug hundreds of Zombie processes?
- What does D state indicate?
- How would you investigate high context-switch rates?

## 5 — System Calls & Interrupts

**Interview Questions**

What is a system call? Why are system calls required? Difference between user mode and kernel mode? Give examples of system calls. What happens when printf() is executed? What is an interrupt?

Explain system call execution flow. Difference between hardware and software interrupts. What is an ISR?

Why are timer interrupts important? Explain mode switching.

How does a system call switch from user mode to kernel mode? Difference between system call and interrupt. How does Linux handle system calls? What happens internally when read() is called? Why are system calls slower than normal function calls? How do interrupts help in multitasking?

## 6 — Memory Management

**Interview Questions**

- What is memory management? Why is memory management required? Explain memory hierarchy. Difference between SRAM and DRAM. What are the responsibilities of memory management? What is contiguous memory allocation? What is non-contiguous memory allocation?

- Explain fixed partitioning. Explain dynamic partitioning. Internal vs external fragmentation. First Fit vs Best Fit vs Worst Fit. What is paging? What is a page? What is a frame? What is a page table? Explain logical and physical addresses. What is segmentation? Paging vs segmentation.

- Explain virtual memory. What is demand paging? What is a page fault? How does MMU work? Explain swapping. What is compaction? How does Linux manage memory? Why is paging preferred over dynamic partitioning? Why is virtual memory slower than RAM?

**Explain modern OS memory management techniques.**

## 6 — Memory Management (Linux-Specific Topics)

**Senior Interview Questions**

1. Explain Linux virtual address space.

2. What is mm_struct ?

3. What is vm_area_struct ?

4. Explain TLB.

5. What is a TLB miss?

6. Why are multi-level page tables used?

7. Explain Copy-on-Write.

8. Explain mmap() .

9. What is Page Cache?

10. Difference between Major and Minor page faults.

11. Explain Buddy Allocator.

12. Why are SLAB/SLUB allocators needed?

- kmalloc() and vmalloc() .

14. What are Linux memory zones?

15. What are Huge Pages?

16. What is the OOM Killer?

17. How do you debug memory leaks?

18. How do you investigate high page faults?

19. Why is cached memory usually high on Linux?

20. Explain the memory layout of a Linux process.

## 7 — Interrupts (Deep Dive)

**Important Interview Questions**

**Q1. What is an interrupt?** A mechanism that allows hardware/software to request CPU attention asynchronously. **Q2. Why use interrupts instead of polling?** Interrupts allow the CPU to perform useful work until an event occurs, reducing unnecessary CPU usage.

**Q3. Can an interrupt handler sleep?** No, hard interrupt context cannot sleep.

**Q4. What is a bottom half?** A mechanism for deferring interrupt-related processing so the hard interrupt handler can return quickly.

**Q5. Softirq vs workqueue?** Softirq → atomic context, cannot sleep. Workqueue → process context, can generally sleep.

**Q6. What is an interrupt storm?** A situation where interrupts occur excessively, consuming significant CPU time.

**Q7. How do you detect an interrupt storm?** Start with cat /proc/interrupts and identify IRQs whose counters are increasing abnormally fast.

**Q8. What is interrupt affinity?** The CPU or set of CPUs to which an interrupt can be routed.

**Q9. What is MSI-X?** A PCI/PCIe interrupt mechanism supporting multiple interrupt vectors, useful for distributing device queues across CPUs.

**Q10. What is NAPI?** Linux networking’s mechanism for combining interrupt-driven notification with polling/batching to handle high packet rates efficiently.

## 8 — Networking Basics

- **Q: What happens when send() is called?**

Strong answer:

Do not say: send() directly sends data to the NIC. There are many kernel layers in between.

- **Q: What happens when a packet arrives?**

Wake waiting process ↓ Scheduler ↓ Application

This is one of the most important Linux networking diagrams to memorize.

- **Q: Why is NAPI used?**

Because handling an interrupt for every incoming packet can create enormous interrupt overhead. NAPI combines:

Interrupt notification + Polling/batching

to improve packet-processing efficiency under load.

- **Q: What is sk_buff ?**

sk_buff is a core Linux networking buffer structure representing packet data and associated metadata as it moves through the networking stack.

Know:

- **Q: Why are RX/TX rings used?**

They provide a queue of descriptors/buffers through which the NIC and driver exchange packet ownership and state. Conceptually:

They support efficient asynchronous DMA-based packet processing.

- **Q: Why can a NIC generate too many interrupts?**

At high packet rates:

1 packet → 1 interrupt

can overwhelm the CPU. Linux/NICs address this using mechanisms such as:

NAPI Interrupt coalescing Batch processing RSS

- **Q: What is RSS?**

Receive Side Scaling distributes incoming traffic across multiple receive queues/CPUs.

This allows packet processing to scale across cores.

- **Q: What is the difference between TCP and UDP from Linux kernel perspective?**

TCP maintains substantial connection state:

Sequence numbers ACKs Retransmissions Congestion control Flow control Connection state

UDP is much simpler:

Datagram + Checksum + Socket delivery

The kernel still performs routing, buffering, socket lookup, and other networking work for both.

- **Q: Why can CLOSE_WAIT indicate an application problem?**

Because it means the remote side has closed its direction of the TCP connection, but the local application has not completed its own close.

A large persistent number of CLOSE_WAIT sockets can indicate leaked connections or incorrect application cleanup.

- **Q: Why does TIME_WAIT exist?**

It helps protect TCP connection correctness by allowing delayed packets from an old connection to expire and supporting safe connection termination semantics.

A high TIME_WAIT count is not automatically a bug.

- **Q: How does Linux networking scale on multicore CPUs?**

Important mechanisms include:

RSS RPS RFS NAPI IRQ affinity CPU affinity Multiple RX/TX queues NUMA-aware placement The goal is: NIC queues ↓ Multiple CPUs ↓ Parallel packet processing

The goal is:

while preserving locality.

- **Q: What causes packet drops?**

Possible causes:

NIC RX ring overflow NAPI budget pressure CPU saturation Socket receive buffer full Memory pressure Network congestion Driver limitations qdisc drops Firewall/filtering Application not consuming data

Use statistics rather than guessing.

- **Q: How would you debug high network CPU usage?**

Start with:

1. CPU utilization

2. /proc/interrupts

3. NIC queue distribution

4. NAPI behavior

5. RSS/RPS configuration

6. Packet rate

7. GRO/GSO/TSO

8. Driver statistics

9. Socket/application behavior

10. perf tracing/profiling

The key is to determine whether CPU is being consumed by:

IRQ NAPI TCP/IP processing Copying Application

- **Q: How does container networking work?**

This connects:

Namespaces + Virtual Ethernet + Bridge + Routing + Netfilter + NIC driver

## 9 — Block I/O

- **Q: What is the Linux block layer?**

It is the kernel subsystem that provides generic block-device I/O infrastructure between filesystems and block-device drivers.

- **Q: What is a BIO?**

A BIO represents an I/O operation at the block layer, describing the operation and associated data segments.

- **Q: What is a request?**

A request represents block-layer work being processed toward a block device. Depending on the I/O path, it can contain or be associated with one or more BIOs.

- **Q: Why do we need an I/O scheduler?**

To manage outstanding block I/O and potentially improve:

- **Q: Why is DMA used?**

To allow devices to transfer data directly between the device and memory without requiring the CPU to copy every byte.

- **Q: What happens when a disk read completes?**

High-level:

- **Q: Why is NVMe faster than traditional SATA storage?**

NVMe is designed for high-performance storage over PCIe and supports substantial parallelism with multiple queues and lower protocol overhead.

- **Q: Buffered I/O vs Direct I/O (diagram)**
Application
 │
 ▼
Page Cache
 │
 ▼
Storage

Direct I/O:

- **Q: Why can a process sleep during I/O?**

If required data is not immediately available, a blocking operation can put the task to sleep instead of wasting CPU cycles. When the I/O completes:

- **Q: What is the difference between sequential and random I/O?**

Sequential:

100 101 102 103 104 Random:

100 9000 32 500 7000

Sequential I/O is generally easier for storage devices to process efficiently, especially on rotational media.

## 10 — Kernel Locking, Synchronization & RCU

**Senior Interview Questions**

1. Why can’t you sleep while holding a spinlock?

2. When would you choose a mutex over a spinlock, and vice versa?

3. What does spin_lock_irqsave() protect against that spin_lock() doesn’t?

4. What is a per-CPU variable and why does it avoid locking overhead?

5. Explain RCU in your own words — what problem does it solve?

6. What is a grace period in RCU?

7. Why is rcu_read_lock() so much cheaper than a spinlock?

8. Can an RCU read-side critical section sleep? Why or why not?

9. What’s the difference between synchronize_rcu() and call_rcu() ?

10. Where does the Linux kernel actually use RCU (give real examples)?

11. What does lockdep detect, and how?

12. Explain priority inversion and how PREEMPT_RT mitigates it.

13. What is a seqlock, and when would you prefer it over RCU?

14. Why is i++ unsafe on a variable shared across CPUs, and what’s the fix?

15. Walk through what happens if a driver forgets call_rcu() and just calls kfree() on data another CPU might be reading.

## PART A.11 — ARM & SoC Internals

**Senior Interview Questions**

1. What are ARM Exception Levels? Map them to the x86 privilege model.

2. Why does ARM need EL2, and what runs there?

3. What is TrustZone, and what does EL3 have to do with it?

4. What instruction does a Linux syscall use on ARM64, and what EL transition does it cause?

5. What is PSCI, and why can’t the kernel just power off a core directly?

6. What problem does Device Tree solve that PCI enumeration/ACPI don’t cover on ARM SoCs?

7. Walk through the boot-time flow from.dtb to a driver’s probe() being called.

- .dts, .dtsi, and.dtb ?

9. Explain the MESI cache coherency protocol states.

10. What does the “Owned” state in MOESI add, and why?

11. Why do you still need memory barriers if caches are coherent?

12. What is false sharing / cache-line bouncing, and how do per-CPU variables help?

13. Difference between cpufreq and cpuidle?

14. What does the schedutil governor do differently from ondemand ?

15. What is Runtime PM, and how does it differ from system suspend/resume?

16. Why do some DMA buffers require explicit cache maintenance ( dma_sync _ * ) while normal CPU-to-CPU memory doesn’t?

## PART A.12 — Kernel Debugging & Crash Analysis

**Senior Interview Questions**

1. What’s the difference between a kernel oops and a panic?

2. Why does an oops in interrupt context typically escalate to a panic?

3. Given an oops’s RIP line, how do you find the exact source line?

4. What does the Tainted: field tell you, and why does it matter?

5. How does kdump capture a crash dump before the system fully halts?

6. What’s the role of kexec in kdump?

7. In the crash tool, why is bt -a more useful than bt for diagnosing a race condition?

8. What’s the difference between ftrace’s function and function_graph tracers?

9. When would you reach for perf instead of ftrace ?

10. What does KASAN actually instrument, and what class of bugs does it catch that a normal build won’t?

11. Why is KFENCE viable in production but KASAN generally isn’t?

12. What does an RCU stall warning actually indicate, and what’s your first debugging step?

13. What does lockdep detect that a plain deadlock reproduction wouldn’t (i.e., before it ever actually deadlocks)?

14. Walk through, end-to-end, how you’d debug an intermittent NULL pointer crash in a workqueue-based driver that only reproduces under high load.

## PART A.13 — Driver Skeleton & Real Kernel Code Walkthrough

**Senior Interview Questions**

1. Walk through what happens, step by step, from mydev_probe() being called to the IRQ handler being registered.

2. Why does remove() need cancel_work_sync() before freeing device state?

3. What does devm _ * do, and why does it simplify error-path cleanup in probe() ?

4. In the IRQ handler example, why is spin_lock() used instead of spin_lock_irqsave() ?

5. Why must copy_to_user() / copy_from_user() be used instead of direct pointer dereference?

6. In pick_next_task_fair, why is the leftmost red-black tree node the one that runs next?

7. What is container_of() , and where did you see it used in this chapter?

8. Why does wait_event_interruptible re-check its condition in a loop instead of a single if ?

9. Given an unfamiliar 40-line kernel function, what’s the first thing you’d try to determine before reading line by line?

10. In the driver skeleton, what would go wrong if IRQF_SHARED were used but the handler didn’t check the status register before acknowledging?

## 15 — Drivers, Real-Time, PCIe & Memory

**Common question**

**kmalloc vs vmalloc?**

`kmalloc()` provides physically contiguous memory for the requested allocation. `vmalloc()` provides virtually contiguous kernel virtual address space backed by potentially non-contiguous physical pages.

**Classic questions**

- Can a hard IRQ handler sleep? **No.**
- Can you take a mutex in hard IRQ context? **No.**
- Why defer work? **To move sleepable or longer processing out of atomic interrupt context.**

## General / Rapid-Fire Review

**PART A.16 — Interview Rapid-Fire Questions**

1. Explain priority inversion and priority inheritance.
2. Why can a hard IRQ handler not sleep?
3. Mutex vs spinlock: how do you decide?
4. What is RCU and what problem does it solve?
5. What happens in a PCI driver's `probe()`?
6. What is a PCI BAR?
7. Why are MMIO registers accessed with special I/O mapping/accessors?
8. What is the difference between CPU virtual, physical and DMA addresses?
9. Why is an IOMMU useful?
10. `kmalloc()` vs `vmalloc()`?
11. `GFP_KERNEL` vs `GFP_ATOMIC`?
12. Why use `copy_from_user()`?
13. What happens if a device writes memory through DMA after the buffer has been freed?
14. How do you safely manage DMA buffer lifetime?
15. What is a wait queue?
16. Why are interrupts often split into top-half/deferred processing?
17. How do you debug a kernel hang?
18. How do you debug a memory corruption issue?
19. How do lockdep, KASAN and KCSAN differ?
20. What changes in PREEMPT_RT?
