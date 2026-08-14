# Chapter 1 — Operating System Fundamentals

## 1. What is an Operating System?

An **Operating System (OS)** is system software that acts as an intermediary between applications and computer hardware.

It has three major roles:

1. **Resource manager**
   - CPU
   - Memory
   - Storage
   - Network
   - Devices

2. **Abstraction provider**
   - Process
   - Thread
   - Virtual memory
   - File
   - Socket

3. **Protection and isolation provider**
   - Prevents one application from directly interfering with another.
   - Controls access to privileged hardware and kernel resources.

Basic relationship:

    +--------------------------------------------------+
    |                  Applications                    |
    |                                                  |
    |  Browser   Database   Shell   Editor   Services  |
    +-------------------------+------------------------+
                              |
                              | APIs / System Calls
                              v
    +--------------------------------------------------+
    |                     OS / Kernel                  |
    |                                                  |
    | Process Management                               |
    | Memory Management                                |
    | Scheduling                                       |
    | Filesystems / VFS                                |
    | Networking                                       |
    | IPC                                              |
    | Device Drivers                                   |
    | Security                                         |
    +-------------------------+------------------------+
                              |
                              v
    +--------------------------------------------------+
    |                    Hardware                      |
    |                                                  |
    | CPU | RAM | Disk | NIC | USB | GPU | Devices   |
    +--------------------------------------------------+

### Interview point

The OS is not simply "software that runs programs."

It provides **controlled access to hardware and manages shared resources among competing applications**.

---

# 2. Why Do We Need an Operating System?

Without an OS, applications would have to understand and directly manage hardware such as:

- CPU
- RAM
- Disk controller
- Network card
- Keyboard
- Display
- USB devices

This would create several problems:

- No resource sharing
- No process isolation
- No standard file interface
- No memory protection
- Difficult hardware portability
- Poor security
- Difficult device management

For example, if two applications want to use a disk:

    Program A ----+
                  |
                  +---- Disk
                  |
    Program B ----+

The OS coordinates access:

    Program A
        |
        v
    +-----------+
    |    OS     |
    +-----------+
        |
        v
       Disk

The OS therefore acts as both a **resource manager** and an **abstraction layer**.

---

# 3. Major Responsibilities of an OS

## 3.1 Process Management

The OS:

- Creates processes
- Terminates processes
- Schedules processes
- Maintains process state
- Performs task switching
- Provides process synchronization
- Provides IPC

Linux examples:

```c
fork();
execve();
waitpid();
_exit();
```

Processes are covered in detail in Chapter 2.

---

## 3.2 Memory Management

The OS manages:

- Physical RAM
- Virtual address spaces
- Page tables
- Page allocation
- Memory protection
- Shared memory
- Copy-On-Write
- Memory mapping
- Swapping/reclaim mechanisms

Basic relationship:

    Process
       |
       v
    Virtual Address
       |
       v
    MMU + Page Tables
       |
       v
    Physical Memory

Virtual memory is covered in detail in Chapters 7 and 8.

---

## 3.3 File-System Management

The OS provides a common abstraction for persistent data.

Instead of applications knowing how every storage device works:

    Application
         |
         v
    open()
    read()
    write()
    close()
         |
         v
    OS / VFS
         |
         +---- ext4
         +---- XFS
         +---- NFS
         +---- tmpfs

The application sees a common file interface.

Linux's VFS allows different filesystem implementations to expose a common interface.

---

## 3.4 I/O Management

The OS manages devices such as:

- Disk
- Network card
- Keyboard
- Mouse
- USB
- GPU
- Serial port

Typical path:

    Application
        |
        v
    System Call
        |
        v
    Kernel
        |
        v
    Device Driver
        |
        v
    Hardware

---

## 3.5 Networking

The OS provides networking abstractions such as sockets.

Example:

```c
int fd = socket(AF_INET, SOCK_STREAM, 0);
```

Conceptually:

    Application
         |
         v
    Socket API
         |
         v
    TCP/IP stack
         |
         v
    Network driver
         |
         v
    NIC

The application normally does not directly program the network controller.

---

## 3.6 Security and Protection

The OS controls:

- Which process can access memory
- Which user can access a file
- Which process can access a resource
- Which operations require privilege

Linux mechanisms include:

    UID/GID
    Capabilities
    SELinux
    AppArmor
    Namespaces
    cgroups

---

# 4. Kernel vs Operating System

These terms are related but not identical.

## Kernel

The **kernel is the privileged core of the operating system**.

It manages core resources and interacts directly with hardware.

    Operating System
    |
    +-- Kernel
    |   |
    |   +-- Process management
    |   +-- Memory management
    |   +-- Scheduler
    |   +-- VFS
    |   +-- Networking
    |   +-- Drivers
    |   +-- IPC
    |
    +-- User-space software
        |
        +-- Shell
        +-- Libraries
        +-- Utilities
        +-- Services
        +-- Applications

### Interview answer

> The kernel is the privileged core of the OS. It manages CPU, memory, devices, filesystems, networking and other system resources. The complete operating-system environment also contains user-space components.

---

# 5. User Space and Kernel Space

Modern operating systems separate application execution from kernel execution.

Conceptually:

    +--------------------------------+
    |          User Space            |
    |                                |
    | Applications                   |
    | Libraries                      |
    | Shell                          |
    | Services                       |
    +--------------------------------+
    |          Kernel Space          |
    |                                |
    | Scheduler                      |
    | Memory Manager                 |
    | VFS                            |
    | Networking                     |
    | Drivers                        |
    +--------------------------------+
    |           Hardware             |
    +--------------------------------+

## Why?

Applications should not have unrestricted access to:

- Kernel memory
- Page tables
- Device registers
- Interrupt configuration
- Other processes' memory

If any application could freely modify these resources, one buggy or malicious program could potentially crash or compromise the whole system.

---

# 6. User Mode and Kernel Mode

This is primarily a **CPU privilege concept**.

Applications normally execute in:

    User mode

The kernel executes privileged operations in:

    Kernel mode

The CPU architecture provides mechanisms to distinguish privilege levels.

### Important distinction

**User space/kernel space** describes protected address-space regions.

**User mode/kernel mode** describes the privilege level at which code is executing.

They are closely related, but they are not the same terminology.

---

# 7. Privileged Instructions

Some CPU operations are restricted to privileged execution.

Examples include operations related to:

- Interrupt control
- Memory-management configuration
- Hardware control
- CPU/system configuration

If user code attempts an operation that requires privilege, the CPU prevents it and raises an appropriate fault/exception.

This protects the system from arbitrary application access to critical resources.

---

# 8. System Calls

A **system call** is the controlled interface through which user-space software requests a service from the kernel.

Example:

```c
write(fd, buffer, size);
```

Conceptually:

    Application
         |
         | write()
         v
    C Library / wrapper
         |
         v
    System-call mechanism
         |
         v
    Kernel
         |
         v
    VFS / Driver / Device

The important distinction is:

    Library API
         ↓
    System-call interface
         ↓
    Kernel implementation

Not every library function is a system call.

---

# 9. Library Function vs System Call

Consider:

```c
printf("Hello\n");
```

`printf()` is a **C library function**, not itself a Linux system call.

It may eventually result in a write operation:

    printf()
       |
       v
    C library
       |
       v
    write()
       |
       v
    Kernel
       |
       v
    stdout

However, stdio buffering means one `printf()` does not necessarily result in one immediate `write()` system call.

Similarly, `malloc()` is a library allocator. It may obtain or manage memory using system calls such as `brk()` or `mmap()`, but `malloc()` itself is not a system call.

---

# 10. What Happens During a System Call?

A simplified flow:

                    USER MODE
                        |
                 Application
                        |
                        v
                 libc wrapper
                        |
                        v
              System-call instruction
                        |
           =========================
              Controlled entry
           =========================
                        |
                        v
                    KERNEL MODE
                        |
                        v
              System call handling
                        |
                        v
               Kernel subsystem
                        |
                        v
                  Return value
                        |
           =========================
               Return to user
           =========================
                        |
                        v
                    USER MODE

On x86-64 Linux, the `syscall` instruction is used for the normal system-call entry path.

The actual entry path contains architecture-specific and kernel-specific details such as register conventions, entry code, validation, tracing/security hooks, and dispatch.

---

# 11. System Call Numbers

Linux identifies system calls using system-call numbers.

Conceptually:

    System call name
           |
           v
    System call number
           |
           v
    System call dispatch
           |
           v
    Kernel implementation

The exact numbering and implementation details depend on architecture and kernel version.

For interview purposes, remember:

    User program
        |
        v
    System-call interface
        |
        v
    Kernel dispatch
        |
        v
    Specific kernel operation

---

# 12. System Call Does NOT Necessarily Mean Context Switch

This is a very important interview point.

A system call causes a controlled transition:

    User Mode
        |
        v
    Kernel Mode

The kernel may finish the operation and return to the same task:

    User
      ↓
    Kernel
      ↓
    Same task
      ↓
    User

A context switch is different:

    Task A
       |
       v
    Task B

Therefore:

    System call != Context switch

A system call can indirectly lead to a context switch if the current task blocks or the scheduler decides another task should run.

---

# 13. System Call vs Function Call

### Normal function call

    User code
       |
       v
    Function
       |
       v
    User code

The execution normally stays in user mode.

### System call

    User code
       |
       v
    System-call entry
       |
       v
    Kernel
       |
       v
    Return to user mode

Therefore a system call has additional privilege-transition and kernel-processing overhead.

---

# 14. Interrupts

An interrupt allows an external event to get the CPU's attention.

Example:

    Network card receives packet
                 |
                 v
           Hardware event
                 |
                 v
              Interrupt
                 |
                 v
                CPU
                 |
                 v
           Kernel handling

Other examples:

- Timer interrupt
- Keyboard/device interrupt
- Storage completion
- Network device interrupt

---

# 15. Why Do We Need Interrupts?

Suppose a network card receives packets.

Without interrupts, the CPU could continuously ask:

    Is data available?
    Is data available?
    Is data available?
    Is data available?
    ...

This is polling.

With interrupts:

    CPU
     |
     | Do useful work
     |
     +----------------------+
                            |
                            v
                      NIC receives data
                            |
                            v
                         Interrupt
                            |
                            v
                           CPU

The CPU can perform other work until the device needs attention.

### Important

Interrupt-driven I/O and polling are not mutually exclusive in every system. High-performance networking may deliberately use polling or hybrid mechanisms.

---

# 16. Hardware Interrupt Flow

Simplified:

    Device
      |
      | Interrupt request
      v
    Interrupt controller
      |
      v
    CPU
      |
      v
    Interrupt entry
      |
      v
    Kernel interrupt handling
      |
      v
    Device-specific handling
      |
      v
    Return to previous execution

Modern systems have additional details such as interrupt controllers, interrupt affinity, top-half/deferred processing, and architecture-specific entry paths.

These will be covered later in Linux kernel internals.

---

# 17. Exceptions

An **exception** is a synchronous event associated with instruction execution.

Examples include:

- Divide by zero
- Invalid instruction
- Page fault
- Protection violation

The key property is that the exception is associated with the execution of a particular instruction.

---

# 18. Example: Divide by Zero

Consider:

```c
int x = 10;
int y = 0;

int z = x / y;
```

The CPU detects the invalid operation.

Conceptually:

    Instruction
        |
        v
    CPU detects exception
        |
        v
    Exception handling
        |
        v
    Kernel / process handling

The exact user-visible behavior depends on the architecture and OS. On Linux, an invalid arithmetic operation such as integer division by zero normally results in a signal such as `SIGFPE` being delivered to the process.

---

# 19. Page Fault

A page fault is a CPU-generated memory-management exception.

For example:

```c
int *p = ...;

*p = 10;
```

If the virtual address cannot currently be translated or the access violates the current page permissions, the CPU can raise a page fault.

The kernel then determines whether the fault is:

1. Recoverable
   - Page needs to be brought into memory
   - Copy-On-Write needs to occur
   - A valid mapping needs to be established

2. Invalid
   - Access violates memory protection
   - Address is not valid

This is covered in detail in Chapter 8.

---

# 20. Trap

A **trap** is a synchronous mechanism associated with software execution.

Historically, software interrupt/trap mechanisms were used for system-call entry.

Modern x86-64 Linux normally uses:

    syscall

for the normal system-call entry path.

Therefore, avoid the outdated statement:

> "Every Linux system call is a software interrupt."

The exact mechanism is architecture-dependent.

---

# 21. Interrupt vs Exception vs Trap vs System Call

| Concept | Typical source | Synchronous? | Purpose |
|---|---|---:|---|
| Hardware interrupt | Device/timer | Usually no | Notify CPU of external event |
| Exception | CPU detects condition | Yes | Handle instruction-related condition |
| Trap | Intentional synchronous event | Yes | Controlled software event |
| System call | Program requests kernel service | Yes | Enter kernel to request service |

Important:

A system call is an OS interface. Its low-level CPU entry mechanism is architecture-dependent.

---

# 22. Protection

The OS must protect resources from incorrect or malicious access.

For example:

    Process A
        |
        X ---> Process B's memory

Instead:

    Process A
        |
        v
    Own virtual address space

Similarly:

    Application
        |
        X ---> Direct hardware access

Instead:

    Application
        |
        v
    System Call
        |
        v
    Kernel
        |
        v
    Driver
        |
        v
    Hardware

---

# 23. Process Isolation

Suppose two processes both use the virtual address:

    0x1000

They may have different mappings:

    Process A
    Virtual 0x1000 ---> Physical Page A

    Process B
    Virtual 0x1000 ---> Physical Page B

Therefore the same virtual address can refer to different physical memory in different processes.

This is one of the fundamental benefits of virtual memory.

---

# 24. Resource Abstraction

One of the most important OS concepts is **abstraction**.

Instead of exposing hardware-specific details, the OS provides standard abstractions.

### CPU

Application sees:

    Process / Thread

### RAM

Application sees:

    Virtual Address Space

### Storage

Application sees:

    Files

### Network

Application sees:

    Sockets

### Synchronization

Application sees:

    Mutex
    Semaphore
    Condition Variable

This abstraction makes applications easier to develop and more portable.

---

# 25. Multiprogramming

Suppose memory contains:

    Process A
    Process B
    Process C

On a single CPU core, only one task executes at a time.

If one task blocks:

    A running
       |
       v
    A waits for I/O
       |
       v
    Scheduler runs B

The CPU can therefore remain productive.

This is the basic idea behind **multiprogramming**.

---

# 26. Multitasking

Multitasking means the OS allows multiple tasks to make progress over time.

On one CPU:

    Time --->

    A A A | B B | C C | A A | B B | C

The CPU rapidly switches between runnable tasks.

On multiple cores, tasks can also execute in true parallelism.

---

# 27. Multiprocessing

Multiprocessing means using multiple CPUs/cores to execute tasks in parallel.

For example:

    CPU 0 -> Task A
    CPU 1 -> Task B
    CPU 2 -> Task C
    CPU 3 -> Task D

Here tasks can genuinely execute simultaneously.

---

# 28. Multithreading

A process may contain multiple threads.

    Process
    |
    +-- Thread 1
    +-- Thread 2
    +-- Thread 3

Threads generally share:

- Code
- Heap
- Global data
- Address space
- Many process resources

Each thread has its own:

- Stack
- Registers
- Program counter
- Thread-local storage

Example:

    Process
       |
       +-- Thread 1 -> Network work
       +-- Thread 2 -> CPU work
       +-- Thread 3 -> Logging

Multithreading is covered in detail in Chapter 3.

---

# 29. Multiprogramming vs Multitasking vs Multiprocessing vs Multithreading

| Concept | Meaning |
|---|---|
| Multiprogramming | Multiple programs/tasks are kept available so CPU can switch among them |
| Multitasking | OS allows multiple tasks to make progress seemingly concurrently |
| Multiprocessing | Multiple CPUs/cores execute tasks in parallel |
| Multithreading | Multiple execution threads exist within a process |

These concepts are related but should not be treated as synonyms.

---

# 30. OS Architecture

Common OS architectural approaches include:

1. Monolithic kernel
2. Microkernel
3. Hybrid designs

The boundaries are sometimes more nuanced than these labels suggest.

---

# 31. Monolithic Kernel

In a monolithic architecture, many major OS services execute in kernel space.

Conceptually:

    +--------------------------------+
    |           Kernel               |
    |                                |
    | Scheduler                      |
    | Memory                         |
    | VFS                            |
    | Networking                     |
    | Drivers                        |
    | IPC                            |
    +--------------------------------+

Advantages:

- Direct communication between subsystems
- Usually good performance
- Efficient internal calls

Disadvantages:

- Large privileged code base
- A kernel bug can have system-wide impact
- Less fault isolation than a strongly user-space-separated architecture

Linux is generally classified as a **monolithic kernel**, while supporting dynamically loadable kernel modules.

---

# 32. Microkernel

A microkernel keeps the privileged kernel relatively small.

Conceptually:

    +-----------------------------+
    | User-space services         |
    |                             |
    | Filesystem                  |
    | Network service             |
    | Drivers                     |
    +-----------------------------+
    | Microkernel                 |
    | IPC                         |
    | Scheduling                  |
    | Basic memory management     |
    +-----------------------------+
    | Hardware                    |
    +-----------------------------+

Advantages can include:

- Smaller privileged code base
- Better fault isolation
- More modular architecture

Potential disadvantages:

- IPC overhead
- More complex communication
- More architectural complexity

---

# 33. Hybrid Kernel

A hybrid design combines ideas from monolithic and microkernel architectures.

The exact meaning of "hybrid" varies by OS.

The important interview point is:

> Kernel architecture is about where responsibilities execute and how kernel components communicate and are isolated.

---

# 34. Monolithic vs Microkernel

| Feature | Monolithic | Microkernel |
|---|---|---|
| Kernel size | Larger | Smaller |
| Services | Mostly kernel space | Many services can run in user space |
| Communication | Direct internal calls | IPC-heavy |
| Fault isolation | Generally lower | Potentially higher |
| Performance | Often efficient | IPC can add overhead |
| Linux | Yes | No |

---

# 35. Linux Architecture

A simplified Linux architecture:

    +------------------------------------------------+
    |                  Applications                  |
    +------------------------------------------------+
    |              User-space libraries              |
    +------------------------------------------------+
    |                  System Calls                  |
    +------------------------------------------------+
    |                    Kernel                      |
    |                                                |
    |  Process Management                            |
    |  Scheduler                                     |
    |  Memory Management                             |
    |  VFS / Filesystems                             |
    |  Networking                                    |
    |  IPC                                           |
    |  Security                                      |
    |  Device Drivers                                |
    +------------------------------------------------+
    |                   Hardware                     |
    +------------------------------------------------+

Major kernel areas:

    Kernel
    |
    +-- Process Management
    +-- Scheduler
    +-- Memory Management
    +-- VFS
    +-- Filesystems
    +-- Block Layer
    +-- Networking
    +-- Device Drivers
    +-- IPC
    +-- Security
    +-- Interrupt Handling

These subsystems interact heavily.

Example storage path:

    read()
      |
      v
    VFS
      |
      v
    Page Cache
      |
      v
    Filesystem
      |
      v
    Block Layer
      |
      v
    Driver
      |
      v
    Storage

---

# 36. CPU, MMU and Memory — Basic Relationship

A simplified hardware/software relationship is:

    CPU
      |
      | Virtual address
      v
    MMU
      |
      v
    Page Tables
      |
      v
    Physical address
      |
      v
    RAM

The CPU executes instructions using virtual addresses.

The MMU translates virtual addresses into physical addresses using page-table information.

The OS creates and manages the page-table structures.

Later chapters will cover:

- Page tables
- TLB
- Page faults
- Virtual memory
- Copy-On-Write
- Memory allocation

in much greater detail.

---

# 37. Context Switching

A **context switch** occurs when the CPU stops executing one task and begins executing another.

Suppose Task A is running:

    Task A
      |
      | Scheduling event
      v
    Save A's execution state
      |
      v
    Scheduler
      |
      | Select Task B
      v
    Restore B's execution state
      |
      v
    Task B

The saved/restored state can include architecture-specific execution state such as:

- General-purpose registers
- Program Counter / instruction pointer
- Stack Pointer
- Processor state
- Other architecture-specific state

For a switch between different address spaces, memory-management context may also need to change.

---

# 38. What Is a Context?

A task's execution context is the CPU and execution state required to resume that task correctly.

At a simplified level:

    Context
    |
    +-- Program Counter
    +-- Stack Pointer
    +-- General-purpose registers
    +-- Processor state
    +-- Architecture-specific state

The exact state saved by Linux depends on:

- CPU architecture
- Type of switch
- Kernel implementation
- Whether the switch is between threads/processes
- Other architecture-specific requirements

---

# 39. Why Does Context Switching Cost Time?

A context switch itself does not perform useful application work.

The kernel must:

1. Save the current task's execution state.
2. Select another runnable task.
3. Restore the next task's state.
4. Resume execution.

There can also be secondary performance costs due to changed locality.

For example:

    Context Switch
         |
         +-- Save/restore CPU state
         |
         +-- Scheduler/bookkeeping
         |
         +-- Possible address-space change
         |
         +-- Cache locality effects
         |
         +-- Possible TLB effects
         |
         +-- Branch-prediction effects
         |
         v
       Overhead

Therefore excessive context switching can reduce performance.

This matters especially in:

- Highly threaded applications
- High-performance networking
- Storage systems
- Low-latency applications
- Real-time systems

---

# 40. Context Switch vs Mode Switch

This is one of the most common OS interview questions.

## Mode switch

    User Mode
        |
        v
    Kernel Mode
        |
        v
    User Mode

The same task can continue running.

## Context switch

    Task A
       |
       v
    Task B

The CPU begins executing another task.

Therefore:

    Mode switch != Context switch

A system call normally causes a privilege/mode transition, but it does not necessarily cause a task switch.

---

# 41. Thread Switch vs Process Switch

A process contains an address space and resources, while threads within a process share the process address space.

Therefore, switching between two threads of the same process can avoid some of the address-space changes associated with switching between threads belonging to different processes.

However, a context switch still involves CPU execution-state changes.

Simplified:

    Thread A
       |
       v
    Thread B
       |
    same address space

versus:

    Process A / Thread A
       |
       v
    Process B / Thread B
       |
    different address space

Do not assume every process switch has exactly the same cost on every architecture. Linux and the CPU architecture determine the details.

---

# 42. Scheduling and Context Switching

The scheduler chooses which runnable task should execute.

Conceptually:

    Runnable tasks
         |
         v
    Scheduler
         |
         v
    Selected task
         |
         v
    CPU executes task

A scheduling decision can result in a context switch, but a scheduling decision and a context switch are conceptually different events.

---

# 43. Blocking and Context Switching

Suppose a task waits for I/O:

    Task A
       |
       v
    read()
       |
       v
    Data not available
       |
       v
    Task blocks
       |
       v
    Scheduler
       |
       v
    Task B runs

When the required event occurs:

    I/O completion
         |
         v
    Task A becomes runnable
         |
         v
    Scheduler may later select A
         |
         v
    A resumes

This is a fundamental relationship between:

    System calls
        +
    Blocking
        +
    Scheduler
        +
    Context switching

---

# 44. Boot Process — Basic Overview

A simplified Linux boot flow is:

    Power On
       |
       v
    Firmware
    (BIOS/UEFI)
       |
       v
    Bootloader
       |
       v
    Linux Kernel
       |
       v
    Kernel initialization
       |
       v
    Initial user-space process
       |
       v
    Services
       |
       v
    Applications

A modern Linux system commonly starts the initial user-space process through a mechanism such as `systemd`, although the exact configuration can vary.

Detailed boot and kernel initialization will be covered later in Linux kernel internals.

---

# 45. Practical Linux Commands

## 45.1 List processes

```bash
ps aux
```

or:

```bash
ps -ef
```

---

## 45.2 Interactive process view

```bash
top
```

or:

```bash
htop
```

---

## 45.3 Process information

```bash
cat /proc/<pid>/status
```

Useful information includes:

- PID
- Parent PID
- State
- Memory information
- Thread information

---

## 45.4 Process memory map

```bash
cat /proc/<pid>/maps
```

This shows the process's virtual-memory mappings.

---

## 45.5 View CPU information

```bash
lscpu
```

Useful for understanding:

- CPU architecture
- Number of CPUs
- Cores
- Threads
- CPU features

---

## 45.6 View memory

```bash
free -h
```

---

## 45.7 View system information

```bash
uname -a
```

---

## 45.8 Observe system calls

Use:

```bash
strace ./program
```

You may see calls such as:

```text
execve(...)
brk(...)
mmap(...)
openat(...)
read(...)
write(...)
close(...)
```

`strace` is an excellent practical tool for understanding the user-space/kernel boundary.

---

# 46. Practical C Program — PID and PPID

```c
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("PID  = %d\n", getpid());
    printf("PPID = %d\n", getppid());

    return 0;
}
```

Compile:

```bash
gcc process.c -o process
```

Run:

```bash
./process
```

Then:

```bash
strace ./process
```

This lets you observe the system calls used by the program.

---

# 47. Practical Experiment — System Call vs Library Function

Consider:

```c
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("Hello using printf\n");

    write(STDOUT_FILENO, "Hello using write\n", 18);

    return 0;
}
```

Compile:

```bash
gcc io.c -o io
```

Run:

```bash
./io
```

Observe system calls:

```bash
strace ./io
```

The important lesson is:

    printf()
        |
        v
    Library function
        |
        v
    Possibly buffered
        |
        v
    write() system call

while:

    write()
        |
        v
    Direct system-call interface
```

---

# 48. Important Interview Questions

## Basic

1. What is an operating system?
2. Why do we need an OS?
3. What are the major responsibilities of an OS?
4. What is a kernel?
5. Kernel vs OS?
6. What is user space?
7. What is kernel space?
8. What is user mode?
9. What is kernel mode?
10. Why are privileged operations required?

## System Calls

11. What is a system call?
12. Is `printf()` a system call?
13. Is `malloc()` a system call?
14. What happens during a system call?
15. Does every system call cause a context switch?
16. System call vs library function?
17. Why are system calls more expensive than normal function calls?
18. How does a user process enter kernel mode?

## CPU and Events

19. What is an interrupt?
20. Why do we need interrupts?
21. What is an exception?
22. What is a trap?
23. Interrupt vs exception?
24. Exception vs trap?
25. System call vs interrupt?
26. What is a page fault?

## Execution Models

27. What is multiprogramming?
28. What is multitasking?
29. What is multiprocessing?
30. What is multithreading?
31. Multiprogramming vs multitasking?
32. Multiprocessing vs multithreading?

## Context Switching

33. What is a context switch?
34. What information is saved during a context switch?
35. Why is context switching expensive?
36. Context switch vs mode switch?
37. Can a system call happen without a context switch?
38. Can a context switch happen because of a blocking system call?
39. Thread switch vs process switch?
40. How does a context switch affect CPU cache/TLB locality?

## Architecture

41. What is a monolithic kernel?
42. What is a microkernel?
43. What is a hybrid kernel?
44. Is Linux monolithic or microkernel?
45. What are major Linux kernel subsystems?

---

# 49. Senior-Level Interview Questions and Answers

## Q1. Does every system call cause a context switch?

**No.**

A system call normally causes:

    User Mode
        ↓
    Kernel Mode
        ↓
    User Mode

The same task may continue running.

A context switch is:

    Task A
        ↓
    Task B

A system call can indirectly cause a context switch if it blocks or if scheduling causes another task to run.

---

## Q2. Why is a system call more expensive than a normal function call?

A normal function call normally stays in user mode.

A system call requires:

- Controlled CPU entry into the kernel
- Privilege transition
- Kernel-side argument validation/processing
- Kernel subsystem execution
- Return from kernel to user mode

The operation itself may also involve:

- Locks
- Memory access
- Filesystem work
- Device I/O
- Scheduling
- Blocking

Therefore the total cost can be much higher than a simple function call.

---

## Q3. Why can't applications directly access hardware?

Because unrestricted hardware access would break:

- Isolation
- Security
- Resource management
- System stability

The intended path is:

    Application
        |
        v
    System Call
        |
        v
    Kernel
        |
        v
    Driver
        |
        v
    Hardware

The kernel controls access.

---

## Q4. Why do we need virtual memory?

Virtual memory provides:

- Process isolation
- Memory protection
- Independent address spaces
- Efficient memory sharing
- Copy-On-Write
- Memory mapping
- Demand allocation

Simplified:

    Process
       |
       v
    Virtual Address
       |
       v
    MMU + Page Tables
       |
       v
    Physical Memory

---

## Q5. What happens when a process accesses an unmapped virtual address?

Simplified flow:

    CPU executes memory access
            |
            v
    MMU performs translation
            |
            v
    Translation/access fails
            |
            v
       Page fault
            |
            v
    Kernel page-fault handler
            |
            +---- Valid/recoverable?
            |        |
            |        +--> Establish mapping / load page / COW
            |
            +---- Invalid?
                     |
                     +--> Fault/signal to process

The exact behavior depends on why the fault occurred.

---

## Q6. What is the difference between an interrupt and an exception?

### Interrupt

Usually caused by an external event:

    Device
       |
       v
    Interrupt
       |
       v
    CPU / Kernel

### Exception

Usually caused synchronously by the instruction being executed:

    Instruction
        |
        v
    CPU detects condition
        |
        v
    Exception

Examples:

    Interrupt:
        Timer
        NIC
        Device completion

    Exception:
        Page fault
        Invalid instruction
        Divide error

---

## Q7. Is a page fault always an error?

**No.**

A page fault is a CPU-generated event indicating that the current memory access cannot proceed normally.

The kernel may resolve it.

Examples:

- Demand paging
- Copy-On-Write
- Lazy allocation
- Valid memory mapping that needs to be established

An invalid access can instead result in a signal such as `SIGSEGV`.

---

## Q8. Why can two processes use the same virtual address?

Because each process normally has its own virtual address space.

For example:

    Process A:
    0x400000 -> Physical Page A

    Process B:
    0x400000 -> Physical Page B

The virtual address is interpreted in the context of the current address space.

---

## Q9. Why are threads generally cheaper than processes for communication?

Threads within a process share the same address space:

    Process
       |
       +-- Thread A
       +-- Thread B

Therefore they can directly access shared memory.

Separate processes have isolated address spaces:

    Process A
       |
    Address Space A

    Process B
       |
    Address Space B

Processes can still communicate using IPC mechanisms such as:

- Pipes
- Shared memory
- Message queues
- Sockets

Thread sharing is easier, but it also introduces synchronization hazards.

---

## Q10. What makes context switching expensive?

The direct cost includes:

- Saving CPU state
- Restoring CPU state
- Scheduler bookkeeping

Additional costs may come from:

- Cache locality changes
- TLB effects
- Branch predictor effects
- Address-space changes
- Synchronization

Therefore reducing unnecessary context switches can improve performance.

---

# 50. Common Interview Traps

## Trap 1

> System call = context switch

**Wrong.**

System call:

    User Mode ↔ Kernel Mode

Context switch:

    Task A → Task B

---

## Trap 2

> `printf()` is a system call.

**Wrong.**

`printf()` is a library function.

It may eventually invoke `write()`.

---

## Trap 3

> Every page fault means invalid memory access.

**Wrong.**

Some page faults are legitimate and are resolved by the kernel.

---

## Trap 4

> Interrupts and exceptions are the same.

**Wrong.**

Interrupts are generally external/asynchronous events.

Exceptions are generally synchronous events associated with instruction execution.

---

## Trap 5

> Linux is a microkernel because drivers can be modules.

**Wrong.**

Linux is generally classified as a monolithic kernel with support for loadable kernel modules.

A module being dynamically loadable does not make the kernel a microkernel.

---

## Trap 6

> User space and user mode are exactly the same thing.

**Not exactly.**

User space refers primarily to the protected application-side address-space region.

User mode refers to CPU privilege level.

---

# 51. Chapter 1 Revision Sheet

## OS

    OS
     |
     +-- Resource Management
     +-- Protection
     +-- Abstraction
     +-- Process Management
     +-- Memory Management
     +-- Filesystems
     +-- I/O
     +-- Networking

---

## User/Kernel Boundary

    Application
        |
        v
    User Mode
        |
        | System Call
        v
    Kernel Mode
        |
        v
    Kernel Subsystem
        |
        v
    Hardware / Resource

---

## System Call

    Application
        |
        v
    Library wrapper
        |
        v
    syscall mechanism
        |
        v
    Kernel
        |
        v
    Return

Remember:

    System Call != Context Switch

---

## Interrupt

    External device/event
             |
             v
         Interrupt
             |
             v
            CPU
             |
             v
          Kernel

---

## Exception

    CPU instruction
          |
          v
      Exception
          |
          v
        Kernel

---

## Process Isolation

    Process A
       |
       v
    Virtual Address Space A
       |
       v
    Page Tables
       |
       v
    Physical Memory

    Process B
       |
       v
    Virtual Address Space B
       |
       v
    Page Tables
       |
       v
    Physical Memory

---

## Context Switch

    Task A
       |
       v
    Save CPU state
       |
       v
    Scheduler
       |
       v
    Restore CPU state
       |
       v
    Task B

---

## Execution Models

    Multiprogramming
        = multiple programs kept available

    Multitasking
        = multiple tasks make progress over time

    Multiprocessing
        = multiple CPUs/cores execute in parallel

    Multithreading
        = multiple threads execute within a process

---

## Linux Architecture

    Applications
         |
         v
    Libraries
         |
         v
    System Calls
         |
         v
    Linux Kernel
         |
         +-- Scheduler
         +-- Process Management
         +-- Memory Management
         +-- VFS
         +-- Filesystems
         +-- Networking
         +-- IPC
         +-- Drivers
         +-- Security
         |
         v
    Hardware

---

# 52. Must-Know Concepts Before Chapter 2

You should be able to explain these without looking at the notes:

1. What an OS does
2. Kernel vs OS
3. User space vs kernel space
4. User mode vs kernel mode
5. Privileged instructions
6. System calls
7. Library call vs system call
8. System-call flow
9. Interrupts
10. Exceptions
11. Traps
12. Interrupt vs exception
13. System call vs context switch
14. Process isolation
15. Virtual memory's basic purpose
16. Multiprogramming
17. Multitasking
18. Multiprocessing
19. Multithreading
20. Monolithic vs microkernel
21. Linux architecture
22. Context switching
23. Why context switching has overhead
24. Thread switch vs process/address-space switch
25. Basic Linux commands such as `ps`, `top`, `/proc`, and `strace`

---

# 53. Chapter 1 Final Mental Model

The complete picture to remember is:

    +----------------------------------------------------+
    |                   APPLICATION                      |
    |                                                    |
    |   Process / Thread / File / Socket / Memory        |
    +-------------------------+--------------------------+
                              |
                              | System Call
                              v
    +----------------------------------------------------+
    |                     LINUX KERNEL                   |
    |                                                    |
    |  Process Management     Scheduler                  |
    |  Memory Management      VFS                        |
    |  Filesystems            Networking                 |
    |  IPC                    Device Drivers             |
    |  Security               Interrupt Handling         |
    +-------------------------+--------------------------+
                              |
                              v
    +----------------------------------------------------+
    |                     HARDWARE                       |
    |                                                    |
    | CPU | MMU | RAM | Disk | NIC | USB | Devices      |
    +----------------------------------------------------+

The most important relationships are:

    Application
        |
        | System Call
        v
    Kernel
        |
        | Driver / Subsystem
        v
    Hardware

    User Mode
        |
        | Controlled entry
        v
    Kernel Mode
        |
        | Return
        v
    User Mode

    Hardware Event
        |
        v
    Interrupt
        |
        v
    Kernel

    CPU Instruction
        |
        v
    Exception
        |
        v
    Kernel

    Task A
        |
        v
    Context Switch
        |
        v
    Task B

    Virtual Address
        |
        v
    MMU + Page Tables
        |
        v
    Physical Address
        |
        v
    RAM

    Application
        |
        v
    VFS
        |
        v
    Filesystem
        |
        v
    Block Layer
        |
        v
    Driver
        |
        v
    Storage

---

# Chapter 1 Complete

Next chapter:

# Chapter 2 — Processes

Topics planned:

- Program vs Process
- What is a Process?
- Process address space
- Process states
- PCB
- Linux `task_struct`
- PID / PPID
- Parent-child relationship
- `fork()`
- `exec()`
- `wait()` / `waitpid()`
- `exit()`
- Zombie process
- Orphan process
- Copy-On-Write
- `fork()` memory behavior
- Process creation flow
- Process termination
- Context switching in detail
- Process scheduling introduction
- `/proc`
- Practical C programs
- Interview questions
- Senior-level questions
- Common interview traps
- Quick revision
