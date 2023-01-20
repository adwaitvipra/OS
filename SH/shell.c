/*
 * BASIC SHELL TO BE EXTENDED
 * USING FORK() AND EXEC()
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

int main(int argc, char *argv[])
{
	int pid;
	char cmdbuf[1024];
	char *xargv[10];
	char *cmd_ret = NULL;

	while(1)
	{
		printf("\n> ");
		cmd_ret = fgets(cmdbuf, 1024, stdin);
		cmdbuf[strlen(cmdbuf) - 1] = '\0'; /* replacing newline with NUL */

		if(cmd_ret && strcmp("exit",cmdbuf))
		{
			//parse_cmd(cmdbuf);
			xargv[0] = strtok(cmdbuf, " "); /* getting the first token */
			for(int i=1; i < 10 && (xargv[i] = strtok(NULL, " ")) != NULL ; i++);

			pid = fork();
			if(!pid)
			{
				execv(xargv[0],xargv);
				printf("exec() failed...\n");
				exit(0);
			}
			else
				wait(NULL);
		}
		else
		{
			printf("exit\n");
			exit(0);
		}

	}
	return 0;
}
