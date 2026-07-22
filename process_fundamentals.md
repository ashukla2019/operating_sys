# C Interview Handbook
# Part 16A - Process Fundamentals

---

# Table of Contents

1. What is a Process?
2. Program vs Process
3. Process Memory Layout
4. Process States
5. Process Control Block (PCB)
6. Context Switching
7. CPU Scheduler
8. Process Lifecycle
9. Zombie Process
10. Orphan Process
11. Daemon Process
12. Process vs Thread
13. Linux Commands
14. Interview Questions
15. Quick Revision

---

# 1. What is a Process?

A **Process** is a **program that is currently executing**.

Think of it as:

```text
Program

↓

Load into RAM

↓

CPU starts executing

↓

Process
```

Example

```text
Executable

ls

↓

Run

↓

Process
```

Every running application is a process.

Examples

```text
Chrome

VS Code

bash

systemd

sshd

nginx
```

---

# 2. Program vs Process

Program

```text
Passive

Stored on Disk

Executable File
```

Example

```text
/bin/ls
```

---

Process

```text
Active

Running in RAM

Uses CPU
```

Example

```bash
ls -l
```

Comparison

| Program | Process |
|----------|----------|
| Stored on disk | Running in memory |
| Passive | Active |
| No CPU usage | Uses CPU |
| One executable | Many processes can run from it |

Example

```text
Google Chrome executable

↓

Open 5 windows

↓

5+ Processes
```

---

# 3. Process Memory Layout

Each process gets its own virtual memory.

```text
High Address
+----------------------+
|      Stack           |
| Local Variables      |
| Function Calls       |
+----------------------+
|                      |
|        ↓             |
|                      |
|        Heap          |
| malloc(), calloc()   |
|        ↑             |
+----------------------+
|        BSS           |
| Uninitialized Global |
+----------------------+
|       Data           |
| Initialized Global   |
+----------------------+
|       Text           |
| Machine Code         |
+----------------------+
Low Address
```

---

## Text Segment

Contains

```text
Compiled Machine Code
```

Example

```c
int add(int a,int b)
{
    return a+b;
}
```

Stored in

```text
Text Segment
```

Usually

```text
Read Only
```

---

## Data Segment

Stores

```text
Initialized Global Variables

Initialized Static Variables
```

Example

```c
int count = 10;
```

---

## BSS Segment

Stores

```text
Uninitialized Globals

Uninitialized Static Variables
```

Example

```c
int value;
```

Automatically initialized to

```text
0
```

---

## Heap

Dynamic memory.

Allocated using

```c
malloc()

calloc()

realloc()
```

Freed using

```c
free()
```

Example

```c
int *arr = malloc(100*sizeof(int));
```

---

## Stack

Stores

- Function calls
- Local variables
- Return addresses
- Parameters

Example

```c
void fun()
{
    int x = 10;
}
```

Variable

```text
x
```

is stored on the stack.

---

# 4. Process States

Linux processes move between states.

```text
        New
         |
         v
      Ready
         |
         v
      Running
      /     \
     /       \
Waiting     Ready
     |
     v
 Terminated
```

---

## New

Process is created.

Example

```text
fork()
```

---

## Ready

Waiting for CPU.

Process has everything except CPU time.

---

## Running

CPU executes instructions.

Only one process per CPU core runs at a time.

---

## Waiting (Blocked)

Waiting for

- Disk
- Keyboard
- Network
- Pipe
- Lock
- Sleep timer

Example

```c
read(fd,...);
```

---

## Terminated

Execution completed.

Resources released after parent collects exit status.

---

# 5. Process Control Block (PCB)

The kernel stores information about every process.

Called

```text
PCB
```

Contains

```text
PID

Registers

Program Counter

Stack Pointer

Scheduling Info

Memory Info

Open Files

Signal Info

Credentials

Process State
```

Think of PCB as the kernel's **record** for the process.

---

# 6. Context Switching

Suppose

```text
CPU

↓

Process A
```

Timer expires.

CPU switches to

```text
Process B
```

Kernel saves

```text
Registers

Program Counter

Stack Pointer
```

Loads

```text
Process B Context
```

Diagram

```text
Process A

↓

Save Context

↓

Scheduler

↓

Load Context

↓

Process B
```

Context switching has overhead because the CPU must save and restore execution state.

---

# 7. CPU Scheduler

Many processes compete for CPU.

Scheduler decides

```text
Who runs?

For how long?
```

Common algorithms

```text
Round Robin

Priority Scheduling

Completely Fair Scheduler (Linux)
```

Linux primarily uses the **Completely Fair Scheduler (CFS)** for normal tasks.

---

# 8. Process Lifecycle

```text
Program

↓

fork()

↓

Child Process

↓

exec()

↓

Running

↓

exit()

↓

Zombie

↓

wait()

↓

Removed
```

This is the complete lifecycle of many Unix processes.

---

# 9. Zombie Process

Definition

A process that has finished execution,

but its parent has **not yet collected** its exit status.

Diagram

```text
Child

↓

exit()

↓

Zombie

↓

Parent calls wait()

↓

Removed
```

Why?

Parent must retrieve

- Exit status
- Resource usage information

Example

```c
wait(NULL);
```

Without `wait()`, zombie processes remain until the parent exits or reaps them.

---

# 10. Orphan Process

Definition

Parent exits first.

Child is still running.

Diagram

```text
Parent

↓

Exit

Child

↓

Running

↓

Adopted by init/systemd
```

Linux reassigns the orphan to **init** (PID 1) or **systemd**, which later reaps it.

---

# 11. Daemon Process

A daemon is a **background service process**.

Examples

```text
systemd

sshd

cron

nginx

dbus-daemon
```

Properties

- No terminal
- Runs in background
- Starts during boot or on demand
- Provides services

Examples

```text
SSH Server

Web Server

Database

Scheduler
```

---

# 12. Process vs Thread

| Process | Thread |
|----------|---------|
| Independent execution unit | Lightweight execution unit |
| Separate virtual address space | Shares process address space |
| Higher creation cost | Lower creation cost |
| IPC needed for communication | Shared memory by default |
| More context-switch overhead | Less context-switch overhead |

---

# 13. Useful Linux Commands

Show running processes

```bash
ps -ef
```

Interactive process viewer

```bash
top
```

Modern process viewer

```bash
htop
```

Find PID

```bash
pidof sshd
```

Kill process

```bash
kill PID
```

Show process tree

```bash
pstree
```

Show memory map

```bash
cat /proc/<pid>/maps
```

Show process status

```bash
cat /proc/<pid>/status
```

---

# 14. Frequently Asked Interview Questions

### Q1. Difference between Program and Process?

Program

```text
Stored executable
```

Process

```text
Running program
```

---

### Q2. What is a Zombie Process?

Finished execution,

waiting for parent to collect exit status.

---

### Q3. What is an Orphan Process?

Parent exits before child.

Child is adopted by init/systemd.

---

### Q4. What is Context Switching?

Saving one process state,

loading another.

---

### Q5. Why is Context Switching expensive?

Saving/restoring CPU state,

scheduler work,

cache/TLB disruption.

---

### Q6. Difference between Heap and Stack?

Stack

```text
Automatic

Fast

Local Variables
```

Heap

```text
Dynamic

Programmer Managed

malloc()/free()
```

---

### Q7. What is a Daemon?

Background service process.

---

# 15. Quick Revision

✓ Process = Running Program

✓ Program = Executable File

✓ Process Memory

```text
Text

↓

Data

↓

BSS

↓

Heap

↓

Stack
```

✓ PCB stores process information.

✓ Scheduler allocates CPU.

✓ Context switch saves/restores execution state.

✓ Zombie = Child exited, waiting for `wait()`.

✓ Orphan = Parent exited first.

✓ Daemon = Background service.

✓ Each process has its own virtual address space.

---

# Interview Memory Trick

```text
Program

↓

Process

↓

Memory

↓

PCB

↓

Scheduler

↓

Running

↓

exit()

↓

Zombie

↓

wait()

↓

Removed
```

---

# Real Linux Example

```text
Terminal

↓

bash

↓

fork()

↓

Child Process

↓

exec("ls")

↓

ls executes

↓

exit()

↓

bash calls wait()

↓

Child removed
```

This sequence occurs every time you execute a simple command in a Unix shell.

---

# Next Part

**Part 16B – Process Creation**

Topics

- fork()
- vfork()
- exec() Family
- wait()
- waitpid()
- exit() vs _exit()
- Copy-on-Write (CoW)
- Parent & Child Process
- Process Tree
- Interview Programs
- -------------------------------------------------------------------------------
# C Interview Handbook
# Part 16B - Process Creation (fork(), exec(), wait())

---

# Table of Contents

1. Process Creation Overview
2. fork()
3. Parent vs Child Process
4. Copy-on-Write (CoW)
5. Process Tree
6. exec() Family
7. fork() + exec() Workflow
8. wait()
9. waitpid()
10. exit() vs _exit()
11. vfork()
12. Common Interview Programs
13. Interview Questions
14. Quick Revision

---

# 1. Process Creation Overview

In Linux, creating a new process usually involves two steps.

```text
Parent Process

↓

fork()

↓

Child Process

↓

exec()

↓

Run New Program
```

Example

```text
Terminal

↓

bash

↓

fork()

↓

Child

↓

exec("ls")

↓

ls Runs
```

---

# 2. fork()

Prototype

```c
#include <unistd.h>

pid_t fork(void);
```

Purpose

Creates a **new child process**.

After a successful `fork()`:

- Parent and child execute independently.
- Both continue from the statement immediately after `fork()`.

---

Example

```c
#include <stdio.h>
#include <unistd.h>

int main()
{
    printf("Before fork\n");

    fork();

    printf("After fork\n");

    return 0;
}
```

Possible Output

```text
Before fork
After fork
After fork
```

Why?

```text
Before fork

↓

One Process

↓

fork()

↓

Parent

+

Child

↓

Both execute printf()
```

---

# 3. Return Value of fork()

```text
fork()

↓

+-----------------------------+
| Parent | Child | Error      |
+---------+-------+------------+
| >0 PID  | 0     | -1         |
+---------+-------+------------+
```

Typical Usage

```c
pid_t pid = fork();

if (pid == 0)
{
    printf("Child\n");
}
else if (pid > 0)
{
    printf("Parent\n");
}
else
{
    perror("fork");
}
```

---

# 4. Parent vs Child Process

After `fork()`

Both processes have

- Same program code
- Same global variables
- Same stack contents
- Same heap contents
- Different PID
- Independent virtual address spaces

Example

```c
int x = 10;

pid_t pid = fork();

if (pid == 0)
{
    x = 20;
}

printf("%d\n", x);
```

Possible Output

```text
Parent

10

Child

20
```

Changing `x` in the child does **not** change the parent's `x`.

---

# 5. Copy-on-Write (CoW)

Question

Does Linux copy the entire process memory immediately?

Answer

**No.**

Linux uses **Copy-on-Write (CoW).**

---

Without CoW

```text
fork()

↓

Copy Entire Memory

↓

Slow
```

---

With CoW

```text
fork()

↓

Parent

↓

Shared Pages

↓

Child

↓

Only when a page is modified

↓

Kernel creates a private copy
```

Memory Diagram

```text
Initially

Parent

↓

Page A

↑

Child

(Both share)

Modify Child

↓

Parent

Page A

Child

Page A Copy
```

Benefits

- Faster `fork()`
- Lower memory usage
- Used heavily by shells and servers

---

# 6. Process Tree

Example

```text
init/systemd

↓

bash

↓

vim
```

Another Example

```text
systemd
|
+-- sshd
|
+-- nginx
|
|   +-- Worker1
|   +-- Worker2
|
+-- mysqld
```

View Process Tree

```bash
pstree
```

---

# 7. exec() Family

Purpose

Replace the current process image with a new program.

Important

```text
exec()

DOES NOT

create a new process.
```

It replaces the existing process.

---

Common Functions

```text
execl()

execlp()

execv()

execvp()

execve()
```

---

Example

```c
#include <unistd.h>

int main()
{
    execl("/bin/ls",
          "ls",
          "-l",
          NULL);

    return 0;
}
```

Process Flow

```text
Child

↓

exec()

↓

Old Program Removed

↓

New Program Loaded

↓

Starts from main()
```

If `exec()` succeeds,

it **never returns**.

If it returns,

an error occurred.

---

# 8. fork() + exec()

This is how shells launch commands.

Example

```text
bash

↓

fork()

↓

Child

↓

exec("ls")

↓

ls executes

↓

exit()

↓

Parent waits
```

Code

```c
pid_t pid = fork();

if (pid == 0)
{
    execl("/bin/ls",
          "ls",
          "-l",
          NULL);

    perror("exec");
    _exit(1);
}

wait(NULL);
```

---

# 9. wait()

Prototype

```c
#include <sys/wait.h>

pid_t wait(int *status);
```

Purpose

Wait until a child process finishes.

Example

```c
wait(NULL);
```

Flow

```text
Parent

↓

wait()

↓

Child exits

↓

Parent resumes
```

Benefits

- Prevents zombie processes
- Retrieves child exit status

---

# 10. waitpid()

Prototype

```c
pid_t waitpid(pid_t pid,
              int *status,
              int options);
```

Advantages

- Wait for a specific child
- Non-blocking mode available
- More control

Example

```c
waitpid(pid,
        NULL,
        0);
```

Non-blocking

```c
waitpid(pid,
        NULL,
        WNOHANG);
```

---

# 11. exit()

Prototype

```c
#include <stdlib.h>

exit(0);
```

Actions

- Flushes stdio buffers
- Calls `atexit()` handlers
- Closes files
- Terminates process

---

# 12. _exit()

Prototype

```c
#include <unistd.h>

_exit(0);
```

Difference

```text
_exit()

↓

Immediately leaves kernel

↓

No stdio flushing

↓

No atexit()
```

Why use `_exit()` after `fork()`?

Suppose

```text
printf("Hello");
```

Output is buffered.

After `fork()`

Both parent and child inherit the buffer.

Calling `exit()` in the child may flush the same buffered output again.

Using `_exit()` avoids this duplicate flush.

---

# 13. vfork()

Prototype

```c
pid_t vfork(void);
```

Purpose

Optimized version of `fork()` for the common pattern:

```text
fork()

↓

exec()
```

Behavior

- Parent is suspended.
- Child shares the parent's address space temporarily.
- Child must quickly call `exec()` or `_exit()`.

Do **not** modify local variables or return from the calling function before `exec()` or `_exit()`.

Modern Linux often makes `fork()` efficient enough because of Copy-on-Write, so `vfork()` is less commonly needed.

---

# 14. Common Interview Programs

## Program 1

```c
printf("A");

fork();

printf("B");
```

Output

```text
ABB
```

Explanation

```text
A

↓

fork()

↓

Parent prints B

Child prints B
```

---

## Program 2

```c
fork();

fork();
```

Number of Processes

```text
4
```

Diagram

```text
P

↓

fork

↓

P

C1

↓

Both fork

↓

P

C1

C2

C3
```

Formula

```text
n forks

↓

2^n processes
```

---

## Program 3

```c
if(fork()==0)
{
    printf("Child");
}
else
{
    printf("Parent");
}
```

Output Order

Not guaranteed.

Scheduling determines which prints first.

---

# 15. Frequently Asked Interview Questions

### Q1. Difference between `fork()` and `exec()`?

| fork() | exec() |
|---------|---------|
| Creates a new process | Replaces current process image |
| Returns twice | Returns only on failure |
| Parent & child continue | Old program disappears |

---

### Q2. Why is Copy-on-Write used?

To avoid copying all memory during `fork()` unless modifications occur.

---

### Q3. Why use `wait()`?

To collect child exit status and prevent zombie processes.

---

### Q4. Difference between `wait()` and `waitpid()`?

`wait()`

```text
Any child
```

`waitpid()`

```text
Specific child

Supports options
```

---

### Q5. Difference between `exit()` and `_exit()`?

| exit() | _exit() |
|----------|----------|
| Flushes stdio | Doesn't flush stdio |
| Runs atexit() handlers | Doesn't run them |
| Library function | System call wrapper |

---

### Q6. Does `exec()` create a new process?

No.

It replaces the current process image.

---

### Q7. Does `fork()` copy all memory?

No.

Linux uses Copy-on-Write.

---

# 16. Quick Revision

✓ `fork()` → Creates child process.

✓ Parent gets child's PID.

✓ Child gets 0.

✓ `exec()` → Replaces current program.

✓ `fork()` + `exec()` → Standard shell pattern.

✓ `wait()` → Wait for child.

✓ `waitpid()` → Wait for a specific child.

✓ Copy-on-Write → Copy pages only when modified.

✓ `_exit()` → Immediate process termination.

✓ `vfork()` → Parent waits until child calls `exec()` or `_exit()`.

---

# Interview Memory Trick

```text
fork()

↓

Child Created

↓

Copy-on-Write

↓

exec()

↓

New Program

↓

exit()

↓

Zombie

↓

wait()

↓

Removed
```

---

# Real Linux Example

```text
User

↓

bash

↓

fork()

↓

Child

↓

exec("/bin/grep")

↓

grep executes

↓

exit()

↓

Parent calls wait()

↓

Child removed
```

This is the workflow used by shells every time you execute a command.

---

# Next Part

**Part 16C – Signals**

Topics

- Signal Basics
- signal()
- sigaction()
- kill()
- raise()
- SIGINT
- SIGTERM
- SIGKILL
- SIGSEGV
- Signal Masking
- Pending Signals
- Real-Time Signals
- Linux Interview Questions
- ---------------------------------------------------------------------
# C Interview Handbook
# Part 16C - Linux Signals

---

# Table of Contents

1. What is a Signal?
2. Why Signals are Needed
3. Signal Flow
4. Common Linux Signals
5. Default Signal Actions
6. signal()
7. sigaction()
8. Sending Signals
9. Signal Masking
10. Pending Signals
11. Signal Safety
12. Real-Time Signals
13. Signal vs IPC
14. Linux Commands
15. Interview Questions
16. Quick Revision

---

# 1. What is a Signal?

A **Signal** is an asynchronous notification sent to a process or thread to notify it that an event has occurred.

Examples

```text
Ctrl + C pressed

↓

SIGINT

Memory access violation

↓

SIGSEGV

Timer expired

↓

SIGALRM

Child exited

↓

SIGCHLD
```

Signals interrupt the normal execution flow.

---

# 2. Why Signals are Needed

Without signals,

a process would have to continuously check for events.

```text
Process

↓

Polling

↓

High CPU Usage
```

With signals

```text
Event Happens

↓

Kernel

↓

Signal

↓

Process Responds
```

Signals provide an event-driven mechanism.

---

# 3. Signal Flow

Example

```text
User presses Ctrl+C

↓

Terminal Driver

↓

Kernel

↓

SIGINT

↓

Process

↓

Default Action

↓

Terminate
```

Another Example

```text
Child exits

↓

Kernel

↓

SIGCHLD

↓

Parent
```

---

# 4. Common Linux Signals

| Signal | Description | Default Action |
|---------|-------------|----------------|
| SIGINT | Ctrl+C | Terminate |
| SIGTERM | Graceful termination | Terminate |
| SIGKILL | Force kill | Terminate |
| SIGSTOP | Stop process | Stop |
| SIGCONT | Continue process | Continue |
| SIGSEGV | Invalid memory access | Terminate + Core Dump |
| SIGABRT | abort() called | Terminate + Core Dump |
| SIGALRM | Alarm timer expired | Terminate |
| SIGCHLD | Child terminated | Ignore (default action is implementation-defined but typically ignored) |
| SIGHUP | Terminal closed/config reload | Terminate |
| SIGPIPE | Write to closed pipe | Terminate |
| SIGUSR1 | User-defined | Terminate |
| SIGUSR2 | User-defined | Terminate |

---

# 5. Signals That Cannot Be Caught

Two signals cannot be blocked, ignored, or handled.

```text
SIGKILL

SIGSTOP
```

Reason

The kernel must always be able to stop or terminate a process.

---

# 6. signal()

Prototype

```c
#include <signal.h>

void (*signal(int sig,
              void (*handler)(int)))(int);
```

Example

```c
#include <stdio.h>
#include <signal.h>

void handler(int sig)
{
    printf("SIGINT Received\n");
}

int main()
{
    signal(SIGINT, handler);

    while (1)
    {
    }
}
```

Press

```text
Ctrl+C
```

Output

```text
SIGINT Received
```

---

Limitations

`signal()` has historical portability differences.

Modern Linux applications should prefer

```text
sigaction()
```

---

# 7. sigaction()

Preferred API.

Prototype

```c
#include <signal.h>

int sigaction(int signum,
              const struct sigaction *act,
              struct sigaction *oldact);
```

Example

```c
#include <signal.h>

void handler(int sig)
{
}

int main()
{
    struct sigaction sa;

    sa.sa_handler = handler;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
}
```

Advantages

- Reliable
- Supports signal masking
- More options
- POSIX standard

---

# 8. Sending Signals

## kill()

Prototype

```c
kill(pid,
     SIGTERM);
```

Example

```bash
kill 1234
```

Default

```text
SIGTERM
```

Force Kill

```bash
kill -9 1234
```

Equivalent

```text
SIGKILL
```

---

## raise()

Send signal to yourself.

```c
raise(SIGINT);
```

Equivalent to

```text
Process

↓

Signal Itself
```

---

## abort()

```c
abort();
```

Generates

```text
SIGABRT
```

Usually produces a core dump.

---

# 9. Signal Masking

Sometimes we don't want a signal immediately.

Example

```text
Critical Section

↓

Don't Interrupt
```

Block signal

```c
sigset_t set;

sigemptyset(&set);

sigaddset(&set, SIGINT);

sigprocmask(SIG_BLOCK,
            &set,
            NULL);
```

Unblock later

```c
sigprocmask(SIG_UNBLOCK,
            &set,
            NULL);
```

---

# 10. Pending Signals

Blocked signals are not immediately delivered.

```text
SIGINT

↓

Blocked

↓

Pending

↓

Unblocked

↓

Delivered
```

Pending signals wait until they can be delivered.

---

# 11. Signal Safety

A signal can interrupt your program at almost any point.

Many library functions are **not** safe inside signal handlers.

Unsafe

```text
printf()

malloc()

free()

sprintf()
```

Safer POSIX function

```text
write()
```

Example

```c
void handler(int sig)
{
    write(1,
          "Signal\n",
          7);
}
```

This is called **async-signal-safe** programming.

---

# 12. Real-Time Signals

POSIX provides real-time signals.

Features

- Queued
- Delivered in order
- Can carry additional data using `sigqueue()`

Unlike standard signals,

multiple identical real-time signals are queued instead of being merged.

---

# 13. Signal vs IPC

| Signal | IPC |
|----------|------|
| Notification | Data Transfer |
| Small information | Large information |
| Very fast | Depends on mechanism |
| No buffer | May have buffers |

Signals

```text
Something happened.
```

Shared Memory

```text
Here is the data.
```

---

# 14. Useful Linux Commands

Show signal list

```bash
kill -l
```

Send SIGTERM

```bash
kill PID
```

Send SIGKILL

```bash
kill -9 PID
```

Send SIGINT

```bash
kill -2 PID
```

Stop process

```bash
kill -STOP PID
```

Resume process

```bash
kill -CONT PID
```

Foreground interrupt

```text
Ctrl+C

↓

SIGINT
```

Suspend terminal job

```text
Ctrl+Z

↓

SIGTSTP
```

---

# 15. Frequently Asked Interview Questions

### Q1. What is a signal?

An asynchronous notification sent to a process or thread.

---

### Q2. Difference between SIGTERM and SIGKILL?

SIGTERM

```text
Graceful

Can Handle
```

SIGKILL

```text
Immediate

Cannot Handle
```

---

### Q3. Difference between signal() and sigaction()?

`sigaction()` is the preferred POSIX interface because it provides reliable and configurable signal handling.

---

### Q4. Which signals cannot be caught?

```text
SIGKILL

SIGSTOP
```

---

### Q5. Why avoid printf() inside signal handlers?

It is not async-signal-safe.

---

### Q6. Difference between raise() and kill()?

`raise()`

```text
Send signal to self
```

`kill()`

```text
Send signal to another process
```

(or to yourself if you specify your own PID).

---

### Q7. Why block signals?

To prevent interruption during critical sections.

---

### Q8. What is SIGCHLD?

Sent to the parent when a child changes state (typically exits or stops).

---

# 16. Quick Revision

✓ Signal = Asynchronous notification.

✓ SIGINT = Ctrl+C.

✓ SIGTERM = Graceful termination.

✓ SIGKILL = Force kill.

✓ SIGSTOP = Stop process.

✓ SIGCONT = Continue process.

✓ SIGSEGV = Invalid memory access.

✓ Use `sigaction()` for new code.

✓ Block signals using `sigprocmask()`.

✓ Avoid non-async-signal-safe functions in handlers.

✓ Real-time signals are queued.

---

# Interview Memory Trick

```text
Ctrl+C

↓

SIGINT

kill

↓

SIGTERM

kill -9

↓

SIGKILL

Signal Handler

↓

sigaction()

Critical Section

↓

Block Signals

Child Exit

↓

SIGCHLD
```

---

# Real Linux Example

```text
bash

↓

Runs Program

↓

User presses Ctrl+C

↓

Kernel

↓

SIGINT

↓

Program Handler

↓

Cleanup

↓

exit()
```

---

# Next Part

**Part 16D – Pipes & FIFOs**

Topics

- pipe()
- FIFO
- mkfifo()
- Anonymous Pipes
- Named Pipes
- Read/Write
- Blocking Behavior
- Producer-Consumer
- Shell Pipelines
- Linux Interview Questions
- ------------------------------------------------------------
# C Interview Handbook
# Part 16D - Pipes & FIFOs (Inter-Process Communication)

---

# Table of Contents

1. What is IPC?
2. What is a Pipe?
3. Anonymous Pipe
4. pipe() System Call
5. Reading & Writing
6. Parent-Child Communication
7. Pipe Buffer
8. Blocking Behavior
9. Half Duplex vs Full Duplex
10. Named Pipes (FIFO)
11. mkfifo()
12. Shell Pipes
13. Pipe vs FIFO
14. Pipe Limitations
15. Real Linux Examples
16. Interview Questions
17. Quick Revision

---

# 1. What is IPC?

IPC stands for

```text
Inter Process Communication
```

Processes have separate virtual memory.

```text
Process A

Memory

XXXXXXXX

Process B

Memory

YYYYYYYY
```

One process **cannot directly access** another process's memory.

To exchange data, Linux provides IPC mechanisms.

Examples

```text
Pipes

FIFO

Shared Memory

Message Queue

Socket

Semaphore

Signals
```

---

# 2. What is a Pipe?

A pipe is a **kernel-managed communication channel**.

It transfers data from one process to another.

Think of it like a water pipe.

```text
Writer

↓

Pipe

↓

Reader
```

Data flows in one direction.

---

# 3. Anonymous Pipe

Anonymous pipes are created using

```c
pipe()
```

Characteristics

- Parent-child communication
- Exists only while processes are running
- Cannot be accessed using a filename

Diagram

```text
Parent

↓

Pipe

↓

Child
```

---

# 4. pipe() System Call

Prototype

```c
#include <unistd.h>

int pipe(int fd[2]);
```

Returns

```text
fd[0]

↓

Read End

fd[1]

↓

Write End
```

Diagram

```text
+-------------------+

fd[1]

↓

Kernel Pipe Buffer

↓

fd[0]

+-------------------+
```

---

Example

```c
#include <unistd.h>
#include <stdio.h>

int main()
{
    int fd[2];

    pipe(fd);

    write(fd[1], "Linux", 5);

    char buf[10];

    read(fd[0], buf, 5);

    buf[5] = '\0';

    printf("%s\n", buf);
}
```

Output

```text
Linux
```

---

# 5. Parent-Child Communication

Most common usage.

```text
Parent

↓

fork()

↓

Child

↓

Both Share Pipe
```

Diagram

```text
Parent

Write

↓

Pipe

↓

Read

Child
```

Example

```c
int fd[2];

pipe(fd);

pid_t pid = fork();

if(pid == 0)
{
    close(fd[1]);

    char buf[100];

    read(fd[0], buf, sizeof(buf));
}
else
{
    close(fd[0]);

    write(fd[1], "Hello", 5);
}
```

Always close the unused end of the pipe.

---

# 6. Pipe Buffer

The kernel stores pipe data in a temporary buffer.

```text
Writer

↓

Pipe Buffer

↓

Reader
```

Memory

```text
+----------------------+

HELLO

+----------------------+
```

The reader consumes the bytes.

---

# 7. Blocking Behavior

### Read

If

```text
Pipe Empty
```

↓

Reader blocks until data arrives.

---

### Write

If

```text
Pipe Full
```

↓

Writer blocks until space becomes available.

Diagram

```text
Writer

↓

Pipe Full

↓

Wait

↓

Reader Reads

↓

Writer Continues
```

Blocking helps synchronize producer and consumer.

---

# 8. EOF on Pipe

Suppose

```text
Writer

↓

Finished

↓

Closed Write End
```

Reader

```text
read()

↓

Returns

0
```

This indicates

```text
End Of File (EOF)
```

---

# 9. Half Duplex vs Full Duplex

Pipe

```text
Parent

------->

Child
```

Only one direction.

This is called

```text
Half Duplex
```

---

To communicate both ways

```text
Parent

<------>

Child
```

Need

```text
Two Pipes
```

One for each direction.

---

# 10. Named Pipe (FIFO)

FIFO means

```text
First In First Out
```

Unlike anonymous pipes,

FIFO exists as a file.

Diagram

```text
Writer

↓

FIFO File

↓

Reader
```

Processes do **not** need to be related.

---

# 11. mkfifo()

Create FIFO.

Prototype

```c
#include <sys/stat.h>

int mkfifo(const char *path,
           mode_t mode);
```

Example

```c
mkfifo("mypipe",
       0666);
```

Shell

```bash
mkfifo mypipe
```

List

```bash
ls -l
```

Output

```text
prw-r--r--
```

The first character

```text
p
```

means FIFO.

---

# 12. Using FIFO

Terminal 1

```bash
echo "Linux IPC" > mypipe
```

Terminal 2

```bash
cat < mypipe
```

Output

```text
Linux IPC
```

Two unrelated processes communicate.

---

# 13. Shell Pipe

Example

```bash
ls -l | grep txt
```

Flow

```text
ls

↓

Pipe

↓

grep
```

Diagram

```text
stdout

↓

Pipe

↓

stdin
```

The shell internally creates a pipe, forks processes, redirects file descriptors, and executes both commands.

---

# 14. Pipe vs FIFO

| Pipe | FIFO |
|------|------|
| Anonymous | Named |
| Parent-child communication | Unrelated processes can communicate |
| Exists in memory | Exists in filesystem |
| Created using `pipe()` | Created using `mkfifo()` |
| Removed automatically | Remains until deleted |

---

# 15. Pipe Limitations

Pipe is

```text
Byte Stream
```

No message boundaries.

---

Cannot

```text
Seek

(lseek())
```

---

Normally

```text
Half Duplex
```

---

Limited kernel buffer size.

---

Requires synchronization for complex communication patterns.

---

# 16. Real Linux Examples

## Shell Pipeline

```bash
ps -ef | grep ssh
```

Flow

```text
ps

↓

Pipe

↓

grep
```

---

## Count Files

```bash
ls | wc -l
```

Flow

```text
ls

↓

Pipe

↓

wc
```

---

## Read Log

```bash
cat log.txt | grep ERROR
```

Flow

```text
cat

↓

Pipe

↓

grep
```

---

# 17. Frequently Asked Interview Questions

### Q1. What is a Pipe?

Kernel mechanism for one-way communication between processes.

---

### Q2. Why use Pipes?

To transfer data between processes.

---

### Q3. Difference between Pipe and FIFO?

Pipe

```text
Anonymous

Parent-Child
```

FIFO

```text
Named

Any Processes
```

---

### Q4. Why close unused pipe ends?

To

- Avoid resource leaks
- Allow EOF detection
- Prevent unnecessary blocking

---

### Q5. Is Pipe Full Duplex?

No.

One pipe is

```text
Half Duplex
```

Two pipes are needed for bidirectional communication.

---

### Q6. What happens if the pipe becomes full?

Writer blocks until space is available (unless the descriptor is configured as non-blocking).

---

### Q7. What happens if the reader reads an empty pipe?

Reader blocks until data arrives or all write ends are closed.

---

### Q8. Why can't unrelated processes use anonymous pipes?

Because anonymous pipe file descriptors are typically inherited through `fork()`. Unrelated processes do not share those descriptors.

---

# 18. Quick Revision

✓ Pipe = One-way IPC.

✓ `pipe()` creates anonymous pipe.

✓ `fd[0]` = Read end.

✓ `fd[1]` = Write end.

✓ Reader blocks on empty pipe.

✓ Writer blocks on full pipe.

✓ Close unused ends.

✓ FIFO = Named pipe.

✓ `mkfifo()` creates FIFO.

✓ Shell `|` uses pipes.

---

# Interview Memory Trick

```text
pipe()

↓

fd[0]

Read

↓

Kernel Buffer

↓

fd[1]

Write

↓

fork()

↓

Parent

↓

Child

↓

Communication
```

---

# Real Linux Workflow

```text
bash

↓

pipe()

↓

fork()

↓

Child 1

exec(ls)

stdout

↓

Pipe

↓

Child 2

exec(grep)

stdin

↓

Output on Terminal
```

This is exactly how commands like:

```bash
ls -l | grep txt
```

work internally.

---

# Next Part

**Part 16E – Shared Memory**

Topics

- Why Shared Memory?
- System V Shared Memory
- POSIX Shared Memory
- shmget()
- shmat()
- shmdt()
- shmctl()
- shm_open()
- mmap()
- Producer-Consumer
- Synchronization Issues
- Linux Interview Questions
- -------------------------------------------------------------------------------------------
# C Interview Handbook
# Part 16E - Shared Memory (Fastest IPC)

---

# Table of Contents

1. What is Shared Memory?
2. Why Shared Memory?
3. How Shared Memory Works
4. Shared Memory vs Pipes
5. System V Shared Memory
6. shmget()
7. shmat()
8. shmdt()
9. shmctl()
10. POSIX Shared Memory
11. shm_open()
12. mmap()
13. Synchronization
14. Producer-Consumer Example
15. Real Linux Examples
16. Interview Questions
17. Quick Revision

---

# 1. What is Shared Memory?

Normally,

each process has its own virtual memory.

```text
Process A

AAAAAAA

Process B

BBBBBBB
```

They cannot directly access each other's memory.

Shared Memory allows multiple processes to map the **same physical memory** into their own virtual address spaces.

```text
Process A

↓

Shared Memory

↑

Process B
```

Both processes can read and write the same data.

---

# 2. Why Shared Memory?

Example using a Pipe

```text
Writer

↓

Kernel Buffer

↓

Reader
```

Data is copied

```text
Process

↓

Kernel

↓

Process
```

Multiple copies reduce performance.

---

Shared Memory

```text
Process A

↓

Shared Memory

↑

Process B
```

Both processes access the same memory directly after it is mapped.

This makes shared memory one of the fastest IPC mechanisms.

---

# 3. How Shared Memory Works

Linux Kernel

```text
Creates Physical Memory

↓

Maps It

↓

Process A

↓

Maps Same Memory

↓

Process B
```

Diagram

```text
Virtual Memory A

↓

Shared Physical Pages

↑

Virtual Memory B
```

Both processes see the same bytes.

---

# 4. Shared Memory vs Pipes

Pipe

```text
Write

↓

Kernel Buffer

↓

Read
```

Shared Memory

```text
Both Read

Both Write

Same Memory
```

Comparison

| Pipe | Shared Memory |
|------|---------------|
| Data copied | No repeated data copying after mapping |
| Slower | Faster |
| Stream | Shared data |
| Simple | Requires synchronization |

---

# 5. System V Shared Memory

Older Unix IPC mechanism.

Functions

```text
shmget()

shmat()

shmdt()

shmctl()
```

Widely available on Unix-like systems.

---

# 6. shmget()

Creates or accesses a shared memory segment.

Prototype

```c
#include <sys/shm.h>

int shmget(key_t key,
           size_t size,
           int shmflg);
```

Example

```c
int shmid;

shmid = shmget(1234,
               1024,
               IPC_CREAT | 0666);
```

Creates

```text
1024 Bytes

Shared Memory
```

---

# 7. shmat()

Attach shared memory.

Prototype

```c
void *shmat(int shmid,
            const void *addr,
            int flag);
```

Example

```c
char *ptr;

ptr = shmat(shmid,
            NULL,
            0);
```

Diagram

```text
Process

↓

Virtual Address

↓

Shared Memory
```

Now

```text
ptr
```

points to the shared memory.

---

# 8. shmdt()

Detach shared memory.

Prototype

```c
int shmdt(const void *addr);
```

Example

```c
shmdt(ptr);
```

Only detaches the mapping.

The shared memory segment may still exist if other processes are attached or until it is marked for deletion.

---

# 9. shmctl()

Control operations.

Prototype

```c
int shmctl(int shmid,
           int cmd,
           struct shmid_ds *buf);
```

Delete segment

```c
shmctl(shmid,
       IPC_RMID,
       NULL);
```

Always remove unused shared memory to avoid resource leaks.

---

# 10. POSIX Shared Memory

Modern POSIX interface.

Functions

```text
shm_open()

ftruncate()

mmap()

munmap()

shm_unlink()
```

Advantages

- Standardized
- File descriptor based
- Common in modern applications

---

# 11. shm_open()

Create or open a shared memory object.

Prototype

```c
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

int shm_open(const char *name,
             int oflag,
             mode_t mode);
```

Example

```c
int fd;

fd = shm_open("/shared",
              O_CREAT | O_RDWR,
              0666);
```

Resize

```c
ftruncate(fd,
          4096);
```

---

# 12. mmap()

Map shared memory into the process.

Prototype

```c
void *mmap(void *addr,
           size_t length,
           int prot,
           int flags,
           int fd,
           off_t offset);
```

Example

```c
char *ptr;

ptr = mmap(NULL,
           4096,
           PROT_READ | PROT_WRITE,
           MAP_SHARED,
           fd,
           0);
```

Diagram

```text
Process

↓

mmap()

↓

Shared Pages

↓

Another Process
```

---

# 13. Synchronization

Problem

Both processes write simultaneously.

```text
Process A

↓

Shared Memory

↑

Process B
```

Possible Result

```text
Race Condition
```

Shared memory **does not provide synchronization**.

Common solutions

```text
Semaphore

Mutex

Condition Variable
```

---

# 14. Producer-Consumer Example

Producer

```text
Create Data

↓

Write Shared Memory

↓

Signal Consumer
```

Consumer

```text
Wait

↓

Read Shared Memory

↓

Process Data
```

Flow

```text
Producer

↓

Shared Memory

↓

Consumer

↓

Semaphore
```

Semaphore coordinates access.

---

# 15. Real Linux Examples

## Database

```text
Client

↓

Shared Memory

↓

Database Server
```

Used to reduce copying and improve throughput.

---

## High-Speed Trading

```text
Process A

↓

Shared Memory

↓

Process B
```

Very low latency communication.

---

## Video Processing

```text
Camera

↓

Shared Memory

↓

Encoder

↓

Display
```

Large frames are shared instead of copied repeatedly.

---

# 16. Shared Memory vs mmap()

System V

```text
shmget()

shmat()
```

POSIX

```text
shm_open()

mmap()
```

File Mapping

```text
mmap(file)
```

Shared Memory

```text
mmap(shared object)
```

`mmap()` is also used for mapping regular files into memory.

---

# 17. Frequently Asked Interview Questions

### Q1. What is Shared Memory?

A region of memory mapped into multiple processes.

---

### Q2. Why is Shared Memory fast?

After mapping, processes access the same physical memory directly instead of repeatedly copying data between processes and the kernel.

---

### Q3. Why are semaphores needed?

Shared memory provides no synchronization.

Semaphores prevent race conditions.

---

### Q4. Difference between Pipe and Shared Memory?

| Pipe | Shared Memory |
|------|---------------|
| Data stream | Shared data |
| Kernel copies data | Direct shared access after mapping |
| Slower | Faster |
| Simple | Requires synchronization |

---

### Q5. Difference between System V and POSIX Shared Memory?

System V

```text
shmget()

shmat()
```

POSIX

```text
shm_open()

mmap()
```

---

### Q6. Can unrelated processes use shared memory?

Yes.

If they use the same shared memory object or key and have appropriate permissions.

---

### Q7. Is shared memory automatically synchronized?

No.

Synchronization must be implemented separately.

---

# 18. Quick Revision

✓ Shared Memory = Fast IPC.

✓ Processes share the same physical pages.

✓ `shmget()` = Create/Get (System V).

✓ `shmat()` = Attach.

✓ `shmdt()` = Detach.

✓ `shmctl()` = Control/Delete.

✓ `shm_open()` = POSIX shared memory.

✓ `mmap()` = Map into address space.

✓ Use semaphores or mutexes for synchronization.

✓ Ideal for large amounts of data.

---

# Interview Memory Trick

```text
Shared Memory

↓

Create

↓

Attach

↓

Read/Write

↓

Synchronize

↓

Detach

↓

Delete
```

---

# Real Linux Workflow

```text
Producer

↓

shm_open()

↓

mmap()

↓

Write Data

↓

Semaphore

↓

Consumer

↓

Read Data

↓

Process

↓

munmap()

↓

shm_unlink()
```

This pattern is commonly used in databases, multimedia applications, high-performance servers, and low-latency systems.

---

# Next Part

**Part 16F – Message Queues**

Topics

- System V Message Queues
- POSIX Message Queues
- msgget()
- msgsnd()
- msgrcv()
- mq_open()
- mq_send()
- mq_receive()
- Message Priorities
- Queue Management
- Linux Interview Questions
- -----------------------------------------------------------------------------------
# C Interview Handbook
# Part 16F - Message Queues (IPC)

---

# Table of Contents

1. What is a Message Queue?
2. Why Message Queues?
3. Message Queue Working
4. Message Queue vs Pipe
5. System V Message Queue
6. msgget()
7. msgsnd()
8. msgrcv()
9. msgctl()
10. POSIX Message Queue
11. mq_open()
12. mq_send()
13. mq_receive()
14. Message Priority
15. Real Linux Examples
16. Comparison
17. Interview Questions
18. Quick Revision

---

# 1. What is a Message Queue?

A Message Queue is a **kernel-managed queue** that stores complete messages.

Instead of reading a continuous stream of bytes,

processes exchange **individual messages**.

Diagram

```text
Producer

↓

+--------------------+
| Message Queue      |
|--------------------|
| Message 1          |
| Message 2          |
| Message 3          |
+--------------------+
        ↓
Consumer
```

---

# 2. Why Message Queues?

Suppose multiple clients send requests.

Without a queue

```text
Client

↓

Server

↓

Busy
```

Requests may be lost or clients may have to wait.

With a queue

```text
Client 1

↓

Client 2

↓

Client 3

↓

Kernel Queue

↓

Server
```

The queue buffers requests until the server processes them.

---

# 3. How Message Queues Work

Producer

```text
Create Message

↓

Send

↓

Kernel Queue

↓

Receive

↓

Consumer
```

Unlike a pipe,

messages remain **separate**.

Example

```text
Hello

World
```

Two independent messages.

---

# 4. Message Queue vs Pipe

Pipe

```text
ABCDEF
```

Byte stream.

No message boundaries.

---

Message Queue

```text
Message 1

Message 2

Message 3
```

Messages remain separate.

Comparison

| Pipe | Message Queue |
|------|---------------|
| Byte stream | Message based |
| FIFO only | Can receive by type/priority (API dependent) |
| Simple | More flexible |
| Usually parent-child | Unrelated processes can communicate |

---

# 5. System V Message Queue

Traditional Unix IPC.

Functions

```text
msgget()

msgsnd()

msgrcv()

msgctl()
```

---

Message Structure

```c
struct msgbuf
{
    long mtype;

    char mtext[100];
};
```

Every message has

- Message Type
- Message Data

---

# 6. msgget()

Create or open a queue.

Prototype

```c
#include <sys/msg.h>

int msgget(key_t key,
           int msgflg);
```

Example

```c
int msqid;

msqid = msgget(1234,
               IPC_CREAT | 0666);
```

Creates

```text
Kernel Message Queue
```

---

# 7. msgsnd()

Send a message.

Prototype

```c
int msgsnd(int msqid,
           const void *msgp,
           size_t size,
           int flag);
```

Example

```c
struct msgbuf msg;

msg.mtype = 1;

strcpy(msg.mtext,
       "Linux");

msgsnd(msqid,
       &msg,
       sizeof(msg.mtext),
       0);
```

Flow

```text
Process

↓

Kernel Queue
```

---

# 8. msgrcv()

Receive a message.

Prototype

```c
ssize_t msgrcv(int msqid,
               void *msgp,
               size_t size,
               long type,
               int flag);
```

Example

```c
msgrcv(msqid,
       &msg,
       sizeof(msg.mtext),
       1,
       0);
```

Output

```text
Linux
```

---

# 9. msgctl()

Control queue.

Prototype

```c
int msgctl(int msqid,
           int cmd,
           struct msqid_ds *buf);
```

Delete queue

```c
msgctl(msqid,
       IPC_RMID,
       NULL);
```

Always remove queues when finished.

---

# 10. POSIX Message Queue

Modern POSIX API.

Functions

```text
mq_open()

mq_send()

mq_receive()

mq_close()

mq_unlink()
```

Advantages

- Standardized
- Supports priorities
- File descriptor-like interface

---

# 11. mq_open()

Create or open a queue.

Prototype

```c
#include <mqueue.h>

mqd_t mq_open(const char *name,
              int oflag,
              ...);
```

Example

```c
mqd_t mq;

mq = mq_open("/queue",
             O_CREAT | O_RDWR,
             0666,
             NULL);
```

---

# 12. mq_send()

Send a message.

Prototype

```c
int mq_send(mqd_t mq,
            const char *msg,
            size_t len,
            unsigned priority);
```

Example

```c
mq_send(mq,
        "Hello",
        5,
        1);
```

---

# 13. mq_receive()

Receive a message.

Prototype

```c
ssize_t mq_receive(mqd_t mq,
                   char *buf,
                   size_t len,
                   unsigned *priority);
```

Example

```c
mq_receive(mq,
           buffer,
           sizeof(buffer),
           NULL);
```

---

# 14. Message Priority

POSIX queues support priorities.

Example

```text
Priority 5

↓

Priority 3

↓

Priority 1
```

Higher-priority messages are delivered before lower-priority ones.

This is useful for

- Real-time systems
- Embedded software
- Critical events

---

# 15. Blocking Behavior

Queue Empty

```text
Receiver

↓

Blocks
```

Queue Full

```text
Sender

↓

Blocks
```

Non-blocking mode

```text
O_NONBLOCK
```

returns immediately with an error instead of waiting.

---

# 16. Real Linux Examples

Job Scheduler

```text
Client

↓

Queue

↓

Worker
```

---

Printer

```text
Applications

↓

Print Queue

↓

Printer
```

---

Embedded System

```text
Sensor

↓

Queue

↓

Controller
```

---

Microservices

```text
Service A

↓

Queue

↓

Service B
```

The same concept is used by systems like RabbitMQ, ActiveMQ, and cloud messaging services.

---

# 17. Comparison

| IPC | Message Boundary | Speed | Best Use |
|-----|------------------|-------|----------|
| Pipe | No | Fast | Parent-child streaming |
| FIFO | No | Fast | Unrelated processes |
| Shared Memory | N/A | Very Fast | Large data |
| Message Queue | Yes | Fast | Structured communication |

---

# 18. Frequently Asked Interview Questions

### Q1. What is a Message Queue?

Kernel-managed queue for exchanging complete messages between processes.

---

### Q2. Difference between Pipe and Message Queue?

Pipe

```text
Byte Stream
```

Message Queue

```text
Message Based
```

---

### Q3. Why use Message Queues?

- Preserve message boundaries
- Decouple sender and receiver
- Support asynchronous communication

---

### Q4. Difference between System V and POSIX Message Queues?

System V

```text
msgget()

msgsnd()

msgrcv()
```

POSIX

```text
mq_open()

mq_send()

mq_receive()
```

---

### Q5. Can unrelated processes communicate?

Yes.

If they open the same queue and have permission.

---

### Q6. Why use priorities?

To process important messages before less important ones.

---

### Q7. Are Message Queues faster than Shared Memory?

No.

Shared memory is generally faster for large data because processes access the same memory directly.

Message queues provide convenience and message management.

---

# 19. Quick Revision

✓ Message Queue = Message-based IPC.

✓ Messages remain separate.

✓ `msgget()` = Create/Open.

✓ `msgsnd()` = Send.

✓ `msgrcv()` = Receive.

✓ `msgctl()` = Control/Delete.

✓ `mq_open()` = POSIX queue.

✓ `mq_send()` = Send.

✓ `mq_receive()` = Receive.

✓ Supports priorities.

✓ Suitable for asynchronous communication.

---

# Interview Memory Trick

```text
Create Queue

↓

Send Message

↓

Kernel Stores

↓

Receive Message

↓

Delete Queue
```

---

# Real Linux Workflow

```text
Client Process

↓

mq_open()

↓

mq_send()

↓

Kernel Queue

↓

mq_receive()

↓

Server Process

↓

Process Request

↓

mq_close()

↓

mq_unlink()
```

---

# Next Part

**Part 16G – Semaphores & Synchronization**

Topics

- Race Condition
- Critical Section
- Binary Semaphore
- Counting Semaphore
- POSIX Semaphores
- System V Semaphores
- sem_init()
- sem_wait()
- sem_post()
- Producer-Consumer
- Reader-Writer
- Dining Philosophers
- Linux Interview Questions
- ------------------------------------------------------------------
# C Interview Handbook
# Part 16G - Semaphores & Synchronization

---

# Table of Contents

1. Why Synchronization?
2. Race Condition
3. Critical Section
4. What is a Semaphore?
5. Binary Semaphore
6. Counting Semaphore
7. POSIX Semaphores
8. sem_init()
9. sem_wait()
10. sem_post()
11. sem_destroy()
12. Producer-Consumer Problem
13. Reader-Writer Problem
14. Dining Philosophers
15. Deadlock
16. Semaphore vs Mutex
17. Linux Examples
18. Interview Questions
19. Quick Revision

---

# 1. Why Synchronization?

Consider two processes updating the same shared variable.

```text
Shared Counter = 100
```

Process A

```text
Read 100
```

Process B

```text
Read 100
```

Process A

```text
Write 101
```

Process B

```text
Write 101
```

Expected

```text
102
```

Actual

```text
101
```

This is incorrect because both processes accessed the shared data at the same time.

---

# 2. Race Condition

A **Race Condition** occurs when multiple execution units access shared data simultaneously and the final result depends on timing.

Diagram

```text
Process A

↓

Shared Data

↑

Process B
```

Without synchronization,

results become unpredictable.

---

# 3. Critical Section

A **Critical Section** is the part of code that accesses shared resources.

Example

```c
counter++;
```

Only one process or thread should execute this code at a time.

Diagram

```text
Process A

↓

Critical Section

↑

Process B

↓

Wait
```

---

# 4. What is a Semaphore?

A semaphore is an integer maintained by the operating system that controls access to shared resources.

Think of it as a traffic signal.

```text
Semaphore = 1

↓

Enter

↓

Semaphore = 0

↓

Others Wait
```

Semaphores are used for

- Synchronization
- Mutual exclusion
- Resource counting

---

# 5. Binary Semaphore

Only two values.

```text
0

1
```

Diagram

```text
Semaphore

↓

1

↓

Process Enters

↓

0

↓

Others Wait
```

Similar to a mutex, although semaphores and mutexes have different ownership semantics.

---

# 6. Counting Semaphore

Stores the number of available resources.

Example

```text
Printer Pool

↓

3 Printers
```

Semaphore

```text
3

↓

2

↓

1

↓

0
```

When it reaches

```text
0
```

new processes must wait.

---

# 7. POSIX Semaphores

Header

```c
#include <semaphore.h>
```

Important APIs

```text
sem_init()

sem_wait()

sem_post()

sem_destroy()
```

---

# 8. sem_init()

Initialize semaphore.

Prototype

```c
int sem_init(sem_t *sem,
             int pshared,
             unsigned value);
```

Example

```c
sem_t sem;

sem_init(&sem,
         0,
         1);
```

Meaning

```text
Semaphore

↓

Value = 1
```

The second parameter (`pshared`) determines whether the semaphore is shared between threads (`0`) or between processes (non-zero with shared memory).

---

# 9. sem_wait()

Acquire semaphore.

Prototype

```c
int sem_wait(sem_t *sem);
```

Flow

```text
Value = 1

↓

sem_wait()

↓

Value = 0

↓

Enter Critical Section
```

If the value is already zero,

the caller blocks until another process or thread releases the semaphore.

---

# 10. sem_post()

Release semaphore.

Prototype

```c
int sem_post(sem_t *sem);
```

Flow

```text
Critical Section Done

↓

sem_post()

↓

Value++

↓

Next Process Runs
```

---

# 11. sem_destroy()

Destroy semaphore.

Prototype

```c
int sem_destroy(sem_t *sem);
```

Example

```c
sem_destroy(&sem);
```

Destroy only after all users have finished with the semaphore.

---

# 12. Complete Example

```c
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

sem_t sem;

void *worker(void *arg)
{
    sem_wait(&sem);

    printf("Critical Section\n");

    sem_post(&sem);

    return NULL;
}

int main()
{
    pthread_t t1, t2;

    sem_init(&sem, 0, 1);

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    sem_destroy(&sem);

    return 0;
}
```

Output

```text
Critical Section

Critical Section
```

Only one thread enters the critical section at a time.

---

# 13. Producer-Consumer Problem

Producer

```text
Create Item

↓

Buffer
```

Consumer

```text
Read Item
```

Need

```text
Semaphore

+

Mutex
```

Diagram

```text
Producer

↓

Buffer

↓

Consumer

↑

Semaphore
```

Semaphores track empty/full slots.

Mutex protects the buffer while it is being modified.

---

# 14. Reader-Writer Problem

Readers

```text
Read Together
```

Writer

```text
Exclusive Access
```

Diagram

```text
Reader

↓

Shared Data

↑

Reader

↓

Writer Waits
```

When a writer enters,

no readers or other writers are allowed.

---

# 15. Dining Philosophers

Problem

```text
5 Philosophers

5 Forks
```

Each philosopher needs two forks to eat.

Diagram

```text
P1 -- F1 -- P2 -- F2 -- P3
```

Incorrect synchronization can cause

- Deadlock
- Starvation

This classic problem is used to teach synchronization strategies.

---

# 16. Deadlock

Definition

Processes wait forever for each other.

Diagram

```text
Process A

↓

Needs Lock B

↑

Process B

↓

Needs Lock A
```

Neither can continue.

---

Four Necessary Conditions

```text
Mutual Exclusion

Hold and Wait

No Preemption

Circular Wait
```

Breaking any one of these conditions prevents deadlock.

---

# 17. Semaphore vs Mutex

| Semaphore | Mutex |
|-----------|-------|
| Counter | Lock |
| Multiple resources | Single owner |
| Ownership not required | Must be unlocked by the owner |
| Used for synchronization and resource counting | Used primarily for mutual exclusion |

---

# 18. Real Linux Examples

Printer Pool

```text
Semaphore = 5

↓

Five Users

↓

No More Printers

↓

Wait
```

---

Database Connection Pool

```text
20 Connections

↓

Semaphore = 20
```

Every acquired connection decrements the count.

Returning the connection increments it.

---

Network Server

```text
Incoming Clients

↓

Semaphore

↓

Maximum Connections
```

Limits concurrent client handling.

---

# 19. Frequently Asked Interview Questions

### Q1. What is a semaphore?

A synchronization primitive used to control access to shared resources.

---

### Q2. Difference between Binary and Counting Semaphore?

Binary

```text
0 or 1
```

Counting

```text
0 to N
```

---

### Q3. Difference between Semaphore and Mutex?

Mutex has ownership.

Semaphore does not.

---

### Q4. What is a Critical Section?

Code that accesses shared data.

---

### Q5. What is a Race Condition?

Unexpected behavior caused by unsynchronized concurrent access to shared data.

---

### Q6. What is Deadlock?

Two or more execution units wait forever for each other.

---

### Q7. Why use sem_wait()?

To acquire a semaphore before entering a protected section.

---

### Q8. Why use sem_post()?

To release the semaphore and allow another waiting execution unit to continue.

---

# 20. Quick Revision

✓ Semaphore controls shared resources.

✓ Binary Semaphore = 0 or 1.

✓ Counting Semaphore = Multiple resources.

✓ `sem_init()` initializes.

✓ `sem_wait()` acquires.

✓ `sem_post()` releases.

✓ `sem_destroy()` destroys.

✓ Protect critical sections.

✓ Prevent race conditions.

✓ Deadlock is a synchronization hazard.

---

# Interview Memory Trick

```text
Shared Resource

↓

Semaphore

↓

sem_wait()

↓

Critical Section

↓

sem_post()

↓

Next Process
```

---

# Real Linux Workflow

```text
Process A

↓

sem_wait()

↓

Access Shared Memory

↓

Update Data

↓

sem_post()

↓

Process B Continues
```

This pattern is widely used in databases, operating systems, networking software, storage systems, and multithreaded applications.

---

# Next Part

**Part 16H – Mutexes & Condition Variables**

Topics

- pthread_mutex_t
- pthread_mutex_lock()
- pthread_mutex_unlock()
- Recursive Mutex
- Condition Variables
- pthread_cond_wait()
- pthread_cond_signal()
- pthread_cond_broadcast()
- Deadlock
- Livelock
- Starvation
- Linux Interview Questions
- ------------------------------------------------------------------------------
# C Interview Handbook
# Part 16H - Mutexes & Condition Variables

---

# Table of Contents

1. Why Mutex?
2. What is a Mutex?
3. Mutex Workflow
4. POSIX Mutex APIs
5. Recursive Mutex
6. Condition Variables
7. pthread_cond_wait()
8. pthread_cond_signal()
9. pthread_cond_broadcast()
10. Producer-Consumer using Mutex + Condition Variable
11. Deadlock
12. Livelock
13. Starvation
14. Semaphore vs Mutex vs Condition Variable
15. Real Linux Examples
16. Interview Questions
17. Quick Revision

---

# 1. Why Mutex?

Suppose two threads increment the same variable.

```c
counter++;
```

Initial value

```text
counter = 100
```

Thread A

```text
Read 100
```

Thread B

```text
Read 100
```

Thread A

```text
Write 101
```

Thread B

```text
Write 101
```

Expected

```text
102
```

Actual

```text
101
```

This happens because both threads execute simultaneously.

A **Mutex** prevents this.

---

# 2. What is a Mutex?

Mutex means

```text
Mutual Exclusion
```

Only **one thread** can own the mutex at a time.

Diagram

```text
Thread A

↓

Lock Mutex

↓

Critical Section

↓

Unlock Mutex

↓

Thread B Runs
```

Think of it as

```text
One Room

One Key

Only one person can enter.
```

---

# 3. Mutex Workflow

```text
Lock

↓

Critical Section

↓

Unlock
```

Example

```text
Thread 1

↓

pthread_mutex_lock()

↓

Update Shared Data

↓

pthread_mutex_unlock()
```

---

# 4. POSIX Mutex APIs

Header

```c
#include <pthread.h>
```

Functions

```text
pthread_mutex_init()

pthread_mutex_lock()

pthread_mutex_unlock()

pthread_mutex_destroy()
```

---

## Initialize

```c
pthread_mutex_t lock;

pthread_mutex_init(&lock, NULL);
```

---

## Lock

```c
pthread_mutex_lock(&lock);
```

If another thread owns the mutex,

the caller waits.

---

## Unlock

```c
pthread_mutex_unlock(&lock);
```

Allows another waiting thread to acquire the mutex.

---

## Destroy

```c
pthread_mutex_destroy(&lock);
```

Destroy after all threads have finished using it.

---

# 5. Complete Example

```c
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t lock;
int counter = 0;

void *worker(void *arg)
{
    pthread_mutex_lock(&lock);

    counter++;

    pthread_mutex_unlock(&lock);

    return NULL;
}

int main()
{
    pthread_t t1, t2;

    pthread_mutex_init(&lock, NULL);

    pthread_create(&t1, NULL, worker, NULL);
    pthread_create(&t2, NULL, worker, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("%d\n", counter);

    pthread_mutex_destroy(&lock);

    return 0;
}
```

Output

```text
2
```

---

# 6. Recursive Mutex

Normally,

locking the same mutex twice by the same thread causes a deadlock.

Example

```text
Thread

↓

Lock

↓

Lock Again

↓

Deadlock
```

Recursive mutex allows

```text
Same Thread

↓

Multiple Locks

↓

Unlock Same Number Of Times
```

Useful in some recursive functions, but should be used only when necessary.

---

# 7. Condition Variables

A condition variable allows a thread to **sleep until a condition becomes true**.

Without it

```text
while(flag == 0)
{
    // Busy Waiting
}
```

CPU usage

```text
High
```

With a condition variable

```text
Sleep

↓

Signal Received

↓

Wake Up
```

---

# 8. POSIX Condition Variable APIs

Header

```c
#include <pthread.h>
```

Functions

```text
pthread_cond_init()

pthread_cond_wait()

pthread_cond_signal()

pthread_cond_broadcast()

pthread_cond_destroy()
```

---

# 9. pthread_cond_wait()

Prototype

```c
pthread_cond_wait(&cond,
                  &lock);
```

Behavior

```text
Unlock Mutex

↓

Sleep

↓

Signal Arrives

↓

Wake Up

↓

Automatically Lock Mutex Again
```

This avoids race conditions while waiting.

---

# 10. pthread_cond_signal()

Wake one waiting thread.

```c
pthread_cond_signal(&cond);
```

Diagram

```text
Thread A Waiting

↓

Signal

↓

Thread A Continues
```

---

# 11. pthread_cond_broadcast()

Wake **all** waiting threads.

```c
pthread_cond_broadcast(&cond);
```

Diagram

```text
Waiting Threads

↓

Broadcast

↓

All Wake Up
```

---

# 12. Producer-Consumer

Producer

```text
Create Item

↓

Buffer

↓

Signal Consumer
```

Consumer

```text
Wait

↓

Read Item
```

Diagram

```text
Producer

↓

Mutex

↓

Buffer

↓

Condition Variable

↓

Consumer
```

Pseudo-code

```text
Producer

Lock

↓

Insert Item

↓

Signal

↓

Unlock
```

Consumer

```text
Lock

↓

Wait If Empty

↓

Read Item

↓

Unlock
```

---

# 13. Deadlock

Definition

Two or more threads wait forever.

Example

```text
Thread A

Lock A

↓

Needs Lock B

Thread B

Lock B

↓

Needs Lock A
```

Neither thread proceeds.

---

## Preventing Deadlock

Always lock resources in the same order.

Example

```text
Correct

Lock A

↓

Lock B

↓

Unlock B

↓

Unlock A
```

---

# 14. Livelock

Threads are active,

but no useful work is completed.

Example

```text
Thread A

↓

Moves Aside

↓

Thread B

↓

Moves Aside

↓

Repeat Forever
```

Unlike deadlock,

threads are **running**.

---

# 15. Starvation

A thread waits indefinitely because other threads keep getting access first.

Example

```text
High Priority Thread

↓

Always Runs

↓

Low Priority Thread

↓

Never Gets CPU
```

---

# 16. Semaphore vs Mutex vs Condition Variable

| Feature | Semaphore | Mutex | Condition Variable |
|----------|-----------|-------|--------------------|
| Ownership | No | Yes | No |
| Counter | Yes | No | No |
| Mutual Exclusion | Yes | Yes | No |
| Signaling | Limited | No | Yes |
| Sleep/Wakeup | No | No | Yes |

---

# 17. Real Linux Examples

## Database

```text
Multiple Threads

↓

Mutex

↓

Shared Cache
```

---

## Web Server

```text
Worker Threads

↓

Condition Variable

↓

New Request
```

Workers sleep until requests arrive.

---

## Network Driver

```text
Interrupt

↓

Signal Condition

↓

Worker Thread

↓

Process Packet
```

---

## Producer-Consumer Queue

```text
Producer

↓

Queue

↓

Condition Variable

↓

Consumer
```

---

# 18. Frequently Asked Interview Questions

### Q1. What is a Mutex?

A synchronization object that allows only one thread to access a critical section at a time.

---

### Q2. Difference between Semaphore and Mutex?

Mutex

```text
Ownership

Only owner unlocks
```

Semaphore

```text
Counter

No ownership
```

---

### Q3. What is a Condition Variable?

A mechanism that allows threads to wait until a specific condition becomes true.

---

### Q4. Why use pthread_cond_wait()?

To sleep efficiently instead of busy waiting.

---

### Q5. Difference between signal() and broadcast()?

`pthread_cond_signal()`

```text
Wake One Thread
```

`pthread_cond_broadcast()`

```text
Wake All Threads
```

---

### Q6. What is Deadlock?

Threads wait forever because each is waiting for a resource held by another.

---

### Q7. What is Livelock?

Threads continue executing but never make progress.

---

### Q8. What is Starvation?

A thread never gets the resource or CPU because others are continually favored.

---

# 19. Quick Revision

✓ Mutex = Mutual Exclusion.

✓ Only owner unlocks mutex.

✓ Lock → Critical Section → Unlock.

✓ Condition Variable = Sleep until event occurs.

✓ `pthread_cond_wait()` releases the mutex while waiting and reacquires it before returning.

✓ `pthread_cond_signal()` wakes one thread.

✓ `pthread_cond_broadcast()` wakes all threads.

✓ Deadlock = Waiting forever.

✓ Livelock = Running without progress.

✓ Starvation = Never gets a chance to run.

---

# Interview Memory Trick

```text
Mutex

↓

Lock

↓

Critical Section

↓

Unlock

↓

Condition Variable

↓

Wait

↓

Signal

↓

Wake Up
```

---

# Real Linux Workflow

```text
Worker Thread

↓

Lock Queue

↓

Queue Empty?

↓

Yes

↓

pthread_cond_wait()

↓

Producer Adds Task

↓

pthread_cond_signal()

↓

Worker Wakes

↓

Process Task
```

This pattern is widely used in web servers, thread pools, databases, networking software, and operating systems.

---

# Next Part

**Part 16I – mmap() & Advanced IPC**

Topics

- mmap()
- munmap()
- Anonymous Mapping
- File Mapping
- MAP_SHARED
- MAP_PRIVATE
- Memory-Mapped Files
- Zero-Copy
- Huge Pages
- Performance Optimizations
- Linux Interview Questions
- ---------------------------------------------------------------------------------------------
# C Interview Handbook
# Part 16I - mmap() & Advanced IPC

---

# Table of Contents

1. What is mmap()?
2. Why mmap()?
3. Virtual Memory Mapping
4. mmap() Prototype
5. Anonymous Mapping
6. File Mapping
7. MAP_SHARED vs MAP_PRIVATE
8. munmap()
9. Memory-Mapped Files
10. mmap() vs read()/write()
11. Zero-Copy
12. Huge Pages
13. mmap() for Shared Memory IPC
14. Real Linux Examples
15. Interview Questions
16. Quick Revision

---

# 1. What is mmap()?

`mmap()` maps a file or anonymous memory into a process's virtual address space.

Instead of reading a file into a buffer,

the file becomes part of the process memory.

```text
Disk File

↓

mmap()

↓

Virtual Memory

↓

Pointer
```

You can access file contents like a normal array.

---

# 2. Why mmap()?

Traditional file access

```text
read()

↓

Kernel Buffer

↓

User Buffer
```

Data is copied.

With mmap()

```text
Disk

↓

Page Cache

↓

Virtual Memory Mapping

↓

Application
```

The application accesses mapped memory directly through virtual addresses.

Advantages

- Faster for many workloads
- Less copying
- Random access
- Large file support

---

# 3. Virtual Memory Mapping

Without mmap()

```text
Application

↓

read()

↓

Kernel Buffer

↓

User Buffer
```

With mmap()

```text
Application

↓

Virtual Address

↓

Mapped Pages

↓

File
```

The kernel loads pages on demand.

---

# 4. mmap() Prototype

```c
#include <sys/mman.h>

void *mmap(
    void *addr,
    size_t length,
    int prot,
    int flags,
    int fd,
    off_t offset
);
```

Return

```text
Success

↓

Pointer

Failure

↓

MAP_FAILED
```

---

# 5. Parameters

### addr

Preferred virtual address.

Usually

```c
NULL
```

Kernel chooses the address.

---

### length

```text
Number of bytes
```

to map.

---

### prot

Memory protection.

```text
PROT_READ

PROT_WRITE

PROT_EXEC

PROT_NONE
```

---

### flags

Mapping type.

```text
MAP_SHARED

MAP_PRIVATE

MAP_ANONYMOUS
```

---

### fd

File descriptor.

---

### offset

Starting position inside the file.

Must typically be page-aligned.

---

# 6. Anonymous Mapping

No file involved.

Example

```c
char *ptr = mmap(
    NULL,
    4096,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS,
    -1,
    0
);
```

Diagram

```text
Process

↓

Anonymous Pages

↓

RAM
```

Useful for

- Dynamic memory
- Shared memory
- Large allocations

---

# 7. File Mapping

Example

```text
notes.txt

↓

mmap()

↓

char *
```

Access

```c
printf("%c", ptr[0]);
```

Instead of

```c
read()
```

you simply dereference the pointer.

---

# 8. MAP_SHARED vs MAP_PRIVATE

## MAP_SHARED

Changes are visible to other mappings of the same object.

```text
Process A

↓

Shared Mapping

↑

Process B
```

Writes may be propagated to the underlying file.

---

## MAP_PRIVATE

Uses Copy-on-Write.

Diagram

```text
File

↓

Private Mapping

↓

Modify

↓

Private Copy
```

Changes are **not** written back to the file and are not visible to other processes mapping the same file.

---

# 9. munmap()

Remove mapping.

Prototype

```c
int munmap(void *addr,
           size_t length);
```

Example

```c
munmap(ptr,
       4096);
```

Always unmap memory when finished.

---

# 10. Memory-Mapped Files

Instead of

```text
read()

↓

Process Buffer
```

Use

```text
mmap()

↓

Pointer

↓

Memory Access
```

Large databases frequently use memory-mapped files.

---

# 11. mmap() vs read()

| read() | mmap() |
|----------|---------|
| Explicit copy to user buffer | Memory mapping |
| Sequential access | Random access |
| Simpler | Better for large/random access files |
| Repeated system calls | One mapping, then normal memory accesses |

---

# 12. Zero-Copy

Traditional

```text
Disk

↓

Kernel Buffer

↓

User Buffer
```

Multiple copies.

Zero-copy idea

```text
Disk

↓

Page Cache

↓

Mapped Memory
```

Less copying means

- Better performance
- Lower CPU usage

Note: `mmap()` reduces copying but the exact data path depends on the OS and storage stack.

---

# 13. Huge Pages

Normally

```text
Page Size

↓

4 KB
```

Huge pages

```text
2 MB

1 GB
```

Benefits

- Fewer page table entries
- Fewer TLB misses
- Better performance for large memory workloads

Commonly used in

- Databases
- Virtual Machines
- High-performance computing

---

# 14. mmap() for IPC

Two processes

```text
Process A

↓

MAP_SHARED

↓

Shared Pages

↑

Process B
```

Both access the same mapped memory.

Synchronization is still required.

Typically

```text
Semaphore

Mutex
```

---

# 15. Real Linux Examples

## Database

```text
Database File

↓

mmap()

↓

Query Engine
```

---

## Web Browser

```text
Cache File

↓

Memory Mapping
```

---

## Video Player

```text
Large Video File

↓

mmap()

↓

Decoder
```

---

## Shared Cache

```text
Process A

↓

Shared Mapping

↑

Process B
```

---

# 16. Frequently Asked Interview Questions

### Q1. What is mmap()?

Maps a file or anonymous memory into a process's virtual address space.

---

### Q2. Why use mmap()?

For efficient file access and shared memory.

---

### Q3. Difference between MAP_SHARED and MAP_PRIVATE?

MAP_SHARED

```text
Changes Visible
```

MAP_PRIVATE

```text
Copy-on-Write
```

---

### Q4. Why use munmap()?

To release the mapping when finished.

---

### Q5. Is mmap() faster than read()?

Often yes for large files or random access patterns, but the best choice depends on the workload.

---

### Q6. Why use Huge Pages?

Reduce page-table overhead and TLB misses.

---

### Q7. Can mmap() be used for IPC?

Yes.

With shared mappings (`MAP_SHARED`) and proper synchronization.

---

# 17. Quick Revision

✓ `mmap()` maps memory.

✓ `munmap()` removes mapping.

✓ Anonymous mapping → No file.

✓ File mapping → Maps a file.

✓ `MAP_SHARED` → Shared changes.

✓ `MAP_PRIVATE` → Copy-on-Write.

✓ Supports memory-mapped files.

✓ Reduces copying for many workloads.

✓ Supports IPC.

✓ Huge pages improve performance for large memory usage.

---

# Interview Memory Trick

```text
File

↓

mmap()

↓

Virtual Memory

↓

Pointer

↓

Read / Write

↓

munmap()
```

---

# Real Linux Workflow

```text
open()

↓

File Descriptor

↓

mmap()

↓

Pointer

↓

Read/Write Like Array

↓

munmap()

↓

close()
```

---

# Next Part

**Part 16J – IPC Comparison & Complete Linux IPC Guide**

Topics

- Pipes
- FIFO
- Shared Memory
- Message Queue
- Semaphore
- Mutex
- Condition Variable
- Signals
- Socket
- Unix Domain Socket
- eventfd
- futex
- Complete IPC Comparison Table
- Interview Decision Guide
- -------------------------------------------------------
# C Interview Handbook
# Part 16I - mmap() & Advanced IPC

---

# Table of Contents

1. What is mmap()?
2. Why mmap()?
3. Virtual Memory Mapping
4. mmap() Prototype
5. Anonymous Mapping
6. File Mapping
7. MAP_SHARED vs MAP_PRIVATE
8. munmap()
9. Memory-Mapped Files
10. mmap() vs read()/write()
11. Zero-Copy
12. Huge Pages
13. mmap() for Shared Memory IPC
14. Real Linux Examples
15. Interview Questions
16. Quick Revision

---

# 1. What is mmap()?

`mmap()` maps a file or anonymous memory into a process's virtual address space.

Instead of reading a file into a buffer,

the file becomes part of the process memory.

```text
Disk File

↓

mmap()

↓

Virtual Memory

↓

Pointer
```

You can access file contents like a normal array.

---

# 2. Why mmap()?

Traditional file access

```text
read()

↓

Kernel Buffer

↓

User Buffer
```

Data is copied.

With mmap()

```text
Disk

↓

Page Cache

↓

Virtual Memory Mapping

↓

Application
```

The application accesses mapped memory directly through virtual addresses.

Advantages

- Faster for many workloads
- Less copying
- Random access
- Large file support

---

# 3. Virtual Memory Mapping

Without mmap()

```text
Application

↓

read()

↓

Kernel Buffer

↓

User Buffer
```

With mmap()

```text
Application

↓

Virtual Address

↓

Mapped Pages

↓

File
```

The kernel loads pages on demand.

---

# 4. mmap() Prototype

```c
#include <sys/mman.h>

void *mmap(
    void *addr,
    size_t length,
    int prot,
    int flags,
    int fd,
    off_t offset
);
```

Return

```text
Success

↓

Pointer

Failure

↓

MAP_FAILED
```

---

# 5. Parameters

### addr

Preferred virtual address.

Usually

```c
NULL
```

Kernel chooses the address.

---

### length

```text
Number of bytes
```

to map.

---

### prot

Memory protection.

```text
PROT_READ

PROT_WRITE

PROT_EXEC

PROT_NONE
```

---

### flags

Mapping type.

```text
MAP_SHARED

MAP_PRIVATE

MAP_ANONYMOUS
```

---

### fd

File descriptor.

---

### offset

Starting position inside the file.

Must typically be page-aligned.

---

# 6. Anonymous Mapping

No file involved.

Example

```c
char *ptr = mmap(
    NULL,
    4096,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS,
    -1,
    0
);
```

Diagram

```text
Process

↓

Anonymous Pages

↓

RAM
```

Useful for

- Dynamic memory
- Shared memory
- Large allocations

---

# 7. File Mapping

Example

```text
notes.txt

↓

mmap()

↓

char *
```

Access

```c
printf("%c", ptr[0]);
```

Instead of

```c
read()
```

you simply dereference the pointer.

---

# 8. MAP_SHARED vs MAP_PRIVATE

## MAP_SHARED

Changes are visible to other mappings of the same object.

```text
Process A

↓

Shared Mapping

↑

Process B
```

Writes may be propagated to the underlying file.

---

## MAP_PRIVATE

Uses Copy-on-Write.

Diagram

```text
File

↓

Private Mapping

↓

Modify

↓

Private Copy
```

Changes are **not** written back to the file and are not visible to other processes mapping the same file.

---

# 9. munmap()

Remove mapping.

Prototype

```c
int munmap(void *addr,
           size_t length);
```

Example

```c
munmap(ptr,
       4096);
```

Always unmap memory when finished.

---

# 10. Memory-Mapped Files

Instead of

```text
read()

↓

Process Buffer
```

Use

```text
mmap()

↓

Pointer

↓

Memory Access
```

Large databases frequently use memory-mapped files.

---

# 11. mmap() vs read()

| read() | mmap() |
|----------|---------|
| Explicit copy to user buffer | Memory mapping |
| Sequential access | Random access |
| Simpler | Better for large/random access files |
| Repeated system calls | One mapping, then normal memory accesses |

---

# 12. Zero-Copy

Traditional

```text
Disk

↓

Kernel Buffer

↓

User Buffer
```

Multiple copies.

Zero-copy idea

```text
Disk

↓

Page Cache

↓

Mapped Memory
```

Less copying means

- Better performance
- Lower CPU usage

Note: `mmap()` reduces copying but the exact data path depends on the OS and storage stack.

---

# 13. Huge Pages

Normally

```text
Page Size

↓

4 KB
```

Huge pages

```text
2 MB

1 GB
```

Benefits

- Fewer page table entries
- Fewer TLB misses
- Better performance for large memory workloads

Commonly used in

- Databases
- Virtual Machines
- High-performance computing

---

# 14. mmap() for IPC

Two processes

```text
Process A

↓

MAP_SHARED

↓

Shared Pages

↑

Process B
```

Both access the same mapped memory.

Synchronization is still required.

Typically

```text
Semaphore

Mutex
```

---

# 15. Real Linux Examples

## Database

```text
Database File

↓

mmap()

↓

Query Engine
```

---

## Web Browser

```text
Cache File

↓

Memory Mapping
```

---

## Video Player

```text
Large Video File

↓

mmap()

↓

Decoder
```

---

## Shared Cache

```text
Process A

↓

Shared Mapping

↑

Process B
```

---

# 16. Frequently Asked Interview Questions

### Q1. What is mmap()?

Maps a file or anonymous memory into a process's virtual address space.

---

### Q2. Why use mmap()?

For efficient file access and shared memory.

---

### Q3. Difference between MAP_SHARED and MAP_PRIVATE?

MAP_SHARED

```text
Changes Visible
```

MAP_PRIVATE

```text
Copy-on-Write
```

---

### Q4. Why use munmap()?

To release the mapping when finished.

---

### Q5. Is mmap() faster than read()?

Often yes for large files or random access patterns, but the best choice depends on the workload.

---

### Q6. Why use Huge Pages?

Reduce page-table overhead and TLB misses.

---

### Q7. Can mmap() be used for IPC?

Yes.

With shared mappings (`MAP_SHARED`) and proper synchronization.

---

# 17. Quick Revision

✓ `mmap()` maps memory.

✓ `munmap()` removes mapping.

✓ Anonymous mapping → No file.

✓ File mapping → Maps a file.

✓ `MAP_SHARED` → Shared changes.

✓ `MAP_PRIVATE` → Copy-on-Write.

✓ Supports memory-mapped files.

✓ Reduces copying for many workloads.

✓ Supports IPC.

✓ Huge pages improve performance for large memory usage.

---

# Interview Memory Trick

```text
File

↓

mmap()

↓

Virtual Memory

↓

Pointer

↓

Read / Write

↓

munmap()
```

---

# Real Linux Workflow

```text
open()

↓

File Descriptor

↓

mmap()

↓

Pointer

↓

Read/Write Like Array

↓

munmap()

↓

close()
```

---

# Next Part

**Part 16J – IPC Comparison & Complete Linux IPC Guide**

Topics

- Pipes
- FIFO
- Shared Memory
- Message Queue
- Semaphore
- Mutex
- Condition Variable
- Signals
- Socket
- Unix Domain Socket
- eventfd
- futex
- Complete IPC Comparison Table
- Interview Decision Guide
- ---------------------------------------------------------------------
