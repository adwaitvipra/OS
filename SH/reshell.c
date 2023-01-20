/*
 * SHELL USING FORK & EXEC
 * TO BE EXTENDED TO SUPPORT PIPES
 */

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>

#define MAX_STR_SIZE 1024
#define MAX_PROMPT_SIZE 1024
#define MAX_PATH_ENTRIES 128 
#define MAX_ARG_CNT 128

const char prompt_tok[] = "PS1=";
const char path_tok[] = "PATH=";

char *path[MAX_PATH_ENTRIES];
char prompt[MAX_PROMPT_SIZE];

void exit_shell()
{

}

bool change_directory(char *cmd)
{
	bool flag = false;
	return flag;
}

bool change_prompt(char *str)
{
	return ;
}

bool change_path(char *str)
{
	return ;
}

void exec_rel_abs(char *str)
{
	return ;
}
void exec_path(char *str)
{
	return ;
}

/* returns array of string from a string */
void parse_str(char *str)
{		
	/* use strtok_r() for thread safety */

	//exit
	if(strcmp("exit", strtok(str, " ")))
		exit_shell();

	//cd --> changes the prompt
	else if(strcmp("cd", strtok(str, " ")))
		change_directory(str, strtok(NULL, " "));

	//<, |, > --> requires fork + exec
	else if();

	//PS1="*" or ["\w$" --> CWD]
	else if(strncmp(prompt_tok, strtok(str, " "), strlen(prompt_tok)))
		change_prompt(str);

	//PATH=/bin:/usr/bin -->
	else if(strncmp(path_tok, strtok(str, " "), strlen(path_tok)))
		change_path(str);

	//CMD with or without /
	else if(strchr(strtok(str, " "), '/'))
		exec_rel_abs(str);
	else
		exec_path(str);
	return ;
}

char **parse_cmd(char *cmd)
{	
	char *save_ptr = NULL;
	char **vector = NULL;

	if((vector = (char**)malloc(sizeof(char*) * MAX_ARG_CNT)))
	{
		vector[0] = strtok_r(cmd, " ", &save_ptr);
		for(int i=1; (vector[i] = strtok_r(NULL, " ", &save_ptr)); i++)
			;
	}

	return vector;
}

int main(const int argc, const char *argv[])
{

	pid_t pid;
	char str[MAX_STR_SIZE];

	while(1)
	{
		if(/*fork required */)
		{
			if(!pid)
			{
				execv();
			}
			else
			{
				wait(NULL);
			}
		}
	}

	return 0;
}
