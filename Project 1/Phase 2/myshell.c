/*CSE4100 Project1 Phase 2 coded by 20181632 ¹Ú¼ºÇö*/
/* $begin shellmain */
#include "myshell.h"
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
void parseline(char *buf, char **argv);
int builtin_command(char **argv); 
int pipeparse(char* line, char** arg1);
void execpipe(char *arg, char *arg2,int i);
void del_colon(char* str);
int count = 0;
int sig = 0;

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    while (1) {
	/* Read */
	printf("CSE4100-SP-P#2> ");      
	count = 0;
	Fgets(cmdline, MAXLINE, stdin); 
	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline);
    } 
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char* cmdline)
{
	char* arg1[MAXARGS];
	char* argv[MAXARGS]; /* Argument list execve() */
	char buf[MAXLINE]; /* Holds modified command line */
	char buf1[MAXLINE];
	pid_t pid;   /* Process id */

	strcpy(buf, cmdline);
	int a = pipeparse(buf, argv);
	
	if (argv[0] == NULL)
		return;   /* Ignore empty lines */
	strcpy(buf1, argv[0]);
	parseline(buf1, arg1);
	if (!builtin_command(arg1)) {
		if ((pid = Fork()) == 0) {
			if (count == 0) { //if there are no pipe, just execute

				if (execvp(arg1[0], arg1) < 0) {
					printf("%s: Command not found.\n", arg1[0]);
					exit(0);
				}
			}
			else
			{
				for (int i = 0;i < count;i++) { // if there are pipe, execute with for loop
					execpipe(argv[i], argv[i + 1], i);
					int status;
					if (wait(&status) < 0) //wait for child to terminate
						unix_error("waitfg: waitpid error");
				}
			}
			//exit(0);
		}
		/* Parent waits for child to terminate*/
		else
		{
			int status;
			if (wait(&status) < 0)
				unix_error("waitfg: waitpid error");
		}
	}
   return; 
}

/*pipe execution*/
void execpipe(char *arg, char *arg2,int i)
{
		int fds[2];
		int a = pipe(fds);	
		pid_t pid; 
		char buf1[MAXLINE];
		char *arga[MAXARGS];
		char *argb[MAXARGS];
		strcpy(buf1, arg);
		parseline(buf1, arga);
		if (!builtin_command(arga)) { //special command(exit or cd)
			if ((pid = Fork()) == 0) {   /* Child runs user job */
					dup2(fds[1], 1); // replace stdout with pipe
					close(fds[0]); // close reading in pipe
					
					if (execvp(arga[0], arga) < 0) {
						printf("%s: Command not found.\n", arga[0]);
						exit(0);
					}
			}
			else
			{
					dup2(fds[0], 0); //replace stdin with pipe
					close(fds[1]);  //close writing in pipe
					if (i == count - 1) {
						strcpy(buf1, arg2);
						parseline(buf1, argb);
						if (execvp(argb[0], argb) < 0) {
							printf("%s: Command not found.\n", argb[0]);
							exit(0);
						}
					}
			}
		}

		return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
	int i=1;
    if (!strcmp(argv[0], "exit")) /* quit command */
		exit(0);  
    if (!strcmp(argv[0], "cd")){    /* change directory using chdir */
	while(argv[i]!=NULL){
		if(chdir(argv[i])!=0){
		printf("no such directory: %s\n",argv[i]);
		break;
		}
		i++;
	}
	return 1;
    }
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
void parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */

   // buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    buf[strlen(buf)] = ' ';
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	if (*buf == 34) {
		argv[argc++] = buf;
		buf++;
		while ((delim = strchr(buf, '"'))) {
			*delim = '\0';
			buf = delim + 1;
		}
	}
	else if (*buf == 39) {
		argv[argc++] = buf;
		buf++;
		while ((delim = strchr(buf, 39))) {
			*delim = '\0';
			buf = delim + 1;
		}
	}
	if(!strcmp(argv[0],"cd")){ /*if command is cd*/
		buf[strlen(buf)-1]='/';/*Replace trailing '\n' with  '/'*/
		if(delim = strchr(buf, ' '))/*if there are ' '*/
		{
			*delim = '\0';
			argv[argc++] = buf;
			argv[argc] = NULL;
			return;/*works only for first input directory*/
		}
        	while ((delim = strchr(buf, '/'))) {/*parse line with '/'*/
            		argv[argc++] = buf;
            		*delim = '\0';
            		buf = delim + 1;
		}
	argv[argc] = NULL;
	return;
	}

	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
	for (int i = 0;i < argc;i++) { // delete " or'
		if (argv[i][0] == 34 || argv[i][0] == 39)
		{
			del_colon(argv[i]);
		}
	}
    argv[argc] = NULL;
    if (argc == 0)  /* Ignore blank line */
	return ;

}
/* $end parseline */

/* pipe parsing*/
int pipeparse(char* line, char** arg1)
{
	char* delim;         /* Points to first space delimiter */
	int argc;            /* Number of args */

	line[strlen(line) - 1] = '|';  /* Replace trailing '\n' with space */
	while (*line && (*line == ' ')) /* Ignore leading spaces */
		line++;

	/* Build the argv list */
	argc = 0;
	while ((delim = strchr(line, '|'))) { //parse with '|' symbol
		arg1[argc++] = line;
		*delim = '\0';
		line = delim + 1;
		while (*line && (*line == ' ')) /* Ignore spaces */
			line++;
	}
	arg1[argc] = NULL;
	count = argc - 1;
	if (argc == 0)  /* Ignore blank line */
		return 0;
	
}

/* delete "or '*/
void del_colon(char* str)
{
	for (int i = 0; i < strlen(str) - 1; i++)
	{
		str[i] = str[i + 1];
	}
	str[strlen(str) - 1] = '\0';
	if (strlen(str) > 0)
	{
		if (str[strlen(str) - 1] == 34 || str[strlen(str) - 1] == 39)
		{
			str[strlen(str) - 1] = '\0';
		}
	}
}

