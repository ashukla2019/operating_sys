# Chapter 2 – Linux Process Internals

## 1. What Is a Process?
A **process** is a running instance of a program. A program is a passive executable file (`server`). When executed (`./server`), Linux creates a process containing:
- Program code, Data, Heap, Stack
- CPU execution state
- Virtual address space
- Open file information
- Signal information
- Scheduling information
- Credentials
- Kernel bookkeeping information

Conceptually:
```
Program --execute--> Process
                        +-- Code
                        +-- Data
                        +-- Heap
                        +-- Stack
                        +-- Registers
                        +-- Virtual Address Space
                        +-- Open Files
                        +-- Kernel State
```

---

## 2. Program vs Process
A **program** is a file containing executable instructions (e.g. `/bin/myserver`). A **process** is an executing instance of that program — running `./myserver` creates one.

Multiple processes can execute the same program:
```
/bin/server
    +---- Process 1000
    +---- Process 1001
    +---- Process 1002
```
Each process has its own process state and normally its own virtual address space.

---

## 3. Linux Process Representation
Linux represents a schedulable task using `struct task_struct` — one of the most important kernel structures for Linux interviews. Conceptually:
```
task_struct
├── Process identity
├── Scheduling information
├── Process state
├── Parent/child relationships
├── Memory information
├── File information
├── Signal information
├── Credentials
├── Thread information
└── Kernel stack information
```
The actual structure is large and changes between kernel versions. For interviews, understand what it represents rather than memorizing every field.

---

## 4. Important Information Associated with task_struct
- **Process Identity:** PID, TID, TGID, Parent, Children
- **Scheduling:** Priority, scheduling policy, CPU affinity, runtime information
- **Memory:** associated with `mm_struct`, which describes the process address space
- **Files:** associated with `files_struct`, which tracks open file descriptors
- **Signals:** contains or references signals, signal handlers, and signal state

---

## 5. Process ID – PID
Every process has a Process ID (`PID`). Example: `ps` might output:
```
PID     CMD
1000    server
1001    worker
1002    shell
```
PID identifies a process within its PID namespace. You can obtain the current process ID using `getpid();`

---

## 6. Parent Process
Processes normally have a parent:
```
PID 1
 +-- shell
      +-- gcc
      +-- server
           +-- worker
```
The parent PID can be obtained using `getppid();` The process hierarchy is important for process creation, process termination, child management, reaping, and signals.

---

## 7. PID 1
The first userspace process is traditionally `PID 1` — on many modern Linux systems, `systemd`:
```
Kernel --> PID 1 --> Services
                 --> Daemons
                 --> Other Processes
```
PID 1 has special responsibilities: system initialization, service management, and reaping orphaned processes.

---

## 8. Process Hierarchy
Processes form a parent-child hierarchy:
```
PID 1
 +-- bash
      +-- gcc
      +-- application
             +-- worker
             +-- worker
```
When a process creates another, `Parent --> Child`. The kernel tracks these relationships.

---

## 9. Process States
A process moves through several states. A useful mental model:
```
RUNNABLE --scheduler selects task--> RUNNING --waits for I/O/event--> WAITING --event occurs--> RUNNABLE
```

## 10. Running vs Runnable
**Running** — the task is currently executing on a CPU (`CPU --- Process A`).
**Runnable** — the task is ready to execute but waiting for CPU time:
```
Run Queue: Process A, Process B, Process C --> CPU
```
The scheduler selects a runnable task.

---

## 11. Waiting
A process may need to wait for an event, e.g. `read(fd, buffer, size);`. If data isn't available, the task sleeps:
```
Running --> Waiting for I/O --> Blocked --I/O completes--> Runnable
```
This allows another task to use the CPU.

---

## 12. Why Sleeping Is Important
A busy-wait loop like `while (!data_available) { }` wastes CPU. Instead, the process sleeps:
```
Process --> Sleep --Event occurs--> Wake up --> Runnable
```
This is fundamental to efficient operating-system design.

---

## 13. Process Creation
Linux provides several mechanisms for creating tasks: `fork()`, `vfork()`, `clone()`. The traditional pattern:
```
fork() --> Child --> execve() --> New Program
```

## 14. fork()
`fork()` creates a child process:
```c
#include <stdio.h>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
        printf("Child\n");
    else if (pid > 0)
        printf("Parent\n");

    return 0;
}
```
After `fork()`: `Parent --> Child`. The child starts as a logical copy of the parent's process state.

---

## 15. fork() Return Value
A common interview question — `pid_t pid = fork();`
- **Parent:** `pid > 0` — the return value is the child's PID
- **Child:** `pid == 0`
- **Failure:** `pid < 0`

So: Parent's `fork()` → child PID; Child's `fork()` → 0; Failure → -1.

---

## 16. Does fork() Copy All Memory?
Logically, the child receives a copy of the parent's address space, but Linux does **not** immediately copy every physical memory page — instead it uses **Copy-on-Write**, making `fork()` much more efficient.

## 17. Copy-on-Write
Before fork, parent virtual address points to Physical Page A. After `fork()`, parent and child virtual addresses can both point to the same Physical Page A initially, sharing it. The kernel marks the relevant pages so a write triggers special handling.

## 18. Child Writes to a Shared Page
Suppose the child modifies a shared page (`x = 100;`). The CPU detects the page can't currently be written, causing a page fault:
```
Child writes --> Page Fault --> Kernel --> Allocate new physical page
--> Copy original page --> Update child's page table --> Retry instruction
```
Now: `Parent ---> Page A`, `Child ----> Page B`. Parent and child have separate physical pages only after modification.

## 19. Why Copy-on-Write?
Without COW: `fork()` on a 1 GB process would copy 1 GB immediately — expensive. With COW: `fork()` shares pages, then copies only modified pages. Benefits: faster process creation, lower memory consumption, especially efficient when `fork()` is immediately followed by `exec()`.

---

## 20. fork() + exec()
A common Linux pattern: `fork() --> Child --> execve() --> New Program`, commonly used by shells. For example, running `ls`:
```
Shell --fork()--> Child --execve("ls", ...)--> ls
```
The parent shell may wait for the child.

## 21. What Does execve() Do?
`execve()` does **not** create a new process — it replaces the current process's program image. Before: PID 1000 contains Shell Code/Data/Heap/Stack. After: PID 1000 contains ls Code/Data/Heap/Stack. The PID (1000) remains; the executing program changes.

## 22. fork() vs exec()
| Feature | fork() | execve() |
|---|---|---|
| Creates child | Yes | No |
| Creates new PID | Yes | No |
| Replaces program image | No | Yes |
| Uses COW | Yes | N/A as creation mechanism |
| Typical use | Create child | Run another program |

Remember: `fork()` → create child; `exec()` → replace program image.

---

## 23. vfork()
`vfork()` is a specialized process-creation mechanism, historically meant to make the common `fork()` + `exec()` pattern more efficient by avoiding some address-space setup:
```
Parent --vfork()--> Child --exec()--> New Program
```
`vfork()` has restrictive semantics because the child temporarily shares the parent's address space. For normal application development, `fork()` + COW is generally the important concept to understand.

## 24. clone()
Linux provides `clone()`, which allows selective resource sharing: address space, file table, filesystem information, signal handlers, and other resources. This flexibility is important for threads, containers, and namespaces.

---

## 25. Process vs Thread
A process provides an execution environment; a thread is an execution context within it:
```
Process
 +-- Thread 1
 +-- Thread 2
 +-- Thread 3
```
Threads in the same process typically share Code, Heap, Global Data, Address Space, and Open File Descriptions. Each thread has its own Registers, Stack, Instruction Pointer, Scheduling state, and Thread ID.

## 26. Process vs Thread
| Resource | Process | Thread |
|---|---|---|
| Address Space | Separate | Shared |
| Code | Separate address space | Shared |
| Heap | Separate | Shared |
| Stack | Own | Own |
| Registers | Own | Own |
| Open Files | Own descriptor table | Usually shared |
| Isolation | Stronger | Weaker |
| Creation overhead | Higher | Lower |

The exact sharing behavior depends on how the task was created.

## 27. Linux Threads
Linux uses the same fundamental task representation for processes and threads (`struct task_struct`). A thread created using `pthread_create()` ultimately uses Linux task-creation mechanisms:
```
pthread_create() --> clone()/clone3() --> New task --> task_struct
```
The sharing flags determine which resources are shared.

## 28. PID, TID and TGID
For a multithreaded process, Linux distinguishes `PID`, `TID`, `TGID`:
- **TID** identifies an individual thread.
- **TGID** identifies the thread group.
- User-space commonly refers to the thread group's ID as the process ID.
- For the main thread: `TID == TGID`.

---

## 29. Context Switching
A CPU can execute only a limited number of tasks simultaneously; the scheduler switches between tasks — a **Context Switch**:
```
CPU --> Task A --context switch--> Task B --context switch--> Task C
```

## 30. What Happens During a Context Switch?
```
Task A running --> Save Task A execution state --> Scheduler chooses Task B
--> Restore Task B execution state --> Task B runs
```
Execution state includes CPU registers, stack pointer, instruction pointer, and architecture-specific CPU state. The exact implementation is architecture-specific.

## 31. Why Context Switching Has a Cost
Context switching is not free. Potential costs: saving/restoring CPU state, scheduler overhead, cache disruption, TLB-related effects, pipeline disruption, loss of useful CPU-local state. So too many context switches can degrade performance — particularly important in high-performance networking, storage, embedded systems, low-latency systems, and CPU-intensive applications.

---

## 32. Kernel Stack
Each task has kernel-stack state used while executing kernel code on its behalf:
```
User Space --system call--> Kernel Entry --> Kernel Stack --> Kernel Functions
```
Example: `Application --read()--> System Call Entry --> VFS --> Filesystem --> Block Layer`. The kernel does not normally execute kernel code using the application's user stack.

## 33. User Stack vs Kernel Stack
```
Process
 +-- User Virtual Space (Stack)
        |  syscall
        v
     Kernel Space (Kernel Stack)
```
This separation is important for security, fault isolation, and kernel execution.

---

## 34. Process Address Space
A typical 64-bit Linux process has a virtual address space with regions:
```
High Address
  Kernel Mapping
  User Stack (grows down)
  mmap() Regions
  Heap (grows up)
  BSS / Data / Read-only Data / Text
Low Address
```
The exact layout depends on CPU architecture, kernel configuration, ASLR, process configuration, and kernel version.

## 35. mm_struct
Linux maintains address-space information using `struct mm_struct`:
```
task_struct --> mm_struct
                  +-- Page tables
                  +-- Virtual memory information
                  +-- Memory mappings
                  +-- VMAs
```
It describes the process's user-space memory environment.

## 36. Virtual Memory Areas
Linux represents contiguous virtual-memory regions using `struct vm_area_struct` — examples: Code, Data, Heap, Stack, Shared Libraries, mmap() regions:
```
Process Address Space
+-- VMA: Code
+-- VMA: Read-only Data
+-- VMA: Data/BSS
+-- VMA: Heap
+-- VMA: Shared Library
+-- VMA: mmap Region
+-- VMA: Stack
```

---

## 37. Process Termination
A process can terminate through `exit()` or `_exit()`, or because of fatal signals, exceptions, or kernel-enforced termination:
```
Running Process --> exit() --> Termination --> Zombie --parent waits--> Reaped
```

## 38. Zombie Process
A zombie is a process that has terminated but whose parent has not yet collected its termination status. Important: a zombie is **not** running — it retains limited kernel bookkeeping information.
```
Parent --> Child --exit()--> Zombie
```
When the parent calls `wait()` or `waitpid()`, the child's exit information is collected.

## 39. Why Do Zombies Exist?
Suppose the child exits with status 10 — the parent may need this information, so the kernel retains it until collected:
```
Child --exit(10)--> Zombie --waitpid()--> Parent receives status --> Child entry fully reaped
```

## 40. Orphan Process
An orphan is a process whose original parent has terminated while the child is still alive. When `Parent` exits, `Child` is reparented to an appropriate process/reaper — in a simple traditional model, PID 1. Modern Linux also has additional mechanisms for selecting subreapers.

## 41. Zombie vs Orphan
| Feature | Zombie | Orphan |
|---|---|---|
| Still executing? | No | Usually yes |
| Original parent alive? | Usually yes | No |
| Exit status retained? | Yes | Not yet, while running |
| Main issue | Not reaped | Parent disappeared |

Remember: Zombie != Orphan.

## 42. wait() and waitpid()
A parent collects child termination info using `wait()` or `waitpid()`:
```c
pid_t pid = fork();

if (pid == 0)
{
    _exit(10);
}
else
{
    int status;
    waitpid(pid, &status, 0);
}
```
Flow: `Parent --- Child --exit(10)--`, `Parent --- waitpid() --> Collect status`.

---

## 43. Signals
Signals are asynchronous notifications delivered to a process or thread — examples: `SIGINT`, `SIGTERM`, `SIGKILL`, `SIGSEGV`, `SIGSTOP`, `SIGCHLD`. Example: `kill -TERM <pid>` — the kernel sends `SIGTERM` to the target.

## 44. SIGTERM vs SIGKILL
A common interview question.
**SIGTERM** — requests graceful termination; an application can catch it and clean up:
```
SIGTERM --> Application: stop accepting work, flush data, close resources, exit
```
**SIGKILL** — forces termination; the process cannot catch or ignore it.

So: `SIGTERM` → request graceful termination; `SIGKILL` → forced termination.

## 45. SIGCHLD
When a child changes state, the parent can receive `SIGCHLD` — particularly relevant when managing child processes:
```
Child --exit--> SIGCHLD --> Parent --> wait()/waitpid()
```

---

## 46. Process Creation – Complete Flow
Simplified `fork()` flow:
```
Parent Process --fork()--> Kernel
   +-- Create task state
   +-- Assign PID
   +-- Establish parent/child relationship
   +-- Set up scheduling state
   +-- Set up memory relationships
   +-- Set up resource references
   --> Child becomes runnable
```
Memory is handled efficiently using Copy-on-Write.

## 47. fork() + exec() – Complete Flow
Typical shell execution:
```
Shell --fork()--> Child --execve()--> Load executable
   +-- Replace process image
   +-- Set up memory mappings
   +-- Load executable segments
   +-- Load dynamic linker/shared libraries
   +-- Set program entry point
   --> New Program
```
Parent: `Shell --waitpid()--> Child exits --> Shell continues`.

## 48. Context Switch – Simplified Flow
```
Task A running --> Scheduler event --> Save Task A CPU state --> Scheduler
--> Select Task B --> Restore Task B CPU state --> Task B runs
```
The real implementation depends on CPU architecture, kernel version, scheduler implementation, and kernel configuration.

---

## 49. CPU Affinity
A process/thread can be restricted to specific CPUs, e.g. `Process A --> CPU 2, CPU 3`. Linux provides APIs/tools for this, e.g. `taskset -c 2 ./application`. CPU affinity can be important for performance tuning, cache locality, real-time workloads, networking, and NUMA systems.

## 50. Namespaces
Linux namespaces provide isolation of system resources — important namespaces include: PID, Mount, Network, UTS, IPC, User, Cgroup, Time. Namespaces are fundamental to containers.

## 51. PID Namespace
PID namespaces allow processes to have different PID views. Host: `PID 5000 --> Container Process`. Inside the container: `PID 1`. The same underlying task can therefore have different PID values from different namespace perspectives.

## 52. Containers and Processes
A container is not a virtual machine — it primarily uses Linux kernel mechanisms: Namespaces + cgroups + Filesystem isolation + Capabilities:
```
Container A, Container B, Container C --> Linux Kernel --> Hardware
```
All containers normally share the same host kernel.

## 53. Process vs Virtual Machine
| Feature | Container | VM |
|---|---|---|
| Kernel | Shared | Separate guest kernel |
| Startup | Fast | Slower |
| Isolation | Kernel mechanisms | Hardware virtualization |
| Memory overhead | Lower | Higher |
| Hardware virtualization | Not required | Usually used |

---

## 54. Important Mental Model
```
                    Process / Task
                          |
                    task_struct
          +---------------+---------------+
          |               |               |
     Scheduling        Memory           Files
                          |               |
                     mm_struct       files_struct
                          |
                  Virtual Address Space
                          |
                     Page Tables
                          |
                     Physical RAM
```

## 55. Process Execution Mental Model
```
Program --> Process --> task_struct --> Scheduler --> CPU
```
When the process needs the kernel:
```
User Code --system call/interrupt--> Kernel --> Kernel Code --> Hardware/Kernel Subsystem
```

---

## 56. Senior Interview Flow: fork()
If asked *"What happens when fork() is called?"* — a strong answer:
```
User process --fork()--> System call/kernel process creation
     +-- Create child task state
     +-- Assign PID/TID information
     +-- Establish parent-child relationship
     +-- Set scheduling state
     +-- Duplicate/share required resources
     +-- Set up address-space relationship
     +-- Use Copy-on-Write for memory
     --> Child becomes runnable
```
Do **not** say "Linux copies the entire process memory" — that's an oversimplification. Instead: "The child gets a logically duplicated address space, while physical pages are initially shared using Copy-on-Write."

## 57. Senior Interview Flow: execve()
If asked *"What happens when execve() is called?"*:
```
Process --execve()--> Kernel
   +-- Validate executable
   +-- Prepare new address space
   +-- Load executable segments
   +-- Set up stack/arguments/environment
   +-- Load dynamic linker when required
   +-- Set instruction pointer to entry point
   --> New Program Starts
```
The PID remains the same.

## 58. Senior Interview Flow: Context Switch
If asked *"What happens during a context switch?"*:
```
Current task --> Save architecture-specific CPU state --> Scheduler selects next task
--> Switch task/kernel state --> Restore next task's CPU state --> Resume execution
```
Also mention: context switches can affect cache locality and performance.

## 59. Senior Interview Flow: Zombie
If asked *"Why does a zombie process exist?"*:
```
Child terminates --> Kernel records exit information --> Child becomes zombie
--> Parent calls wait()/waitpid() --> Exit status collected --> Child is reaped
```
The zombie is not consuming CPU.

---

## 60. Important Interview Questions

**Q1. What is a process?**
A process is an executing instance of a program together with its virtual address space, execution state, resources, and kernel bookkeeping.

**Q2. What is task_struct?**
The primary kernel data structure representing a schedulable task. It contains or references identity, scheduling, memory, files, signals, credentials, and parent/child relationships.

**Q3. Does fork() immediately copy all physical memory?**
No — Linux uses Copy-on-Write so parent and child can initially share physical pages.

**Q4. Does execve() create a new process?**
No — it replaces the current process's program image. The PID remains unchanged.

**Q5. Why is fork() + exec() commonly used?**
It separates process creation from program loading, allowing a shell or supervisor to create a child and then have that child execute another program.

**Q6. What is the difference between a process and a thread?**
Processes normally have separate address spaces, while threads within the same process share the address space and many resources but have independent execution state and stacks.

**Q7. What is Copy-on-Write?**
COW allows parent and child to initially share physical pages after `fork()`. A page is copied only when a process needs to modify it.

**Q8. What is a zombie?**
A terminated child that has not yet been reaped by its parent.

**Q9. What is an orphan?**
A process whose original parent has terminated while the child is still alive.

**Q10. What is a context switch?**
The process of switching CPU execution from one task to another by saving the current execution state and restoring another task's state.

**Q11. Why are threads cheaper than processes?**
Threads can share the same address space and many resources, reducing creation and management overhead.

**Q12. What are PID, TID and TGID?**
PID/TGID identify a thread group from the user-visible process perspective; TID identifies an individual thread; the main thread's TID normally equals the thread group's ID.

---

## 61. Questions You Should Be Able to Explain on a Whiteboard
Before moving to the next chapter, you should be able to draw and explain:
1. Process hierarchy
2. task_struct
3. fork()
4. Copy-on-Write
5. fork() + exec()
6. Process vs Thread
7. Context Switch
8. User Stack vs Kernel Stack
9. Process Address Space
10. Zombie vs Orphan
11. PID Namespace
12. Container vs VM

---

## 62. Interview Priority

**Must Know:** task_struct, Process states, fork(), execve(), Copy-on-Write, Process vs Thread, Context switching, Zombie, Orphan, wait(), waitpid(), PID/TID/TGID, Kernel stack, mm_struct, Virtual address space

**Should Know:** clone(), vfork(), Signals, SIGTERM, SIGKILL, SIGCHLD, CPU affinity, Namespaces, VMAs

**Deep Dive When Role Requires It:** Scheduler run queues, CFS internals, Real-time scheduling, CPU affinity internals, NUMA scheduling, PID allocation internals, clone3(), cgroups, Subreapers

---

## 63. Quick Revision Sheet
```
PROCESS
   +-- task_struct
   +-- Address Space
   +-- Open Files
   +-- Signals
   +-- Scheduling State
```
Process creation: `fork() --> Child --> Copy-on-Write`
Program replacement: `execve() --> New Program Image`
Threads: `Process --> Thread, Thread, Thread`
Termination: `exit() --> Zombie --wait()--> Reaped`
Scheduling: `Runnable --> Scheduler --> Running --> Waiting --> Runnable`
Containers: `Namespaces + cgroups --> Containers`

---

## 64. Chapter Summary
The key mental model:
```
                 Linux Task
                     |
                task_struct
        +------------+------------+
        |            |            |
   Scheduling      Memory       Files
                      |
                 mm_struct
                      |
              Virtual Memory
                      |
                 Page Tables
                      |
                 Physical RAM
```
Process creation: `fork() --> Child task --> COW memory`
Program execution: `fork() --> Child --> execve() --> New Program`
Process termination: `exit() --> Zombie --> wait()/waitpid() --> Reaped`

The most important senior-level concept is to understand the **execution flow**, not merely memorize API definitions.

---
