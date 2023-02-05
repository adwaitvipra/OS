/* 
 * creates hierarchy (chain) of threads and prints pid of last thread.
 * total number of threads created including main is first argument or MAX_FORK.
 * second argument is time for sleeping of each thread.
 */
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

#define DEF_NAP 300
#define MAX_FORK 100

int main(const int argc, const char *argv[])
{
	pid_t cpid = 0;
	int nap = DEF_NAP;
	static int cnt = 1,
		   max = MAX_FORK;

	if (argc > 1)
	{
		max = atoi(argv[1]);
		if (argc == 2)
			nap = atoi(argv[2]);
	}

	while (cnt < max && !(cpid = fork()))
		cnt++;

	if (cpid)
	{
		sleep(nap);
		exit(0);
	}

	printf("%d\n", getpid());
	sleep(nap);

	return 0;
}
