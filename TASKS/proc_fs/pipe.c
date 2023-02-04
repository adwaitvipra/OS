#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>

int main(void)
{
	int n, pid;
	int pipefd[2];
	int from = open("/etc/passwd", O_RDONLY);
	int to = open("pipelog", O_CREAT|O_WRONLY, 0666);
	char buf[1024];

	if(!pipe(pipefd)) {
		pid = fork();
		if (!pid) {
			close(0);
			dup(pipefd[0]);
			close(pipefd[1]);

			while((n = read(0, buf, sizeof(buf))) > 0)
				write(to, buf, n);

			sleep(5);
		} else {
			close(1);
			dup(pipefd[1]);
			close(pipefd[0]);

			while((n = read(from, buf, sizeof(buf))) > 0)
				write(1, buf, n);

			close(1);
			dup(2);
			fprintf(stderr, "pid = %d\tcpid = %d\n", getpid(), pid);
			system("ps");
		}

		close(from);
		close(to);
		close(pipefd[0]);
		close(pipefd[1]);

	}
	else
		printf("pipe failed... \n");
	return 0;
}
