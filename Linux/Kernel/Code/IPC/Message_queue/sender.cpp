#include <iostream>
#include <mqueue.h>
#include <cstring>
#include <fcntl.h>

int main() {
    const char* name = "/myqueue";

    struct mq_attr attr{};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 100;

    mqd_t mq = mq_open(name, O_CREAT | O_WRONLY, 0666, &attr);

    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    const char* msg = "Hello from sender!";

    if (mq_send(mq, msg, strlen(msg) + 1, 0) == -1) {
        perror("mq_send");
        return 1;
    }

    std::cout << "Message sent\n";

    mq_close(mq);

    return 0;
}
