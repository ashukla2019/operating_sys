#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>

int main()
{
	const char* name = "/my_shared_memory";
	const size_t SIZE = 1024;
	
	// Create shared memory
	int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
	// Set size
    ftruncate(fd, SIZE);

    // Map into process memory
    char* ptr = (char*)mmap(
        nullptr, SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd, 0
    );

    // Read data
    std::cout << "Data: " << ptr << "\n";

    // Cleanup
    munmap(ptr, SIZE);
    close(fd);

    // Remove shared memory object
    shm_unlink(name);

    return 0;
}
