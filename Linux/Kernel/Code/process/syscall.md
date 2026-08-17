# Linux Process Management — C++ Interview Notes

Simple, small, working Linux C++ examples for:

1. `fork()`
2. `getpid()` / `getppid()`
3. `wait()`
4. `waitpid()`
5. `exit()` / `_exit()`
6. `exec()`
7. `execl()`
8. `execlp()`
9. `execv()`
10. `execvp()`
11. `fork() + exec()`
12. `fork() + exec() + wait()`
13. Orphan process
14. Zombie process
15. Copy-on-write
16. Multiple `fork()`
17. `fork() + pipe()`
18. `fork() + exec() + pipe()`
19. `system()` vs `exec()`
20. `vfork()`
21. Process groups
22. Signals
23. `kill()`
24. `signal()`
25. `sigaction()`
26. Signal + `wait()`
27. Process states
28. Common interview questions
29. One-page revision

---

# 1. Process Basics

A process is a running program.

For example:

```text
program
   |
   | execute
   v
process
```

A process has:

```text
PID
PPID
Address space
Code
Data
Stack
Heap
File descriptors
Registers
```

Important functions:

```cpp
fork()
exec()
wait()
exit()
getpid()
getppid()
```

---

# 2. `getpid()`

`getpid()` returns the PID of the current process.

Example:

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    cout << "My PID = "
         << getpid()
         << endl;

    return 0;
}
```

Compile:

```bash
g++ pid.cpp -o pid
```

Run:

```bash
./pid
```

Example output:

```text
My PID = 12345
```

---

# 3. `getppid()`

`getppid()` returns the PID of the parent process.

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    cout << "My PID = "
         << getpid()
         << endl;

    cout << "Parent PID = "
         << getppid()
         << endl;

    return 0;
}
```

Output:

```text
My PID = 12345
Parent PID = 1000
```

Remember:

```text
getpid()
    -> current process

getppid()
    -> parent process
```

---

# 4. `fork()`

`fork()` creates a new process.

Before:

```text
Parent
```

After:

```text
        Parent
           |
        fork()
           |
      +----+----+
      |         |
   Parent      Child
```

The child is initially a copy of the parent's process image.

---

# 5. Simplest `fork()` Example

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    fork();

    cout << "Hello\n";

    return 0;
}
```

Output:

```text
Hello
Hello
```

Why?

Before `fork()`:

```text
1 process
```

After:

```text
2 processes
```

Both execute:

```cpp
cout << "Hello\n";
```

Therefore:

```text
Hello
Hello
```

---

# 6. `fork()` Return Value

This is extremely important.

```cpp
pid_t pid = fork();
```

There are three possibilities:

```text
pid < 0
    -> fork failed

pid == 0
    -> child process

pid > 0
    -> parent process
    -> value is child's PID
```

Typical pattern:

```cpp
pid_t pid = fork();

if (pid < 0)
{
    // Error
}
else if (pid == 0)
{
    // Child
}
else
{
    // Parent
}
```

Memorize this.

---

# 7. Parent and Child Example

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        cout << "Child\n";
        cout << "Child PID = "
             << getpid()
             << endl;
    }
    else
    {
        cout << "Parent\n";
        cout << "Parent PID = "
             << getpid()
             << endl;
        cout << "Child PID = "
             << pid
             << endl;
    }

    return 0;
}
```

Possible output:

```text
Parent
Parent PID = 1000
Child PID = 1001

Child
Child PID = 1001
```

The order is not guaranteed.

---

# 8. Important `fork()` Point

After:

```cpp
pid_t pid = fork();
```

there are two processes executing from approximately the same point.

```text
Before fork:

A
|
fork()
|
+----------------+
|                |
v                v
Parent           Child
```

Both continue execution after `fork()`.

---

# 9. Variables After `fork()`

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    int x = 10;

    pid_t pid = fork();

    if (pid == 0)
    {
        x = 20;

        cout << "Child x = "
             << x
             << endl;
    }
    else
    {
        cout << "Parent x = "
             << x
             << endl;
    }

    return 0;
}
```

Output:

```text
Child x = 20
Parent x = 10
```

Why?

Each process gets its own virtual address space.

Conceptually:

```text
Parent:
x = 10

Child:
x = 20
```

Changing child's `x` does not change parent's `x`.

---

# 10. Copy-on-Write

After `fork()`, parent and child initially share physical memory pages where possible.

The OS uses:

```text
Copy-on-Write
```

Initially:

```text
Parent ----+
           |
           v
       Physical Page
           ^
           |
Child -----+
```

If child modifies a page:

```text
Parent ----> Page A

Child -----> Copy of Page A
```

So `fork()` does not necessarily copy the entire memory immediately.

---

# 11. `wait()`

`wait()` allows the parent to wait for a child process.

Include:

```cpp
#include <sys/wait.h>
```

Simple example:

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        cout << "Child running\n";
        sleep(2);
        cout << "Child finished\n";
    }
    else
    {
        cout << "Parent waiting...\n";

        wait(nullptr);

        cout << "Parent continues\n";
    }

    return 0;
}
```

Output:

```text
Parent waiting...
Child running
Child finished
Parent continues
```

---

# 12. Why Use `wait()`?

Without `wait()`:

```text
Parent
   |
   +---- Child
```

Parent may finish before child.

With:

```cpp
wait(nullptr);
```

parent waits for a child to finish.

```text
Parent
   |
   | wait()
   v
Child finishes
   |
   v
Parent continues
```

---

# 13. `wait()` and Zombie Processes

When a child terminates, the OS keeps some information about the child until the parent collects it.

If the parent doesn't call:

```cpp
wait()
```

or:

```cpp
waitpid()
```

the child can remain as a:

```text
Zombie
```

Therefore:

```text
Child exits
     |
     v
Zombie
     |
     | parent calls wait()
     v
Zombie removed
```

---

# 14. `waitpid()`

`waitpid()` allows the parent to wait for a specific child.

Syntax:

```cpp
waitpid(
    pid,
    nullptr,
    0
);
```

Example:

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        cout << "Child running\n";
        sleep(2);
        cout << "Child done\n";
    }
    else
    {
        cout << "Waiting for child...\n";

        waitpid(
            pid,
            nullptr,
            0
        );

        cout << "Child finished\n";
    }

    return 0;
}
```

---

# 15. `wait()` vs `waitpid()`

```text
wait()
    -> Wait for any child

waitpid(pid, ...)
    -> Wait for specific child
```

Example:

```cpp
wait(nullptr);
```

vs:

```cpp
waitpid(
    child_pid,
    nullptr,
    0
);
```

---

# 16. `waitpid()` Non-Blocking

Normally:

```cpp
waitpid(
    pid,
    nullptr,
    0
);
```

blocks.

With:

```cpp
WNOHANG
```

the parent does not block.

Example:

```cpp
pid_t result = waitpid(
    pid,
    nullptr,
    WNOHANG
);
```

Possible result:

```text
result == 0
    -> child still running

result == child PID
    -> child finished

result == -1
    -> error
```

---

# 17. `exit()`

`exit()` terminates the process.

```cpp
#include <iostream>
#include <cstdlib>

using namespace std;

int main()
{
    cout << "Before exit\n";

    exit(0);

    cout << "After exit\n";

    return 0;
}
```

Output:

```text
Before exit
```

This is never executed:

```cpp
cout << "After exit\n";
```

---

# 18. Exit Status

A child can return an exit status.

Child:

```cpp
exit(42);
```

Parent can collect it.

Example:

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

using namespace std;

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        exit(42);
    }
    else
    {
        int status;

        waitpid(
            pid,
            &status,
            0
        );

        if (WIFEXITED(status))
        {
            cout << "Exit status = "
                 << WEXITSTATUS(status)
                 << endl;
        }
    }

    return 0;
}
```

Output:

```text
Exit status = 42
```

---

# 19. `_exit()`

`_exit()` immediately terminates the process.

```cpp
#include <unistd.h>

int main()
{
    _exit(0);
}
```

Important interview difference:

```text
exit()
    -> performs normal user-space cleanup
    -> flushes stdio buffers

_exit()
    -> immediately terminates process
    -> does not perform those stdio cleanup operations
```

This difference matters particularly around:

```text
fork()
```

and buffered output.

---

# 20. `exec()` — Most Important Concept

`exec()` does NOT create a new process.

It replaces the current process's program.

Before:

```text
Process
   |
   v
Program A
```

After:

```text
Process
   |
   v
Program B
```

The PID remains the same.

Remember:

```text
fork()
    -> creates process

exec()
    -> replaces program
```

---

# 21. Why `fork() + exec()`?

This is one of the most important Linux process patterns.

```text
Parent
   |
   | fork()
   |
   +---------> Child
                  |
                  | exec()
                  v
              New Program
```

Parent remains running.

Child becomes another program.

---

# 22. `execl()`

Example:

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    cout << "Before exec\n";

    execl(
        "/bin/ls",
        "ls",
        "-l",
        nullptr
    );

    cout << "After exec\n";

    return 0;
}
```

Output will contain the `ls -l` result.

You will NOT normally see:

```text
After exec
```

because if `exec` succeeds, it replaces the current program.

---

# 23. What If `exec()` Fails?

Important:

`exec()` only returns if it fails.

Example:

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    execl(
        "/does/not/exist",
        "test",
        nullptr
    );

    cout << "exec failed\n";

    return 0;
}
```

Output:

```text
exec failed
```

Production code should normally check the error:

```cpp
perror("execl");
```

---

# 24. `execl()`

The `l` means:

```text
list
```

Arguments are passed as a list.

Example:

```cpp
execl(
    "/bin/ls",
    "ls",
    "-l",
    "-a",
    nullptr
);
```

Conceptually:

```text
path
argv[0]
argv[1]
argv[2]
NULL
```

---

# 25. `execlp()`

The `p` means it searches the `PATH`.

Example:

```cpp
#include <unistd.h>

int main()
{
    execlp(
        "ls",
        "ls",
        "-l",
        nullptr
    );

    return 0;
}
```

Difference:

```text
execl()
    -> path explicitly specified

execlp()
    -> searches PATH
```

---

# 26. `execv()`

The `v` means arguments are supplied as a vector/array.

```cpp
#include <unistd.h>

int main()
{
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

    return 0;
}
```

---

# 27. `execvp()`

`execvp()` combines:

```text
v = vector/array arguments

p = search PATH
```

Example:

```cpp
#include <unistd.h>

int main()
{
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

    return 0;
}
```

---

# 28. `exec` Family

The common variants:

```text
execl()
execlp()

execv()
execvp()
```

Meaning:

```text
l = list
v = vector/array

p = search PATH
```

Quick table:

| Function | Arguments | PATH |
|---|---|---|
| `execl` | list | No |
| `execlp` | list | Yes |
| `execv` | array | No |
| `execvp` | array | Yes |

---

# 29. Easy Way to Remember exec Functions

```text
l = list

v = vector

p = PATH
```

Therefore:

```text
execl
    -> list

execlp
    -> list + PATH

execv
    -> vector

execvp
    -> vector + PATH
```

---

# 30. `fork() + exec()`

This is probably the most important example.

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main()
{
    pid_t pid = fork();

    if (pid < 0)
    {
        cout << "fork failed\n";
        return 1;
    }

    if (pid == 0)
    {
        cout << "Child executing ls\n";

        execlp(
            "ls",
            "ls",
            "-l",
            nullptr
        );

        cout << "exec failed\n";
    }
    else
    {
        cout << "Parent waiting\n";

        waitpid(
            pid,
            nullptr,
            0
        );

        cout << "Child finished\n";
    }

    return 0;
}
```

Compile:

```bash
g++ fork_exec.cpp -o fork_exec
```

Run:

```bash
./fork_exec
```

---

# 31. What Happens in `fork() + exec()`?

```text
Parent
  |
  |
 fork()
  |
  +----------------+
  |                |
Parent            Child
  |                |
  |                |
wait()            exec()
                   |
                   v
                 ls
```

The child keeps its PID but gets a new program image.

---

# 32. Does `exec()` Change PID?

No.

Example:

```text
Before exec:

PID = 5000
Program = A

exec(B)

After:

PID = 5000
Program = B
```

This is a very common interview question.

---

# 33. `fork()` + `exec()` + `wait()`

Complete pattern:

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execlp(
            "date",
            "date",
            nullptr
        );

        return 1;
    }

    waitpid(
        pid,
        nullptr,
        0
    );

    cout << "Child completed\n";

    return 0;
}
```

Flow:

```text
fork()
   |
   v
Child
   |
   v
exec("date")
   |
   v
date runs
   |
   v
Child exits
   |
   v
Parent wait() returns
```

---

# 34. Orphan Process

An orphan is a child whose parent terminates before it.

Example:

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        sleep(3);

        cout << "Child PID = "
             << getpid()
             << endl;

        cout << "New Parent PID = "
             << getppid()
             << endl;
    }
    else
    {
        cout << "Parent exiting\n";
    }

    return 0;
}
```

The child may become adopted by a system process.

Conceptually:

```text
Original Parent
      |
      v
    Child

Parent exits

    Child
      |
      v
adopted by system
```

Modern Linux uses special mechanisms for orphan adoption; the exact visible PPID depends on the environment.

---

# 35. Zombie Process

A zombie is a child that has terminated but whose parent hasn't collected its termination status.

Example:

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        cout << "Child exiting\n";
        return 0;
    }
    else
    {
        cout << "Parent sleeping\n";

        sleep(10);
    }

    return 0;
}
```

During the parent's sleep, the child may appear as:

```text
Z
```

in process listings.

Parent should normally call:

```cpp
wait()
```

or:

```cpp
waitpid()
```

to reap the child.

---

# 36. Zombie vs Orphan

Very important:

```text
Zombie
-------
Child has finished.
Parent is still alive.
Parent has not collected status.

Orphan
------
Parent has finished.
Child is still running.
```

Table:

| | Zombie | Orphan |
|---|---|---|
| Child | Finished | Running |
| Parent | Alive | Finished |
| Problem | Not reaped | Parent disappeared |
| Solution | `wait()` / `waitpid()` | System adoption |

---

# 37. Multiple `fork()`

Example:

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    fork();
    fork();

    cout << "Hello\n";

    return 0;
}
```

How many processes?

First fork:

```text
1 -> 2
```

Second fork:

```text
2 -> 4
```

Therefore:

```text
4 processes
```

Output:

```text
Hello
Hello
Hello
Hello
```

---

# 38. Three Forks

```cpp
fork();
fork();
fork();
```

Number of processes:

```text
2^3 = 8
```

General rule:

```text
n fork() calls

maximum processes = 2^n
```

Assuming every fork succeeds and all resulting processes reach the subsequent fork.

---

# 39. `fork()` + Pipe

Parent sends data to child.

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

using namespace std;

int main()
{
    int fd[2];

    pipe(fd);

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

        cout << "Child received: "
             << buffer
             << endl;

        close(fd[0]);
    }
    else
    {
        close(fd[0]);

        const char* msg =
            "Hello child";

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

# 40. `fork() + exec() + pipe()`

This pattern is extremely important because it is conceptually similar to how a shell connects commands.

Example:

```text
Parent
   |
   +---- pipe ----+
                  |
                Child
                  |
                 exec()
                  |
                  v
              some program
```

Simple example:

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

using namespace std;

int main()
{
    int fd[2];

    pipe(fd);

    pid_t pid = fork();

    if (pid == 0)
    {
        // Child reads from pipe

        close(fd[1]);

        dup2(
            fd[0],
            STDIN_FILENO
        );

        close(fd[0]);

        execlp(
            "wc",
            "wc",
            "-c",
            nullptr
        );

        return 1;
    }
    else
    {
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
    }

    return 0;
}
```

Compile:

```bash
g++ pipe_exec.cpp -o pipe_exec
```

Run:

```bash
./pipe_exec
```

The child executes:

```text
wc -c
```

and receives the parent's pipe data as standard input.

---

# 41. `dup2()`

`dup2()` duplicates a file descriptor.

Example:

```cpp
dup2(
    fd[0],
    STDIN_FILENO
);
```

This means:

```text
stdin
  |
  v
pipe read end
```

Therefore the program executed by `exec()` reads from the pipe when it reads standard input.

---

# 42. Standard File Descriptors

Remember:

```text
0 -> stdin
1 -> stdout
2 -> stderr
```

Therefore:

```cpp
STDIN_FILENO
    -> 0

STDOUT_FILENO
    -> 1

STDERR_FILENO
    -> 2
```

---

# 43. Redirect stdout to a File

Example:

```cpp
#include <fcntl.h>
#include <unistd.h>

int main()
{
    int fd = open(
        "output.txt",
        O_WRONLY | O_CREAT | O_TRUNC,
        0644
    );

    dup2(
        fd,
        STDOUT_FILENO
    );

    close(fd);

    execlp(
        "ls",
        "ls",
        "-l",
        nullptr
    );

    return 0;
}
```

Now:

```text
ls output
     |
     v
output.txt
```

---

# 44. `system()` vs `exec()`

`system()`:

```cpp
system("ls -l");
```

runs a command through a shell.

`exec()`:

```cpp
execlp(
    "ls",
    "ls",
    "-l",
    nullptr
);
```

directly replaces the current process image.

Conceptually:

```text
system()
    -> shell involved

exec()
    -> directly execute program
```

For process-control interviews, understand:

```text
fork()
+
exec()
```

rather than using `system()`.

---

# 45. Simple `system()` Example

```cpp
#include <iostream>
#include <cstdlib>

using namespace std;

int main()
{
    cout << "Running command\n";

    system("ls -l");

    cout << "Command finished\n";

    return 0;
}
```

---

# 46. `vfork()`

`vfork()` is related to `fork()` but has stricter semantics.

Basic idea:

```text
vfork()
    -> creates child
    -> parent is suspended
    -> child should quickly call exec() or _exit()
```

Example:

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

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

    cout << "Parent\n";

    return 0;
}
```

Important:

Do not treat `vfork()` like a normal `fork()`.

For normal interview code, prefer:

```cpp
fork()
+
exec()
```

---

# 47. Process Groups

Processes can belong to a process group.

A process has:

```text
PID
PPID
PGID
SID
```

Useful function:

```cpp
getpgrp()
```

Example:

```cpp
#include <iostream>
#include <unistd.h>

using namespace std;

int main()
{
    cout << "PID  = "
         << getpid()
         << endl;

    cout << "PPID = "
         << getppid()
         << endl;

    cout << "PGID = "
         << getpgrp()
         << endl;

    return 0;
}
```

---

# 48. Signals

A signal is an asynchronous notification sent to a process.

Examples:

```text
SIGINT
SIGTERM
SIGKILL
SIGSTOP
SIGCHLD
```

Common meanings:

```text
SIGINT
    -> Interrupt, commonly Ctrl+C

SIGTERM
    -> Request termination

SIGKILL
    -> Force termination

SIGSTOP
    -> Stop process

SIGCHLD
    -> Child state changed
```

---

# 49. `kill()`

Despite its name:

```cpp
kill()
```

does not necessarily mean "kill the process."

It sends a signal.

Example:

```cpp
kill(
    pid,
    SIGTERM
);
```

This sends:

```text
SIGTERM
```

to the process.

---

# 50. Send Signal to Child

```cpp
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

using namespace std;

int main()
{
    pid_t pid = fork();

    if (pid == 0)
    {
        while (true)
        {
            cout << "Child running\n";
            sleep(1);
        }
    }
    else
    {
        sleep(3);

        cout << "Parent sending SIGTERM\n";

        kill(
            pid,
            SIGTERM
        );

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

# 51. `SIGKILL`

You can send:

```cpp
kill(
    pid,
    SIGKILL
);
```

Important:

```text
SIGKILL cannot be caught.
SIGKILL cannot be ignored.
SIGKILL cannot be handled by user code.
```

Similarly:

```text
SIGSTOP
```

cannot be caught or ignored.

---

# 52. `signal()`

You can register a signal handler.

Example:

```cpp
#include <iostream>
#include <unistd.h>
#include <signal.h>

using namespace std;

void handler(int signal)
{
    cout << "Signal received: "
         << signal
         << endl;
}

int main()
{
    signal(
        SIGINT,
        handler
    );

    while (true)
    {
        cout << "Running...\n";
        sleep(1);
    }

    return 0;
}
```

Run:

```bash
./signal
```

Press:

```text
Ctrl+C
```

The handler runs.

---

# 53. Important Signal Rule

Do not assume every normal C++ operation is safe inside a signal handler.

For simple interview examples, keep handlers minimal.

For robust POSIX code, `sigaction()` is generally preferred over `signal()`.

---

# 54. `sigaction()`

Basic example:

```cpp
#include <iostream>
#include <unistd.h>
#include <signal.h>

using namespace std;

void handler(
    int signal
)
{
    cout << "Signal received\n";
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
    {
        sleep(1);
    }

    return 0;
}
```

Compile:

```bash
g++ sigaction.cpp -o sigaction
```

---

# 55. `signal()` vs `sigaction()`

For interviews:

```text
signal()
    -> simple API

sigaction()
    -> more robust POSIX interface
    -> more control over signal handling
```

Modern POSIX code generally prefers:

```cpp
sigaction()
```

---

# 56. `SIGCHLD`

`SIGCHLD` is sent to a parent when a child changes state, commonly when it terminates.

Conceptually:

```text
Parent
   |
   +---- Child
            |
            v
          exits
            |
            v
        SIGCHLD
            |
            v
         Parent
```

The parent can use this together with:

```cpp
waitpid()
```

to reap children.

---

# 57. `SIGCHLD` Example

```cpp
#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

using namespace std;

void handler(int signal)
{
    cout << "SIGCHLD received\n";
}

int main()
{
    signal(
        SIGCHLD,
        handler
    );

    pid_t pid = fork();

    if (pid == 0)
    {
        sleep(2);
        return 0;
    }

    waitpid(
        pid,
        nullptr,
        0
    );

    cout << "Child reaped\n";

    return 0;
}
```

For production-quality asynchronous child reaping, use careful `sigaction()`/`waitpid()` handling rather than relying on `cout` inside a signal handler.

---

# 58. Process States

A process can conceptually be in states such as:

```text
NEW
 |
 v
READY
 |
 v
RUNNING
 |
 +-------> WAITING
 |             |
 |             v
 +---------- READY
 |
 v
TERMINATED
```

Linux tools may show states such as:

```text
R -> Running
S -> Sleeping
D -> Uninterruptible sleep
T -> Stopped
Z -> Zombie
```

---

# 59. `ps` Command

Useful command:

```bash
ps
```

More detailed:

```bash
ps aux
```

Process tree:

```bash
pstree
```

Find process:

```bash
ps -ef
```

---

# 60. Parent-Child Process Tree

Example:

```text
init/systemd
     |
     +---- shell
             |
             +---- program
                     |
                     +---- child
                             |
                             +---- grandchild
```

Each process has:

```text
PID
PPID
```

---

# 61. Important Difference: `fork()` vs `exec()`

This should be memorized.

```text
fork()
------
Creates a new process.

Parent and child both continue.

PID:
    Parent -> unchanged
    Child  -> new PID


exec()
------
Does not create a process.

Replaces current process image.

PID:
    unchanged
```

---

# 62. `fork()` vs `exec()` Example

```text
fork():

       Parent PID 100
             |
           fork()
             |
       +-----+-----+
       |           |
 PID 100        PID 101
 Parent          Child


exec():

PID 101
  |
 exec()
  |
  v
new program

PID remains 101
```

---

# 63. `fork()` vs `vfork()`

```text
fork()
    -> Parent and child can run independently.
    -> Copy-on-write.

vfork()
    -> Parent is suspended until child execs/exits.
    -> Has stricter usage requirements.
```

For most application code:

```text
fork()
+
exec()
```

is the normal pattern.

---

# 64. `fork()` + `wait()`

```text
Parent
   |
 fork()
   |
   +--------> Child
   |            |
 wait()         |
   |            |
   |         finishes
   |            |
   +<-----------+
```

Use this when parent needs to wait for child completion.

---

# 65. `fork()` + `exec()`

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

This is the basic model behind launching another program.

---

# 66. `fork()` + `pipe()` + `exec()`

```text
Parent
   |
   | write()
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

This is a very useful pattern for understanding shell pipelines.

---

# 67. Shell Pipeline Concept

When you run:

```bash
ls | wc -l
```

conceptually:

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

The shell can create processes, set up pipes, redirect descriptors with `dup2()`, and then use `exec()` to run the programs.

---

# 68. Example: `ls | wc -l`

A simplified implementation:

```cpp
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main()
{
    int fd[2];

    pipe(fd);

    pid_t p1 = fork();

    if (p1 == 0)
    {
        // First child: ls

        dup2(
            fd[1],
            STDOUT_FILENO
        );

        close(fd[0]);
        close(fd[1]);

        execlp(
            "ls",
            "ls",
            nullptr
        );

        _exit(1);
    }

    pid_t p2 = fork();

    if (p2 == 0)
    {
        // Second child: wc

        dup2(
            fd[0],
            STDIN_FILENO
        );

        close(fd[0]);
        close(fd[1]);

        execlp(
            "wc",
            "wc",
            "-l",
            nullptr
        );

        _exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(
        p1,
        nullptr,
        0
    );

    waitpid(
        p2,
        nullptr,
        0
    );

    return 0;
}
```

Compile:

```bash
g++ pipeline.cpp -o pipeline
```

Run:

```bash
./pipeline
```

Conceptually this implements:

```bash
ls | wc -l
```

---

# 69. Why Close Pipe Ends in Pipeline?

Suppose:

```text
ls ----> pipe ----> wc
```

After setting up `dup2()`:

```text
ls
 |
stdout
 |
pipe
 |
stdin
 |
wc
```

Unused descriptors should be closed.

Otherwise the reader may keep waiting because some process still has the write end open.

This is a very common pipe interview question.

---

# 70. File Descriptors After `fork()`

After:

```cpp
fork();
```

the child inherits copies of the parent's file descriptors.

Conceptually:

```text
Parent
 fd 3 ----+
          |
          v
       Open File
          ^
          |
 fd 3 ----+
Child
```

The descriptors in parent and child refer to the same underlying open-file description.

This is why pipes can be used naturally with `fork()`.

---

# 71. File Descriptors and `exec()`

By default, many file descriptors remain open across `exec()` unless marked close-on-exec.

Therefore:

```text
fork()
   |
   v
child inherits FD
   |
   v
dup2()
   |
   v
exec()
   |
   v
new program uses redirected FD
```

This is the basis of:

```text
stdin/stdout redirection
pipes
shell pipelines
```

---

# 72. `exec()` Does Not Return on Success

Very important:

```cpp
execlp(
    "ls",
    "ls",
    nullptr
);

cout << "hello";
```

If `exec()` succeeds:

```text
cout << "hello";
```

is never executed.

If `exec()` fails:

```text
execution continues
```

Therefore:

```cpp
exec(...);

perror("exec failed");
```

is a common pattern.

---

# 73. Why `fork()` Before `exec()`?

Suppose you want to run:

```bash
ls
```

but don't want your parent program to disappear.

If you directly:

```cpp
exec(...);
```

your current process becomes `ls`.

Instead:

```text
Parent
  |
 fork()
  |
  +---- Child
          |
         exec()
          |
          v
          ls

Parent continues
```

This is why:

```text
fork() + exec()
```

is so common.

---

# 74. Process Creation Pattern

Memorize this template:

```cpp
pid_t pid = fork();

if (pid < 0)
{
    // fork failed
}
else if (pid == 0)
{
    // child

    execlp(
        "program",
        "program",
        nullptr
    );

    _exit(1);
}
else
{
    // parent

    waitpid(
        pid,
        nullptr,
        0
    );
}
```

This is one of the most useful Linux process templates for interviews.

---

# 75. Important Error Handling

System calls can fail.

For example:

```cpp
pid_t pid = fork();

if (pid == -1)
{
    perror("fork");
}
```

For `exec()`:

```cpp
if (execlp(...) == -1)
{
    perror("exec");
}
```

For `pipe()`:

```cpp
if (pipe(fd) == -1)
{
    perror("pipe");
}
```

For `waitpid()`:

```cpp
if (waitpid(...) == -1)
{
    perror("waitpid");
}
```

---

# 76. Common Interview Question

## Q: Does `fork()` copy the entire memory immediately?

Answer:

```text
Not necessarily.

Modern systems generally use Copy-on-Write.

Parent and child initially share physical pages
where possible.

When one process modifies a page,
the OS creates a private copy.
```

---

# 77. Common Interview Question

## Q: Does `exec()` create a new process?

Answer:

```text
No.

exec() replaces the current process image.

The PID remains the same.
```

---

# 78. Common Interview Question

## Q: Why use fork() + exec()?

Answer:

```text
fork() creates a child process.

exec() replaces the child with another program.

This allows the parent to continue running
while the child executes another program.
```

---

# 79. Common Interview Question

## Q: What happens to PID after exec()?

Answer:

```text
PID does not change.
```

Example:

```text
PID 1000
Program A

exec(Program B)

PID 1000
Program B
```

---

# 80. Common Interview Question

## Q: What happens if exec() fails?

Answer:

```text
exec() returns -1.

The current program continues executing
from the next statement.
```

Therefore:

```cpp
execlp(...);

perror("exec");
_exit(1);
```

---

# 81. Common Interview Question

## Q: What is a zombie?

Answer:

```text
A child that has terminated but whose parent
has not yet collected its termination status.
```

Usually:

```cpp
wait();
```

or:

```cpp
waitpid();
```

reaps it.

---

# 82. Common Interview Question

## Q: What is an orphan?

Answer:

```text
A child whose parent has terminated while
the child is still running.
```

The child is adopted by an appropriate system process/subreaper.

---

# 83. Common Interview Question

## Q: Can parent and child have the same PID?

Answer:

```text
No.

Every process has a unique PID at a given time.

Parent keeps its PID.
Child gets a new PID.
```

---

# 84. Common Interview Question

## Q: Is memory shared after fork()?

Answer:

```text
Parent and child have separate virtual address spaces.

Initially, physical pages may be shared using
Copy-on-Write.

A normal modification becomes private to that process.
```

---

# 85. Common Interview Question

## Q: What does the child get from fork()?

Conceptually, the child gets a copy of the parent's process state, including:

```text
Virtual address space
File descriptor table entries
Environment
Program state
```

But it has its own:

```text
PID
execution context
address-space identity
```

---

# 86. Common Interview Question

## Q: What happens to file descriptors after fork()?

The child inherits copies of the parent's file descriptors.

This is why:

```text
fork()
+
pipe()
```

works naturally.

---

# 87. Common Interview Question

## Q: What happens to file descriptors after exec()?

Normally, open file descriptors remain open across `exec()` unless configured close-on-exec.

This allows:

```text
fork()
dup2()
exec()
```

to implement:

```text
stdin redirection
stdout redirection
pipes
```

---

# 88. Common Interview Question

## Q: `wait()` vs `waitpid()`?

```text
wait()
    -> wait for a child

waitpid()
    -> wait for a specific child
    -> supports options such as WNOHANG
```

---

# 89. Common Interview Question

## Q: `exit()` vs `_exit()`?

```text
exit()
    -> normal process termination
    -> performs stdio/user-space cleanup

_exit()
    -> immediate process termination
    -> commonly used in forked child after exec failure
```

Example:

```cpp
if (exec_failed)
{
    _exit(1);
}
```

---

# 90. Common Interview Question

## Q: `system()` vs `exec()`?

```text
system()
    -> runs command through shell

exec()
    -> replaces current process image
```

For example:

```cpp
system("ls -l");
```

vs:

```cpp
execlp(
    "ls",
    "ls",
    "-l",
    nullptr
);
```

---

# 91. Common Interview Question

## Q: Why use `_exit()` after failed exec in a child?

After:

```text
fork()
```

the child inherits the parent's user-space state, including stdio buffers.

Using:

```cpp
_exit()
```

avoids normal stdio cleanup that could otherwise duplicate buffered output.

Common pattern:

```cpp
exec(...);

perror("exec failed");

_exit(1);
```

---

# 92. Common Interview Question

## Q: What is `dup2()` used for?

`dup2()` is commonly used for I/O redirection.

Example:

```cpp
dup2(
    pipefd[1],
    STDOUT_FILENO
);
```

Means:

```text
stdout
  |
  v
pipe write end
```

Then:

```cpp
exec(...)
```

causes the new program to inherit that redirection.

---

# 93. Common Interview Question

## Q: How does a shell implement `ls | wc`?

Conceptually:

```text
1. Create pipe

2. fork child for ls

3. Child:
       dup2(pipe_write, stdout)
       exec(ls)

4. fork child for wc

5. Child:
       dup2(pipe_read, stdin)
       exec(wc)

6. Parent closes pipe descriptors

7. Parent waits
```

Diagram:

```text
        PIPE
       /    \
      /      \
     v        v
    ls       wc
   stdout    stdin
```

---

# 94. Common Interview Question

## Q: How many processes after N forks?

If every fork succeeds and every resulting process executes every fork:

```text
2^N
```

Example:

```text
1 fork  -> 2
2 forks -> 4
3 forks -> 8
4 forks -> 16
```

---

# 95. Common Interview Question

Consider:

```cpp
fork();
cout << "A\n";
fork();
cout << "B\n";
```

Process count:

```text
Initial = 1

First fork:
    2

Second fork:
    4
```

Therefore:

```text
A -> printed 2 times
B -> printed 4 times
```

---

# 96. Common Interview Question

Consider:

```cpp
fork();

if (fork() == 0)
{
    cout << "Child\n";
}
```

After first fork:

```text
2 processes
```

Both execute second fork:

```text
4 processes
```

The second fork creates one child for each existing process.

Only the two newly created children enter:

```cpp
if (fork() == 0)
```

Therefore:

```text
"Child"
```

prints twice.

---

# 97. Process Creation Cheat Sheet

```text
fork()
    -> create process

getpid()
    -> current PID

getppid()
    -> parent PID

wait()
    -> wait for child

waitpid()
    -> wait for specific child

exit()
    -> terminate normally

_exit()
    -> terminate immediately

exec()
    -> replace program

kill()
    -> send signal
```

---

# 98. Exec Cheat Sheet

```text
l = list
v = vector
p = PATH
```

Therefore:

```text
execl
    list

execlp
    list + PATH

execv
    vector

execvp
    vector + PATH
```

---

# 99. Process Relationship Cheat Sheet

```text
fork()
   |
   +---- Parent
   |
   +---- Child
```

Child:

```text
new PID
same initial process state
separate virtual address space
```

Then:

```text
child
  |
 exec()
  |
  v
new program
```

Parent:

```text
waitpid()
```

---

# 100. Zombie vs Orphan Cheat Sheet

```text
ZOMBIE
------
Child finished
Parent alive
Parent hasn't reaped child

Fix:
wait()
waitpid()


ORPHAN
------
Parent finished
Child still running

System adopts child
```

---

# 101. Pipe + Process Cheat Sheet

```text
pipe(fd)

fd[0] = read
fd[1] = write

fork()

Parent:
    close(fd[0])
    write(fd[1], ...)

Child:
    close(fd[1])
    read(fd[0], ...)
```

---

# 102. Pipe + Exec Cheat Sheet

```text
pipe()

fork()

Child:

dup2(
    pipefd[0],
    STDIN_FILENO
);

close(...);

exec(...);
```

For stdout:

```cpp
dup2(
    pipefd[1],
    STDOUT_FILENO
);
```

---

# 103. Process + Signal Cheat Sheet

```text
kill(pid, signal)
```

Common:

```text
SIGINT
    Ctrl+C

SIGTERM
    graceful termination request

SIGKILL
    force termination

SIGSTOP
    stop process

SIGCHLD
    child state changed
```

---

# 104. Important Compile Commands

Basic process program:

```bash
g++ program.cpp -o program
```

Run:

```bash
./program
```

For pthread programs:

```bash
g++ program.cpp -pthread -o program
```

For most process examples in this file:

```bash
g++ program.cpp -o program
```

---

# 105. Most Important Code Templates

## Template 1 — fork

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

# 106. Template 2 — fork + wait

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

# 107. Template 3 — fork + exec

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

# 108. Template 4 — fork + exec + wait

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

# 109. Template 5 — Pipe

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

# 110. Template 6 — Pipe + exec

```cpp
int fd[2];

pipe(fd);

pid_t pid = fork();

if (pid == 0)
{
    dup2(
        fd[0],
        STDIN_FILENO
    );

    close(fd[0]);
    close(fd[1]);

    execlp(
        "program",
        "program",
        nullptr
    );

    _exit(1);
}
```

---

# 111. Final Mental Model

The most important Linux process concepts can be remembered as:

```text
                 PROCESS
                    |
        +-----------+-----------+
        |           |           |
        v           v           v
      fork()      exec()      wait()
        |           |           |
        v           v           v
    new process   replace     parent waits
```

Then IPC:

```text
Process A
    |
    +---- pipe -------> Process B
    |
    +---- FIFO -------> Process B
    |
    +---- shared -----> Process B
    |      memory
    |
    +---- message ----> Process B
           queue
```

And signals:

```text
Process A
    |
    | kill(pid, SIGTERM)
    v
Process B
```

---

# 112. Final One-Page Revision

```text
FORK
----
fork()
    -> creates child

return value:
    < 0  -> failure
    == 0 -> child
    > 0  -> parent, value = child PID


PID
---
getpid()
    -> current PID

getppid()
    -> parent PID


WAIT
----
wait()
    -> wait for any child

waitpid()
    -> wait for specific child

WNOHANG
    -> don't block


EXEC
----
exec()
    -> replaces current process image
    -> does NOT create process
    -> PID remains same

l = list
v = vector
p = PATH

execl
execlp
execv
execvp


EXIT
----
exit()
    -> normal termination

_exit()
    -> immediate termination
    -> commonly used after failed exec in child


ZOMBIE
------
Child finished
Parent hasn't reaped child

Use:
wait()
waitpid()


ORPHAN
------
Parent finished
Child still running

System adopts child


COPY-ON-WRITE
-------------
fork()
    -> virtual address spaces separate
    -> physical pages shared where possible
    -> copy made when modified


PIPE
----
fd[0] = read
fd[1] = write

pipe(fd)


DUP2
----
dup2(
    fd,
    STDIN_FILENO
);

dup2(
    fd,
    STDOUT_FILENO
);


SIGNALS
-------
kill(pid, signal)

SIGINT
SIGTERM
SIGKILL
SIGSTOP
SIGCHLD


SIGNAL HANDLING
---------------
signal()
sigaction()


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

Parent:
    waitpid()
```

---

# 113. Interview Golden Rules

```text
1. fork() creates a new process.

2. exec() does NOT create a new process.

3. exec() replaces the current process image.

4. PID remains unchanged across exec().

5. fork() returns:
       0  -> child
       >0 -> parent
       <0 -> error

6. Child gets a new PID.

7. fork() uses Copy-on-Write.

8. wait()/waitpid() reaps children.

9. Zombie = child finished, parent hasn't reaped it.

10. Orphan = parent finished, child still running.

11. Pipe is byte-stream IPC.

12. dup2() is commonly used for redirection.

13. fork() + exec() is the standard process-launch pattern.

14. system() typically involves a shell.

15. SIGKILL cannot be caught or ignored.

16. SIGSTOP cannot be caught or ignored.

17. Always close unused pipe ends.

18. exec() returns only on failure.

19. Use _exit() in a forked child after exec failure.

20. For:
       ls | wc -l

    think:

       pipe()
       fork()
       dup2()
       exec()
       fork()
       dup2()
       exec()
       close()
       waitpid()
```

# End

