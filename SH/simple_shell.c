/*
 * BASIC SHELL TO BE EXTENDED
 * USING FORK() AND EXEC()
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_ARG_CNT 128
char **command = NULL;

FILE *fh = NULL;
char **parse_cmd(char *cmd)
{
	cmd = strdup(cmd);
	char *token = NULL;
	char **vector = NULL;

	if ((vector = (char **)malloc(sizeof(char *) * MAX_ARG_CNT)))
	{
		for (int i = 0; i < MAX_ARG_CNT; i++)
			vector[i] = NULL;
		vector[0] = strdup(strtok(cmd, " "));
		fprintf(fh, "\n[ [%s]\t", vector[0]);
		for (int i = 1; (token = strtok(NULL, " ")); i++)
		{
			vector[i] = strdup(token);
			fprintf(fh, "[%s]\t", vector[i]);
		}

		printf("\n[");
		for (int i = 0; vector[i]; i++)
			printf("%s; ", vector[i]);
		printf("]\n");
	}

	free(cmd);
	return vector;
}

int main(int argc, char *argv[])
{
	int pid;
	char cmdbuf[1024];
	char *cmd_ret = NULL;
	fh = fopen("logsh", "w");

	while (1)
	{
		printf("> ");
		cmd_ret = fgets(cmdbuf, 1024, stdin);
		cmdbuf[strlen(cmdbuf) - 1] = '\0'; /* replacing newline with NUL */
		fprintf(fh, "%s\n", cmdbuf);

		if (cmd_ret && strcmp("exit", cmdbuf))
		{
			command = parse_cmd(cmdbuf);

			pid = fork();
			if (!pid)
			{
				fprintf(fh, "[");
				for (int i = 0; command[i]; i++)
					fprintf(fh, "[%s], ", command[i]);
				fprintf(fh, "]\n");
				execv(command[0], command);
				printf("exec() failed...\n");
				exit(0);
			}
			else
			{
				free(command);
				wait(NULL);
			}
		}
		else
		{
			fclose(fh);
			fprintf(fh, "%sexit\n", cmd_ret ? "" : "\n");
			exit(0);
		}
	}
	fclose(fh);
	return 0;
}
