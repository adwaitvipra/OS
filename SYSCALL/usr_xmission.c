#include<linux/kernel.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<stdio.h>
#include<errno.h>

#define __NR_xmission 442
int main(const int argc, const char *argv[])
{
	long retval;
	retval = syscall(__NR_xmission);
	if (retval < 0)
		perror("syscall failed ");
	else
		printf("syscall executed...\n");
	return 0;
}
