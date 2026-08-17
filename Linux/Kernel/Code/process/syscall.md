# Linux Process Management — C++ Interview Notes

Concise Linux process-management notes with practical C++/POSIX examples for senior Linux/system-programming interviews.

---

# 1. Process Basics

A **process** is a program in execution.

A process has:

```text
PID
PPID
Virtual address space
Code / Data / Heap / Stack
Registers
File descriptors
Process state
Scheduling information
```

Important process APIs:

```text
fork()       -> create process
exec*()      -> replace process image
wait()       -> wait for child
waitpid()    -> wait for specific child
exit()       -> terminate normally
_exit()      -> terminate immediately
getpid()     -> current PID
getppid()    -> parent PID
kill()       -> send signal
```

---

# 2. `getpid()` and `getppid()`

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    std::cout << "PID  = " << getpid() << '\n';
    std::cout << "PPID = " << getppid() << '\n';

    return 0;
}
```

Remember:

```text
getpid()  -> current process PID
getppid() -> parent process PID
```

---

# 3. `fork()`

`fork()` creates a new child process.

```text
Before:

        Parent
          |
        fork()
          |
After:

       +--------+
       |        |
    Parent    Child
```

Example:

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    fork();

    std::cout << "Hello\n";

    return 0;
}
```

Output:

```text
Hello
Hello
```

After `fork()`, both processes continue execution from approximately the point where `fork()` returned.

---

# 4. `fork()` Return Value

This is one of the most important interview facts.

```cpp
pid_t pid = fork();

if (pid < 0)
{
    // fork failed
}
else if (pid == 0)
{
    // child
}
else
{
    // parent
    // pid = child's PID
}
```

Meaning:

```text
pid < 0  -> failure

pid == 0 -> child process

pid > 0  -> parent process
            return value = child's PID
```

---

# 5. Parent and Child PIDs

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        std::cout << "Child PID = " << getpid() << '\n';
        std::cout << "Parent PID = " << getppid() << '\n';
    }
    else if (pid > 0)
    {
        std::cout << "Parent PID = " << getpid() << '\n';
        std::cout << "Child PID  = " << pid << '\n';
    }

    return 0;
}
```

The output order is not guaranteed because scheduling is independent.

---

# 6. What Does `fork()` Copy?

The child gets a new process with a logically copied process state.

Important points:

```text
Parent and child have separate virtual address spaces.

Child gets its own PID.

Memory pages are initially shared physically where possible
using Copy-on-Write.

File descriptors are inherited.

Parent and child execute independently.
```

---

# 7. Variables After `fork()`

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    int x = 10;

    pid_t pid = fork();

    if (pid == 0)
    {
        x = 20;
        std::cout << "Child  x = " << x << '\n';
    }
    else
    {
        std::cout << "Parent x = " << x << '\n';
    }

    return 0;
}
```

Conceptually:

```text
Parent:
    x = 10

Child:
    x = 20
```

Changing the child's `x` does not modify the parent's `x`.

---

# 8. Copy-on-Write (CoW)

`fork()` does not normally copy every physical memory page immediately.

Initially:

```text
Parent virtual memory
          |
          v
      Physical Page
          ^
          |
Child virtual memory
```

The pages can be shared read-only.

If one process writes:

```text
Parent -----> Page A

Child ------> Copy of Page A
```

Therefore:

```text
fork()
   |
   v
Pages shared where possible
   |
   | write
   v
Private physical copy
```

### Interview answer

> `fork()` uses Copy-on-Write, so parent and child initially share physical pages where possible; a page is copied only when one process modifies it.

---

# 9. `wait()`

`wait()` allows a parent to wait for a child.

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        std::cout << "Child running\n";
        sleep(2);
        std::cout << "Child finished\n";
    }
    else
    {
        std::cout << "Parent waiting\n";

        wait(nullptr);

        std::cout << "Parent continues\n";
    }

    return 0;
}
```

Flow:

```text
Parent
   |
 fork()
   |
   +------> Child
   |           |
 wait()        |
   |           v
   |         exits
   |           |
   +-----------+
   |
   v
Parent continues
```

---

# 10. `waitpid()`

`waitpid()` allows the parent to wait for a specific child.

```cpp
#include <sys/wait.h>

waitpid(
    pid,
    nullptr,
    0
);
```

Example:

```cpp
pid_t pid = fork();

if (pid == 0)
{
    sleep(2);
}
else
{
    waitpid(pid, nullptr, 0);
}
```

### `wait()` vs `waitpid()`

```text
wait()
    -> wait for a child

waitpid(pid, ...)
    -> wait for a specific child
    -> supports options such as WNOHANG
```

---

# 11. Non-Blocking `waitpid()`

Normally:

```cpp
waitpid(pid, nullptr, 0);
```

blocks.

Use:

```cpp
waitpid(
    pid,
    nullptr,
    WNOHANG
);
```

to avoid blocking.

Return value:

```text
0
    -> child still running

child PID
    -> child has changed state / finished

-1
    -> error
```

---

# 12. Collecting Child Exit Status

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        exit(42);
    }

    int status;

    waitpid(
        pid,
        &status,
        0
    );

    if (WIFEXITED(status))
    {
        std::cout
            << "Exit status = "
            << WEXITSTATUS(status)
            << '\n';
    }

    return 0;
}
```

Important macros:

```text
WIFEXITED(status)
    -> child exited normally

WEXITSTATUS(status)
    -> exit status

WIFSIGNALED(status)
    -> child terminated by signal

WTERMSIG(status)
    -> signal that terminated child
```

---

# 13. `exit()` vs `_exit()`

### `exit()`

```cpp
exit(0);
```

Performs normal user-space termination processing, including flushing standard C stdio streams.

### `_exit()`

```cpp
_exit(0);
```

Terminates immediately without normal stdio cleanup.

Important after `fork()`:

```cpp
exec(...);

if exec fails
{
    perror("exec");
    _exit(1);
}
```

Using `_exit()` in the forked child avoids running inherited user-space cleanup/stdio handling again.

---

# 14. `exec()` — Replaces the Process Image

`exec()` **does not create a process**.

It replaces the current process image with another program.

```text
Before:

PID 5000
   |
Program A


exec()


After:

PID 5000
   |
Program B
```

The PID remains the same.

### Memorize

```text
fork()
    -> creates a process

exec()
    -> replaces the program inside the process
```

---

# 15. `exec()` Returns Only on Failure

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    execlp(
        "ls",
        "ls",
        "-l",
        nullptr
    );

    std::cout << "exec failed\n";

    return 0;
}
```

If `exec()` succeeds:

```text
Current program disappears
New program runs
"exec failed" is never executed
```

If `exec()` fails:

```text
exec() returns -1
execution continues
```

Production-style pattern:

```cpp
execlp(
    "ls",
    "ls",
    "-l",
    nullptr
);

perror("execlp");
_exit(1);
```

---

# 16. `exec` Family

The common variants:

```text
execl()
execlp()
execv()
execvp()
```

Remember:

```text
l = list
v = vector/array
p = search PATH
```

| Function   | Arguments | PATH search |
| ---------- | --------- | ----------- |
| `execl()`  | List      | No          |
| `execlp()` | List      | Yes         |
| `execv()`  | Array     | No          |
| `execvp()` | Array     | Yes         |

### `execl()`

```cpp
execl(
    "/bin/ls",
    "ls",
    "-l",
    nullptr
);
```

### `execlp()`

```cpp
execlp(
    "ls",
    "ls",
    "-l",
    nullptr
);
```

### `execv()`

```cpp
char* args[] =
{
    (char*)"ls",
    (char*)"-l",
    nullptr
};

execv(
    "/bin/ls",
    args
);
```

### `execvp()`

```cpp
char* args[] =
{
    (char*)"ls",
    (char*)"-l",
    nullptr
};

execvp(
    "ls",
    args
);
```

---

# 17. `fork() + exec()`

This is one of the most important Linux process patterns.

```text
Parent
   |
 fork()
   |
   +--------> Child
                 |
                exec()
                 |
                 v
             New Program
```

Example:

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        execlp(
            "ls",
            "ls",
            "-l",
            nullptr
        );

        perror("exec");
        _exit(1);
    }

    waitpid(
        pid,
        nullptr,
        0
    );

    std::cout << "Child finished\n";

    return 0;
}
```

This gives the classic:

```text
fork()
  |
  v
child
  |
exec()
  |
  v
new program
  |
  v
exit
  |
  v
parent waitpid() returns
```

---

# 18. Why `fork() + exec()`?

If a process directly executes:

```cpp
exec(...);
```

the current program is replaced.

If the parent should continue running:

```text
Parent
   |
 fork()
   |
   +------> Child
               |
              exec()
               |
               v
          Other Program

Parent continues
```

This is the basic model used when a process launches another program.

---

# 19. Zombie Process

A **zombie** is a child that has terminated but whose parent has not yet collected its termination status.

```text
Child
  |
 exits
  |
  v
Zombie
  |
  | wait()/waitpid()
  v
Reaped
```

Example:

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        std::cout << "Child exiting\n";
        return 0;
    }

    std::cout << "Parent sleeping\n";
    sleep(10);

    return 0;
}
```

During the parent's sleep, the child can appear as:

```text
Z
```

The parent should reap it using:

```cpp
wait();
```

or:

```cpp
waitpid();
```

---

# 20. Orphan Process

An orphan is a child that is still running after its parent terminates.

```text
Parent
   |
   +----> Child

Parent exits

Child
   |
   v
Adopted by appropriate system process/subreaper
```

Example:

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        sleep(3);

        std::cout
            << "PID  = " << getpid() << '\n'
            << "PPID = " << getppid() << '\n';
    }
    else
    {
        std::cout << "Parent exiting\n";
    }

    return 0;
}
```

### Zombie vs Orphan

|           | Zombie                 | Orphan             |
| --------- | ---------------------- | ------------------ |
| Child     | Finished               | Still running      |
| Parent    | Alive                  | Terminated         |
| Key issue | Not reaped             | Parent disappeared |
| Handling  | `wait()` / `waitpid()` | System adoption    |

---

# 21. Multiple `fork()`

Example:

```cpp
fork();
fork();

std::cout << "Hello\n";
```

Process count:

```text
Initial       = 1
After fork 1  = 2
After fork 2  = 4
```

Therefore:

```text
Hello
Hello
Hello
Hello
```

For `N` unconditional `fork()` calls:

```text
Maximum processes = 2^N
```

Example:

```text
3 fork() calls -> 8 processes
```

This assumes every `fork()` succeeds and every resulting process reaches every subsequent `fork()`.

---

# 22. `fork()` and File Descriptors

After `fork()`, the child inherits copies of the parent's file descriptors.

Conceptually:

```text
Parent FD 3 ----+
                |
                v
          Open File Description
                ^
                |
Child FD 3 -----+
```

This is important for:

```text
pipes
file redirection
fork() + exec()
```

---

# 23. Pipes

A pipe provides byte-stream IPC.

```cpp
int fd[2];

pipe(fd);
```

Meaning:

```text
fd[0] -> read end
fd[1] -> write end
```

Typical parent-to-child pattern:

```text
Parent
   |
 write(fd[1])
   |
   v
 PIPE
   |
 read(fd[0])
   |
   v
Child
```

Example:

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

int main()
{
    int fd[2];

    if (pipe(fd) == -1)
    {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        close(fd[1]);

        char buffer[100] = {};

        read(
            fd[0],
            buffer,
            sizeof(buffer)
        );

        std::cout
            << "Child received: "
            << buffer
            << '\n';

        close(fd[0]);
    }
    else
    {
        close(fd[0]);

        const char* msg = "Hello child";

        write(
            fd[1],
            msg,
            strlen(msg) + 1
        );

        close(fd[1]);

        waitpid(
            pid,
            nullptr,
            0
        );
    }

    return 0;
}
```

---

# 24. Why Close Unused Pipe Ends?

If the parent only writes:

```cpp
close(fd[0]);
```

If the child only reads:

```cpp
close(fd[1]);
```

Unused descriptors must be closed.

Otherwise, a reader may not receive EOF because some process still has a write end open.

### Interview rule

> After `fork()`, each process should close the pipe ends it does not use.

---

# 25. `dup2()` — File Descriptor Redirection

`dup2()` makes one file descriptor refer to the same open file description as another descriptor.

Common use:

```cpp
dup2(
    fd[0],
    STDIN_FILENO
);
```

Meaning:

```text
stdin (0)
   |
   v
pipe read end
```

Similarly:

```cpp
dup2(
    fd[1],
    STDOUT_FILENO
);
```

means:

```text
stdout (1)
   |
   v
pipe write end
```

Standard descriptors:

```text
0 -> stdin
1 -> stdout
2 -> stderr
```

---

# 26. `fork() + pipe() + exec()`

This is an important interview pattern.

```text
Parent
   |
   | write
   v
+-------+
| PIPE  |
+-------+
   |
   | stdin
   v
Child
   |
  exec()
   |
   v
Program
```

Example:

```cpp
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

int main()
{
    int fd[2];

    if (pipe(fd) == -1)
        return 1;

    pid_t pid = fork();

    if (pid == 0)
    {
        // Child reads from pipe as stdin

        dup2(
            fd[0],
            STDIN_FILENO
        );

        close(fd[0]);
        close(fd[1]);

        execlp(
            "wc",
            "wc",
            "-c",
            nullptr
        );

        _exit(1);
    }

    // Parent writes to pipe

    close(fd[0]);

    const char* msg =
        "Hello from parent\n";

    write(
        fd[1],
        msg,
        strlen(msg)
    );

    close(fd[1]);

    waitpid(
        pid,
        nullptr,
        0
    );

    return 0;
}
```

The executed `wc -c` receives the pipe data through standard input.

---

# 27. Shell Pipeline: `ls | wc -l`

A shell pipeline can be understood as:

```text
ls
 |
 | stdout
 v
PIPE
 |
 | stdin
 v
wc -l
```

Conceptually the shell performs:

```text
1. pipe()

2. fork() -> ls child
       dup2(pipe_write, stdout)
       exec(ls)

3. fork() -> wc child
       dup2(pipe_read, stdin)
       exec(wc)

4. Parent closes pipe descriptors

5. Parent waits
```

This is a very common senior Linux interview topic.

---

# 28. `system()` vs `exec()`

### `system()`

```cpp
system("ls -l");
```

Runs a command through a shell.

### `exec()`

```cpp
execlp(
    "ls",
    "ls",
    "-l",
    nullptr
);
```

Replaces the current process image directly with the specified program.

Remember:

```text
system()
    -> shell command

exec()
    -> replace current process image
```

For process-control code, `fork() + exec()` gives much more control.

---

# 29. `vfork()`

`vfork()` is a specialized process-creation mechanism with stricter semantics.

Basic idea:

```text
vfork()
    |
    +--> Child runs
    |
    +--> Parent is suspended
          until child calls exec() or _exit()
```

Example:

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    pid_t pid = vfork();

    if (pid == 0)
    {
        execlp(
            "ls",
            "ls",
            nullptr
        );

        _exit(1);
    }

    std::cout << "Parent\n";

    return 0;
}
```

Important:

```text
Do NOT treat vfork() like normal fork().
```

For normal application code:

```text
fork() + exec()
```

is the standard pattern.

---

# 30. `fork()` vs `vfork()`

| Feature         | `fork()`                 | `vfork()`                                               |
| --------------- | ------------------------ | ------------------------------------------------------- |
| Purpose         | General process creation | Specialized creation before exec/exit                   |
| Parent          | Can run independently    | Suspended until child execs/exits                       |
| Memory          | CoW                      | Temporarily shares address space under strict semantics |
| Usage           | General                  | Restricted                                              |
| Typical pattern | `fork() + exec()`        | `vfork() + exec()`                                      |

---

# 31. Process Groups

A process belongs to a process group.

Useful IDs:

```text
PID -> Process ID
PPID -> Parent Process ID
PGID -> Process Group ID
SID -> Session ID
```

Example:

```cpp
#include <iostream>
#include <unistd.h>

int main()
{
    std::cout << "PID  = " << getpid() << '\n';
    std::cout << "PPID = " << getppid() << '\n';
    std::cout << "PGID = " << getpgrp() << '\n';

    return 0;
}
```

Process groups are important for:

```text
job control
shells
signal delivery to groups
```

---

# 32. Signals

A signal is an asynchronous notification sent to a process.

Important signals:

```text
SIGINT
    -> interrupt, commonly Ctrl+C

SIGTERM
    -> termination request

SIGKILL
    -> force termination

SIGSTOP
    -> stop process

SIGCHLD
    -> child state changed
```

---

# 33. `kill()`

Despite its name, `kill()` means **send a signal**.

```cpp
kill(
    pid,
    SIGTERM
);
```

This sends `SIGTERM` to the target process.

Example:

```cpp
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        while (true)
            sleep(1);
    }

    sleep(3);

    kill(
        pid,
        SIGTERM
    );

    waitpid(
        pid,
        nullptr,
        0
    );

    return 0;
}
```

---

# 34. `SIGKILL` and `SIGSTOP`

These signals cannot be caught or ignored:

```text
SIGKILL
SIGSTOP
```

Therefore a program cannot install a handler for them.

---

# 35. `signal()`

Simple signal handler:

```cpp
#include <iostream>
#include <unistd.h>
#include <signal.h>

void handler(int sig)
{
    std::cout
        << "Signal received: "
        << sig
        << '\n';
}

int main()
{
    signal(
        SIGINT,
        handler
    );

    while (true)
        sleep(1);

    return 0;
}
```

Press:

```text
Ctrl+C
```

to generate `SIGINT`.

For robust POSIX programs, prefer `sigaction()`.

---

# 36. `sigaction()`

More controllable POSIX signal-handling interface:

```cpp
#include <unistd.h>
#include <signal.h>

void handler(int)
{
}

int main()
{
    struct sigaction action{};

    action.sa_handler = handler;

    sigemptyset(
        &action.sa_mask
    );

    action.sa_flags = 0;

    sigaction(
        SIGINT,
        &action,
        nullptr
    );

    while (true)
        sleep(1);

    return 0;
}
```

### `signal()` vs `sigaction()`

```text
signal()
    -> simple interface

sigaction()
    -> POSIX interface
    -> more control
    -> preferred for robust signal handling
```

Signal handlers should perform only async-signal-safe operations; avoid things such as `std::cout` in production signal handlers.

---

# 37. `SIGCHLD`

A parent can receive `SIGCHLD` when a child changes state, commonly when it terminates.

```text
Child
  |
 exits
  |
  v
SIGCHLD
  |
  v
Parent
```

The parent can then use:

```cpp
waitpid()
```

to reap the child.

Important:

```text
SIGCHLD notification
+
waitpid()
```

are often used together for child-process management.

---

# 38. Process States

Conceptually:

```text
NEW
 |
 v
READY
 |
 v
RUNNING
 |   \
 |    \
 v     v
WAITING  TERMINATED
 |
 v
READY
```

Linux process-state examples:

```text
R -> Running / runnable
S -> Interruptible sleep
D -> Uninterruptible sleep
T -> Stopped
Z -> Zombie
```

Useful commands:

```bash
ps
ps -ef
ps aux
pstree
```

---

# 39. Important Process Templates

## Template 1 — `fork()`

```cpp
pid_t pid = fork();

if (pid < 0)
{
    // error
}
else if (pid == 0)
{
    // child
}
else
{
    // parent
}
```

---

## Template 2 — `fork() + waitpid()`

```cpp
pid_t pid = fork();

if (pid == 0)
{
    // child
}
else
{
    waitpid(
        pid,
        nullptr,
        0
    );
}
```

---

## Template 3 — `fork() + exec()`

```cpp
pid_t pid = fork();

if (pid == 0)
{
    execlp(
        "program",
        "program",
        nullptr
    );

    _exit(1);
}
```

---

## Template 4 — `fork() + exec() + waitpid()`

```cpp
pid_t pid = fork();

if (pid == 0)
{
    execlp(
        "program",
        "program",
        nullptr
    );

    _exit(1);
}
else
{
    waitpid(
        pid,
        nullptr,
        0
    );
}
```

---

## Template 5 — Pipe

```cpp
int fd[2];

pipe(fd);

pid_t pid = fork();

if (pid == 0)
{
    close(fd[1]);

    read(
        fd[0],
        buffer,
        size
    );

    close(fd[0]);
}
else
{
    close(fd[0]);

    write(
        fd[1],
        data,
        size
    );

    close(fd[1]);

    waitpid(
        pid,
        nullptr,
        0
    );
}
```

---

# 40. Common Interview Questions

### Q1. Does `fork()` create a new program?

No.

```text
fork()
    -> creates a new process
```

The child initially has the same program image.

---

### Q2. Does `exec()` create a new process?

No.

```text
exec()
    -> replaces the current process image
```

---

### Q3. Does PID change after `exec()`?

No.

```text
PID remains unchanged.
```

---

### Q4. Does `fork()` copy the entire memory immediately?

No.

Linux uses Copy-on-Write, so physical pages are copied only when required.

---

### Q5. Does parent and child share variables?

They have separate virtual address spaces.

Initially physical pages may be shared through CoW, but normal modifications are private.

---

### Q6. What does the child inherit after `fork()`?

Important inherited state includes:

```text
Memory mappings
File descriptors
Environment
Working directory
Process attributes
```

The child gets a new:

```text
PID
process identity
```

---

### Q7. What happens to file descriptors after `fork()`?

The child inherits copies of the parent's file descriptors, referring to the same underlying open-file descriptions.

---

### Q8. What happens to file descriptors after `exec()`?

They normally remain open unless marked close-on-exec (`FD_CLOEXEC`).

This enables:

```text
fork()
dup2()
exec()
```

for pipes and redirection.

---

### Q9. What is a zombie?

```text
Child terminated
+
Parent hasn't collected its termination status
=
Zombie
```

Use:

```cpp
wait()
```

or:

```cpp
waitpid()
```

to reap it.

---

### Q10. What is an orphan?

```text
Parent terminates
+
Child is still running
=
Orphan
```

The child is adopted by an appropriate system process/subreaper.

---

### Q11. `wait()` vs `waitpid()`?

```text
wait()
    -> waits for a child

waitpid()
    -> can wait for a specific child
    -> supports WNOHANG and other options
```

---

### Q12. `exit()` vs `_exit()`?

```text
exit()
    -> normal user-space termination processing

_exit()
    -> immediate process termination
    -> commonly used in forked child after exec failure
```

---

### Q13. Why use `_exit()` after failed `exec()`?

Because the child inherited the parent's user-space state and stdio buffers after `fork()`.

```cpp
exec(...);

perror("exec failed");
_exit(1);
```

avoids normal stdio cleanup being performed again in the child.

---

### Q14. What is `dup2()` used for?

Primarily file-descriptor redirection.

Example:

```cpp
dup2(
    pipefd[1],
    STDOUT_FILENO
);
```

means the program's stdout goes to the pipe.

---

### Q15. How does a shell implement `ls | wc -l`?

Conceptually:

```text
pipe()
   |
fork() -> ls
   |       |
   |    dup2(pipe_write, stdout)
   |       |
   |     exec(ls)
   |
fork() -> wc
           |
        dup2(pipe_read, stdin)
           |
         exec(wc)

close unused FDs
waitpid()
```

---

### Q16. How many processes after N unconditional forks?

```text
2^N
```

assuming every fork succeeds and every resulting process reaches all subsequent forks.

---

### Q17. Can `SIGKILL` be caught?

No.

```text
SIGKILL -> cannot catch
SIGKILL -> cannot ignore
```

Same for:

```text
SIGSTOP
```

---

### Q18. Why is `fork() + exec()` so common?

Because it separates:

```text
process creation
    +
program execution
```

The parent can remain alive while the child becomes another program.

---

# 41. Final Interview Revision Sheet

```text
PROCESS
-------
Process = program in execution.

PID
---
getpid()  -> current PID
getppid() -> parent PID


FORK
----
fork()
    -> creates child process

return:
    < 0  -> failure
    == 0 -> child
    > 0  -> parent, return value = child PID


MEMORY
------
Parent and child have separate virtual address spaces.

fork() uses Copy-on-Write.

Physical pages are shared where possible
until one process modifies a page.


WAIT
----
wait()
    -> wait for a child

waitpid()
    -> wait for specific child

WNOHANG
    -> non-blocking wait


EXEC
----
exec()
    -> replaces current process image
    -> does not create process
    -> PID remains unchanged
    -> returns only on failure

l = list
v = vector
p = PATH

execl()
execlp()
execv()
execvp()


EXIT
----
exit()
    -> normal termination processing

_exit()
    -> immediate termination
    -> common after failed exec in child


ZOMBIE
------
Child finished
Parent alive
Parent hasn't reaped child

Use:
wait()
waitpid()


ORPHAN
------
Parent finished
Child still running

System adopts child.


PIPE
----
pipe(fd)

fd[0] -> read
fd[1] -> write

Close unused ends.


DUP2
----
dup2(fd, STDIN_FILENO)
dup2(fd, STDOUT_FILENO)

0 -> stdin
1 -> stdout
2 -> stderr


SIGNALS
-------
kill(pid, signal)

SIGINT  -> interrupt
SIGTERM -> termination request
SIGKILL -> force termination
SIGSTOP -> stop
SIGCHLD -> child state change

SIGKILL and SIGSTOP cannot be caught or ignored.


SIGNAL HANDLING
---------------
signal()
sigaction()

Prefer sigaction() for robust POSIX code.


VFORk
-----
vfork()
    -> parent suspended
    -> child should quickly exec() or _exit()

Use normal fork() + exec() unless vfork()
is specifically required.


PROCESS GROUP
-------------
PID
PPID
PGID
SID


MOST IMPORTANT PATTERN
----------------------

Parent
   |
 fork()
   |
   +------> Child
               |
              exec()
               |
               v
          New Program
               |
             exit
               |
               v
        parent waitpid()
```

---

# 42. Golden Rules for Interviews

```text
1. fork() creates a process.

2. exec() replaces a process image.

3. exec() does NOT create a process.

4. PID remains unchanged across exec().

5. fork() returns 0 to child.

6. fork() returns child's PID to parent.

7. fork() uses Copy-on-Write.

8. Parent and child have separate virtual address spaces.

9. wait()/waitpid() reap children.

10. Zombie = terminated child not yet reaped.

11. Orphan = running child whose parent terminated.

12. fork() + exec() is the standard process-launch pattern.

13. dup2() is used for descriptor redirection.

14. Pipe:
       fd[0] = read
       fd[1] = write

15. Always close unused pipe ends.

16. exec() returns only when it fails.

17. Use _exit() in a forked child after exec failure.

18. system() normally invokes a shell.

19. SIGKILL and SIGSTOP cannot be caught or ignored.

20. Shell pipeline:
       pipe()
       fork()
       dup2()
       exec()
       close()
       waitpid()
```

# End
