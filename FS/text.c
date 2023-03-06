#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<limits.h>

int main(const int argc, const char *argv[])
{
	int fd;
	char buffer[4096];
	unsigned int blk_cnt = (unsigned int) (UINT_MAX / 4096);

	if (argc > 2)
		blk_cnt = atol(argv[2]);

	for (int i = 0; i < 4096; i++)
		buffer[i] = 'A' + i % 26;

	fd = open(argv[1], O_CREAT|O_APPEND|O_WRONLY);
	if (fd != -1)
	{
		for (unsigned int x = 0; x < blk_cnt; x++)
			write(fd, buffer, sizeof buffer);
		close(fd);
	}
	else
		perror("failed to open file\n");
	return 0;
}
