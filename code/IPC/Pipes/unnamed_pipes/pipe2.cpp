/*


cat file1.txt | wc => cat command o/p is written to pipe and then wc will read it from pipe and will give o/p
unnamed pipe is unidirectional.

Reading from pipe:
When process reads bytes from pipe, then those bytes are removed from pipe.
If pipe is empty and reader tries to reaad then read call blocks until some bytes are written to pipe.
If two process try to read from same pipe then one process will get some bytes and other get other bytes. so without synchronization
it's not possible to read by two process at same time.
If write end is closed and process tries to read, eOF will be received by reader process.

Writing to pipe:
If pipe is full and writer process attempts to write then write call blocks until there is enough space to pipe.
If read end of pipe is closed then write pipe will receive SIGPIPE signal by kernel.

*/
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <cstring>
#include <stdlib.h>

int main() 
{
	int ret_val;
	int pfd[2];
	char buff[32];
	char string1[]="String for pipe I/O";

	ret_val = pipe(pfd);                 /* Create pipe */
	if (ret_val != 0) 
	{             /* Test for success */
		printf("Unable to create a pipe; errno=%d\n",errno);

		exit(1);                         /* Print error message and exit */
	}
	if (fork() == 0) 
	{
		/* child program */
		close(pfd[0]); /* close the read end */
		ret_val = write(pfd[1],string1,strlen(string1)); /*Write to pipe*/
		if (ret_val != strlen(string1)) 
		{
		printf("Write did not return expected value\n");
		exit(2);                       /* Print error message and exit */
		}
	}
	else 
	{
		/* parent program */
		close(pfd[1]); /* close the write end of pipe */
		ret_val = read(pfd[0],buff,strlen(string1)); /* Read from pipe */
		if (ret_val != strlen(string1)) 
		{
			printf("Read did not return expected value\n");
			exit(3);                       /* Print error message and exit */
		}
		printf("parent read %s from the child program\n",buff);
	}
	exit(0);
}
