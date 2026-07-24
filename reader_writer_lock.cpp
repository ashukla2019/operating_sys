#include <iostream>
#include <thread>
#include <pthread.h>
#include <chrono>

using namespace std;

int sharedData = 0;

// Reader-Writer Lock
pthread_rwlock_t rwlock;

// Reader
void reader(int id)
{
    // Acquire read lock
    pthread_rwlock_rdlock(&rwlock);

    cout << "Reader " << id
         << " reads : "
         << sharedData << endl;

    this_thread::sleep_for(chrono::seconds(1));

    // Release read lock
    pthread_rwlock_unlock(&rwlock);
}

// Writer
void writer(int id)
{
    // Acquire write lock
    pthread_rwlock_wrlock(&rwlock);

    sharedData++;

    cout << "Writer " << id
         << " writes : "
         << sharedData << endl;

    this_thread::sleep_for(chrono::seconds(1));

    // Release write lock
    pthread_rwlock_unlock(&rwlock);
}

int main()
{
    // Initialize reader-writer lock
    pthread_rwlock_init(&rwlock, nullptr);

    thread r1(reader, 1);
    thread r2(reader, 2);
    thread w1(writer, 1);

    r1.join();
    r2.join();
    w1.join();

    // Destroy lock
    pthread_rwlock_destroy(&rwlock);

    return 0;
}
