#include <iostream>
#include <pthread.h>
#include<mutex>
#include<condition_variable>

using namespace std;

bool isReady = false;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void* consumer(void* arg)
{
	pthread_mutex_lock(&m);
	while(!isReady)
	{
		pthread_cond_wait(&cond, &m);
	}
	cout<<"Consumer is unblocked\n";
	pthread_mutex_unlock(&m);
}

void* producer(void* arg)
{
	pthread_mutex_lock(&m);
	isReady = true;
	pthread_cond_signal(&cond);
	cout<<"Producer sent signal\n";
	pthread_mutex_unlock(&m);
}

int main()
{
	pthread_t t1,t2;
	pthread_create(&t1, nullptr, producer, nullptr);
	pthread_create(&t2, nullptr, consumer, nullptr);
	pthread_join(t1, nullptr);
	pthread_join(t2, nullptr);
	return 0;
}
