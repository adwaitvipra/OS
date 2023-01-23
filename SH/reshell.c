/*
 * SHELL USING FORK & EXEC
 * TO BE EXTENDED TO SUPPORT PIPES
 */

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<sys/stat.h>

#define MAX_ARG_CNT 128
#define MAX_STR_SIZE 1024
#define MAX_CWD_SIZE 1024
#define MAX_PROMPT_SIZE 1024
#define MAX_PATH_ENTRIES 128 

void exit_shell(void);
bool change_directory(char *);
bool change_prompt(char *);
bool change_path(char *);
void clean_vector(char **);
short parse_str(char *);
char **parse_cmd(char *);
void syntax_err(void);
void execute(char *);
void exec_path(char *);
void exec_rdir(char *);
void exec_pipe(char *);
void exec_both(char *);

FILE *fh = NULL;

const char prompt_tok[] = "PS1=";
const char path_tok[] = "PATH=";

char cwd[MAX_CWD_SIZE];
char home[MAX_CWD_SIZE];
char user_prompt[MAX_PROMPT_SIZE];
char *prompt = NULL;
char *path[MAX_PATH_ENTRIES];
char **command = NULL;

void exit_shell(void)
{
	printf("exit_shell()\n");
	if(fh)
		fclose(fh);
	clean_vector(path);
	clean_vector(command);
	if(command)
		free(command);	
	exit(0);
	return ;
}

/*
 * syntax:
 * 	cd path
 * 	path: null|dir
 *	null: no args
 *	dir : a valid dir
 *
 * truth:
 * 	chdir(home) : cd
 * 	chdir(dir)  : cd dir
 */
bool change_directory(char *cmd)
{
	bool flag = false;
	extern char home[], cwd[];
	char *arg = NULL, *more = NULL, *cwd_ptr = NULL;
	char tmp_buf[1024];
	cmd = strdup(cmd);

	fprintf(fh, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	fprintf(fh, "change_directory(%s)--> (cwd = %s == %s)\n", cmd, getcwd(tmp_buf, 1024), cwd);

	if(!strcmp("cd" ,strtok(cmd, " ")))
	{	arg = strtok(NULL, " ");

		fprintf(fh, "arg = %s\n", arg);

		if(!arg)
		{
			if(!chdir(home))
			{
				strcpy(cwd, home);
				flag = true;
			}
		}
		else if(!(more = strtok(NULL, " ")) && !chdir(arg))
		{
			strcpy(cwd, getcwd(tmp_buf, sizeof(tmp_buf)));
			flag = true;
		}
		else
		{
			if(more)
				printf("resh: cd: too many arguments\n");
			else
				printf("resh: cd: No such a file or directory\n");
		}
	}

	free(cmd);

	fprintf(fh, "change_directory = %s == %s --> (arg = %s\n, more = %s)\n", getcwd(tmp_buf, 1024), cwd, arg, more);
	fprintf(fh, "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	return flag;
}

/*
 * syntax:
 * 	change prompt to cwd: PS1="\w$"
 * 	change prompt to str: PS1="str"
 * 	change prompt to null string: PS1=
 * errors:
 * 	matching " not found
 */
bool change_prompt(char *str)
{
	bool flag = false;
	extern char cwd[], user_prompt[], *prompt;
	char *token = NULL;
	char *prompt_dup = NULL; 
	char *quote_ptr = NULL;
	size_t str_size;
	str = strdup(str);
	prompt_dup = strdup(prompt);

	fprintf(fh, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	fprintf(fh, "change_prompt(%s)-->(prompt = %s, prompt_dup = %s)\n", str, prompt, prompt_dup);

	quote_ptr = strchr(str, '\"');
	if(quote_ptr == strrchr(str, '\"'))
		quote_ptr = NULL;
	if(!strstr(str, "PS1=\""))
	{
		user_prompt[0] = '\0';
		prompt = user_prompt;
		flag = true;
	}
	else if(!strcmp(prompt_tok, strtok(str, "\"")))
	{
		token = strtok(NULL, "\"");

		if(!token || !quote_ptr)
		{
			printf("resh: unexpected EOF while looking for matching \"\n");
			printf("resh: syntax:\n"
					"\t\tchange prompt to cwd		: PS1=\"\\w$\"\n"
					"\t\tchange prompt to any string	: PS1=\"str\"\n"
					"\t\tchange prompt to null string	: PS1=\n");
		}
		else if(!strcmp("\\w$", token)) 
		{
			prompt = cwd;
			flag = true;
		}
		else if(token)
		{
			if(strlen(token) < MAX_PROMPT_SIZE)
				strcpy(user_prompt, token);
			else
			{
				strncpy(user_prompt, token, (size_t)MAX_PROMPT_SIZE);
				user_prompt[MAX_PROMPT_SIZE - 1] = '\0';
			}
			prompt = user_prompt;
			flag = true;
		}
	}

	free(str);
	free(prompt_dup);
	fprintf(fh, "prompt = %s\n", prompt);
	fprintf(fh, "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	return flag;
}

/*
 * syntax:
 * 	change path to dirs: PATH=dir1:dir2:...:dirN
 * 	change path to null: PATH=
 */
bool change_path(char *str)
{
	int i;
	bool flag = false;
	extern char *path[];
	char *token = NULL;
	char *path_str = NULL;

	str = strdup(str);

	fprintf(fh, "\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	fprintf(fh, "change_path(%s)-->(token = %s, path = %s)\n", str, token, path[0]);
	fprintf(fh, "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	fprintf(fh, "[");
	for(i=0; path[i]; i++)
		fprintf(fh, "\"%s\", ", path[i]);
	fprintf(fh, "]\n");
	fprintf(fh, "-----------------------------------------------------------------\n");

	if(strstr((path_str = strtok(str, " ")), path_tok))
	{
		path_str += 5;
		fprintf(fh, "\npath_str = (%s)\n", path_str);
		token = strtok(NULL, " ");
		if(!token) //assign all the paths to path vector 
		{	

			i = 0;
			clean_vector(path);
			token = strtok(path_str,":");
			while(token)
			{
				path[i++] = strdup(token);
				token = strtok(NULL, ":");
			}
			flag = true;

			fprintf(fh, "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
			fprintf(fh, "[");
			for(i=0; path[i]; i++)
				fprintf(fh, "\"%s\", ", path[i]);
			fprintf(fh, "]\n");
			fprintf(fh, "-----------------------------------------------------------------\n");
		}
		else
			printf("resh: invalid syntax to set path\n"
					"resh: syntax:\n"
					"\t\tchange path to dirs: PATH=dir1:dir2:...:dirN\n"
					"\t\tchange path to null: PATH=\n");
	}
	free(str);

	fprintf(fh, "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	return flag;
}

void clean_vector(char **vector)
{	
	extern char **command;
	extern char *path[];
	char **vector_ptrx = NULL;
	char **vector_ptry = NULL;

	vector_ptrx = vector;
	while(*vector_ptrx)
	{
		vector_ptry = vector_ptrx;
		free(*vector_ptrx);
		vector_ptrx++;
		*vector_ptry = NULL;
	}
	return ;
}

/* returns array of string from a string */
short parse_str(char *str)
{		
	/* use strtok_r() for thread safety */
	short ret = -1;

	str = strdup(str);

	char *pipe = strchr(str, '|'); 
	char *rdir = strpbrk(str, "<>");
	char *token = strtok(str, " ");

	fprintf(fh, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	fprintf(fh,"parse_str(%s) --> (token = %s, pipe = %s,rdir = %s)\n", str, token, pipe, rdir);

	/* exit */
	if(!strcmp("exit", token))
		ret = 0;	

	/* cd --> changes the prompt */
	else if(!strcmp("cd", token))
		ret = 1;

	/* PS1="*" or ["\w$" --> CWD] */
	else if(!strncmp(prompt_tok, token, strlen(prompt_tok)))
		ret = 2;

	/* PATH=/bin:/usr/bin --> */
	else if(!strncmp(path_tok, token, strlen(path_tok)))
		ret = 3;

	/* cmd without slash and no <, |, > */
	else if(!strchr(token, '/') && !pipe && !rdir)
		ret = 4;

	/* cmd with slash no <, |, > */
	else if(strchr(token, '/') && !pipe && !rdir)
		ret = 5;

	else if(rdir && !pipe)
		ret = 6;
	/* | --> requires fork + exec */
	else if(pipe && !rdir)
		ret = 7;

	else if(pipe && rdir)
		ret = 8;

	free(str);
	fprintf(fh, "ret = %d\n", ret);
	fprintf(fh, "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	return ret;
}

char **parse_cmd(char *cmd)
{	
	cmd = strdup(cmd);
	char *token = NULL;
	char **vector = NULL;

	fprintf(fh, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if((vector = (char**)malloc(sizeof(char*) * MAX_ARG_CNT)))
	{
		fprintf(fh, "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		fprintf(fh, "[");
		for(int i=0; vector[i]; i++)
			fprintf(fh, "\"%s\", ", vector[i]);
		fprintf(fh, "]\n");
		fprintf(fh, "-----------------------------------------------------------------\n");

		token = strtok(cmd, " ");
		if(token)
			vector[0] = strdup(token); 
		for(int i=1; (token = strtok(NULL, " ")); i++)
			vector[i] = strdup(token);

		fprintf(fh, "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		fprintf(fh, "[");
		for(int i=0; vector[i]; i++)
			fprintf(fh, "\"%s\", ", vector[i]);
		fprintf(fh, "]\n");
		fprintf(fh, "-----------------------------------------------------------------\n");
	}
	free(cmd);
	fprintf(fh, "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	return vector;
}

void syntax_err(void)
{
	printf("resh: syntax error\n");
	return ;
}

void execute(char *cmd)
{
	pid_t pid;
	char **argv = NULL;
	extern char **command;

	fprintf(fh, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	fprintf(fh,"execute(%s)\n", cmd);

	argv = parse_cmd(cmd);
	command = argv;

	if(argv)
	{
		if(fh)
			fclose(fh);
		pid = fork();

		if(!pid)
		{
			int i = 0;
			int pabs_len;
			char *path_ptr = NULL;
			char abs_path[MAX_CWD_SIZE];
			struct stat statbuf;

			while((path_ptr = path[i]))
			{
				strcpy(abs_path, path_ptr);

				pabs_len = strlen(abs_path);
				abs_path[pabs_len++] = '/';
				abs_path[pabs_len] = '\0';

				strcat(abs_path, argv[0]);
				if(!stat(abs_path, &statbuf))
				{
					execv(abs_path, argv);
					printf("execute: execv: failed...\n");
					exit(0);
				}
				i++;
			}
			printf("resh: %s: command not found\n", argv[0]);
			exit(0);
		}
		else
		{
			wait(NULL);
			clean_vector(argv);
			fh = fopen("log", "a");
		}
	}
	fprintf(fh, "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	return ;
}

void exec_path(char *cmd)
{
	pid_t pid;
	char **argv = NULL;
	extern char **command;

	fprintf(fh, ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	fprintf(fh,"exec_path(%s)\n", cmd);

	argv = parse_cmd(cmd);	
	command = argv;

	if(argv)
	{
		if(fh)
			fclose(fh);

		pid = fork();
		if(!pid)
		{
			execv(argv[0], argv);
			printf("exec_path: execv: failed...\n");
			exit(0);
		}
		else
		{
			wait(NULL);
			clean_vector(argv);
			fh = fopen("log", "a");
		}
	}
	fprintf(fh, "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
	return ;
}
void exec_rdir(char *cmd)
{
	fprintf(fh,"exec_rdir(%s)\n", cmd);
	return ;
}

void exec_pipe(char *cmd)
{
	fprintf(fh,"exec_pipe(%s)\n", cmd);
	return ;
}

void exec_both(char *cmd)
{
	fprintf(fh, "exec_both(%s)\n", cmd);
	return ;
}

int main(const int argc, const char *argv[])
{
	short opt;
	pid_t pid;
	extern char home[], cwd[], *path[];
	char *rd_ret = NULL;
	char str[MAX_STR_SIZE];
	char **path_ptrx = NULL;
	char **path_ptry = NULL;
	fh = fopen("log", "w");
	getcwd(home, MAX_CWD_SIZE);
	strcpy(cwd, home);
	prompt = cwd;

	/* make all path pointers null */
	for(int i=0; i< MAX_PATH_ENTRIES; i++)
		path[i] = NULL;
	/* setting default path */
	path[0] = strdup("/bin");
	path[1] = strdup("/usr/bin");

	while(1)
	{
		printf("%s%c ", prompt, (prompt == cwd? '$':'\0'));
		if((rd_ret = fgets(str, sizeof(str), stdin)) && strlen(str) > 1)
		{	
			str[strlen(str)-1] = '\0';
			fprintf(fh,"\npid = %d\tresh: input = [%s]\n",getpid(), str);
			opt = parse_str(str);
			switch(opt)
			{
				case 0:
					{
						exit_shell();
						break;
					}
				case 1:
					{
						change_directory(str);
						break;
					}
				case 2:
					{
						change_prompt(str);
						break;
					}
				case 3:
					{
						change_path(str);
						break;
					}
				case 4:
					{
						execute(str);
						break;
					}
				case 5:
					{
						exec_path(str);
						break;
					}
				case 6:
					{
						exec_rdir(str);
						break;
					}
				case 7:
					{
						exec_pipe(str);
						break;
					}
				case 8:
					{
						exec_both(str);
						break;
					}
				default :
					{
						syntax_err();
						break;
					}
			};
		}
		else if(!rd_ret)/* CTRL-D */
			exit_shell();
	}

	clean_vector(path);
	clean_vector(command);
	if(command)
		free(command);	
	if(fh)
		fclose(fh);
	return 0;
}
