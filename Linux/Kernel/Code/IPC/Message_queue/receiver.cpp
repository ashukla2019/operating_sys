#include <iostream>
#include <mqueue.h>
#include <fcntl.h>

int main() {
    const char* name = "/myqueue";

    mqd_t mq = mq_open(name, O_RDONLY);

    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    char buffer[100];

    if (mq_receive(mq, buffer, sizeof(buffer), nullptr) == -1) {
        perror("mq_receive");
        return 1;
    }

    std::cout << "Received: " << buffer << "\n";

    mq_close(mq);
    mq_unlink(name);

    return 0;
}
