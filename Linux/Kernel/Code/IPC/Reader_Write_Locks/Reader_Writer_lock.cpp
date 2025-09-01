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
    std::mutex mtx;
    std::condition_variable cv;

    int readers = 0;        // active readers
    int writers = 0;        // active writers (0 or 1)
    int waiting_writers = 0; // helps give priority to writers

public:
    // Reader acquires shared access
    void readerLock() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() {
            return writers == 0 && waiting_writers == 0;
        });
        readers++;
    }

    void readerUnlock() {
        std::unique_lock<std::mutex> lock(mtx);
        readers--;
        if (readers == 0) {
            cv.notify_all();  // Notify waiting writers
        }
    }

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

    void writerUnlock() {
        std::unique_lock<std::mutex> lock(mtx);
        writers = 0;
        cv.notify_all();  // Notify both readers and writers
    }
};
