# Chapter 2 — Processes

## 1. What is a Process?

A **process is a program in execution**.

A program is a passive object:

```text
program on disk
     |
     v
 executable file
```

A process is the active execution of that program:

```text
Executable
    |
    | process creation
    v
Process
    |
    +-- Virtual address space
    +-- CPU execution state
    +-- Registers
    +-- Stack
    +-- Heap
    +-- Open files
    +-- Signals
    +-- Credentials
    +-- Scheduling information
```

A process therefore contains much more than the executable code.

---

# 2. Program vs Process

| Program | Process |
|---|---|
| Passive | Active |
| Stored on disk | Running/in memory |
| Contains instructions/data | Contains execution state + resources |
| No CPU state | Has CPU state |
| No PID | Has PID |
| Can create processes | Is created from a program/process |

Example:

```bash
./app
```

The executable `app` is a program.

When the OS starts it, an executing instance becomes a process.

You can have:

```text
./app
./app
./app
```

These can be three separate processes running the same program.

---

# 3. Why Do We Need Processes?

Processes provide:

- Execution environment
- Resource ownership
- Isolation
- Scheduling unit
- Protection
- Process identity

Conceptually:

```text
+---------------------+
|      Process A      |
|                     |
|  Code               |
|  Data               |
|  Heap               |
|  Stack              |
+---------------------+

+---------------------+
|      Process B      |
|                     |
|  Code               |
|  Data               |
|  Heap               |
|  Stack              |
+---------------------+
```

Process A normally cannot directly access Process B's memory.

---

# 4. Process Address Space

A process normally sees a **virtual address space**.

A simplified process layout:

```text
High Address
+---------------------------+
| Kernel address space      |
|                           |
|   protected from user     |
+---------------------------+
| Stack                     |
|                           |
|        grows downward     |
+---------------------------+
|                           |
| Memory mappings           |
| Shared libraries          |
| mmap() regions            |
|                           |
+---------------------------+
| Heap                      |
|                           |
|        grows upward       |
+---------------------------+
| BSS                       |
+---------------------------+
| Initialized data          |
+---------------------------+
| Read-only data            |
+---------------------------+
| Text / Code               |
+---------------------------+
Low Address
```

This is a simplified conceptual layout. Exact layout depends on architecture, ASLR, executable format, kernel configuration, and process state.

---

# 5. Major Process Memory Regions

## Text

Contains executable machine instructions.

Usually:

- Readable
- Executable
- Not writable

Example:

```c
int add(int a, int b)
{
    return a + b;
}
```

The compiled instructions belong to the code/text region.

---

## Read-only Data

May contain constants and other read-only data.

Example:

```c
const char *msg = "Hello";
```

The string literal is typically stored in a read-only section.

---

## Data

Contains initialized global/static variables.

Example:

```c
int global = 10;
static int count = 5;
```

---

## BSS

Contains zero-initialized or uninitialized global/static variables.

Example:

```c
int global_array[1000];
static int counter;
```

The loader/kernel establishes the required memory state.

---

## Heap

Used for dynamic allocation.

Example:

```c
int *p = malloc(sizeof(int));
```

The C library allocator manages heap allocations and may obtain memory from the kernel using mechanisms such as `brk()` and `mmap()`.

---

## Stack

Used for function-call state and automatic local variables.

Example:

```c
void function(void)
{
    int x = 10;
}
```

The stack contains execution information associated with function calls.

Each thread has its own stack.

---

# 6. Process Control Block

The OS needs to maintain information about every process.

This information is commonly described as a:

**Process Control Block (PCB)**

Conceptually:

```text
PCB
 |
 +-- PID
 +-- Process state
 +-- CPU context
 +-- Scheduling information
 +-- Memory information
 +-- Open-file information
 +-- Credentials
 +-- Signal information
 +-- Parent/child information
 +-- Accounting information
```

The exact data structures are OS-specific.

In Linux, process/task information is represented using kernel structures centered around `struct task_struct` and related structures.

---

# 7. Linux `task_struct`

Linux uses:

```c
struct task_struct
```

to represent a task.

It contains or references information needed by the kernel for task management.

Conceptually:

```text
task_struct
 |
 +-- PID / identity
 +-- Scheduling information
 +-- State
 +-- Parent/child relationships
 +-- Memory-management reference
 +-- File-system information
 +-- Open files reference
 +-- Signal information
 +-- Credentials
 +-- CPU/context-related information
```

Important:

Linux often uses the term **task** rather than treating the process as the only schedulable entity.

Threads are also represented as tasks.

---

# 8. Process ID — PID

Every Linux process has a process ID.

Example:

```c
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("PID = %d\n", getpid());
    return 0;
}
```

Compile:

```bash
gcc pid.c -o pid
```

Run:

```bash
./pid
```

Example output:

```text
PID = 12345
```

The PID identifies the process within its PID namespace.

---

# 9. Parent PID — PPID

A process normally has a parent.

Use:

```c
getppid();
```

Example:

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

Relationship:

```text
Parent
  |
  +------> Child
```

---

# 10. Process Tree

Linux maintains parent/child relationships.

Example:

```text
systemd
 |
 +-- sshd
 |    |
 |    +-- bash
 |         |
 |         +-- ./app
 |
 +-- service
      |
      +-- worker
```

Useful command:

```bash
pstree
```

or:

```bash
ps -ef --forest
```

---

# 11. Creating a Process

A process can create another process.

Conceptually:

```text
Parent Process
      |
      | process creation
      v
Child Process
```

On Linux, the most important process-creation interfaces to understand are:

```c
fork()
clone()
```

For normal application-level process creation, `fork()` is the classic API.

---

# 12. `fork()`

`fork()` creates a new process.

Example:

```c
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        printf("Child:  PID = %d\n", getpid());
    }
    else
    {
        printf("Parent: PID = %d, Child PID = %d\n",
               getpid(), pid);
    }

    return 0;
}
```

Compile:

```bash
gcc fork.c -o fork
```

Run:

```bash
./fork
```

---

# 13. What Does `fork()` Return?

This is a very important interview question.

```c
pid_t pid = fork();
```

There are three cases:

### Failure

```text
pid < 0
```

No child was created.

### Child

```text
pid == 0
```

The child sees `0` as the return value.

### Parent

```text
pid > 0
```

The parent receives the child's PID.

Conceptually:

```text
                 fork()
                   |
             +-----+-----+
             |           |
           Parent       Child
             |           |
        return child   return 0
           PID
```

---

# 14. Why Does `fork()` Appear to Return Twice?

Because after `fork()` succeeds, there are now two processes executing from the point after the `fork()` call.

Conceptually:

```text
Before fork:

        Parent
          |
        fork()
          |
          ?

After fork:

        Parent              Child
           |                  |
           +------ fork ------+
                  return
```

Each process gets a different return value.

This is not one function literally returning twice in one CPU execution. Two processes continue execution independently.

---

# 15. `fork()` and Memory

A common misconception is:

> `fork()` immediately copies the entire process memory.

Modern Linux uses **Copy-On-Write (COW)**.

Initially, parent and child can reference the same physical pages:

```text
Parent virtual memory
        |
        v
   +-----------+
   | Physical  |
   |   Page    |
   +-----------+
        ^
        |
Child virtual memory
```

Pages are initially shared where appropriate.

If one process writes to a COW page:

```text
Parent
   |
   v
Page A
   ^
   |
Child

Child writes
   |
   v
Page fault / COW handling
   |
   +----> New physical page
```

The child receives a private copy of the modified page.

---

# 16. Why Copy-On-Write?

Without COW:

```text
fork()
  |
  +-- Copy all memory immediately
```

This could be expensive.

With COW:

```text
fork()
  |
  +-- Share pages initially
  |
  +-- Copy only when a write requires it
```

This makes `fork()` much more efficient for many workloads, especially when followed quickly by `exec()`.

---

# 17. `fork()` + `exec()`

A very important Linux process pattern is:

```text
Parent
   |
   | fork()
   v
Child
   |
   | exec()
   v
New program
```

`fork()` creates the child.

`exec()` replaces the child process's program image with another executable.

The PID normally remains the same across `exec()`.

---

# 18. What Does `exec()` Do?

`exec` is a family of functions:

```c
execl()
execv()
execvp()
execve()
...
```

The important concept is:

> `exec()` replaces the current process image with a new program.

Example:

```c
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("Before exec\n");

    execl("/bin/ls", "ls", "-l", NULL);

    perror("execl");
    return 1;
}
```

If `execl()` succeeds, execution does not continue with the next normal statement.

The current process image has been replaced.

---

# 19. `fork()` vs `exec()`

| `fork()` | `exec()` |
|---|---|
| Creates a new process | Replaces current process image |
| Parent and child exist | Same process continues with new program |
| Child gets a new PID | PID normally remains unchanged |
| Uses COW | Loads new executable image |
| Returns in parent and child | Returns only on failure |

Typical shell flow:

```text
Shell
  |
  | fork()
  v
Child
  |
  | exec()
  v
Requested command
```

---

# 20. Example: `fork()` + `exec()`

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        execlp("ls", "ls", "-l", NULL);

        perror("execlp");
        _exit(1);
    }

    waitpid(pid, NULL, 0);

    printf("Child finished\n");

    return 0;
}
```

Flow:

```text
Parent
  |
  +---- fork() ----+
  |                |
  |              Child
  |                |
  |              exec(ls)
  |                |
  |              ls runs
  |                |
  +---- wait() <---+
  |
  v
Continue
```

---

# 21. Why Does a Shell Use `fork()` + `exec()`?

Suppose you type:

```bash
ls -l
```

The shell should remain alive.

Therefore:

```text
Shell
 |
 +-- fork()
       |
       +-- Child
             |
             +-- exec("ls")
```

The child becomes `ls`.

The shell remains the parent.

For a foreground command, the shell can wait for the child.

---

# 22. `wait()` and `waitpid()`

A parent can wait for a child.

Example:

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        printf("Child running\n");
        sleep(2);
        printf("Child exiting\n");
        _exit(42);
    }

    waitpid(pid, NULL, 0);

    printf("Parent: child completed\n");

    return 0;
}
```

The parent blocks until the child state can be collected.

---

# 23. Why Does a Parent Need `wait()`?

A terminated child leaves exit-status information that the parent can collect.

If the parent does not collect the child state, the child can temporarily remain as a **zombie**.

---

# 24. Process Termination

A process can terminate through mechanisms such as:

```c
exit(status);
```

or:

```c
_exit(status);
```

A process can also be terminated by a signal.

Examples:

```text
SIGTERM
SIGKILL
SIGSEGV
```

Conceptually:

```text
Running
   |
   v
Termination
   |
   v
Exit state
   |
   v
Parent collects status
   |
   v
Resources released
```

---

# 25. `exit()` vs `_exit()`

This is important when using `fork()`.

### `exit()`

A C library function.

It performs user-space cleanup such as flushing stdio streams and running registered `atexit()` handlers.

### `_exit()`

A system-level process termination interface that terminates the process without performing the normal stdio cleanup done by `exit()`.

After `fork()`, especially in the child of a process that will not successfully `exec()`, `_exit()` is commonly preferred to avoid accidentally repeating parent-side stdio-buffer effects.

---

# 26. Zombie Process

A **zombie** is a terminated child whose parent has not yet collected its termination status.

Conceptually:

```text
Child
  |
  | exits
  v
Zombie
  |
  | parent calls wait()/waitpid()
  v
Collected
```

The zombie is not actively executing.

It exists so the kernel can retain relevant termination information until the parent collects it.

---

# 27. Zombie Example

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        _exit(0);
    }

    printf("Parent sleeping. Child is now terminated.\n");

    sleep(20);

    waitpid(pid, NULL, 0);

    return 0;
}
```

During the parent's sleep, inspect:

```bash
ps -o pid,ppid,state,cmd
```

The child may appear with state:

```text
Z
```

which indicates zombie state.

---

# 28. Orphan Process

An orphan is a process whose original parent has terminated while the child continues running.

Conceptually:

```text
Parent
  |
  +-- Child
       |
       | Parent exits
       v
     Orphan
```

The orphan is adopted/reparented by an appropriate process in its PID namespace, commonly a subreaper or the namespace's init process.

A simplified traditional model is:

```text
init/system manager
       |
       +-- orphaned child
```

---

# 29. Zombie vs Orphan

| Zombie | Orphan |
|---|---|
| Child has terminated | Child is still running |
| Parent has not collected status | Original parent has terminated |
| Appears as `Z` | Continues execution |
| `wait()`/`waitpid()` collects it | Reparenting occurs |

Very important:

```text
Zombie = dead child, not yet collected

Orphan = living child whose original parent is gone
```

---

# 30. Process States

A simplified process-state model:

```text
                 +----------+
                 |  RUNNING |
                 +----------+
                    |    ^
          schedule  |    |
                    v    |
              +-------------+
              |  RUNNABLE   |
              +-------------+
                    |
                    | waits for event/I/O
                    v
              +-------------+
              |   BLOCKED   |
              +-------------+
                    |
                    | event occurs
                    v
              +-------------+
              |  RUNNABLE   |
              +-------------+

Running
   |
   | exit
   v
Zombie / terminated state
```

Linux has more detailed internal task states than this simplified model.

---

# 31. Runnable vs Running

This distinction is important.

### Running

The task is currently executing on a CPU.

### Runnable

The task is ready to execute but may be waiting for CPU time.

On a system with one CPU:

```text
Running:
    Task A

Runnable:
    Task B
    Task C
```

The scheduler chooses which runnable task gets CPU time.

---

# 32. Blocked / Sleeping

A process may wait for:

- I/O
- Lock
- Timer
- Child process
- Condition
- Event

Example:

```text
Process
   |
   | read()
   |
   | data unavailable
   v
Blocked
   |
   | I/O completion
   v
Runnable
```

Blocking avoids wasting CPU cycles continuously checking for the event.

---

# 33. Process Scheduling

The scheduler selects a runnable task to execute.

Conceptually:

```text
Runnable tasks
     |
     v
+------------+
| Scheduler  |
+------------+
     |
     v
Selected task
     |
     v
CPU
```

Scheduling is covered in detail in Chapter 4.

---

# 34. Process Context

A process/task needs execution state to resume correctly.

Simplified:

```text
Task context
 |
 +-- Program Counter
 +-- Stack Pointer
 +-- CPU registers
 +-- Processor state
 +-- Address-space context
```

When switching tasks:

```text
Task A
  |
  | save context
  v
Scheduler
  |
  | restore context
  v
Task B
```

Detailed context-switch mechanics belong to Chapter 4 and Linux kernel internals.

---

# 35. Process and Address Space

A process generally owns or references an address space.

Conceptually:

```text
Process
 |
 +-- mm_struct
       |
       +-- Virtual memory areas
       +-- Page tables
       +-- Memory mappings
       +-- Address-space information
```

In Linux, `struct mm_struct` represents a process's memory-management context.

Threads within the same process normally share the same `mm_struct`.

This is one of the key differences between processes and threads.

---

# 36. Process and Open Files

A process can have file descriptors:

```text
Process
 |
 +-- FD 0 -> stdin
 +-- FD 1 -> stdout
 +-- FD 2 -> stderr
 +-- FD 3 -> file/socket
 +-- FD 4 -> file/socket
```

Example:

```c
int fd = open("data.txt", O_RDONLY);
```

The returned integer is a file descriptor.

The kernel maintains the underlying file/object state.

The exact Linux relationship is roughly:

```text
Process
   |
   v
File descriptor table
   |
   v
struct file
   |
   v
VFS object / inode / filesystem
```

This is covered in detail in the Files and I/O chapter.

---

# 37. Process and Signals

Signals provide asynchronous notifications to processes/threads.

Examples:

```text
SIGTERM
SIGKILL
SIGINT
SIGSEGV
SIGCHLD
```

Example:

```bash
kill -TERM <pid>
```

A signal can cause:

- Default action
- User-defined handler
- Ignoring, where permitted
- Process termination
- Stop/continue behavior

Signal handling will be covered in more detail with IPC and Linux system programming.

---

# 38. Process Credentials

A process has credentials associated with it.

Important concepts include:

```text
UID
GID
Capabilities
```

These influence what the process is allowed to do.

Example:

```bash
id
```

---

# 39. `/proc` and Process Inspection

Linux exposes process information through `/proc`.

For PID 1234:

```bash
ls /proc/1234
```

Useful files:

```text
/proc/1234/status
/proc/1234/maps
/proc/1234/fd/
/proc/1234/cmdline
/proc/1234/stat
```

Examples:

```bash
cat /proc/1234/status
cat /proc/1234/maps
ls -l /proc/1234/fd
```

This is extremely useful for debugging Linux processes.

---

# 40. Inspect Process Memory

Run:

```bash
cat /proc/<pid>/maps
```

You may see mappings such as:

```text
00400000-00401000 r--p ...
00401000-00402000 r-xp ...
...
7f....        shared library
...
7fff....      [stack]
```

This allows you to observe the process's virtual address-space mappings.

---

# 41. Inspect File Descriptors

Run:

```bash
ls -l /proc/<pid>/fd
```

Example:

```text
0 -> /dev/pts/0
1 -> /dev/pts/0
2 -> /dev/pts/0
3 -> /tmp/data.txt
```

This connects the abstract file descriptor concept to actual Linux process state.

---

# 42. Process Creation Experiment

Program:

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        printf("Child:\n");
        printf("  PID  = %d\n", getpid());
        printf("  PPID = %d\n", getppid());
        _exit(0);
    }

    printf("Parent:\n");
    printf("  PID      = %d\n", getpid());
    printf("  Child PID = %d\n", pid);

    waitpid(pid, NULL, 0);

    return 0;
}
```

Compile:

```bash
gcc process_tree.c -o process_tree
```

Run:

```bash
./process_tree
```

---

# 43. `fork()` Memory Experiment

```c
#include <stdio.h>
#include <unistd.h>

int value = 10;

int main(void)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        value = 20;

        printf("Child value  = %d\n", value);
    }
    else
    {
        sleep(1);

        printf("Parent value = %d\n", value);
    }

    return 0;
}
```

Expected conceptual result:

```text
Child value  = 20
Parent value = 10
```

Why?

After `fork()`, the processes have logically independent memory.

Initially pages may be shared using COW.

When the child writes:

```c
value = 20;
```

the relevant page can be copied.

---

# 44. Observe `fork()` with `strace`

Run:

```bash
strace -f ./process_tree
```

The `-f` option follows child processes.

This is useful for observing process creation and subsequent system calls.

---

# 45. `exec()` Experiment

```c
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("Before exec\n");

    execlp("echo", "echo", "Hello from new program", NULL);

    perror("execlp");
    return 1;
}
```

Run:

```bash
gcc exec.c -o exec
./exec
```

If `exec()` succeeds, the original process image is replaced by `echo`.

---

# 46. `fork()` + `exec()` + `waitpid()` — Complete Example

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        printf("Child: executing ls\n");

        execlp("ls", "ls", "-l", NULL);

        perror("execlp");
        _exit(127);
    }

    printf("Parent: waiting for child %d\n", pid);

    int status;

    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status))
    {
        printf("Child exited with status %d\n",
               WEXITSTATUS(status));
    }

    return 0;
}
```

Important concepts demonstrated:

```text
fork()
  ↓
child
  ↓
exec()
  ↓
new program
  ↓
parent waitpid()
  ↓
parent continues
```

---

# 47. What Happens Internally During `fork()`?

A simplified conceptual flow:

```text
User process
    |
    | fork()
    v
System-call entry
    |
    v
Kernel process-creation code
    |
    +-- Create new task/process structures
    |
    +-- Establish parent/child relationship
    |
    +-- Establish memory-management state
    |
    +-- Set up file/signal/credential references
    |
    +-- Establish COW memory mappings
    |
    +-- Make child runnable
    |
    v
Parent and child continue
```

The exact Linux implementation is more detailed and version-dependent.

---

# 48. What Happens During `exec()`?

A simplified conceptual flow:

```text
Existing process
      |
      | execve()
      v
Kernel
      |
      +-- Validate executable
      |
      +-- Read executable format
      |
      +-- Establish new memory mappings
      |
      +-- Load program segments
      |
      +-- Set up stack/arguments/environment
      |
      +-- Set instruction pointer
      |
      v
New program starts
```

The process identity such as PID remains associated with the same task while its program image is replaced.

---

# 49. `fork()` + `exec()` Why This Design?

Separating creation from program replacement provides flexibility.

For example, a shell can:

```text
fork()
   |
   +-- child
         |
         +-- redirect stdin/stdout
         +-- set environment
         +-- set credentials/attributes where appropriate
         +-- exec()
```

This allows the shell to prepare the child before executing the requested program.

Example:

```bash
ls > output.txt
```

Conceptually:

```text
Shell
 |
 +-- fork()
       |
       +-- Child
             |
             +-- open output.txt
             +-- dup2(...)
             +-- exec(ls)
```

This separation is fundamental to Unix process creation.

---

# 50. Process Descriptor vs Process Address Space

Do not confuse:

```text
task_struct
```

with:

```text
mm_struct
```

Conceptually:

```text
task_struct
 |
 +-- identity
 +-- scheduling state
 +-- relationships
 +-- credentials
 +-- files
 +-- signals
 +-- ---> mm_struct
             |
             +-- address space
             +-- page tables
             +-- VM mappings
```

Multiple threads in a process can point to the same `mm_struct`.

---

# 51. Process vs Thread — Preview

Process:

```text
Process
 |
 +-- Address space
 +-- Resources
 +-- Files
 +-- Credentials
 +-- Threads
```

Thread:

```text
Thread
 |
 +-- Registers
 +-- Program counter
 +-- Stack
 +-- Scheduling state
```

Threads within the same process share many resources.

Detailed thread internals are covered in Chapter 3.

---

# 52. Important Process Relationships

Remember this chain:

```text
Program
   |
   | execution
   v
Process
   |
   +-- Address space
   +-- Resources
   +-- One or more threads
   |
   +-- Parent/child relationship
   |
   +-- PID
```

Process creation:

```text
Parent
   |
 fork()
   |
   v
Child
   |
 exec()
   |
   v
New program
```

Termination:

```text
Running
   |
   v
Exit
   |
   v
Zombie
   |
   | wait()/waitpid()
   v
Collected
```

Orphaning:

```text
Parent
   |
   +-- Child
        |
        | parent exits
        v
     Orphan
        |
        v
     Reparented
```

---

# 53. Common Interview Traps

## Trap 1 — `fork()` copies all memory immediately

Not necessarily.

Linux uses Copy-On-Write so that memory pages can initially be shared.

---

## Trap 2 — `exec()` creates a new process

No.

`exec()` replaces the current process image.

---

## Trap 3 — `exec()` changes the PID

Normally no.

The process keeps its PID while the program image changes.

---

## Trap 4 — Zombie is a running process

No.

A zombie has terminated. Its parent has not yet collected its termination status.

---

## Trap 5 — Orphan is a dead process

No.

An orphan is still running, but its original parent has terminated.

---

## Trap 6 — Parent and child continue from the beginning after `fork()`

No.

They continue from the point after the successful `fork()` call.

---

## Trap 7 — `fork()` returns the child's PID to both processes

No.

```text
Parent -> child PID
Child  -> 0
Failure -> -1
```

---

## Trap 8 — Process means only one execution thread

Not necessarily.

A process can contain multiple threads.

---

## Trap 9 — `wait()` kills the child

No.

`wait()` collects the child's termination status and allows the parent to synchronize with the child.

---

## Trap 10 — `exit()` and `_exit()` are identical

No.

`exit()` is a C library function with user-space cleanup semantics.

`_exit()` terminates without the normal stdio flushing and `atexit()` processing performed by `exit()`.

---

# 54. Senior Interview Questions

## Q1. Explain `fork()` internally.

A strong answer:

> `fork()` requests creation of a new process/task. Linux creates the necessary task-management structures and establishes the parent-child relationship. The child's memory is initially handled using Copy-On-Write rather than eagerly copying every physical page. Relevant resources such as file-descriptor state and credentials are established according to their sharing/reference semantics. The child becomes runnable and both parent and child continue after the `fork()` point with different return values.

---

## Q2. Why is `fork()` efficient even though the child gets a copy of the parent's address space?

Because the copy is logically duplicated but physical pages can initially be shared using Copy-On-Write.

Only when a process writes to a COW page does the kernel need to create a private copy.

---

## Q3. Why is `fork()` usually followed by `exec()` in a shell?

`fork()` creates a separate child while preserving the shell.

The child can then configure its environment, file descriptors, redirections, etc., and call `exec()` to replace its program image with the requested command.

---

## Q4. What happens if `exec()` fails?

`exec()` returns `-1` and sets `errno`.

Therefore:

```c
execlp(...);

perror("execlp");
_exit(127);
```

is a common pattern in a child process.

---

## Q5. Why should the child use `_exit()` after failed `exec()`?

Because after `fork()`, the child may have inherited stdio buffers and other user-space state from the parent.

Calling `exit()` can repeat stdio flushing and `atexit()` processing that should belong only to the original process.

`_exit()` avoids those normal C-library termination actions.

---

## Q6. What is a zombie and why does it exist?

A zombie is a terminated child for which the parent has not yet collected the termination status.

The kernel retains the required exit information so the parent can call:

```c
wait()
waitpid()
```

and obtain the child's result.

---

## Q7. What happens when a parent dies before its child?

The child becomes orphaned.

The kernel reparents it to an appropriate process, such as a configured subreaper or the PID namespace's init process.

---

## Q8. Can a zombie consume CPU?

No.

It has terminated and is not executing.

It can still consume a small amount of kernel bookkeeping resources until it is collected.

---

## Q9. Can a process have multiple threads?

Yes.

A process provides the shared resource/address-space environment, while threads provide multiple execution contexts within that environment.

---

## Q10. What does a process own and what does it share?

A process has an address space and many associated resources.

Threads in the same process generally share:

```text
Code
Heap
Global data
Address space
Many file/resource references
```

while each thread has its own:

```text
Registers
Program counter
Stack
Thread-local state
Scheduling state
```

---

# 55. Practical Debugging Checklist

When debugging a Linux process, start with:

```bash
ps -ef
```

Then:

```bash
cat /proc/<pid>/status
```

Memory:

```bash
cat /proc/<pid>/maps
```

File descriptors:

```bash
ls -l /proc/<pid>/fd
```

System calls:

```bash
strace -f -p <pid>
```

Threads:

```bash
ps -T -p <pid>
```

or:

```bash
ls /proc/<pid>/task
```

Process tree:

```bash
pstree -p
```

CPU/memory activity:

```bash
top
```

This gives a practical picture of:

```text
Process
  |
  +-- PID
  +-- Parent
  +-- Threads
  +-- Memory
  +-- File descriptors
  +-- System calls
  +-- CPU activity
```

---

# 56. Process Interview Quick Revision

## Definition

```text
Process = program in execution
```

---

## Process contains

```text
Process
 |
 +-- Address space
 +-- CPU execution state
 +-- Resources
 +-- PID
 +-- Scheduling state
 +-- Files
 +-- Signals
 +-- Credentials
 +-- Threads
```

---

## Process creation

```text
fork()
```

---

## Program replacement

```text
exec()
```

---

## Parent waits

```text
wait()
waitpid()
```

---

## Termination

```text
exit()
_exit()
signals
```

---

## Zombie

```text
Child exits
   |
   v
Parent has not waited
   |
   v
Zombie
```

---

## Orphan

```text
Parent exits
   |
   v
Child continues
   |
   v
Reparented
```

---

## Memory

```text
fork()
   |
   v
COW pages
   |
   v
Write
   |
   v
Private copy when required
```

---

## Shell

```text
Shell
 |
 +-- fork()
       |
       +-- Child
             |
             +-- setup/redirection
             |
             +-- exec()
```

---

# 57. Must-Know Commands

```bash
ps -ef
ps aux
ps -T -p <pid>
top
pstree -p
cat /proc/<pid>/status
cat /proc/<pid>/maps
ls -l /proc/<pid>/fd
ls /proc/<pid>/task
strace -f ./program
strace -f -p <pid>
```

---

# 58. Must-Know System Calls / APIs

For interviews and Linux system programming, know the purpose and basic usage of:

```text
fork()
execve()
execl()
execvp()
wait()
waitpid()
_exit()
getpid()
getppid()
kill()
```

Also understand:

```text
clone()
```

because Linux uses the underlying task model to support processes and threads.

---

# 59. Final Mental Model

Keep this complete picture in mind:

```text
                    PROGRAM
                       |
                       | fork()
                       v
                    PROCESS
                       |
        +--------------+--------------+
        |              |              |
        v              v              v
   Address Space    Resources       Threads
        |              |              |
        |              |              +-- CPU state
        |              |              +-- Stack
        |              |
        |              +-- Files
        |              +-- Signals
        |              +-- Credentials
        |
        +-- Text
        +-- Data
        +-- BSS
        +-- Heap
        +-- mmap
        +-- Stack
        |
        v
    Virtual Memory
        |
        v
    Page Tables / MMU
        |
        v
    Physical Memory


Process creation:

    Parent
       |
       | fork()
       +------------------+
       |                  |
       v                  v
    Parent              Child
                           |
                           | exec()
                           v
                      New Program
                           |
                           v
                         exit()
                           |
                           v
                        Zombie
                           |
                           | wait()/waitpid()
                           v
                       Collected


Scheduling:

    Runnable Tasks
          |
          v
      Scheduler
          |
          v
        CPU
          |
          v
     Running Task
          |
          +---- blocks ----> Waiting
          |
          +---- preempt --> Runnable
          |
          +---- exits ----> Terminated
```

---

# Chapter 2 — Key Takeaways

1. A process is a program in execution.
2. A process has an address space, execution state, resources and identity.
3. Linux represents tasks using `task_struct` and related kernel structures.
4. `fork()` creates a new process/task.
5. `fork()` returns the child PID to the parent and `0` to the child.
6. Linux uses Copy-On-Write for efficient process creation.
7. `exec()` replaces the current process image.
8. `fork()` + `exec()` is the classic Unix process-launch pattern.
9. `wait()` / `waitpid()` allow a parent to collect child termination status.
10. A zombie has terminated but has not been collected.
11. An orphan is still running after its original parent has terminated.
12. A process has a virtual address space.
13. Threads within a process generally share that address space.
14. A process can have multiple threads.
15. Processes have PIDs and parent/child relationships.
16. `/proc` provides extremely useful process-inspection information.
17. `strace` is useful for observing system calls.
18. Process state and scheduling state determine when a task can execute.
19. Process creation, memory management, scheduling and file descriptors are tightly connected.
20. The process model is the foundation for understanding Linux system programming and kernel internals.

---

# Next Chapter

## Chapter 3 — Threads

Planned topics:

- What is a thread?
- Process vs thread
- User-level vs kernel-level threads
- Linux task model
- `pthread_create()`
- `pthread_join()`
- Thread lifecycle
- Thread stack
- Thread-local storage
- Shared address space
- Race conditions
- Thread cancellation
- Detached vs joinable threads
- Thread attributes
- `clone()` and Linux thread creation
- Context switching
- Thread scheduling
- Practical C/C++ programs
- Synchronization preview
- Interview questions
- Senior-level questions
- Common interview traps
- Quick revision
