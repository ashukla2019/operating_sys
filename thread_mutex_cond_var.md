# ATM Example - Understanding Threads, Mutex & Condition Variable

We will solve the same problem in three stages.

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
Condition Variable
        ↓
Busy Waiting Removed
```

---

# Program 1 - Threads Only (Race Condition)

## Scenario

Initial Balance

```
₹1000
```

Customer 1

```
Deposit ₹1
100000 times
```

Customer 2

```
Withdraw ₹1
100000 times
```

Expected

```
1000
```

Actual

```
985

1007

963

1021
```

Every run is different.

## Code

```cpp
#include <iostream>
#include <thread>

using namespace std;

int balance = 1000;

void deposit()
{
    for(int i=0;i<100000;i++)
        balance++;
}

void withdraw()
{
    for(int i=0;i<100000;i++)
        balance--;
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

## Why?

Deposit thread

```
Read balance

↓

1000

↓

1001

↓

Write
```

Withdraw thread

```
Read balance

↓

1000

↓

999

↓

Write
```

Both read

```
1000
```

One update gets lost.

This is called

```
Race Condition
```

---

# Program 2 - Mutex

Now only one customer can use the ATM.

```
Customer A

↓

ATM

↓

Customer B waits
```

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
    for(int i=0;i<100000;i++)
    {
        lock_guard<mutex> lock(atmMutex);
        balance++;
    }
}

void withdraw()
{
    for(int i=0;i<100000;i++)
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

Output

```
1000
```

Always.

---

## Problem Solved

Mutex guarantees

```
Only one thread

↓

Access balance
```

---

# New Requirement

Balance

```
₹500
```

Customer wants

```
Withdraw ₹1000
```

Customer should wait

until salary is deposited.

---

# Program 3 - Mutex Only (Busy Waiting)

Without condition variable

Customer keeps checking.

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

Output

```
Waiting...

Waiting...

Waiting...

Waiting...

Salary Deposited

Withdraw Successful
```

---

## Problem

Withdraw thread

```
Lock

↓

Check

↓

Unlock

↓

Sleep

↓

Repeat
```

This is

```
Busy Waiting
```

CPU keeps waking the thread just to check the balance again.

---

# Program 4 - Condition Variable

Now customer sleeps.

Bank wakes customer.

## Code

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

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

Output

```
Waiting for salary...

(3 seconds)

Salary Deposited

Withdraw Successful

Final Balance = 500
```

---

# What Happens Internally?

### Withdraw Thread

```
Lock Mutex

↓

Balance >=1000 ?

↓

No

↓

Automatically Unlock Mutex

↓

Sleep
```

---

### Deposit Thread

```
Lock Mutex

↓

Deposit Salary

↓

Unlock Mutex

↓

notify_one()
```

---

### Withdraw Thread

```
Wake Up

↓

Lock Mutex Again

↓

Balance >=1000 ?

↓

Yes

↓

Withdraw

↓

Exit
```

---

# Why Do We Still Need a Mutex?

A condition variable **does not protect shared data**.

It only puts a thread to sleep and wakes it up.

The mutex is still required because both threads access the shared variable:

```cpp
balance
```

---

# Summary

| Problem | Solution |
|----------|----------|
| Run two tasks simultaneously | `std::thread` |
| Prevent race condition | `std::mutex` |
| Avoid busy waiting | `std::condition_variable` |

---

# Memory Trick

```
Threads

↓

Race Condition

↓

Mutex

↓

Busy Waiting

↓

Condition Variable
```
