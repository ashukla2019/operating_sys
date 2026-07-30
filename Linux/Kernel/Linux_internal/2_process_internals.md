# Chapter 2 – Linux Process Internals

---

## 1. What Is a Process?

A **process** is a running instance of a program.

A program is a passive executable file:

```text
server
```

When the program is executed:

```bash
./server
```

Linux creates a process containing:

* Program code
* Data
* Heap
* Stack
* CPU execution state
* Virtual address space
* Open file information
* Signal information
* Scheduling information
* Credentials
* Kernel bookkeeping information

Conceptually:

```text
Program
   |
   | execute
   v
Process
   |
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

# 2. Program vs Process

A **program** is a file containing executable instructions.

A **process** is an executing instance of that program.

Example:

```text
/bin/myserver
```

is a program.

Running:

```bash
./myserver
```

creates a process.

Multiple processes can execute the same program:

```text
/bin/server
    |
    +---- Process 1000
    |
    +---- Process 1001
    |
    +---- Process 1002
```

Each process has its own process state and normally its own virtual address space.

---

# 3. Linux Process Representation

Linux represents a schedulable task using:

```c
struct task_struct
```

`task_struct` is one of the most important kernel structures to understand for Linux interviews.

Conceptually:

```text
task_struct
│
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

The actual structure is large and changes between kernel versions.

For interviews, understand what it represents rather than memorizing every field.

---

# 4. Important Information Associated with task_struct

## Process Identity

```text
PID
TID
TGID
Parent
Children
```

## Scheduling

```text
Priority
Scheduling policy
CPU affinity
Runtime information
```

## Memory

Associated with:

```text
mm_struct
```

which describes the process address space.

## Files

Associated with:

```text
files_struct
```

which tracks open file descriptors.

## Signals

Contains or references information related to:

```text
Signals
Signal handlers
Signal state
```

---

# 5. Process ID – PID

Every process has a Process ID:

```text
PID
```

Example:

```bash
ps
```

Possible output:

```text
PID     CMD
1000    server
1001    worker
1002    shell
```

PID identifies a process within its PID namespace.

You can obtain the current process ID using:

```c
getpid();
```

---

# 6. Parent Process

Processes normally have a parent.

Example:

```text
PID 1
 |
 +-- shell
      |
      +-- gcc
      |
      +-- server
           |
           +-- worker
```

The parent PID can be obtained using:

```c
getppid();
```

The process hierarchy is important for:

* Process creation
* Process termination
* Child management
* Reaping
* Signals

---

# 7. PID 1

The first userspace process is traditionally:

```text
PID 1
```

On many modern Linux systems it is:

```text
systemd
```

Conceptually:

```text
Kernel
   |
   v
PID 1
   |
   +-- Services
   +-- Daemons
   +-- Other Processes
```

PID 1 has special responsibilities related to:

* System initialization
* Service management
* Reaping orphaned processes

---

# 8. Process Hierarchy

Processes form a parent-child hierarchy.

Example:

```text
PID 1
 |
 +-- bash
      |
      +-- gcc
      |
      +-- application
             |
             +-- worker
             +-- worker
```

When a process creates another process:

```text
Parent
   |
   +---- Child
```

The kernel tracks these relationships.

---

# 9. Process States

A process can move through several states during its lifetime.

Simplified model:

```text
             +-----------+
             | RUNNING   |
             +-----------+
                /     \
               /       \
              v         v
        WAITING       STOPPED
           |
           v
        RUNNABLE
```

A more useful mental model is:

```text
RUNNABLE
   |
   | scheduler selects task
   v
RUNNING
   |
   | waits for I/O/event
   v
WAITING
   |
   | event occurs
   v
RUNNABLE
```

---

# 10. Running vs Runnable

These terms are often confused.

## Running

The task is currently executing on a CPU.

```text
CPU
 |
 +---- Process A
```

## Runnable

The task is ready to execute but is waiting for CPU time.

```text
Run Queue

Process A
Process B
Process C

      |
      v
     CPU
```

The scheduler selects a runnable task.

---

# 11. Waiting

A process may need to wait for an event.

Example:

```c
read(fd, buffer, size);
```

If data is not currently available, the task may sleep.

Conceptually:

```text
Running
   |
   v
Waiting for I/O
   |
   v
Blocked
   |
   | I/O completes
   v
Runnable
```

This allows another task to use the CPU.

---

# 12. Why Sleeping Is Important

Consider:

```c
while (!data_available)
{
    // continuously check
}
```

This wastes CPU.

Instead, the process can sleep:

```text
Process
   |
   v
Sleep
   |
   | Event occurs
   v
Wake up
   |
   v
Runnable
```

This is fundamental to efficient operating-system design.

---

# 13. Process Creation

Linux provides several mechanisms for creating tasks.

Important APIs include:

```text
fork()
vfork()
clone()
```

The traditional process-creation pattern is:

```text
fork()
   |
   v
Child
   |
   v
execve()
   |
   v
New Program
```

---

# 14. fork()

`fork()` creates a child process.

Example:

```c
#include <stdio.h>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        printf("Child\n");
    }
    else if (pid > 0)
    {
        printf("Parent\n");
    }

    return 0;
}
```

After `fork()`:

```text
Parent
   |
   +------ Child
```

The child starts as a logical copy of the parent's process state.

---

# 15. fork() Return Value

This is a common interview question.

```c
pid_t pid = fork();
```

### Parent

```text
pid > 0
```

The return value is the child's PID.

### Child

```text
pid == 0
```

### Failure

```text
pid < 0
```

Therefore:

```text
Parent:
fork() -> Child PID

Child:
fork() -> 0

Failure:
fork() -> -1
```

---

# 16. Does fork() Copy All Memory?

Logically, the child receives a copy of the parent's address space.

But Linux does **not** immediately copy every physical memory page.

Instead, Linux uses:

```text
Copy-on-Write
```

This makes `fork()` much more efficient.

---

# 17. Copy-on-Write

Suppose the parent has:

```text
Parent
Virtual Address
       |
       v
Physical Page A
```

After `fork()`:

```text
Parent Virtual Address
       |
       +----------------+
                        |
                        v
                   Physical Page A
                        ^
                        |
       +----------------+
       |
Child Virtual Address
```

Parent and child can initially share the same physical page.

The kernel marks the relevant pages so that a write triggers special handling.

---

# 18. Child Writes to a Shared Page

Suppose the child modifies a shared page:

```c
x = 100;
```

The CPU detects that the page cannot currently be written.

This causes a page fault.

Simplified flow:

```text
Child writes
     |
     v
Page Fault
     |
     v
Kernel
     |
     v
Allocate new physical page
     |
     v
Copy original page
     |
     v
Update child's page table
     |
     v
Retry instruction
```

Now:

```text
Parent ---> Page A

Child ----> Page B
```

The parent and child have separate physical pages only after modification.

---

# 19. Why Copy-on-Write?

Without COW:

```text
fork()

Process = 1 GB

        |
        v
Copy 1 GB immediately
```

This would be expensive.

With COW:

```text
fork()

Share pages
    |
    v
Copy only modified pages
```

Benefits:

* Faster process creation
* Lower memory consumption
* Especially efficient when `fork()` is immediately followed by `exec()`

---

# 20. fork() + exec()

A common Linux pattern is:

```text
fork()
  |
  v
Child
  |
  v
execve()
  |
  v
New Program
```

This is commonly used by shells.

For example:

```bash
ls
```

A simplified flow is:

```text
Shell
 |
 | fork()
 v
Child
 |
 | execve("ls", ...)
 v
ls
```

The parent shell may wait for the child.

---

# 21. What Does execve() Do?

`execve()` does **not** create a new process.

It replaces the current process's program image.

Before:

```text
PID 1000

+------------------+
| Shell Code       |
| Shell Data       |
| Shell Heap       |
| Shell Stack      |
+------------------+
```

After:

```text
PID 1000

+------------------+
| ls Code          |
| ls Data          |
| ls Heap          |
| ls Stack         |
+------------------+
```

The PID remains:

```text
1000
```

The executing program changes.

---

# 22. fork() vs exec()

| Feature                | fork()       | execve()                  |
| ---------------------- | ------------ | ------------------------- |
| Creates child          | Yes          | No                        |
| Creates new PID        | Yes          | No                        |
| Replaces program image | No           | Yes                       |
| Uses COW               | Yes          | N/A as creation mechanism |
| Typical use            | Create child | Run another program       |

Remember:

```text
fork()  -> create child

exec()  -> replace program image
```

---

# 23. vfork()

`vfork()` is a specialized process-creation mechanism.

Its historical purpose was to make the common:

```text
fork()
+
exec()
```

pattern more efficient by avoiding some address-space setup.

Conceptually:

```text
Parent
   |
 vfork()
   |
   v
Child
   |
 exec()
   |
   v
New Program
```

`vfork()` has restrictive semantics because the child temporarily shares the parent's address space.

For normal application development, `fork()` + COW is generally the important concept to understand.

---

# 24. clone()

Linux provides:

```c
clone()
```

which allows selective resource sharing.

Conceptually:

```text
clone()
 |
 +-- Share address space
 +-- Share file table
 +-- Share filesystem information
 +-- Share signal handlers
 +-- Share other resources
```

This flexibility is important for:

* Threads
* Containers
* Namespaces

---

# 25. Process vs Thread

A process provides an execution environment.

A thread is an execution context within that environment.

Example:

```text
Process
 |
 +-- Thread 1
 +-- Thread 2
 +-- Thread 3
```

Threads in the same process typically share:

```text
Code
Heap
Global Data
Address Space
Open File Descriptions
```

Each thread has its own:

```text
Registers
Stack
Instruction Pointer
Scheduling state
Thread ID
```

---

# 26. Process vs Thread

| Resource          | Process                | Thread         |
| ----------------- | ---------------------- | -------------- |
| Address Space     | Separate               | Shared         |
| Code              | Separate address space | Shared         |
| Heap              | Separate               | Shared         |
| Stack             | Own                    | Own            |
| Registers         | Own                    | Own            |
| Open Files        | Own descriptor table   | Usually shared |
| Isolation         | Stronger               | Weaker         |
| Creation overhead | Higher                 | Lower          |

The exact sharing behavior depends on how the task was created.

---

# 27. Linux Threads

Linux uses the same fundamental task representation for processes and threads:

```c
struct task_struct
```

A thread created using:

```c
pthread_create()
```

ultimately uses Linux task-creation mechanisms.

Conceptually:

```text
pthread_create()
       |
       v
clone()/clone3()
       |
       v
New task
       |
       v
task_struct
```

The sharing flags determine which resources are shared.

---

# 28. PID, TID and TGID

For a multithreaded process:

```text
Process
 |
 +-- Thread 1
 +-- Thread 2
 +-- Thread 3
```

Linux distinguishes:

```text
PID
TID
TGID
```

Conceptually:

* **TID** identifies an individual thread.
* **TGID** identifies the thread group.
* User-space commonly refers to the thread group's ID as the process ID.

For the main thread:

```text
TID == TGID
```

---

# 29. Context Switching

A CPU can execute only a limited number of tasks simultaneously.

The scheduler switches between tasks.

This is called a:

```text
Context Switch
```

Example:

```text
CPU
 |
 +--> Task A
 |
 | context switch
 v
Task B
 |
 | context switch
 v
Task C
```

---

# 30. What Happens During a Context Switch?

Conceptually:

```text
Task A running
      |
      v
Save Task A execution state
      |
      v
Scheduler chooses Task B
      |
      v
Restore Task B execution state
      |
      v
Task B runs
```

Execution state includes things such as:

* CPU registers
* Stack pointer
* Instruction pointer
* Architecture-specific CPU state

The exact implementation is architecture-specific.

---

# 31. Why Context Switching Has a Cost

Context switching is not free.

Potential costs include:

* Saving/restoring CPU state
* Scheduler overhead
* Cache disruption
* TLB-related effects
* Pipeline disruption
* Loss of useful CPU-local state

Therefore:

```text
Too many context switches
        |
        v
Potential performance degradation
```

This is particularly important in:

* High-performance networking
* Storage
* Embedded systems
* Low-latency systems
* CPU-intensive applications

---

# 32. Kernel Stack

Each task has kernel-stack state used while executing kernel code on behalf of that task.

Simplified:

```text
User Space
    |
    | system call
    v
Kernel Entry
    |
    v
Kernel Stack
    |
    v
Kernel Functions
```

Example:

```text
Application
    |
    | read()
    v
System Call Entry
    |
    v
VFS
    |
    v
Filesystem
    |
    v
Block Layer
```

The kernel does not normally execute kernel code using the application's user stack.

---

# 33. User Stack vs Kernel Stack

```text
Process
 |
 +----------------------+
 | User Virtual Space   |
 |                      |
 | Stack                |
 +----------------------+
          |
          | syscall
          v
 +----------------------+
 | Kernel Space         |
 |                      |
 | Kernel Stack         |
 +----------------------+
```

This separation is important for:

* Security
* Fault isolation
* Kernel execution

---

# 34. Process Address Space

A typical 64-bit Linux process has a virtual address space containing regions such as:

```text
High Address
+----------------------+
| Kernel Mapping       |
+----------------------+
| User Stack           |
|        ↓             |
|                      |
| mmap() Regions       |
|                      |
|        ↑             |
| Heap                 |
+----------------------+
| BSS                  |
| Data                 |
| Read-only Data       |
| Text                 |
+----------------------+
Low Address
```

The exact layout depends on:

* CPU architecture
* Kernel configuration
* ASLR
* Process configuration
* Kernel version

---

# 35. mm_struct

Linux maintains address-space information using:

```c
struct mm_struct
```

Conceptually:

```text
task_struct
      |
      v
  mm_struct
      |
      +-- Page tables
      +-- Virtual memory information
      +-- Memory mappings
      +-- VMAs
```

It describes the process's user-space memory environment.

---

# 36. Virtual Memory Areas

Linux represents contiguous virtual-memory regions using:

```c
struct vm_area_struct
```

Examples include:

```text
Code
Data
Heap
Stack
Shared Libraries
mmap() regions
```

Conceptually:

```text
Process Address Space
|
+-- VMA: Code
+-- VMA: Read-only Data
+-- VMA: Data/BSS
+-- VMA: Heap
+-- VMA: Shared Library
+-- VMA: mmap Region
+-- VMA: Stack
```

---

# 37. Process Termination

A process can terminate through:

```c
exit()
```

or:

```c
_exit()
```

It can also terminate because of:

* Fatal signals
* Exceptions
* Kernel-enforced termination

Simplified:

```text
Running Process
      |
      v
     exit()
      |
      v
Termination
      |
      v
Zombie
      |
      | parent waits
      v
Reaped
```

---

# 38. Zombie Process

A zombie is a process that has terminated but whose parent has not yet collected its termination status.

Important:

```text
Zombie is NOT running.
```

It retains limited kernel bookkeeping information.

Example:

```text
Parent
 |
 +-- Child
       |
       +-- exit()
       |
       v
     Zombie
```

When the parent calls:

```c
wait()
```

or:

```c
waitpid()
```

the child's exit information is collected.

---

# 39. Why Do Zombies Exist?

Suppose the child exits with:

```text
exit status = 10
```

The parent may need this information.

The kernel therefore retains the necessary termination information until the parent collects it.

Flow:

```text
Child
 |
 | exit(10)
 v
Zombie
 |
 | waitpid()
 v
Parent receives status
 |
 v
Child entry fully reaped
```

---

# 40. Orphan Process

An orphan is a process whose original parent has terminated while the child is still alive.

Example:

```text
Parent
 |
 +-- Child
```

Parent exits:

```text
Parent
   X

Child
```

The child is reparented to an appropriate process/reaper.

In a simple traditional model, this is often described as PID 1.

Modern Linux also has additional mechanisms for selecting subreapers.

---

# 41. Zombie vs Orphan

| Feature                | Zombie      | Orphan                 |
| ---------------------- | ----------- | ---------------------- |
| Still executing?       | No          | Usually yes            |
| Original parent alive? | Usually yes | No                     |
| Exit status retained?  | Yes         | Not yet, while running |
| Main issue             | Not reaped  | Parent disappeared     |

Remember:

```text
Zombie != Orphan
```

---

# 42. wait() and waitpid()

A parent can collect child termination information using:

```c
wait()
```

or:

```c
waitpid()
```

Example:

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

Flow:

```text
Parent
 |
 +---- Child
 |       |
 |       +-- exit(10)
 |
 +---- waitpid()
          |
          v
     Collect status
```

---

# 43. Signals

Signals are asynchronous notifications delivered to a process or thread.

Examples:

```text
SIGINT
SIGTERM
SIGKILL
SIGSEGV
SIGSTOP
SIGCHLD
```

Example:

```bash
kill -TERM <pid>
```

The kernel sends `SIGTERM` to the target.

---

# 44. SIGTERM vs SIGKILL

This is a common interview question.

## SIGTERM

```text
SIGTERM
```

Requests graceful termination.

An application can catch it and perform cleanup.

Example:

```text
SIGTERM
   |
   v
Application
   |
   +-- Stop accepting work
   +-- Flush data
   +-- Close resources
   +-- Exit
```

## SIGKILL

```text
SIGKILL
```

Forces termination.

The process cannot catch or ignore `SIGKILL`.

Therefore:

```text
SIGTERM -> Request graceful termination

SIGKILL -> Forced termination
```

---

# 45. SIGCHLD

When a child changes state, the parent can receive:

```text
SIGCHLD
```

This is particularly relevant when managing child processes.

Typical model:

```text
Child
 |
 | exit
 v
SIGCHLD
 |
 v
Parent
 |
 v
wait()/waitpid()
```

---

# 46. Process Creation – Complete Flow

Simplified `fork()` flow:

```text
Parent Process
      |
      v
    fork()
      |
      v
Kernel
      |
      +-- Create task state
      |
      +-- Assign PID
      |
      +-- Establish parent/child relationship
      |
      +-- Set up scheduling state
      |
      +-- Set up memory relationships
      |
      +-- Set up resource references
      |
      v
Child becomes runnable
```

Memory is handled efficiently using Copy-on-Write.

---

# 47. fork() + exec() – Complete Flow

Typical shell execution:

```text
Shell
 |
 | fork()
 v
Child
 |
 | execve()
 v
Load executable
 |
 +-- Replace process image
 +-- Set up memory mappings
 +-- Load executable segments
 +-- Load dynamic linker/shared libraries
 +-- Set program entry point
 |
 v
New Program
```

Parent:

```text
Shell
 |
 | waitpid()
 v
Child exits
 |
 v
Shell continues
```

---

# 48. Context Switch – Simplified Flow

```text
Task A running
      |
      v
Scheduler event
      |
      v
Save Task A CPU state
      |
      v
Scheduler
      |
      v
Select Task B
      |
      v
Restore Task B CPU state
      |
      v
Task B runs
```

The real implementation depends on:

* CPU architecture
* Kernel version
* Scheduler implementation
* Kernel configuration

---

# 49. CPU Affinity

A process/thread can be restricted to specific CPUs.

Conceptually:

```text
Process A
   |
   +-- CPU 2
   +-- CPU 3
```

Linux provides APIs/tools for CPU affinity.

Example command:

```bash
taskset -c 2 ./application
```

CPU affinity can be important for:

* Performance tuning
* Cache locality
* Real-time workloads
* Networking
* NUMA systems

---

# 50. Namespaces

Linux namespaces provide isolation of system resources.

Important namespaces include:

```text
PID
Mount
Network
UTS
IPC
User
Cgroup
Time
```

Namespaces are fundamental to containers.

---

# 51. PID Namespace

PID namespaces allow processes to have different PID views.

Host:

```text
Host PID Namespace

PID 5000
   |
   +-- Container Process
```

Inside the container:

```text
Container PID Namespace

PID 1
```

The same underlying task can therefore have different PID values from different namespace perspectives.

---

# 52. Containers and Processes

A container is not a virtual machine.

A container primarily uses Linux kernel mechanisms such as:

```text
Namespaces
+
cgroups
+
Filesystem isolation
+
Capabilities
```

Conceptually:

```text
Container A
    |
Container B
    |
Container C
    |
    v
Linux Kernel
    |
    v
Hardware
```

All containers normally share the same host kernel.

---

# 53. Process vs Virtual Machine

| Feature                 | Container         | VM                      |
| ----------------------- | ----------------- | ----------------------- |
| Kernel                  | Shared            | Separate guest kernel   |
| Startup                 | Fast              | Slower                  |
| Isolation               | Kernel mechanisms | Hardware virtualization |
| Memory overhead         | Lower             | Higher                  |
| Hardware virtualization | Not required      | Usually used            |

---

# 54. Important Mental Model

Keep this relationship in mind:

```text
                    Process / Task
                          |
                          v
                    task_struct
                          |
          +---------------+---------------+
          |               |               |
          v               v               v
     Scheduling        Memory           Files
                          |               |
                          v               v
                     mm_struct       files_struct
                          |
                          v
                  Virtual Address Space
                          |
                          v
                     Page Tables
                          |
                          v
                     Physical RAM
```

---

# 55. Process Execution Mental Model

```text
Program
   |
   v
Process
   |
   v
task_struct
   |
   v
Scheduler
   |
   v
CPU
```

When the process needs the kernel:

```text
User Code
   |
   | system call / interrupt
   v
Kernel
   |
   v
Kernel Code
   |
   v
Hardware / Kernel Subsystem
```

---

# 56. Senior Interview Flow: fork()

If asked:

> What happens when fork() is called?

A strong answer:

```text
User process
     |
     v
fork()
     |
     v
System call / kernel process creation
     |
     +-- Create child task state
     +-- Assign PID/TID information
     +-- Establish parent-child relationship
     +-- Set scheduling state
     +-- Duplicate/share required resources
     +-- Set up address-space relationship
     +-- Use Copy-on-Write for memory
     |
     v
Child becomes runnable
```

Do not say:

> "Linux copies the entire process memory."

That is an oversimplification.

Say:

> "The child gets a logically duplicated address space, while physical pages are initially shared using Copy-on-Write."

---

# 57. Senior Interview Flow: execve()

If asked:

> What happens when execve() is called?

Answer conceptually:

```text
Process
   |
   v
execve()
   |
   v
Kernel
   |
   +-- Validate executable
   +-- Prepare new address space
   +-- Load executable segments
   +-- Set up stack/arguments/environment
   +-- Load dynamic linker when required
   +-- Set instruction pointer to entry point
   |
   v
New Program Starts
```

The PID remains the same.

---

# 58. Senior Interview Flow: Context Switch

If asked:

> What happens during a context switch?

Answer:

```text
Current task
     |
     v
Save architecture-specific CPU state
     |
     v
Scheduler selects next task
     |
     v
Switch task/kernel state
     |
     v
Restore next task's CPU state
     |
     v
Resume execution
```

Also mention:

```text
Context switches can affect cache locality and performance.
```

---

# 59. Senior Interview Flow: Zombie

If asked:

> Why does a zombie process exist?

Answer:

```text
Child terminates
      |
      v
Kernel records exit information
      |
      v
Child becomes zombie
      |
      v
Parent calls wait()/waitpid()
      |
      v
Exit status collected
      |
      v
Child is reaped
```

The zombie is not consuming CPU.

---

# 60. Important Interview Questions

### Q1. What is a process?

A process is an executing instance of a program together with its virtual address space, execution state, resources, and kernel bookkeeping.

---

### Q2. What is task_struct?

`task_struct` is the primary kernel data structure representing a schedulable task.

It contains or references information about:

* Identity
* Scheduling
* Memory
* Files
* Signals
* Credentials
* Parent/child relationships

---

### Q3. Does fork() immediately copy all physical memory?

No.

Linux uses Copy-on-Write so parent and child can initially share physical pages.

---

### Q4. Does execve() create a new process?

No.

It replaces the current process's program image.

The PID remains unchanged.

---

### Q5. Why is fork() + exec() commonly used?

It separates:

```text
Process creation
```

from:

```text
Program loading
```

This allows a shell or supervisor to create a child and then make that child execute another program.

---

### Q6. What is the difference between a process and a thread?

Processes normally have separate address spaces, while threads within the same process share the address space and many resources but have independent execution state and stacks.

---

### Q7. What is Copy-on-Write?

COW allows parent and child to initially share physical pages after `fork()`. A page is copied only when a process needs to modify it.

---

### Q8. What is a zombie?

A terminated child that has not yet been reaped by its parent.

---

### Q9. What is an orphan?

A process whose original parent has terminated while the child is still alive.

---

### Q10. What is a context switch?

The process of switching CPU execution from one task to another by saving the current execution state and restoring another task's state.

---

### Q11. Why are threads cheaper than processes?

Threads can share the same address space and many resources, reducing creation and management overhead.

---

### Q12. What are PID, TID and TGID?

* PID/TGID identify a thread group from the user-visible process perspective.
* TID identifies an individual thread.
* The main thread's TID normally equals the thread group's ID.

---

# 61. Questions You Should Be Able to Explain on a Whiteboard

Before moving to the next chapter, you should be able to draw and explain:

```text
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
```

---

# 62. Interview Priority

## Must Know

```text
task_struct
Process states
fork()
execve()
Copy-on-Write
Process vs Thread
Context switching
Zombie
Orphan
wait()
waitpid()
PID/TID/TGID
Kernel stack
mm_struct
Virtual address space
```

## Should Know

```text
clone()
vfork()
Signals
SIGTERM
SIGKILL
SIGCHLD
CPU affinity
Namespaces
VMAs
```

## Deep Dive When Role Requires It

```text
Scheduler run queues
CFS internals
Real-time scheduling
CPU affinity internals
NUMA scheduling
PID allocation internals
clone3()
cgroups
Subreapers
```

---

# 63. Quick Revision Sheet

```text
PROCESS
   |
   +-- task_struct
   |
   +-- Address Space
   |
   +-- Open Files
   |
   +-- Signals
   |
   +-- Scheduling State
```

Process creation:

```text
fork()
   |
   v
Child
   |
   v
Copy-on-Write
```

Program replacement:

```text
execve()
   |
   v
New Program Image
```

Threads:

```text
Process
 |
 +-- Thread
 +-- Thread
 +-- Thread
```

Termination:

```text
exit()
   |
   v
Zombie
   |
   | wait()
   v
Reaped
```

Scheduling:

```text
Runnable
   |
   v
Scheduler
   |
   v
Running
   |
   v
Waiting
   |
   v
Runnable
```

Containers:

```text
Namespaces + cgroups
        |
        v
    Containers
```

---

# 64. Chapter Summary

The key mental model is:

```text
                 Linux Task
                     |
                     v
                task_struct
                     |
        +------------+------------+
        |            |            |
        v            v            v
   Scheduling      Memory       Files
                      |
                      v
                 mm_struct
                      |
                      v
              Virtual Memory
                      |
                      v
                 Page Tables
                      |
                      v
                 Physical RAM
```

Process creation:

```text
fork()
  |
  v
Child task
  |
  v
COW memory
```

Program execution:

```text
fork()
  |
  v
Child
  |
  v
execve()
  |
  v
New Program
```

Process termination:

```text
exit()
  |
  v
Zombie
  |
  v
wait()/waitpid()
  |
  v
Reaped
```

The most important senior-level concept is to understand the **execution flow**, not merely memorize API definitions.

---

# Next Chapter

## Chapter 3 – Linux Memory Management

Topics:

```text
Virtual Memory
        |
        v
Virtual Address
        |
        v
Page Tables
        |
        v
TLB
        |
        v
Physical Memory

Page Faults
Demand Paging
Copy-on-Write
mmap()
VMA
Buddy Allocator
SLAB / SLUB
Page Cache
Huge Pages
NUMA
Memory Reclaim
```

Memory management is one of the highest-value Linux-internals topics for senior C/C++ systems, embedded, CPU, networking, storage, and accelerator roles.
