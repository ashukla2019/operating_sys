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
    nullptr,       // Let OS choose address
    SIZE,           // Size to map
    PROT_READ | PROT_WRITE, // Permissions
    MAP_SHARED,    // Changes visible to other processes
    fd,            // Shared memory file descriptor
    0              // Offset
);

    // Write data
    strcpy(ptr, "Hello from shared memory!");

    std::cout << "Data written.\n";

    // Cleanup
    munmap(ptr, SIZE);
    close(fd);

    return 0;

}
