# ATM Example - Understanding Threads, Mutex & Condition Variable

We will solve the same problem in four stages.

```
Program 1
---------
Threads only
        ↓
Race Condition


Program 2
---------
Mutex
        ↓
Race Condition Fixed


Program 3
---------
Mutex + Polling
        ↓
Repeated Checking Problem


Program 4
---------
Condition Variable
        ↓
Waiting Without Wasting CPU
```

---

# Program 1 - Threads Only (Race Condition)

## Scenario

Initial Balance

```
$1000
```

Customer 1

```
Deposit $1
100000 times
```

Customer 2

```
Withdraw $1
100000 times
```

Expected result:

```
$1000
```

Possible results:

```
985

1007

963

1021
```

Every run may produce a different result.

---

## Code

```cpp
#include <iostream>
#include <thread>

using namespace std;

int balance = 1000;


void deposit()
{
    for(int i = 0; i < 100000; i++)
    {
        balance++;
    }
}


void withdraw()
{
    for(int i = 0; i < 100000; i++)
    {
        balance--;
    }
}


int main()
{
    thread t1(deposit);
    thread t2(withdraw);

    t1.join();
    t2.join();

    cout << "Final Balance = "
         << balance << endl;
}
```

---

# Why Race Condition Happens?

`balance++` is not a single operation internally.

It is actually:

```
Read balance

↓

Increase value

↓

Write balance
```

Similarly:

```
balance--
```

is:

```
Read balance

↓

Decrease value

↓

Write balance
```

Example:

Deposit thread:

```
Read balance

1000

↓

Increase

1001

↓

Write
```

Withdraw thread:

```
Read balance

1000

↓

Decrease

999

↓

Write
```

Both threads read the same old value.

One update is lost.

This is called:

```
Race Condition
```

---

# Program 2 - Mutex

Now we protect the shared variable.

A mutex allows only one thread at a time to execute the critical section.

```
Thread A

    |
    v

 lock mutex

    |
    v

 Update balance

    |
    v

 unlock mutex


Thread B waits
```

---

## Code

```cpp
#include <iostream>
#include <thread>
#include <mutex>

using namespace std;


int balance = 1000;

mutex atmMutex;


void deposit()
{
    for(int i = 0; i < 100000; i++)
    {
        lock_guard<mutex> lock(atmMutex);

        balance++;
    }
}


void withdraw()
{
    for(int i = 0; i < 100000; i++)
    {
        lock_guard<mutex> lock(atmMutex);

        balance--;
    }
}


int main()
{
    thread t1(deposit);
    thread t2(withdraw);

    t1.join();
    t2.join();

    cout << "Final Balance = "
         << balance << endl;
}
```

Output:

```
1000
```

---

## What Mutex Solves

Mutex guarantees:

```
Only one thread

        ↓

Can modify balance

        ↓

At a time
```

The race condition is removed.

---

# New Requirement

Current balance:

```
$500
```

Customer wants:

```
Withdraw $1000
```

The customer should wait until money is deposited.

---

# Program 3 - Mutex Only (Polling)

Without a condition variable, the thread repeatedly checks the condition.

---

## Code

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;


int balance = 500;

mutex atmMutex;


void withdraw()
{
    while(true)
    {
        atmMutex.lock();

        if(balance >= 1000)
        {
            balance -= 1000;

            cout << "Withdraw Successful\n";

            atmMutex.unlock();

            break;
        }

        atmMutex.unlock();


        cout << "Waiting for money...\n";


        this_thread::sleep_for(
            chrono::milliseconds(500));
    }
}


void deposit()
{
    this_thread::sleep_for(
        chrono::seconds(3));


    lock_guard<mutex> lock(atmMutex);


    balance += 1000;

    cout << "Salary Deposited\n";
}


int main()
{
    thread t1(withdraw);
    thread t2(deposit);


    t1.join();
    t2.join();


    cout << "Balance = "
         << balance << endl;
}
```

---

## Problem

Withdraw thread does:

```
Lock mutex

↓

Check balance

↓

Unlock mutex

↓

Sleep

↓

Check again

↓

Repeat
```

The thread is not doing useful work.

It is repeatedly asking:

```
"Is money available now?"
```

This is called:

```
Polling
```

---

# Program 4 - Condition Variable

Instead of repeatedly checking, the customer sleeps.

The bank wakes the customer when money arrives.

---

## Code

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace std;


int balance = 500;


mutex atmMutex;

condition_variable cv;


void withdraw()
{
    unique_lock<mutex> lock(atmMutex);


    cout << "Waiting for salary...\n";


    cv.wait(lock, []()
    {
        return balance >= 1000;
    });


    balance -= 1000;


    cout << "Withdraw Successful\n";
}


void deposit()
{
    this_thread::sleep_for(
        chrono::seconds(3));


    {
        lock_guard<mutex> lock(atmMutex);


        balance += 1000;


        cout << "Salary Deposited\n";
    }


    cv.notify_one();
}


int main()
{
    thread t1(withdraw);
    thread t2(deposit);


    t1.join();
    t2.join();


    cout << "Final Balance = "
         << balance << endl;
}
```

---

## Output

```
Waiting for salary...

(3 seconds)

Salary Deposited

Withdraw Successful

Final Balance = 500
```

---

# What Happens Internally?

## Withdraw Thread

```
Acquire Mutex

↓

Check condition:

balance >= 1000 ?

↓

False

↓

Release Mutex automatically

↓

Sleep
```

---

## Deposit Thread

```
Acquire Mutex

↓

Increase balance

↓

Release Mutex

↓

notify_one()
```

---

## Withdraw Thread

```
Wake Up

↓

Acquire Mutex Again

↓

Check condition again

↓

balance >= 1000 ?

↓

True

↓

Withdraw money

↓

Continue execution
```

---

# Understanding the Lambda Condition

```cpp
cv.wait(lock, []()
{
    return balance >= 1000;
});
```

The lambda returns:

```
true
```

when:

```
balance >= 1000
```

Otherwise:

```
false
```

Internally, it behaves like:

```cpp
while(balance < 1000)
{
    cv.wait(lock);
}
```

The thread exits `wait()` only when the condition becomes true.

---

# Why Do We Still Need Mutex?

A condition variable does not protect shared data.

It only provides:

```
Sleep

and

Wake Up
```

The shared variable:

```cpp
balance
```

is still accessed by multiple threads.

Therefore, we need a mutex.

---

# Summary

| Requirement | Solution |
|------------|----------|
| Run tasks concurrently | `std::thread` |
| Protect shared data | `std::mutex` |
| Avoid repeated checking | `std::condition_variable` |
| Wake waiting thread | `notify_one()` / `notify_all()` |

---

# Memory Trick

```
Threads

↓

Race Condition

↓

Mutex

↓

Polling Problem

↓

Condition Variable
```
