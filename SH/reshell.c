/*
 * SHELL USING FORK & EXEC
 * TO BE EXTENDED TO SUPPORT PIPES
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_ARG_CNT 128
#define MAX_STR_SIZE 1024
#define MAX_CWD_SIZE 1024
#define MAX_PROMPT_SIZE 1024
#define MAX_PATH_ENTRIES 128

enum option {
	EXIT, HISTORY, CD, PS1, PATH, EXECUTE, EXEC_PATH, EXEC_RDIR, EXEC_PIPE, EXEC_BOTH 
};

const char prompt_tok[] = "PS1=";
const char path_tok[] = "PATH=";

char cwd[MAX_CWD_SIZE];
char home[MAX_CWD_SIZE];
char user_prompt[MAX_PROMPT_SIZE];
char *path[MAX_PATH_ENTRIES];

char *prompt = NULL;

FILE *fh_hist = NULL;

void exit_shell(void);
void history(char *);
bool change_directory(char *);
bool change_prompt(char *);
bool change_path(char *);
void clean_vector(char **);
int parse_str(char *);
char **parse_cmd(char *);
void syntax_err(void);
void execute(char *, char *, char *);
void exec_path(char *, char *, char *);
void exec_rdir(char *);
void exec_pipe(char *);
void exec_both(char *);

void exit_shell(void)
{
	fclose(fh_hist);
	clean_vector(path);
	free(path[0]);
	free(path[1]);
	exit(0);
}

/*
 * syntax:
 *	history		: print indexed history
 *	history -c	: clear history
 */
void history(char *cmd)
{
	int idx;
	char cmd_buf[MAX_STR_SIZE];
	char *token = NULL;
	const char *hist_usage = "resh: history: usage:\n"
					"history	: print indexed history of commands\n"
					"history -c	: clear history\n";
	if(!strcmp("history", strtok(cmd, " ")))
	{
		token = strtok(NULL, " ");
		if(!token)
		{
			/* print the history */
			idx = 1;
			fseek(fh_hist, 0L, SEEK_SET);
			while(fgets(cmd_buf, sizeof(cmd_buf), fh_hist) && (fprintf(stdout, "%4d  %s", idx++, cmd_buf) > 0));
			fseek(fh_hist, 0L, SEEK_END);
		}
		else if(!strcmp("-c", token))
		{
			/* clear history */
			ftruncate(fileno(fh_hist), 0L);
			rewind(fh_hist);	
		}
		else
			fprintf(stderr, "%s", hist_usage);
	}
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
	char tmp_buf[1024];
	extern char home[], cwd[];
	char *arg = NULL, *more = NULL, *cwd_ptr = NULL;

	cmd = strdup(cmd);

	if (!strcmp("cd", strtok(cmd, " ")))
	{
		arg = strtok(NULL, " ");
		if (!arg)
		{
			if (!chdir(home))
			{
				strcpy(cwd, home);
				flag = true;
			}
		}
		else if (!(more = strtok(NULL, " ")) && !chdir(arg))
		{
			strcpy(cwd, getcwd(tmp_buf, sizeof(tmp_buf)));
			flag = true;
		}
		else
		{
			if (more)
				printf("resh: cd: too many arguments\n");
			else
				printf("resh: cd: No such a file or directory\n");
		}
	}

	free(cmd);
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
	size_t str_size;
	bool flag = false;
	extern char cwd[], user_prompt[], *prompt;
	char *token = NULL, *prompt_dup = NULL, *quote_ptr = NULL;

	str = strdup(str);
	prompt_dup = strdup(prompt);

	quote_ptr = strchr(str, '\"');
	if (quote_ptr == strrchr(str, '\"'))
		quote_ptr = NULL;
	if (!strstr(str, "PS1=\""))
	{
		user_prompt[0] = '\0';
		prompt = user_prompt;
		flag = true;
	}
	else if (!strcmp(prompt_tok, strtok(str, "\"")))
	{
		token = strtok(NULL, "\"");

		if (!token || !quote_ptr)
		{
			printf("resh: unexpected EOF while looking for matching \"\n");
			printf("resh: syntax:\n"
					"\t\tchange prompt to cwd		: PS1=\"\\w$\"\n"
					"\t\tchange prompt to any string	: PS1=\"str\"\n"
					"\t\tchange prompt to null string	: PS1=\n");
		}
		else if (!strcmp("\\w$", token))
		{
			prompt = cwd;
			flag = true;
		}
		else if (token)
		{
			if (strlen(token) < MAX_PROMPT_SIZE)
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
	char *token = NULL, *path_str = NULL;

	str = strdup(str);

	if (strstr((path_str = strtok(str, " ")), path_tok))
	{
		path_str += 5;
		token = strtok(NULL, " ");

		if (!token) // assign all the paths to path vector
		{

			i = 0;
			clean_vector(path);
			token = strtok(path_str, ":");
			while (token)
			{
				path[i++] = strdup(token);
				token = strtok(NULL, ":");
			}
			flag = true;
		}
		else
			printf("resh: invalid syntax to set path\n"
					"resh: syntax:\n"
					"\t\tchange path to dirs: PATH=dir1:dir2:...:dirN\n"
					"\t\tchange path to null: PATH=\n");
	}

	free(str);
	return flag;
}

void clean_vector(char **vector)
{
	char **vector_ptrx = NULL;
	char **vector_ptry = NULL;

	vector_ptrx = vector;
	while (vector_ptrx && *vector_ptrx)
	{
		vector_ptry = vector_ptrx;
		free(*vector_ptrx);
		vector_ptrx++;
		*vector_ptry = NULL;
	}
	return;
}

/* returns array of string from a string */
int parse_str(char *str)
{
	int ret = -1;
	char *pipe, *rdir, *token;

	str = strdup(str);

	pipe = strchr(str, '|');
	rdir = strpbrk(str, "<>");
	token = strtok(str, " ");

	/* exit */
	if (!strcmp("exit", token))
		ret = EXIT;

	/* history */
	else if (!strcmp("history", token))
		ret = HISTORY;

	/* cd --> changes the prompt */
	else if (!strcmp("cd", token))
		ret = CD;

	/* PS1="*" or ["\w$" --> CWD] */
	else if (!strncmp(prompt_tok, token, strlen(prompt_tok)))
		ret = PS1;

	/* PATH=/bin:/usr/bin --> */
	else if (!strncmp(path_tok, token, strlen(path_tok)))
		ret = PATH;

	/* cmd without slash and no <, |, > */
	else if (!strchr(token, '/') && !pipe && !rdir)
		ret = EXECUTE;

	/* cmd with slash no <, |, > */
	else if (strchr(token, '/') && !pipe && !rdir)
		ret = EXEC_PATH;

	else if (rdir && !pipe)
		ret = EXEC_RDIR;
	/* | --> requires multiple fork + exec */
	else if (pipe && !rdir)
		ret = EXEC_PIPE;

	else if (pipe && rdir)
		ret = EXEC_BOTH;

	free(str);
	return ret;
}

char **parse_cmd(char *cmd)
{
	char *token = NULL;
	char **vector = NULL;

	cmd = strdup(cmd);

	if ((vector = (char **)malloc(sizeof(char *) * MAX_ARG_CNT)))
	{
		token = strtok(cmd, " ");
		if (token)
			vector[0] = strdup(token);
		for (int i = 1; (token = strtok(NULL, " ")); i++)
			vector[i] = strdup(token);
	}

	free(cmd);
	return vector;
}

char **parse_rdir(char *rcmd)
{
	char *argv_rcmd = NULL;
	char *in_ptr = NULL;
	char *out_ptr = NULL;
	char **retv = NULL;

	argv_rcmd = strdup(rcmd);

	retv = (char **)malloc(sizeof(char *) * 4);
	for (int i = 0; i < 4; i++)
		retv[i] = NULL;

	in_ptr = strchr(argv_rcmd, '<');
	out_ptr = strchr(argv_rcmd, '>');

	if ((!out_ptr && in_ptr) || (in_ptr && (in_ptr < out_ptr)))
		retv[0] = strdup(strtok(argv_rcmd, "<"));
	else if ((!in_ptr && out_ptr) || (out_ptr && (in_ptr > out_ptr)))
		retv[0] = strdup(strtok(argv_rcmd, ">"));
	else if (!in_ptr && !out_ptr) /* no redirection */
	{
		free(retv);
		retv = NULL;
	}

	free(argv_rcmd);
	argv_rcmd = strdup(rcmd);

	if (retv) /* redirection exists */
	{
		in_ptr = strrchr(argv_rcmd, '<');
		out_ptr = strrchr(argv_rcmd, '>');
		if (in_ptr)
		{
			in_ptr = strtok(++in_ptr, " ");
			retv[1] = strdup(in_ptr);
		}
		if (out_ptr)
		{
			out_ptr = strtok(++out_ptr, " ");
			retv[2] = strdup(out_ptr);
		}
	}
	free(argv_rcmd);
	return retv;
}

void execute(char *cmd, char *fin, char *fout)
{
	pid_t pid;
	int i = 0, pabs_len;
	int fd[2] = {-1, -1};
	char **argv = NULL;
	char *path_ptr = NULL;
	char abs_path[MAX_CWD_SIZE];
	struct stat statbuf;

	argv = parse_cmd(cmd);

	if (argv)
	{
		pid = fork();

		if (!pid)
		{
			while ((path_ptr = path[i]))
			{
				strcpy(abs_path, path_ptr);

				pabs_len = strlen(abs_path);
				abs_path[pabs_len++] = '/';
				abs_path[pabs_len] = '\0';

				strcat(abs_path, argv[0]);
				if (!stat(abs_path, &statbuf))
				{
					if (fin)
					{
						fd[0] = open(fin, O_RDONLY);

						if (fd[0] > 0)
						{
							close(0);
							dup(fd[0]);
						}
						else
							fprintf(stderr, "resh: execute: unable to open "
									"input redirection file\n");
					}

					if (fout)
					{
						fd[1] = open(fout, O_CREAT | O_WRONLY, 0666);

						if (fd[1] > 0)
						{
							close(1);
							dup(fd[1]);
						}
						else
							fprintf(stderr, "resh: execute: "
									"unable to open or create"
									" output redirection file\n");
					}

					execv(abs_path, argv);

					fprintf(stderr, "execute: execv: failed...\n");
					if (fin)
						close(0);
					if (fout)
						close(1);
					exit(0);
				}

				i += 1;
			}

			fprintf(stderr, "resh: %s: command not found\n", argv[0]);
			exit(0);
		}
		else
		{
			wait(NULL);
			clean_vector(argv);
			free(argv);
		}
	}
	return;
}

void exec_path(char *cmd, char *fin, char *fout)
{
	pid_t pid;
	int fd[2] = {-1, -1};
	char **argv = NULL;

	argv = parse_cmd(cmd);

	if (argv)
	{
		pid = fork();
		if (!pid)
		{
			if (fin)
			{
				fd[0] = open(fin, O_RDONLY);
				if (fd[0] > 0)
				{
					close(0);
					dup(fd[0]);
				}
				else
					fprintf(stderr, "resh: exec_path: "
							"unable to open input redirection file\n");
			}
			if (fout)
			{
				fd[1] = open(fout, O_CREAT | O_WRONLY, 0666);

				if (fd[1] > 0)
				{
					close(1);
					dup(fd[1]);
				}
				else
					fprintf(stderr, "resh: exec_path: "
							"unable to open or create"
							" output redirection file\n");
			}

			execv(argv[0], argv);

			fprintf(stderr, "exec_path: execv: failed...\n");
			if (fin)
				close(0);
			if (fout)
				close(1);
			exit(0);
		}
		else
		{
			wait(NULL);
			clean_vector(argv);
			free(argv);
		}
	}
	return;
}

/*
 *  syntax:
 *
 *  	simple	: cmd -opts args < fin > fout
 *  			* space is must after < or >
 *  			* if multiple redirections are present,
 *  				only recent ones are considered.
 *
 *  	complex	: cmd with any number of redirections at any position
 *  			after cmd-name combined with options and arguments
 *  	complex cmd not yet handled
 */

void exec_rdir(char *rcmd)
{
	int retval;
	char *cmd = NULL, *fin = NULL, *fout = NULL;
	char **argio = NULL;
	struct stat statbuf;

	rcmd = strdup(rcmd);
	argio = parse_rdir(rcmd); /* to be freed */

	if (argio[0])
	{
		cmd = strdup(argio[0]);

		if (argio[1] && !stat(argio[1], &statbuf))
			fin = strdup(argio[1]);
		if (argio[2])
			fout = strdup(argio[2]);
		retval = parse_str(cmd);

		if (retval == EXECUTE)
			execute(cmd, fin, fout);
		else if (retval == EXEC_PATH)
			exec_path(cmd, fin, fout);
		else
			fprintf(stderr, "resh: exec_dir: parse_cmd returned %d\n", retval);
		if(fin)
			free(fin);
		if(fout)
			free(fout);
		free(cmd);
	}

	clean_vector(argio);
	free(argio);
	free(rcmd);
	return;
}

void exec_pipe(char *cmd)
{
	fprintf(stderr, "exec_pipe(%s)\n", cmd);
	return;
}

void exec_both(char *cmd)
{
	fprintf(stderr, "exec_both(%s)\n", cmd);
	return;
}

int main(const int argc, const char *argv[])
{
	short opt;
	pid_t pid;
	char *rd_ret = NULL;
	const char *fhist = ".resh_history";
	char str[MAX_STR_SIZE];
	extern char home[], cwd[], *path[];

	getcwd(home, MAX_CWD_SIZE);
	strcpy(cwd, home);
	prompt = cwd;

	/* make all path pointers null */
	for (int i = 0; i < MAX_PATH_ENTRIES; i++)
		path[i] = NULL;

	/* setting default path */
	path[0] = strdup("/bin");
	path[1] = strdup("/usr/bin");

	 /* open history file */
	if(!(fh_hist = fopen(fhist, "a+")))
		fprintf(stderr, "resh: error in opening history file\n");

	while (1)
	{
		printf("%s%c ", prompt, (prompt == cwd ? '$' : '\0'));
		if ((rd_ret = fgets(str, sizeof(str), stdin)) && strlen(str) > 1)
		{
			fprintf(fh_hist, "%s", str);
			fflush(fh_hist);

			str[strlen(str) - 1] = '\0';
			opt = parse_str(str);

			switch (opt)
			{
				case EXIT:
					{
						exit_shell();
						break;
					}
				case HISTORY:
					{
						history(str);
						break;
					}
				case CD:
					{
						change_directory(str);
						break;
					}
				case PS1:
					{
						change_prompt(str);
						break;
					}
				case PATH:
					{
						change_path(str);
						break;
					}
				case EXECUTE:
					{
						execute(str, NULL, NULL);
						break;
					}
				case EXEC_PATH:
					{
						exec_path(str, NULL, NULL);
						break;
					}
				case EXEC_RDIR:
					{
						exec_rdir(str);
						break;
					}
				case EXEC_PIPE:
					{
						exec_pipe(str);
						break;
					}
				case EXEC_BOTH:
					{
						exec_both(str);
						break;
					}
				default:
					{
						fprintf(stderr, "resh: syntax error, unable to parse\n");
						break;
					}
			};
		}
		else if (!rd_ret) /* CTRL-D */
		{
			putchar('\n');
			break;
		}
	}

	exit_shell();
	return 0;
}
