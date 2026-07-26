# Reader-Writer Lock (Read-Write Lock) in C++

## What is a Reader-Writer Lock?

A **Reader-Writer Lock (RW Lock)** is a synchronization mechanism that allows:

- Multiple threads to **read** a shared resource simultaneously.
- Only one thread to **write** at a time.
- While a writer is writing, **no readers** are allowed to read.

It improves concurrency compared to a normal mutex when read operations are much more frequent than write operations.

---

# Why Not Use `std::mutex`?

A normal mutex allows only **one thread** to access the resource at any time.

Suppose three threads only want to read a shared variable.

```
Reader1 ----+
            |
Reader2 ----+----> Shared Data
            |
Reader3 ----+
```

With a mutex, execution looks like:

```
Reader1  Running
Reader2  Waiting
Reader3  Waiting
```

Although all three readers are only reading (not modifying anything), only one is allowed to proceed.

This unnecessarily reduces performance.

---

# Reader-Writer Lock Behavior

With a Reader-Writer Lock:

```
Reader1  Running
Reader2  Running
Reader3  Running

Writer   Waiting
```

Multiple readers execute concurrently.

When a writer arrives:

```
Reader1  Running
Reader2  Running
Reader3  Running

Writer   Waiting
```

After all readers finish:

```
Writer Running
```

While the writer is writing:

```
Reader4 Waiting
Reader5 Waiting
```

After writing completes:

```
Reader4 Running
Reader5 Running
```

---

# Basic Implementation (Reader Preference)

```cpp
#include <iostream>
#include <mutex>
#include <condition_variable>

class ReaderWriterLock
{
private:

    std::mutex mtx;
    std::condition_variable cv;

    int activeReaders = 0;
    bool writerActive = false;

public:

    //---------------------------------------------------
    // Acquire Read Lock
    //---------------------------------------------------
    void lockRead()
    {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait while a writer is writing.
        cv.wait(lock, [this]
        {
            return !writerActive;
        });

        ++activeReaders;
    }

    //---------------------------------------------------
    // Release Read Lock
    //---------------------------------------------------
    void unlockRead()
    {
        std::unique_lock<std::mutex> lock(mtx);

        --activeReaders;

        // If this is the last reader, wake waiting threads.
        if(activeReaders == 0)
        {
            cv.notify_all();
        }
    }

    //---------------------------------------------------
    // Acquire Write Lock
    //---------------------------------------------------
    void lockWrite()
    {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait until:
        // 1. No writer is active
        // 2. No readers are active
        cv.wait(lock, [this]
        {
            return !writerActive &&
                   activeReaders == 0;
        });

        writerActive = true;
    }

    //---------------------------------------------------
    // Release Write Lock
    //---------------------------------------------------
    void unlockWrite()
    {
        std::unique_lock<std::mutex> lock(mtx);

        writerActive = false;

        cv.notify_all();
    }
};
```

---

# Using the Lock

```cpp
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

ReaderWriterLock rwLock;

int sharedData = 0;

void reader(int id)
{
    rwLock.lockRead();

    std::cout << "Reader "
              << id
              << " reads "
              << sharedData
              << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    rwLock.unlockRead();
}

void writer(int id)
{
    rwLock.lockWrite();

    ++sharedData;

    std::cout << "Writer "
              << id
              << " writes "
              << sharedData
              << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    rwLock.unlockWrite();
}

int main()
{
    std::vector<std::thread> threads;

    threads.emplace_back(reader, 1);
    threads.emplace_back(reader, 2);
    threads.emplace_back(reader, 3);

    threads.emplace_back(writer, 1);

    threads.emplace_back(reader, 4);

    for(auto& t : threads)
    {
        t.join();
    }

    return 0;
}
```

---

# Possible Output

```
Reader 1 reads 0
Reader 2 reads 0
Reader 3 reads 0
Writer 1 writes 1
Reader 4 reads 1
```

Notice:

- Readers 1, 2, and 3 run together.
- Writer waits until every reader exits.
- Reader 4 waits until the writer finishes.

---

# How `lockRead()` Works

```cpp
void lockRead()
{
    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock,
    [this]
    {
        return !writerActive;
    });

    ++activeReaders;
}
```

## Step 1

Acquire the mutex.

```
Mutex Locked
```

Only one thread updates internal counters at a time.

---

## Step 2

Wait until

```
writerActive == false
```

If a writer is currently writing:

```
Reader
   │
   ▼
Wait
```

The reader sleeps instead of consuming CPU.

---

## Step 3

When no writer exists:

```
activeReaders++
```

Now this reader becomes active.

Multiple readers can repeat this process.

Example

```
Reader1

activeReaders = 1

Reader2

activeReaders = 2

Reader3

activeReaders = 3
```

All three readers are reading simultaneously.

---

# How `unlockRead()` Works

```cpp
--activeReaders;
```

Reader count decreases.

Example

```
Before

activeReaders = 3

Reader exits

activeReaders = 2
```

Nothing special happens.

When the last reader exits

```
activeReaders = 0
```

the waiting writer can proceed.

Therefore

```cpp
cv.notify_all();
```

wakes all sleeping threads.

---

# How `lockWrite()` Works

```cpp
void lockWrite()
{
    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock,
    [this]
    {
        return !writerActive &&
               activeReaders == 0;
    });

    writerActive = true;
}
```

A writer waits until

```
No active readers

AND

No active writer
```

Suppose

```
Readers = 3
Writer = false
```

Writer arrives.

```
Readers = 3

Writer Waiting
```

Writer cannot continue.

After every reader exits

```
Readers = 0
```

Writer wakes.

Then

```
writerActive = true
```

Now no other reader or writer can enter.

---

# How `unlockWrite()` Works

```cpp
writerActive = false;

cv.notify_all();
```

The writer finishes.

```
Writer Active = false
```

Now waiting readers and writers wake up and compete for the lock.

---

# Why Use `condition_variable`?

Without it, we might write

```cpp
while(writerActive)
{
}
```

This is called **busy waiting**.

Problems:

- Wastes CPU cycles.
- Continuously checks the condition.
- Poor performance.

Instead,

```cpp
cv.wait(lock, condition);
```

puts the thread to sleep.

It wakes only when another thread calls

```cpp
notify_all();
```

This is much more efficient.

---

# Internal State Example

Initially

```
Readers = 0
Writer = false
```

Reader1 arrives

```
Readers = 1
```

Reader2 arrives

```
Readers = 2
```

Reader3 arrives

```
Readers = 3
```

Writer arrives

```
Readers = 3

Writer Waiting
```

Reader1 exits

```
Readers = 2
```

Reader2 exits

```
Readers = 1
```

Reader3 exits

```
Readers = 0
```

Writer wakes

```
Writer = true
```

Writer finishes

```
Writer = false
```

Readers wake again.

---

# State Transition Diagram

```
              Reader Arrives
                     |
                     v
             +----------------+
             | Readers > 0    |
             +----------------+
                     |
          More Readers Allowed
                     |
                     v
             +----------------+
             | Writer Waiting |
             +----------------+
                     |
         Last Reader Leaves
                     |
                     v
             +----------------+
             | Writer Active  |
             +----------------+
                     |
             Writer Completes
                     |
                     v
             Readers Allowed Again
```

---

# Problem: Writer Starvation

This implementation gives priority to readers.

Suppose readers continuously arrive.

```
Reader
Reader
Reader
Reader
Reader
Reader
Reader
...
```

Each new reader increments

```
activeReaders
```

before it becomes zero.

The writer keeps waiting forever.

This is called

**Writer Starvation**

---

# Writer-Priority Solution

Add another variable.

```cpp
int waitingWriters = 0;
```

When a writer arrives

```cpp
waitingWriters++;
```

Readers should now wait if

```
writerActive == true

OR

waitingWriters > 0
```

Reader condition becomes

```cpp
cv.wait(lock,
[this]
{
    return !writerActive &&
           waitingWriters == 0;
});
```

When the writer starts

```cpp
waitingWriters--;
writerActive = true;
```

Now no new readers enter while writers are waiting.

This avoids starvation.

---

# Time Complexity

| Operation | Complexity |
|-----------|------------|
| lockRead() | O(1) |
| unlockRead() | O(1) |
| lockWrite() | O(1) |
| unlockWrite() | O(1) |

---

# Advantages

- Allows concurrent readers.
- Better performance for read-heavy workloads.
- Efficient synchronization.
- Eliminates busy waiting.
- Commonly used in caches, databases, and file systems.

---

# Disadvantages

- More complex than a mutex.
- Incorrect implementation may cause starvation.
- Slightly higher overhead than a simple mutex.

---

# C++17 Standard Solution

C++17 provides a built-in Reader-Writer Lock.

```cpp
#include <shared_mutex>

std::shared_mutex rwLock;
```

## Reader

```cpp
std::shared_lock<std::shared_mutex> lock(rwLock);
```

Many readers can hold this lock simultaneously.

---

## Writer

```cpp
std::unique_lock<std::shared_mutex> lock(rwLock);
```

Only one writer is allowed.

---

# Custom RW Lock vs `std::shared_mutex`

| Feature | Custom RW Lock | `std::shared_mutex` |
|----------|----------------|---------------------|
| Learning | Excellent | Limited |
| Production Use | Rare | Recommended |
| Performance | Depends on implementation | Highly optimized |
| Thread Safety | Programmer responsibility | Standard library tested |
| Starvation Handling | Must be implemented | Implementation dependent |

---

# Interview Questions

### Why is a Reader-Writer Lock faster than a mutex?

Because multiple readers can execute simultaneously, while a mutex serializes all access.

---

### Why is `condition_variable` used?

To avoid busy waiting. Threads sleep until notified instead of continuously checking a condition.

---

### Can multiple writers execute together?

No.

Only one writer is allowed at a time.

---

### Can readers execute while a writer is writing?

No.

Readers must wait until the writer finishes.

---

### What is Writer Starvation?

When readers continuously acquire the lock, preventing waiting writers from ever executing.

---

### When should a Reader-Writer Lock be used?

Use it when:

- Reads are much more frequent than writes.
- Shared data is mostly read-only.
- High concurrency is required.

Examples include:

- Database systems
- Caches
- Configuration data
- File systems
- In-memory key-value stores
