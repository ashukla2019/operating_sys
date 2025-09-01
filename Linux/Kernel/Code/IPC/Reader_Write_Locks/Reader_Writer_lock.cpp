"Multiple readers can access the resource simultaneously."

Ex: Imagine a shared resource, like a file or data in memory.
Readers are operations or threads that only look at (read) the data without changing it.
This means any number of readers can access the data at the same time without problems.
Since they are just reading, there's no risk of corrupting the data if multiple readers work in parallel.

"Writers must have exclusive access."
Writers are operations or threads that modify (write to) the shared data.
When a writer is writing:
No other writer can write at the same time.
No readers can read at the same time.
This exclusivity prevents data corruption or inconsistencies that can happen if someone reads or writes while the data is being changed.

Example:
If someone wants to edit (write in) the book, everyone else must stop reading or writing until that edit is done.

---------------------------------------------------------------------------------------------------------------------------------  
#include <mutex>
#include <condition_variable>

class RWLock {
private:
    //mtx — protects the internal state variables (like readers, writers, etc.).
    //cv — used to block and wake up threads based on lock availability.
    std::mutex mtx;
    std::condition_variable cv;

    int readers = 0;        // Number of active readers inside the lock
    int writers = 0;        // Number of active writers inside the lock (0 or 1)
    int waiting_writers = 0; // Number of writers waiting to acquire the lock

public:
    // Reader acquires shared access
    Lock the mutex (mtx) to modify shared state safely.
/*
Wait until:
No active writers (writers == 0)
No writers waiting (waiting_writers == 0) — this means writers get priority (no new readers if writers are waiting).
Once the wait condition is met:
Increment readers count.
This means this thread is now an active reader.
Then the function returns, allowing the reader thread to proceed concurrently with other readers.
*/
    void readerLock() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() {
            return writers == 0 && waiting_writers == 0;
        });
        readers++;
    }
    /*
    Lock the mutex to safely update shared state.
    Decrement the readers count because this reader is done.
    If this is the last reader (readers==0), notify all waiting threads (potentially writers) that they can try to acquire the lock now.
    */
    void readerUnlock() {
        std::unique_lock<std::mutex> lock(mtx);
        readers--;
        if (readers == 0) {
            cv.notify_all();  // Notify waiting writers
        }
    }

    /*
    Lock the mutex to update shared variables.
    Increase waiting_writers count, signaling this thread is a writer waiting for the lock.
    Wait until no active readers and no active writers (readers == 0 && writers == 0).
    Once the lock is acquired:
    Decrement waiting_writers (no longer waiting).
    Set writers = 1 indicating writer holds the lock.
    */
    // Writer acquires exclusive access
    void writerLock() {
        std::unique_lock<std::mutex> lock(mtx);
        waiting_writers++;
        cv.wait(lock, [this]() {
            return readers == 0 && writers == 0;
        });
        waiting_writers--;
        writers = 1;
    }
    /*
    Lock the mutex.
    Reset writers to zero, indicating no active writer.
    Notify all waiting threads (both readers and writers) that the lock may be available
    */
    void writerUnlock() {
        std::unique_lock<std::mutex> lock(mtx);
        writers = 0;
        cv.notify_all();  // Notify both readers and writers
    }
};
