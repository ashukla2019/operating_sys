/*
Include semaphore.h
Compile the code by linking with -lpthread -lrt

To lock a semaphore or wait we can use the sem_wait function:
int sem_wait(sem_t *sem);

To release or signal a semaphore, we use the sem_post function:
int sem_post(sem_t *sem);

A semaphore is initialised by using sem_init(for processes or threads) or sem_open (for IPC).
sem_init(sem_t *sem, int pshared, unsigned int value);
*/
/*
In what conditions is Semaphore preferred over Mutex?
A semaphore is suitable when you have requirements that a given resource can be used by N different 
threads concurrently at the same time. A shared semaphore initialized to N will do the job. However, 
a mutex will not be sufficient since it allows at most 1 thread to execute the critical section 
at any given time.
There is one buffer of 4 kb and it has to be used by two threads: writer and reader. one thread can execute
critical section at a time.--> use mutex
Now 4kb buffer is divided into 4 parts and you have more than 4 threads for writing then semaphore would be 
correct choice.
*/

//code:

// C program to demonstrate working of Semaphores
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

sem_t mysem;

void* thread(void* arg)
{
	//wait
	sem_wait(&mysem);
	printf("\nEntered..\n");

	//critical section
	sleep(4);
	
	//signal
	printf("\nJust Exiting...\n");
	sem_post(&mysem);
}


int main()
{
	sem_init(&mysem, 0, 1); //
	pthread_t t1,t2;
	pthread_create(&t1,NULL,thread,NULL);
	sleep(2);
	pthread_create(&t2,NULL,thread,NULL);
	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	sem_destroy(&mysem);
	return 0;
}
