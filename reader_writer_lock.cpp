#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

class ReaderWriterLock
{
private:

    std::mutex mtx;
    std::condition_variable cv;

    int activeReaders = 0;
    bool writerActive = false;

public:

    //------------------------------------
    // Acquire Read Lock
    //------------------------------------
    void lockRead()
    {
        std::unique_lock<std::mutex> lock(mtx);

        // Wait while a writer is writing
        cv.wait(lock, [this]
        {
            return !writerActive;
        });

        ++activeReaders;
    }

    //------------------------------------
    // Release Read Lock
    //------------------------------------
    void unlockRead()
    {
        std::unique_lock<std::mutex> lock(mtx);

        --activeReaders;

        if(activeReaders == 0)
            cv.notify_all();
    }

    //------------------------------------
    // Acquire Write Lock
    //------------------------------------
    void lockWrite()
    {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this]
        {
            return !writerActive && activeReaders == 0;
        });

        writerActive = true;
    }

    //------------------------------------
    // Release Write Lock
    //------------------------------------
    void unlockWrite()
    {
        std::unique_lock<std::mutex> lock(mtx);

        writerActive = false;

        cv.notify_all();
    }
};
