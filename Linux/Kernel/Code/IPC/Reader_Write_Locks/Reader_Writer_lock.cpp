#include <mutex>
#include <condition_variable>

class RWLock {
private:
    std::mutex mtx;
    std::condition_variable cv;

    int readers = 0;            // Number of active readers
    bool writer_active = false; // Whether a writer currently holds the lock
    int waiting_writers = 0;    // Number of writers waiting

public:
    // Acquire read lock.
    // Multiple readers can hold the lock simultaneously.
    // New readers are blocked if a writer is waiting.
    void readerLock() {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this]() {
            return !writer_active && waiting_writers == 0;
        });

        ++readers;
    }

    // Release read lock.
    void readerUnlock() {
        std::unique_lock<std::mutex> lock(mtx);

        --readers;

        // If this was the last reader,
        // a waiting writer may now proceed.
        if (readers == 0) {
            cv.notify_all();
        }
    }

    // Acquire write lock.
    // Only one writer can hold the lock.
    // Writers have priority over new readers.
    void writerLock() {
        std::unique_lock<std::mutex> lock(mtx);

        ++waiting_writers;

        cv.wait(lock, [this]() {
            return readers == 0 && !writer_active;
        });

        --waiting_writers;
        writer_active = true;
    }

    // Release write lock.
    void writerUnlock() {
        std::unique_lock<std::mutex> lock(mtx);

        writer_active = false;

        // Wake waiting readers/writers so they can
        // re-check their conditions.
        cv.notify_all();
    }
};
