#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>

bool run;
long var = 0, cntx = 0, cnty = 0;

void *thrdx(void *arg)
{
	while (run)
	{
		var++;
		cntx++;
	}

	return NULL;
}

void *thrdy(void *arg)
{
	while (run)
	{
		var++;
		cnty++;
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thrds[2];
	void *(*fn[])(void *) = {[0] thrdx, [1] thrdy};

	run = true;
	for (int n = 0; n < 2; n++)
		if(pthread_create(&thrds[n], NULL, fn[n], NULL))
			fprintf(stderr, "failed to create a thread...\n");

	sleep((argc > 1) ? atoi(argv[1]) : 3);
	run = false;

	printf("var = %ld\ntot = %ld\n", var, cntx + cnty);
	return 0;
}
