/*CSE4100 Project1 Phase 1 coded by 20181632 ¹Ú¼ºÇö*/
/* $begin shellmain */
#include "myshell.h"
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
void parseline(char *buf, char **argv);
int builtin_command(char **argv); 
void del_colon(char* str);

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	printf("CSE4100-SP-P#1> ");                   
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
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    parseline(buf, argv); 
    if (argv[0] == NULL)  
	return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //special command(exit or cd)
        if ((pid = Fork()) == 0) {   /* Child runs user job */
            if (execvp(argv[0], argv) < 0) {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }

	/* Parent waits for child to terminate*/
	    int status;
		if (waitpid(pid, &status, 0) < 0)
			unix_error("waitfg: waitpid error");
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

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	if (*buf == 34){ 
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
	for (int i = 0;i < argc;i++) {
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


