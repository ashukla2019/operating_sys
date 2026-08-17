# POSIX Threads (`pthread`) — C++ Interview Notes

Simple interview-oriented notes covering:

- `pthread_create()`
- `pthread_join()`
- `pthread_detach()`
- `pthread_exit()`
- Passing arguments
- Returning values
- Multiple threads
- Race conditions
- Mutex
- `trylock`
- Deadlock
- Condition variables
- Producer-consumer
- Interview cheat sheet

---

# 1. Compile pthread Programs

```bash
g++ main.cpp -std=c++17 -pthread -o main
```

Run:

```bash
./main
```

---

# 2. Basic `pthread_create()`

## Syntax

```cpp
pthread_create(
    &thread,
    attributes,
    function,
    argument
);
```

The thread function must have this signature:

```cpp
void* function(void* arg)
```

## Example

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

void* task(void* arg)
{
    cout << "Child thread\n";

    return nullptr;
}

int main()
{
    pthread_t t1;

    pthread_create(
        &t1,
        nullptr,
        task,
        nullptr
    );

    pthread_join(t1, nullptr);

    cout << "Main thread\n";

    return 0;
}
```

Possible output:

```text
Child thread
Main thread
```

The order can vary because threads execute concurrently.

---

# 3. Why Does the pthread Function Return `void*`?

`pthread_create()` expects:

```cpp
void* function(void* arg)
```

The `void*` allows us to:

1. Pass any type of argument.
2. Return any type of result.

Example:

```cpp
void* task(void* arg)
{
    return nullptr;
}
```

---

# 4. Passing an Argument to a Thread

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

void* task(void* arg)
{
    int* value = static_cast<int*>(arg);

    cout << "Value = " << *value << endl;

    return nullptr;
}

int main()
{
    pthread_t t1;

    int x = 10;

    pthread_create(
        &t1,
        nullptr,
        task,
        &x
    );

    pthread_join(t1, nullptr);

    return 0;
}
```

Here:

```cpp
&x
```

is passed into:

```cpp
void* arg
```

Then we convert it back:

```cpp
int* value = static_cast<int*>(arg);
```

---

# 5. Passing Multiple Arguments

A common approach is to use a `struct`.

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

struct Data
{
    int a;
    int b;
};

void* add(void* arg)
{
    Data* data = static_cast<Data*>(arg);

    int result = data->a + data->b;

    cout << "Result = " << result << endl;

    return nullptr;
}

int main()
{
    pthread_t t1;

    Data data{10, 20};

    pthread_create(
        &t1,
        nullptr,
        add,
        &data
    );

    pthread_join(t1, nullptr);

    return 0;
}
```

Output:

```text
Result = 30
```

---

# 6. Returning a Value From a Thread

A thread can return a pointer.

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

void* calculate(void* arg)
{
    int* result = new int(10 + 20);

    return result;
}

int main()
{
    pthread_t t1;

    pthread_create(
        &t1,
        nullptr,
        calculate,
        nullptr
    );

    void* returnValue = nullptr;

    pthread_join(
        t1,
        &returnValue
    );

    int* result = static_cast<int*>(returnValue);

    cout << "Result = " << *result << endl;

    delete result;

    return 0;
}
```

Important:

```cpp
pthread_join(t1, &returnValue);
```

gets the value returned by the thread.

---

# 7. Multiple Threads

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

void* task(void* arg)
{
    int id = *static_cast<int*>(arg);

    cout << "Thread " << id << endl;

    return nullptr;
}

int main()
{
    pthread_t threads[3];

    int ids[3] = {1, 2, 3};

    for (int i = 0; i < 3; i++)
    {
        pthread_create(
            &threads[i],
            nullptr,
            task,
            &ids[i]
        );
    }

    for (int i = 0; i < 3; i++)
    {
        pthread_join(
            threads[i],
            nullptr
        );
    }

    return 0;
}
```

Possible output:

```text
Thread 1
Thread 3
Thread 2
```

The order is NOT guaranteed.

---

# 8. `pthread_join()`

## Syntax

```cpp
pthread_join(thread, return_value);
```

Example:

```cpp
pthread_t t1;

pthread_create(
    &t1,
    nullptr,
    task,
    nullptr
);

pthread_join(
    t1,
    nullptr
);
```

Meaning:

```text
Main thread waits until t1 finishes.
```

---

# 9. Why `pthread_join()` Is Important

Consider:

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

void* task(void* arg)
{
    cout << "Child thread\n";

    return nullptr;
}

int main()
{
    pthread_t t1;

    pthread_create(
        &t1,
        nullptr,
        task,
        nullptr
    );

    cout << "Main exits\n";

    return 0;
}
```

The process can terminate before the child thread gets a chance to finish.

Better:

```cpp
pthread_join(t1, nullptr);
```

This makes the main thread wait.

---

# 10. `pthread_detach()`

Sometimes we don't want to join a thread.

We can detach it:

```cpp
pthread_detach(t1);
```

Example:

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

void* task(void* arg)
{
    cout << "Child thread\n";

    return nullptr;
}

int main()
{
    pthread_t t1;

    pthread_create(
        &t1,
        nullptr,
        task,
        nullptr
    );

    pthread_detach(t1);

    return 0;
}
```

A detached thread automatically releases its thread resources when it finishes.

Important:

```cpp
pthread_join()
```

and:

```cpp
pthread_detach()
```

are different approaches.

Do not join a thread after detaching it.

---

# 11. `pthread_exit()`

A thread can explicitly terminate itself:

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

void* task(void* arg)
{
    cout << "Child thread\n";

    pthread_exit(nullptr);
}

int main()
{
    pthread_t t1;

    pthread_create(
        &t1,
        nullptr,
        task,
        nullptr
    );

    pthread_join(
        t1,
        nullptr
    );

    return 0;
}
```

Usually:

```cpp
return nullptr;
```

is simpler.

---

# 12. Race Condition

Suppose two threads increment the same variable.

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

int counter = 0;

void* increment(void* arg)
{
    for (int i = 0; i < 100000; i++)
    {
        counter++;
    }

    return nullptr;
}

int main()
{
    pthread_t t1;
    pthread_t t2;

    pthread_create(
        &t1,
        nullptr,
        increment,
        nullptr
    );

    pthread_create(
        &t2,
        nullptr,
        increment,
        nullptr
    );

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    cout << "Counter = "
         << counter
         << endl;

    return 0;
}
```

You might expect:

```text
Counter = 200000
```

But you may get a smaller value.

For example:

```text
Counter = 173421
```

or:

```text
Counter = 185932
```

The exact result depends on thread scheduling.

This is a **race condition**.

---

# 13. Why Does `counter++` Cause a Race?

This:

```cpp
counter++;
```

is conceptually:

```text
1. Read counter
2. Add 1
3. Write counter
```

Suppose:

```text
counter = 10
```

Thread 1:

```text
Read 10
```

Thread 2:

```text
Read 10
```

Thread 1:

```text
10 + 1
Write 11
```

Thread 2:

```text
10 + 1
Write 11
```

Expected:

```text
12
```

Actual:

```text
11
```

One increment was lost.

---

# 14. Critical Section

A critical section is the part of code that accesses shared data and must be protected.

Example:

```cpp
counter++;
```

If multiple threads can execute it simultaneously, we need synchronization.

---

# 15. Solution: Mutex

A mutex provides mutual exclusion.

Only one thread can hold the mutex at a time.

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

int counter = 0;

pthread_mutex_t mutex =
    PTHREAD_MUTEX_INITIALIZER;

void* increment(void* arg)
{
    for (int i = 0; i < 100000; i++)
    {
        pthread_mutex_lock(&mutex);

        counter++;

        pthread_mutex_unlock(&mutex);
    }

    return nullptr;
}

int main()
{
    pthread_t t1;
    pthread_t t2;

    pthread_create(
        &t1,
        nullptr,
        increment,
        nullptr
    );

    pthread_create(
        &t2,
        nullptr,
        increment,
        nullptr
    );

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    cout << "Counter = "
         << counter
         << endl;

    pthread_mutex_destroy(&mutex);

    return 0;
}
```

Expected:

```text
Counter = 200000
```

---

# 16. Basic Mutex Pattern

Memorize this:

```cpp
pthread_mutex_lock(&mutex);

// Critical Section

pthread_mutex_unlock(&mutex);
```

Example:

```cpp
pthread_mutex_lock(&mutex);

counter++;

pthread_mutex_unlock(&mutex);
```

The code between lock and unlock is the critical section.

---

# 17. Creating a Mutex Manually

Instead of:

```cpp
pthread_mutex_t mutex =
    PTHREAD_MUTEX_INITIALIZER;
```

you can do:

```cpp
pthread_mutex_t mutex;

pthread_mutex_init(
    &mutex,
    nullptr
);
```

Then:

```cpp
pthread_mutex_lock(&mutex);

// Critical section

pthread_mutex_unlock(&mutex);
```

Finally:

```cpp
pthread_mutex_destroy(&mutex);
```

Complete example:

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

int counter = 0;

pthread_mutex_t mutex;

void* increment(void* arg)
{
    for (int i = 0; i < 100000; i++)
    {
        pthread_mutex_lock(&mutex);

        counter++;

        pthread_mutex_unlock(&mutex);
    }

    return nullptr;
}

int main()
{
    pthread_mutex_init(
        &mutex,
        nullptr
    );

    pthread_t t1;
    pthread_t t2;

    pthread_create(
        &t1,
        nullptr,
        increment,
        nullptr
    );

    pthread_create(
        &t2,
        nullptr,
        increment,
        nullptr
    );

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    cout << counter << endl;

    pthread_mutex_destroy(&mutex);

    return 0;
}
```

---

# 18. `pthread_mutex_trylock()`

Normally:

```cpp
pthread_mutex_lock(&mutex);
```

waits until the mutex becomes available.

But:

```cpp
pthread_mutex_trylock(&mutex);
```

does not wait.

Example:

```cpp
if (pthread_mutex_trylock(&mutex) == 0)
{
    cout << "Got the mutex\n";

    // Critical section

    pthread_mutex_unlock(&mutex);
}
else
{
    cout << "Mutex is already locked\n";
}
```

Difference:

```text
pthread_mutex_lock()
    |
    +-- Waits for mutex

pthread_mutex_trylock()
    |
    +-- Does not wait
```

---

# 19. Common Mutex Mistake

BAD:

```cpp
pthread_mutex_lock(&mutex);

if (condition)
{
    return nullptr;
}

pthread_mutex_unlock(&mutex);
```

If `condition` is true, the mutex is never unlocked.

This can cause other threads to wait forever.

Better:

```cpp
pthread_mutex_lock(&mutex);

if (condition)
{
    pthread_mutex_unlock(&mutex);

    return nullptr;
}

pthread_mutex_unlock(&mutex);
```

---

# 20. Deadlock

Deadlock occurs when threads wait for each other forever.

Suppose:

```cpp
pthread_mutex_t m1 =
    PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t m2 =
    PTHREAD_MUTEX_INITIALIZER;
```

Thread 1:

```cpp
pthread_mutex_lock(&m1);
pthread_mutex_lock(&m2);
```

Thread 2:

```cpp
pthread_mutex_lock(&m2);
pthread_mutex_lock(&m1);
```

Possible situation:

```text
Thread 1 owns m1
Thread 2 owns m2

Thread 1 waits for m2
Thread 2 waits for m1
```

Both wait forever.

---

# 21. Avoiding Deadlock

Always acquire multiple locks in the same order.

Thread 1:

```cpp
pthread_mutex_lock(&m1);
pthread_mutex_lock(&m2);
```

Thread 2:

```cpp
pthread_mutex_lock(&m1);
pthread_mutex_lock(&m2);
```

Both use:

```text
m1 -> m2
```

instead of:

```text
Thread 1: m1 -> m2
Thread 2: m2 -> m1
```

---

# 22. Condition Variable

A condition variable allows a thread to sleep until some condition becomes true.

Important functions:

```cpp
pthread_cond_wait()
pthread_cond_signal()
pthread_cond_broadcast()
```

Usually used together with a mutex.

Basic pattern:

```cpp
pthread_mutex_lock(&mutex);

while (!condition)
{
    pthread_cond_wait(
        &cond,
        &mutex
    );
}

// Condition is true

pthread_mutex_unlock(&mutex);
```

Another thread:

```cpp
pthread_mutex_lock(&mutex);

condition = true;

pthread_cond_signal(&cond);

pthread_mutex_unlock(&mutex);
```

---

# 23. Why Condition Variables?

BAD:

```cpp
while (!dataAvailable)
{
    // Keep checking
}
```

This is **busy waiting**.

The CPU keeps executing the loop.

Instead:

```cpp
while (!dataAvailable)
{
    pthread_cond_wait(
        &cond,
        &mutex
    );
}
```

The thread sleeps until it is notified.

---

# 24. Simple Condition Variable Example

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

pthread_mutex_t mutex =
    PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond =
    PTHREAD_COND_INITIALIZER;

bool ready = false;

void* consumer(void* arg)
{
    pthread_mutex_lock(&mutex);

    while (!ready)
    {
        pthread_cond_wait(
            &cond,
            &mutex
        );
    }

    cout << "Consumer: data is ready\n";

    pthread_mutex_unlock(&mutex);

    return nullptr;
}

void* producer(void* arg)
{
    pthread_mutex_lock(&mutex);

    ready = true;

    cout << "Producer: data is ready\n";

    pthread_cond_signal(&cond);

    pthread_mutex_unlock(&mutex);

    return nullptr;
}

int main()
{
    pthread_t producerThread;
    pthread_t consumerThread;

    pthread_create(
        &consumerThread,
        nullptr,
        consumer,
        nullptr
    );

    pthread_create(
        &producerThread,
        nullptr,
        producer,
        nullptr
    );

    pthread_join(
        consumerThread,
        nullptr
    );

    pthread_join(
        producerThread,
        nullptr
    );

    pthread_mutex_destroy(&mutex);

    pthread_cond_destroy(&cond);

    return 0;
}
```

---

# 25. What Does `pthread_cond_wait()` Do?

This:

```cpp
pthread_cond_wait(
    &cond,
    &mutex
);
```

conceptually does:

```text
1. Release the mutex
2. Put the thread to sleep
3. Another thread changes the state
4. Another thread calls signal()
5. Waiting thread wakes up
6. Waiting thread reacquires the mutex
7. Thread continues
```

Conceptually:

```text
LOCK mutex
     |
     v
condition false?
     |
    YES
     |
     v
pthread_cond_wait()
     |
     +---- unlock mutex
     |
     +---- sleep
     |
     |      another thread
     |      changes state
     |
     |      signal()
     |
     v
wake up
     |
     v
re-lock mutex
     |
     v
check condition again
```

---

# 26. Why Does `pthread_cond_wait()` Need a Mutex?

Because the condition and shared state need to be checked and changed safely.

Example:

```cpp
pthread_mutex_lock(&mutex);

while (buffer.empty())
{
    pthread_cond_wait(
        &notEmpty,
        &mutex
    );
}

int value = buffer.front();

pthread_mutex_unlock(&mutex);
```

The mutex protects the shared buffer.

The condition variable allows the thread to wait efficiently.

---

# 27. Why `while`, Not `if`?

Always use:

```cpp
while (!condition)
{
    pthread_cond_wait(
        &cond,
        &mutex
    );
}
```

instead of:

```cpp
if (!condition)
{
    pthread_cond_wait(
        &cond,
        &mutex
    );
}
```

After waking up, the condition must be checked again.

A wake-up does not necessarily mean the condition is still true.

Therefore:

```cpp
while
```

is the standard pattern.

---

# 28. `pthread_cond_signal()`

```cpp
pthread_cond_signal(&cond);
```

Wakes up one waiting thread.

Example:

```cpp
pthread_mutex_lock(&mutex);

ready = true;

pthread_cond_signal(&cond);

pthread_mutex_unlock(&mutex);
```

Meaning:

```text
Something changed.
Wake one waiting thread so it can check again.
```

---

# 29. `pthread_cond_broadcast()`

```cpp
pthread_cond_broadcast(&cond);
```

wakes up all threads waiting on that condition variable.

Example:

```cpp
pthread_mutex_lock(&mutex);

ready = true;

pthread_cond_broadcast(&cond);

pthread_mutex_unlock(&mutex);
```

Use:

```cpp
pthread_cond_signal()
```

when one waiting thread is enough.

Use:

```cpp
pthread_cond_broadcast()
```

when all waiting threads need to wake up and re-check the condition.

---

# 30. Producer-Consumer Problem

Classic problem:

```text
Producer
    |
    v
+-----------+
|  Buffer   |
|   Queue   |
+-----------+
    |
    v
Consumer
```

Producer puts data into the queue.

Consumer removes data from the queue.

Problems:

```text
1. Producer must wait if buffer is full.
2. Consumer must wait if buffer is empty.
3. Buffer must be protected from simultaneous access.
```

We use:

```text
mutex
+
condition variables
```

---

# 31. Very Simple Producer-Consumer

```cpp
#include <iostream>
#include <queue>
#include <pthread.h>
#include <unistd.h>

using namespace std;

queue<int> buffer;

const int MAX_SIZE = 5;

pthread_mutex_t mutex =
    PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t notEmpty =
    PTHREAD_COND_INITIALIZER;

pthread_cond_t notFull =
    PTHREAD_COND_INITIALIZER;

void* producer(void* arg)
{
    for (int i = 1; i <= 10; i++)
    {
        pthread_mutex_lock(&mutex);

        while (buffer.size() == MAX_SIZE)
        {
            pthread_cond_wait(
                &notFull,
                &mutex
            );
        }

        buffer.push(i);

        cout << "Produced: "
             << i
             << endl;

        pthread_cond_signal(&notEmpty);

        pthread_mutex_unlock(&mutex);

        usleep(100000);
    }

    return nullptr;
}

void* consumer(void* arg)
{
    for (int i = 1; i <= 10; i++)
    {
        pthread_mutex_lock(&mutex);

        while (buffer.empty())
        {
            pthread_cond_wait(
                &notEmpty,
                &mutex
            );
        }

        int value = buffer.front();

        buffer.pop();

        cout << "Consumed: "
             << value
             << endl;

        pthread_cond_signal(&notFull);

        pthread_mutex_unlock(&mutex);

        usleep(150000);
    }

    return nullptr;
}

int main()
{
    pthread_t producerThread;
    pthread_t consumerThread;

    pthread_create(
        &producerThread,
        nullptr,
        producer,
        nullptr
    );

    pthread_create(
        &consumerThread,
        nullptr,
        consumer,
        nullptr
    );

    pthread_join(
        producerThread,
        nullptr
    );

    pthread_join(
        consumerThread,
        nullptr
    );

    pthread_mutex_destroy(&mutex);

    pthread_cond_destroy(&notEmpty);

    pthread_cond_destroy(&notFull);

    return 0;
}
```

---

# 32. Producer-Consumer: Producer Explained

Producer first locks:

```cpp
pthread_mutex_lock(&mutex);
```

Then checks:

```cpp
while (buffer.size() == MAX_SIZE)
{
    pthread_cond_wait(
        &notFull,
        &mutex
    );
}
```

Meaning:

```text
If buffer is full:
    Sleep
```

Then:

```cpp
buffer.push(i);
```

Producer adds data.

Then:

```cpp
pthread_cond_signal(&notEmpty);
```

Meaning:

```text
There is now data available.
Wake a consumer.
```

Finally:

```cpp
pthread_mutex_unlock(&mutex);
```

---

# 33. Producer-Consumer: Consumer Explained

Consumer locks:

```cpp
pthread_mutex_lock(&mutex);
```

Then:

```cpp
while (buffer.empty())
{
    pthread_cond_wait(
        &notEmpty,
        &mutex
    );
}
```

Meaning:

```text
If buffer is empty:
    Sleep
```

Then:

```cpp
int value = buffer.front();

buffer.pop();
```

Consumer removes data.

Then:

```cpp
pthread_cond_signal(&notFull);
```

Meaning:

```text
There is now space in the buffer.
Wake a producer.
```

Finally:

```cpp
pthread_mutex_unlock(&mutex);
```

---

# 34. Producer-Consumer Mental Model

Producer:

```text
             LOCK
               |
               v
        Is buffer full?
          /         \
        YES          NO
         |            |
         v            v
       WAIT         PUSH
         |            |
         |            v
         |         SIGNAL
         |        notEmpty
         |            |
         +------------+
               |
             UNLOCK
```

Consumer:

```text
             LOCK
               |
               v
       Is buffer empty?
          /          \
        YES           NO
         |             |
         v             v
       WAIT          POP
         |             |
         |             v
         |          SIGNAL
         |           notFull
         |             |
         +-------------+
               |
             UNLOCK
```

---

# 35. Producer-Consumer Template

## Producer

```cpp
pthread_mutex_lock(&mutex);

while (buffer_is_full)
{
    pthread_cond_wait(
        &notFull,
        &mutex
    );
}

produce();

pthread_cond_signal(&notEmpty);

pthread_mutex_unlock(&mutex);
```

## Consumer

```cpp
pthread_mutex_lock(&mutex);

while (buffer_is_empty)
{
    pthread_cond_wait(
        &notEmpty,
        &mutex
    );
}

consume();

pthread_cond_signal(&notFull);

pthread_mutex_unlock(&mutex);
```

This is a very important pthread interview pattern.

---

# 36. Mutex vs Condition Variable

## Mutex

Purpose:

```text
Protect shared data.
```

Example:

```cpp
pthread_mutex_lock(&mutex);

counter++;

pthread_mutex_unlock(&mutex);
```

---

## Condition Variable

Purpose:

```text
Wait efficiently until some condition/state changes.
```

Example:

```cpp
while (buffer.empty())
{
    pthread_cond_wait(
        &notEmpty,
        &mutex
    );
}
```

---

# 37. Mutex Does NOT Mean "Wait for a Condition"

This:

```cpp
pthread_mutex_lock(&mutex);
```

means:

```text
Wait until I can acquire the mutex.
```

It does NOT mean:

```text
Wait until buffer has data.
```

For that, we use:

```cpp
pthread_cond_wait();
```

---

# 38. Simple Two-Thread Mutex Example

```cpp
#include <iostream>
#include <pthread.h>

using namespace std;

int counter = 0;

pthread_mutex_t mutex =
    PTHREAD_MUTEX_INITIALIZER;

void* worker(void* arg)
{
    for (int i = 0; i < 1000; i++)
    {
        pthread_mutex_lock(&mutex);

        counter++;

        pthread_mutex_unlock(&mutex);
    }

    return nullptr;
}

int main()
{
    pthread_t t1;
    pthread_t t2;

    pthread_create(
        &t1,
        nullptr,
        worker,
        nullptr
    );

    pthread_create(
        &t2,
        nullptr,
        worker,
        nullptr
    );

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    cout << "Final counter = "
         << counter
         << endl;

    pthread_mutex_destroy(&mutex);

    return 0;
}
```

Expected:

```text
Final counter = 2000
```

---

# 39. Multiple Shared Variables

Suppose:

```cpp
int balance;
int transactionCount;
```

Both are shared.

We can protect both with one mutex:

```cpp
pthread_mutex_lock(&mutex);

balance += 100;
transactionCount++;

pthread_mutex_unlock(&mutex);
```

Another thread cannot enter the protected section at the same time.

---

# 40. Mutex Initialization

Static initialization:

```cpp
pthread_mutex_t mutex =
    PTHREAD_MUTEX_INITIALIZER;
```

Dynamic initialization:

```cpp
pthread_mutex_t mutex;

pthread_mutex_init(
    &mutex,
    nullptr
);
```

Destroy:

```cpp
pthread_mutex_destroy(&mutex);
```

---

# 41. Condition Variable Initialization

Static initialization:

```cpp
pthread_cond_t cond =
    PTHREAD_COND_INITIALIZER;
```

Dynamic initialization:

```cpp
pthread_cond_t cond;

pthread_cond_init(
    &cond,
    nullptr
);
```

Destroy:

```cpp
pthread_cond_destroy(&cond);
```

---

# 42. Check Return Values

Real production code should check pthread errors.

Example:

```cpp
int result = pthread_create(
    &t1,
    nullptr,
    task,
    nullptr
);

if (result != 0)
{
    cout << "pthread_create failed\n";
}
```

`pthread_join()`:

```cpp
int result = pthread_join(
    t1,
    nullptr
);

if (result != 0)
{
    cout << "pthread_join failed\n";
}
```

Mutex:

```cpp
int result =
    pthread_mutex_lock(&mutex);

if (result != 0)
{
    cout << "Lock failed\n";
}
```

For simple interview code, error checking is often omitted to focus on concurrency logic.

---

# 43. `pthread_create()` Arguments

The four arguments are:

```cpp
pthread_create(
    &thread,
    nullptr,
    function,
    argument
);
```

Meaning:

```text
&thread
    |
    +-- Where pthread_t is stored

nullptr
    |
    +-- Thread attributes
        (default)

function
    |
    +-- Function executed by new thread

argument
    |
    +-- Data passed to the function
```

Example:

```cpp
int x = 10;

pthread_create(
    &t1,
    nullptr,
    task,
    &x
);
```

---

# 44. `pthread_join()` Arguments

```cpp
pthread_join(
    t1,
    nullptr
);
```

First argument:

```text
t1
```

The thread we want to wait for.

Second argument:

```text
nullptr
```

We don't care about the thread's return value.

If we want the return value:

```cpp
void* result;

pthread_join(
    t1,
    &result
);
```

---

# 45. `pthread_join()` vs `pthread_detach()`

## Join

```cpp
pthread_join(
    t1,
    nullptr
);
```

Meaning:

```text
Wait for thread to finish.
```

Use when you need to synchronize with the thread.

## Detach

```cpp
pthread_detach(t1);
```

Meaning:

```text
I don't need to join this thread.
Clean up its resources automatically when it finishes.
```

Do not do:

```cpp
pthread_detach(t1);

pthread_join(t1, nullptr);
```

---

# 46. Race Condition vs Deadlock

## Race Condition

Multiple threads access shared data incorrectly.

Example:

```cpp
counter++;
```

Possible result:

```text
Incorrect value
```

Solution:

```cpp
mutex
```

---

## Deadlock

Threads wait for each other forever.

Example:

```text
Thread 1:
    lock A
    lock B

Thread 2:
    lock B
    lock A
```

Possible result:

```text
Both threads wait forever.
```

Solution:

```text
Consistent lock ordering
```

---

# 47. Race Condition vs Busy Waiting vs Deadlock

```text
Race Condition
    |
    +-- Shared data accessed without proper synchronization
    |
    +-- Result may be incorrect


Busy Waiting
    |
    +-- Thread continuously checks a condition
    |
    +-- Wastes CPU


Deadlock
    |
    +-- Threads wait for each other forever
    |
    +-- Program can get stuck
```

---

# 48. Most Important APIs

## Thread

```cpp
pthread_create()
pthread_join()
pthread_detach()
pthread_exit()
```

## Mutex

```cpp
pthread_mutex_init()
pthread_mutex_lock()
pthread_mutex_trylock()
pthread_mutex_unlock()
pthread_mutex_destroy()
```

## Condition Variable

```cpp
pthread_cond_init()
pthread_cond_wait()
pthread_cond_signal()
pthread_cond_broadcast()
pthread_cond_destroy()
```

---

# 49. Interview Cheat Sheet

## Create Thread

```cpp
pthread_t t;

pthread_create(
    &t,
    nullptr,
    function,
    argument
);
```

## Thread Function

```cpp
void* function(void* arg)
{
    // Work

    return nullptr;
}
```

## Wait

```cpp
pthread_join(
    t,
    nullptr
);
```

## Mutex

```cpp
pthread_mutex_lock(&mutex);

// Critical section

pthread_mutex_unlock(&mutex);
```

## Condition Variable

```cpp
pthread_mutex_lock(&mutex);

while (!condition)
{
    pthread_cond_wait(
        &cond,
        &mutex
    );
}

// Work

pthread_mutex_unlock(&mutex);
```

## Signal

```cpp
pthread_cond_signal(&cond);
```

## Broadcast

```cpp
pthread_cond_broadcast(&cond);
```

---

# 50. Most Important Interview Questions

## Threads

1. What is a thread?
2. What is `pthread_create()`?
3. What arguments does `pthread_create()` take?
4. Why does the thread function return `void*`?
5. How do you pass arguments to a pthread?
6. How do you return a value from a pthread?
7. What does `pthread_join()` do?
8. What happens if you don't join a thread?
9. What is `pthread_detach()`?
10. Difference between `join` and `detach`.

## Mutex

11. What is a mutex?
12. Why do we need a mutex?
13. What is a critical section?
14. What is a race condition?
15. Why is `counter++` not thread-safe?
16. What does `pthread_mutex_lock()` do?
17. What does `pthread_mutex_unlock()` do?
18. What is `pthread_mutex_trylock()`?
19. What happens if you forget to unlock a mutex?

## Deadlock

20. What is deadlock?
21. How can deadlock happen?
22. How can you prevent deadlock?
23. What is lock ordering?

## Condition Variables

24. What is a condition variable?
25. Why do we need condition variables?
26. What does `pthread_cond_wait()` do?
27. Why does `pthread_cond_wait()` take a mutex?
28. Why should we use `while` instead of `if`?
29. Difference between `pthread_cond_signal()` and `pthread_cond_broadcast()`.

## Producer-Consumer

30. Explain producer-consumer.
31. What happens when the buffer is full?
32. What happens when the buffer is empty?
33. Why do we need two condition variables?
34. Why do we need a mutex?
35. Explain the complete producer-consumer flow.

---

# 51. The Three Core Problems

## Problem 1: Race Condition

Without mutex:

```cpp
counter++;
```

Multiple threads modify shared data.

Solution:

```cpp
pthread_mutex_lock(&mutex);

counter++;

pthread_mutex_unlock(&mutex);
```

---

## Problem 2: Busy Waiting

Bad:

```cpp
while (buffer.empty())
{
}
```

Solution:

```cpp
while (buffer.empty())
{
    pthread_cond_wait(
        &notEmpty,
        &mutex
    );
}
```

---

## Problem 3: Producer-Consumer

Use:

```text
mutex
+
notEmpty condition variable
+
notFull condition variable
```

Producer:

```text
lock
while full:
    wait(notFull)

push

signal(notEmpty)
unlock
```

Consumer:

```text
lock
while empty:
    wait(notEmpty)

pop

signal(notFull)
unlock
```

---

# 52. One Final Mental Model

Think about pthread synchronization like this:

```text
                     SHARED DATA
                         |
                         v
                      [ MUTEX ]
                         |
              +----------+----------+
              |                     |
              v                     v
          Producer               Consumer
              |                     |
              |                     |
         buffer full?          buffer empty?
              |                     |
             YES                   YES
              |                     |
              v                     v
       wait(notFull)         wait(notEmpty)
              |                     |
              +----------+----------+
                         |
                         v
                  STATE CHANGES
                         |
                         v
                 signal/broadcast
```

---

# 53. The Most Important Pattern

If you remember only one condition-variable pattern, remember this:

```cpp
pthread_mutex_lock(&mutex);

while (!condition)
{
    pthread_cond_wait(
        &cond,
        &mutex
    );
}

// Condition is true.
// Do the work.

pthread_mutex_unlock(&mutex);
```

---

# 54. Producer-Consumer Pattern to Memorize

## Producer

```cpp
pthread_mutex_lock(&mutex);

while (buffer_full)
{
    pthread_cond_wait(
        &notFull,
        &mutex
    );
}

produce();

pthread_cond_signal(&notEmpty);

pthread_mutex_unlock(&mutex);
```

## Consumer

```cpp
pthread_mutex_lock(&mutex);

while (buffer_empty)
{
    pthread_cond_wait(
        &notEmpty,
        &mutex
    );
}

consume();

pthread_cond_signal(&notFull);

pthread_mutex_unlock(&mutex);
```

---

# 55. Final Revision Table

| API | Purpose |
|---|---|
| `pthread_create()` | Create a thread |
| `pthread_join()` | Wait for a thread |
| `pthread_detach()` | Detach a thread |
| `pthread_exit()` | Exit current thread |
| `pthread_mutex_init()` | Initialize mutex |
| `pthread_mutex_lock()` | Acquire mutex |
| `pthread_mutex_trylock()` | Try to acquire mutex without waiting |
| `pthread_mutex_unlock()` | Release mutex |
| `pthread_mutex_destroy()` | Destroy mutex |
| `pthread_cond_init()` | Initialize condition variable |
| `pthread_cond_wait()` | Sleep until condition notification |
| `pthread_cond_signal()` | Wake one waiting thread |
| `pthread_cond_broadcast()` | Wake all waiting threads |
| `pthread_cond_destroy()` | Destroy condition variable |

---

# 56. Final Interview Summary

Remember these relationships:

```text
pthread_create()
        |
        v
   CREATE THREAD
```

```text
pthread_join()
        |
        v
   WAIT FOR THREAD
```

```text
pthread_mutex_lock()
        |
        v
   ENTER CRITICAL SECTION
```

```text
pthread_mutex_unlock()
        |
        v
   LEAVE CRITICAL SECTION
```

```text
pthread_cond_wait()
        |
        v
   SLEEP UNTIL STATE MAY CHANGE
```

```text
pthread_cond_signal()
        |
        v
   WAKE ONE WAITING THREAD
```

```text
pthread_cond_broadcast()
        |
        v
   WAKE ALL WAITING THREADS
```

And the core concepts are:

```text
Race Condition
    -> Mutex

Busy Waiting
    -> Condition Variable

Producer-Consumer
    -> Mutex + Condition Variables

Deadlock
    -> Consistent Lock Ordering
```

---

# 57. Minimal Templates

## Thread

```cpp
void* task(void* arg)
{
    // work

    return nullptr;
}
```

## Create + Join

```cpp
pthread_t t;

pthread_create(
    &t,
    nullptr,
    task,
    nullptr
);

pthread_join(
    t,
    nullptr
);
```

## Mutex

```cpp
pthread_mutex_lock(&mutex);

// critical section

pthread_mutex_unlock(&mutex);
```

## Condition Variable

```cpp
pthread_mutex_lock(&mutex);

while (!condition)
{
    pthread_cond_wait(
        &cond,
        &mutex
    );
}

// work

pthread_mutex_unlock(&mutex);
```

## Producer

```cpp
pthread_mutex_lock(&mutex);

while (buffer_full)
{
    pthread_cond_wait(
        &notFull,
        &mutex
    );
}

produce();

pthread_cond_signal(&notEmpty);

pthread_mutex_unlock(&mutex);
```

## Consumer

```cpp
pthread_mutex_lock(&mutex);

while (buffer_empty)
{
    pthread_cond_wait(
        &notEmpty,
        &mutex
    );
}

consume();

pthread_cond_signal(&notFull);

pthread_mutex_unlock(&mutex);
```

---

# 58. Compile Any Example

```bash
g++ main.cpp -std=c++17 -pthread -o main
```

Run:

```bash
./main
```

---

# End

The most important things to master first are:

1. `pthread_create()`
2. `pthread_join()`
3. Passing arguments
4. Race condition
5. Mutex
6. Deadlock
7. `pthread_cond_wait()`
8. `pthread_cond_signal()`
9. Producer-consumer
10. Why `while` is used with condition variables

