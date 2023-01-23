#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

int main(const int argc, const char *argv[])
{
	int ret = 0;
	pid_t pid;
	pid = fork();
	if(!pid)
	{
		ret = execl(argv[1], argv[1], NULL);
		printf("exec failed... = %d\n", ret);
	}
	else
	{
		wait(NULL);
	}
	return 0;
}

