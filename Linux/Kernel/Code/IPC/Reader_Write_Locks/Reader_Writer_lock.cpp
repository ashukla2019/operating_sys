#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

class RWLock {
private:
    std::mutex mtx;
    std::condition_variable cv;

    int readers = 0;
    bool writer = false;

public:
    // Reader lock
    void readLock() {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait while a writer is active
        cv.wait(lock, [this]() {
            return !writer;
        });

        readers++;
    }

    // Reader unlock
    void readUnlock() {
        std::unique_lock<std::mutex> lock(mtx);

        readers--;

        // If this was the last reader, wake waiting threads
        if (readers == 0)
            cv.notify_all();
    }

    // Writer lock
    void writeLock() {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait until no readers and no writer
        cv.wait(lock, [this]() {
            return readers == 0 && !writer;
        });

        writer = true;
    }

    // Writer unlock
    void writeUnlock() {
        std::unique_lock<std::mutex> lock(mtx);

        writer = false;

        cv.notify_all();
    }
};

int sharedData = 0;
RWLock rwlock;

void reader(int id) {
    rwlock.readLock();

    std::cout << "Reader " << id
              << " reads: " << sharedData << "\n";

    rwlock.readUnlock();
}

void writer(int id) {
    rwlock.writeLock();

    sharedData++;

    std::cout << "Writer " << id
              << " writes: " << sharedData << "\n";

    rwlock.writeUnlock();
}

int main() {
    std::vector<std::thread> threads;

    threads.emplace_back(reader, 1);
    threads.emplace_back(reader, 2);

    threads.emplace_back(writer, 1);

    threads.emplace_back(reader, 3);

    threads.emplace_back(writer, 2);

    for (auto& t : threads)
        t.join();

    return 0;
}
