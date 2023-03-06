/*
 * SHELL USING FORK & EXEC
 * TO BE EXTENDED TO SUPPORT PIPES
 */

#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

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

enum option
{
	EXIT,
	HISTORY,
	CD,
	PS1,
	PATH,
	EXECUTE,
	EXEC_PATH,
	EXEC_RDIR,
	EXEC_PIPE
};
enum pipe_ops
{
	READ,
	WRITE
};

typedef struct job_node{
	unsigned int idx;
	pid_t pid;
	char state; /* RUNNING, STOPPED, TERMINATED */
	job_node *next;
	job_node *prev;
}job_node;

/* CREATE DCLL OF THE JOBS FOR JOB CONTROL */
typedef struct jobs{
	unsigned int count;
	struct job_node *head;
}jobs;

/* will be initiated by main */
jobs *job_list = NULL;

const char prompt_tok[] = "PS1=";
const char path_tok[] = "PATH=";

char cwd[MAX_CWD_SIZE];
char home[MAX_CWD_SIZE];
char user_prompt[MAX_PROMPT_SIZE];
char *path[MAX_PATH_ENTRIES];

char *prompt = NULL;

FILE *fh_hist = NULL;

void handle_sigint(int sig)
{
	sig = job;
	kill(getpid(), SIGTERM); 
	return;
}

void handle_sigstop(int sig)
{
	sig = job;
	kill(getpid(), SIGTSTP);
	return;
}

job_node *new_job(void)
{
	job_node *new = NULL;

	if ((new = (job_node *) malloc(sizeof(job_node))))
	{
		new->idx = -1;
		new->pid = -1;
		new->state = '\0';
		new->next = new->prev = NULL;
	}
	return new;
}

void push_job(job_node *new_node)
{
	job_node *head = NULL;
	if (new_job && job_list)
	{
		if (job_list->head)
		{
			head = job_list->head;

			new_node->next = head;
			new_node->prev = head->prev;
			head->prev->next = new_node;
			head->prev = new_node;

			job_list->head = new_node;
		}
		else
			job_list->head = new_node;

	}
	return ;
}
void exit_shell(void);
void history(char *);
bool change_directory(char *);
bool change_prompt(char *);
bool change_path(char *);
void clean_vector(char **);
int parse_str(char *);
char **parse_cmd(char *);
void syntax_err(void);
pid_t execute(char *, char *, char *);
pid_t exec_path(char *, char *, char *);
void exec_rdir(char *);
void exec_pipe(char *);
void pipe_helper(char *);

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
	if (!strcmp("history", strtok(cmd, " ")))
	{
		token = strtok(NULL, " ");
		if (!token)
		{
			/* print the history */
			idx = 1;
			fflush(fh_hist);
			fseek(fh_hist, 0L, SEEK_SET);
			while (fgets(cmd_buf, sizeof(cmd_buf), fh_hist) && (fprintf(stdout, "%5d  %s", idx++, cmd_buf) > 0))
				;
			fseek(fh_hist, 0L, SEEK_END);
		}
		else if (!strcmp("-c", token))
		{
			/* clear history */
			__fpurge(fh_hist);
			ftruncate(fileno(fh_hist), 0L);
			rewind(fh_hist);
		}
		else
			fprintf(stderr, "%s", hist_usage);
	}
	return;
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
				fprintf(stderr, "resh: cd: too many arguments\n");
			else
				fprintf(stderr, "resh: cd: No such a file or directory\n");
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
			fprintf(stderr, "resh: unexpected EOF while looking for matching \"\n");
			fprintf(stderr, "resh: syntax:\n"
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
			fprintf(stderr, "resh: invalid syntax to set path\n"
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

pid_t execute(char *cmd, char *fin, char *fout)
{
	pid_t pid = SHRT_MAX;
	int ret, wstatus;
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
			ret = waitpid(pid, &wstatus, 0);
			clean_vector(argv);
			free(argv);
		}
	}
	return pid;
}

pid_t exec_path(char *cmd, char *fin, char *fout)
{
	pid_t pid = SHRT_MAX;
	int ret, wstatus;
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
			ret = waitpid(pid, &wstatus, 0);
			clean_vector(argv);
			free(argv);
		}
	}
	return pid;
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

		switch (retval)
		{
			case EXECUTE:
				{
					execute(cmd, fin, fout);
					break;
				}
			case EXEC_PATH:
				{
					exec_path(cmd, fin, fout);
					break;
				}
			default:
				{
					fprintf(stderr, "resh: exec_dir: "
							"parse_cmd returned %d\n",
							retval);
					break;
				}
		};

		if (fin)
			free(fin);
		if (fout)
			free(fout);
		free(cmd);
	}

	clean_vector(argio);
	free(argio);
	free(rcmd);
	return;
}

/*
 * observations:
 * 	1. two processes are always connected with one pipe.
 * 	2. every single process except first & last,
 * 		is always connected to two other processes using two pipes.
 * 	3. processes are active entities and pipes are passive entities.
 * 	4. although there are multiple processes concurrently executing,
 * 		most of them are waiting (blocked by read or write) for data.
 * 	5. thus at any given time current process is waiting for data
 * 		 from previous process same applies for next process.
 * 	6. all the data transfers for every process except first & last,
 * 		are done through exactly two pipes.
 * 	7. hence not more than two pipes are used at the same time.
 * 	8. first and last process have 'output to' and 'input from' pipe,
 * 		similarly 'input from' and 'output to'
 * 			stdin/file and stdout/file respectively.
 * 	9. thus first and last process have to be explicitly managed,
 * 		other processes between pipes are implicitly managed.
 * 	10. read(2) & write(2) are blocking syscalls,
 * 		can be made non-blocking.
 */
void exec_pipe(char *cmd)
{
	bool flag;
	int retval;
	int wstatus;
	int xret, yret;
	pid_t xpid, ypid;
	int left_pipefd[2], right_pipefd[2];
	char *cmd_left = NULL, *cmd_right = NULL;

	cmd_left = strtok(cmd, "|");
	if (cmd_left)
		cmd_left = strdup(cmd_left);

	if (!pipe(left_pipefd) && !pipe(right_pipefd))
	{
		/*
		 * no cmd on left of current cmd existed.
		 */
		flag = false;

		while (cmd_left)
		{
			/* get next cmd */
			cmd_right = strtok(NULL, "|");
			if (cmd_right)
				cmd_right = strdup(cmd_right);

			xpid = fork();
			if (!xpid)
			{
				/*
				 * CAUTION : BLOCKING READ
				 * 	how --> read(2) is blocking syscall.
				 * 	why --> no data in left pipe &
				 * 		no data is being written to left pipe.
				 */

				/*
				 * if cmd on left of current cmd existed
				 * 	read data from left pipe
				 * else
				 * 	read data from stdin
				 */

				/*
				 * flag indicates if cmd on left
				 * 	 of current cmd existed or not.
				 *
				 * set flag [implicitly] implies that,
				 * 	data is written to left pipe, hence
				 * 		data is to be read from left pipe.
				 */

				if (flag)
				{
					close(0);
					dup(left_pipefd[READ]);
					close(left_pipefd[WRITE]);
				}
				else
				{
					close(left_pipefd[READ]);
					close(left_pipefd[WRITE]);
				}

				/*
				 * if cmd on right of current cmd exists
				 * 	write data to right pipe
				 * else
				 * 	write data to stdout
				 */
				if (cmd_right)
				{
					close(1);
					dup(right_pipefd[WRITE]);
					close(right_pipefd[READ]);
				}
				else
				{
					close(right_pipefd[READ]);
					close(right_pipefd[WRITE]);
				}

				pipe_helper(cmd_left);
				close(left_pipefd[READ]);
				close(left_pipefd[WRITE]);
				if (cmd_right)
					close(right_pipefd[WRITE]);
				exit(0);
			}
			else
			{
				/*
				 * cmd left becomes irrelevant
				 */
				if (cmd_left)
				{
					free(cmd_left);
					cmd_left = NULL;
				}

				if (cmd_right)
				{
					/* get next cmd */
					cmd_left = strtok(NULL, "|");
					if (cmd_left)
					{
						cmd_left = strdup(cmd_left);
						flag = true;
					}

					ypid = fork();
					if (!ypid)
					{
						/*
						 * cmd on left of current cmd existed
						 * 	read data from right pipe
						 */
						close(0);
						dup(right_pipefd[READ]);
						close(right_pipefd[WRITE]);

						/*
						 * CAUTION : BLOCKING WRITE
						 * 	how --> write(2) is blocking syscall.
						 * 	why --> data in right pipe &
						 * 		data is not being read from right pipe.
						 */

						/*
						 * if cmd on right of current cmd exists
						 * 	write data to left pipe
						 * else
						 * 	write data to stdout
						 */
						if (cmd_left)
						{
							close(1);
							dup(left_pipefd[WRITE]);
							close(left_pipefd[READ]);
						}
						else
						{
							close(left_pipefd[READ]);
							close(left_pipefd[WRITE]);
						}

						pipe_helper(cmd_right);

						if (cmd_left)
							close(left_pipefd[WRITE]);
						close(right_pipefd[READ]);
						close(right_pipefd[WRITE]);
						exit(0);
					}
					else
					{
						/* wait until both childs are finished executing */
						xret = waitpid(xpid, &wstatus, WNOHANG);
						yret = waitpid(ypid, &wstatus, WNOHANG);
						if (cmd_right)
						{
							free(cmd_right);
							cmd_right = NULL;
						}
					}
				}
				else
				{
					/* wait until single child has finished executing */
					xret = waitpid(xpid, &wstatus, WNOHANG);
					if (cmd_left)
					{
						free(cmd_left);
						cmd_left = NULL;
					}
				}
			}
		}
		while (waitpid(0, &wstatus, WNOHANG) > 0)
			;
		close(left_pipefd[READ]);
		close(left_pipefd[WRITE]);
		close(right_pipefd[READ]);
		close(right_pipefd[WRITE]);
	}
	return;
}

void pipe_helper(char *cmd)
{
	int retval;
	retval = parse_str(cmd);

	switch (retval)
	{
		case EXECUTE:
			{
				execute(cmd, NULL, NULL);
				break;
			}
		case EXEC_PATH:
			{
				exec_path(cmd, NULL, NULL);
				break;
			}
		case EXEC_RDIR:
			{
				exec_rdir(cmd);
				break;
			}
		default:
			{
				fprintf(stderr, "resh: exec_pipe: unable to parse the string\n");
				break;
			}
	};
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
	if (!(fh_hist = fopen(fhist, "a+")))
		fprintf(stderr, "resh: error in opening history file\n");

	/* signal handling */
	signal(SIGINT, handle_sigint);
	while (1)
	{	
		job = SHRT_MAX;
		fprintf(stderr, "%s%c ", prompt, (prompt == cwd ? '$' : '\0'));
		if ((rd_ret = fgets(str, sizeof(str), stdin)) && strlen(str) > 1)
		{
			fprintf(fh_hist, "%s", str);

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
						job = execute(str, NULL, NULL);
						break;
					}
				case EXEC_PATH:
					{
						job = exec_path(str, NULL, NULL);
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
