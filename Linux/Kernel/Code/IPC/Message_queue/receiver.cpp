#include <iostream>
#include <mqueue.h>

int main() {
    const char* name = "/myqueue";

    mqd_t mq = mq_open(name, O_RDONLY);

    char buffer[100];

    mq_receive(mq, buffer, sizeof(buffer), nullptr);

    std::cout << "Received: " << buffer << "\n";

    mq_close(mq);
    mq_unlink(name);

    return 0;
}
