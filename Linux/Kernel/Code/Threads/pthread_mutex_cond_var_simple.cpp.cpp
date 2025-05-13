#include <iostream>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
bool data_ready = false;

void* consumer_thread(void* arg) {
    pthread_mutex_lock(&mutex);
    while (!data_ready) {
        pthread_cond_wait(&condition, &mutex);
    }
    std::cout << "Consumer: Data is ready, processing..." << std::endl;
    pthread_mutex_unlock(&mutex);
    return nullptr;
}

void* producer_thread(void* arg) {
    pthread_mutex_lock(&mutex);
    std::cout << "Producer: Producing data..." << std::endl;
    data_ready = true;
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&mutex);
    return nullptr;
}

int main() {
    pthread_t consumer, producer;

    pthread_create(&consumer, nullptr, consumer_thread, nullptr);
    pthread_create(&producer, nullptr, producer_thread, nullptr);

    pthread_join(consumer, nullptr);
    pthread_join(producer, nullptr);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condition);

    return 0;
}