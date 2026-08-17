#include <iostream>
#include <mqueue.h>
#include <cstring>

int main() {
    const char* name = "/myqueue";

    mqd_t mq = mq_open(name, O_CREAT | O_WRONLY, 0666, nullptr);

    const char* msg = "Hello from sender!";
    mq_send(mq, msg, strlen(msg) + 1, 0);

    std::cout << "Message sent\n";

    mq_close(mq);
    return 0;
}
