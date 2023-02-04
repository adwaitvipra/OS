#include<linux/kernel.h>
#include<sys/syscall.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>
#include<errno.h>

#define __NR_ancestry 443

int main(const int argc, const char *argv[])
{
	int pid;
	if(argc > 1)
		pid = atoi(argv[1]);
	else
		pid = getpid();
	long retval;

	retval = syscall(__NR_ancestry, pid);

	if(retval < 0)
		perror("syscall: ancestry: failed...\t");
	else
		printf("syscall: ancestry: successful...\n");

	return 0;
}
