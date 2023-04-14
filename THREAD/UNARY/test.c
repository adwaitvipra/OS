#include "thread.h"
#include <stdio.h>
#include <stdlib.h>

struct array
{
	int sum;
	int *arr;
	int start, end;
}array;

int fn(void *arg)
{
	fprintf(stderr, "inside fn\n");
	int ret = -1;
	struct array *arr = (struct array *) arg;
	if (arr)
	{
		ret = 0;
		for (int idx = arr->start; idx < arr->end; idx++)
		{
			fprintf(stderr, "arr[%d] = %d\n", idx, arr->arr[idx]);
			arr->sum += arr->arr[idx];
		}
	}
	fprintf(stderr, "returning fn\n");
	return ret;
}

int test(void *arg)
{
	mythread_exit();
	return 0;
}

int main(int argc, char *argv[])
{
	mythread_t thrds[3], sample;
	int cnt, sum = 0, arr[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	struct array arr_struct[3];
	for (int i = 0; i < 3; i++)
	{
		arr_struct[i].sum = 0;
		arr_struct[i].arr = arr;
		arr_struct[i].start = i*3;
		arr_struct[i].end = i*3 + 3;
	}

	cnt = 0;
	for (struct array *ptr = arr_struct; cnt < 3; ptr++, cnt++)
		mythread_create(&thrds[cnt], fn, (void *) ptr);
	for (cnt = 0; cnt < 3; cnt++)
		mythread_join(&thrds[cnt], NULL);
	for (cnt = 0; cnt < 3; cnt++)
		sum += arr_struct[cnt].sum;
	printf("sum = %d\n", sum);
	mythread_create(&sample, test, NULL);
	return 0;
}
