/* COMPETITION OR CO-OPERATION FOR NO RACE? */
/* PETERSON'S SOLUTION (S/W) FOR RACING */

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>

int turn;
bool run, atom = true;
bool flag[] = {false, false};

unsigned long var = 0, cntx = 0, cnty = 0;
void * thrdx (void *arg)
{
	do 
	{
		/* entry section, assuming atomicity */
		if (atom) 
		{
			flag[0]= true;
			turn = 1;
			atom = false;
		}
		else
			continue;

		/* co-operation */
		while (flag[1] && turn == 1)
			;
		/* critical section */
		var++;

		/* exit section */
		flag[0] = false;

		/* remainder section */
		cntx++;
	}while (run);

	return NULL;
}

void * thrdy (void *arg)
{
	do 
	{
		if (!atom)
		{
			flag[1] = true;
			turn = 0;
			atom = true;
		}
		else 
			continue;

		while (flag[0] && turn == 0)
			;

		var++;

		flag[1] = false;

		cnty++;
	}while (run);

	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thrds[2];
	void *(*fnarr[2])(void *) = {[0] thrdx, [1] thrdy};

	turn = 0;
	run = true;

	for (int n = 0; n < 2; n++)
	{
		if (pthread_create(&thrds[n], NULL, fnarr[n], NULL))
		{
			fprintf(stderr, "failed to create a thread...\n");
			return n;
		}
	}

	sleep((argc > 1) ? atoi(argv[1]) : 3);
	run = false;
	for (int n = 0; n < 2; n++)
		pthread_join(thrds[n], NULL);

	printf("var = %lu\ntot = %lu\n", var, cntx + cnty);

	return 0;
}
