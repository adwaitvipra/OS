/*
 Reads a CSV file with columns Name, MISID, Marks;
 then calculates the average, total, and median marks,
 and saves all the result in another CSV file with columns  statistics, number.
 The filenames should be accepted as command-line arguments.
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

/*
typedef struct{
	char str[128];
	unsigned int mis;
	float score;
}stud_rec;
*/

double total(float *, int );
double average(float *, int );
float *readcsv(const char *, int *);
bool writecsv(const char *, float *, int );
void swap(float *, float *);
bool sort(float *, int );
float median(float *, int );

double total(float *arr, int size)
{
	double sum;
	for(sum = 0.0; size > 0; size--)
		sum += arr[size - 1];
	return sum;
}

double average(float *arr, int size)
{
	return total(arr, size) / size;
}

float *readcsv(const char *filename, int *cnt)
{
	int rec_count;
	float *arr = NULL;
	FILE *fh = NULL;
	char strbuf[512], *sptr;

	if((fh = fopen(filename, "r")))
	{
		rec_count = -1;
		while(fscanf(fh, "%s", strbuf) != EOF)
		       rec_count++;	
		*cnt = rec_count;

		rewind(fh);
		if(rec_count && (arr = (float *)malloc(sizeof(float)*(rec_count))))
		{
			fscanf(fh, "%s", strbuf);
			for(int i=0; i < rec_count && (fscanf(fh, "%s", strbuf) != EOF); i++)
			{
				sptr = rindex(strbuf,',');
				arr[i] = atof(++sptr);
			}
		}
		fclose(fh);
	}
	return arr;
}

bool writecsv(const char *filename, float *arr, int size)
{
	bool flag;
	FILE *fh = NULL;

	flag = false;
	if(arr && size > 0 &&
			(fh = fopen(filename, "w")))
	{
		sort(arr, size);
		fprintf(fh,"STATISTICS,VALUE\n");
		fprintf(fh,"SIZE,%d\n", size);
		fprintf(fh,"TOTAL,%.2f\n", total(arr, size));
		fprintf(fh,"AVERAGE,%3.2f\n", average(arr, size));
		fprintf(fh,"MEDIAN,%3.2f\n", median(arr, size));

		fclose(fh);
		flag = true;
	}
	return flag;
}

void swap(float *x, float *y)
{
	float tmp;
	tmp = *x;
	*x = *y;
	*y = tmp;
	return ;
}

bool sort(float *arr, int size)
{
	bool flag;
	int i, min, j;

	flag = false;
	if(arr && size > 0)
	{
		for(i=0; i<size; i++)
		{
			min = i;
			for(j=i+1; j<size; j++)
			{
				if(arr[j] < arr[min])
					min = j;
			}
			swap(&arr[min], &arr[i]);
		}
		flag = true;
	}

	return flag;
}

float median(float *arr, int size)
{
	float med = 0.0;
	if(arr && size > 0)
	{
		if(!(size%2))
			med = (arr[(size - 1)/2] + arr[(size - 1)/2 + 1])/2;
		else
			med = arr[size/2];
	}
	return med;
}

int main(int argc, const char *argv[])
{
	float *arr = NULL;
	int size;
	arr = readcsv(argv[1], &size);
	writecsv(argv[2], arr, size);
	free(arr);
	return 0;
}
