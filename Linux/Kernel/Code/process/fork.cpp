#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        std::cout << "Child: PID = " << getpid() << "\n";
    }
    else if (pid > 0) {
        // Parent process
        std::cout << "Parent: PID = " << getpid() << "\n";

        wait(nullptr);  // Wait for child
    }
    else {
        // fork() failed
        std::cerr << "fork() failed\n";
    }

    return 0;
}
