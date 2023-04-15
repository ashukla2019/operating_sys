/*
A thread is an execution unit that has its own program counter, 
a stack and a set of registers that reside in a process.

Shared data b/w all threads of process: code segment, data segment and heap memory.
It is most effective on multi-processor or multi-core systems where the process 
flow can be scheduled to run on another processor thus gaining speed
through parallel or distributed processing.

Why threads are lightweight:
thread creation and termination takes less time than process creation and termination.
context switching b/w threads are faster.
communication b/w threads are faster than processes.

Thread Switching : Thread switching is a type of context switching from one 
thread to another thread in the same process. Thread switching is very efficient
and much cheaper because it involves switching out only identities and resources
such as the program counter, registers and stack pointers. 
The cost of thread-to-thread switching is about the same as the cost of 
entering and exiting the kernel.
    
Process Switching : Process switching is a type of context switching where 
we switch one process with another process.
It involves switching of all the process resources with those needed by 
a new process. This means switching the memory address space. 
This includes memory addresses, page tables, and kernel resources, caches
in the processor.

Thread models:
Thread Pool (Boss/Worker): One thread dispatches other threads to do useful work which are usually part of a worker thread pool. This thread pool is usually pre-allocated before the boss (or master) begins dispatching threads to work. Although threads are lightweight, they still incur overhead when they are created.

Peer (Workcrew):The peer model is similar to the boss/worker model except once the worker pool has been created, the boss becomes the another thread in the thread pool, and is thus, a peer to the other threads.

Pipeline:Similar to how pipelining works in a processor, each thread is part of a long chain in a processing factory. Each thread works on data processed by the previous thread and hands it off to the next thread. You must be careful to equally distribute work and take extra steps to ensure non-blocking behavior in this thread model or you could experience pipeline "stalls." 

Thread-safeness:Thread-safeness: in a nutshell, refers an application's ability to execute multiple threads simultaneously without "clobbering" shared data or creating "race"conditions

Three classes of Pthreads routines:
Thread management: creating, detaching, and joining threads, etc. They include functions to set/query thread attributes  (joinable, scheduling etc.)

Mutexes:  Mutex functions provide for creating, destroying, locking and unlocking mutexes.  They are also supplemented by mutex attribute functions that  set or modify attributes associated with mutexes.

•Condition variables: The third class of functions address  communications between threads that share a mutex. They are based upon  programmer specified  conditions. This class includes functions to create, destroy,  wait and signal based upon specified variable values.  Functions to set/query condition variable attributes are also included.

*/

//Passing arguments to thread function:

#include <stdio.h>
#include <pthread.h>

void * hello(void *arg) 
{
    printf("%s\n", (char *)arg);
    pthread_exit(NULL);
}

int main(void) 
{
    pthread_t tid;
    pthread_create(&tid, NULL, hello, "hello world");
    pthread_join(tid, NULL);
    return 0;
}

//Returning from thread:
void *myThread(void *args)
{
    return (void *)42;
}

int main(void)
{
    pthread_t tid;
    void *status;
    int ret;

    pthread_create(&tid, NULL, myThread, NULL);
    ret = pthread_join(tid, &status);

    if (ret) {
        fprintf(stderr, "pthread_join() failed\n");
        return -1;
    }

    /* pthread_join() copies the exit status of the target thread
	(i.e., the value that the target thread supplied to pthread_exit(3)) 
	into the location pointed to by *retval
     */
    printf("%ld\n", status);

    return 0;
}