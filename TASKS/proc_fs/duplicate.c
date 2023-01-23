/*
 * READ FROM ARG1, ARG2, WRITE TO ARG3
 * CONCATENATE INPUT OF ARG1 AND ARG2 
 */
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

int main(const int argc, const char *argv[])
{
	int n, fd[3];
	char buf[1024];
	for(int i=1; i<4;i++)
	{
		if(i < 3)
			fd[i-1] = open(argv[i], O_RDONLY);
		else
			fd[i-1] = open(argv[i], O_CREAT| O_WRONLY | O_APPEND, 0666);
	}

	close(0);
	dup(fd[0]);

	close(1);
	dup(fd[2]);

	while((n = read(0, buf, sizeof(buf))) > 0)
		write(1, buf, n);
	close(0);
	dup(fd[1]);
	while((n = read(0, buf, sizeof(buf))) > 0)
		write(1, buf, n);
	close(0);
	close(1);
	close(fd[0]);
	close(fd[1]);
	close(fd[2]);
	return 0;
}
