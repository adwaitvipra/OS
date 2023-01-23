/*
 * CREATING EXACTLY 3 PROC'S
 * USING SLEEP() FOR STOPPING CHILD PROC'S
 * PARENT USES WAIT()
 */
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>

int main()
{
	int cnt = 0;
	pid_t pid = 0;

	printf("%d) %d\n", cnt, getpid());

	pid = fork();
	if(!pid)
	{
		printf("%d) %d\n", ++cnt, getpid());
		sleep(3);
		exit(0);
	}
	else
	{
		if(pid)
			cnt++;
		pid = fork();
		if(!pid)
		{
			printf("%d) %d\n", ++cnt, getpid());
			sleep(2);
			exit(0);
		}
		else
		{
			if(pid)
				cnt++;
			pid = fork();
			if(!pid)
			{
				printf("%d) %d\n", ++cnt, getpid());
				sleep(1);
				exit(0);
			}
			else
			{
				if(pid)
					cnt++;
				system("ps");
				wait(NULL);
			}
		}
	}
	return 0;
}
