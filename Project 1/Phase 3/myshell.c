/*CSE4100 Project1 Phase 3 coded by 20181632 ¹Ú¼ºÇö
 details are written in readme.txt*/
/* $begin shellmain */
#include "myshell.h"
#define MAXARGS   128
#define MAXJOBS   128
typedef struct job_l {
	pid_t pid;
	int id;
	char state;
	int fb; //0 ->foreground, 1->background
	char cmd[50];
}job_l;

job_l jobs[MAXJOBS];

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv); 
int pipeparse(char* line, char** arg1);
void execpipe(char *arg, char *arg2,int i);
void execfg(char** argv);
void execbg(char** argv);
void execkill(char** argv);
job_l *getjbid(int jid);
job_l* getjbpid(pid_t pid);
pid_t getfgpid();
void waitfg(pid_t pid);
int getidpid(pid_t pid);
void del_colon(char* str);
void initjobtable();
void insertjb(pid_t pid, char state, char* cmd, int fb);
void deljob(pid_t pid);
void printjob();
void prsinglejob(pid_t pid);
void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
int count = 0;
int j_id = 1;
int j_temp = 1;


int main() 
{
	
	/*set signals*/
	Signal(SIGCHLD, sigchld_handler);
	Signal(SIGINT, sigint_handler);
	Signal(SIGTSTP, sigtstp_handler);

	initjobtable();
    char cmdline[MAXLINE]; /* Command line */
    while (1) {
	/* Read */
	printf("CSE4100-SP-P#3> ");      
	count = 0;
	Fgets(cmdline, MAXLINE, stdin); 
	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline);
    } 
}
/* $end shellmain */

/* start job table*/
void initjobtable() {
	for (int i = 0; i < MAXJOBS;i++) {
		jobs[i].fb = 2;
		jobs[i].cmd[0] = '\0';
		jobs[i].id = 0;
		jobs[i].state = 0;
		jobs[i].pid = 0;
	}
}

void sigtstp_handler(int sig) {
	pid_t pid = getfgpid();
	if (pid  == 0) {
		return;
	}

	for (int i = 0;i < MAXJOBS;i++) { //update job table
		if (jobs[i].pid == pid) {
			jobs[i].state = 's';
			jobs[i].fb = 1;
			if (jobs[i].id == 0) { //when  new running foreground job is stopped
				jobs[i].id = j_temp++;
			}
			else {                // when existing running foreground job is stopped
				//jobs[i].id = j_temp - 1;
			}
			if (kill(-pid, SIGTSTP) < -1)
			{
				printf("kill error\n");
			}
			return;
		}
	}
	printf("NO process to stop\n");
	return;
}

void sigchld_handler(int sig) {
	
	int status;
	pid_t pid;
	job_l* cur_job;
	
	
	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) //get signal from child with status
	{
		cur_job = getjbpid(pid);
		if (WIFSTOPPED(status)) {         // when child is stopped by signal   
			
			printf("\nJob [%d] stopped by signal %d \n", getidpid(pid), WSTOPSIG(status));
			cur_job->state = 's';
		}

		else if (WIFEXITED(status)) {        //when child terminated normally
			if (cur_job->fb == 1) {
				printf("\nJob [%d]  terminated\n", getidpid(pid));
			}
			deljob(pid);
		}

		else if (WIFSIGNALED(status)) {     //when child terminated by signal
			
			deljob(pid);
		}
	}
	
	return;
}

void sigint_handler(int sig) {
	
	pid_t pid = getfgpid();
	if (pid == 0) {
		return;
	}
	if (kill(-pid, SIGINT) < -1)
	{
		printf("kill error\n");
	}
	return;
}

pid_t getfgpid() {
	
	for (int i = 0;i < MAXJOBS;i++) {
		if (jobs[i].fb == 0) {
			return jobs[i].pid;
		}
	}
	return 0;
}

/*insert jobs from parsed arguments*/
void insertjb(pid_t pid, char state, char* cmd, int fb)
{
	jobs[j_id - 1].fb = fb;
	jobs[j_id - 1].pid = pid;
	if (fb == 1) {  //when a new job is added in background
		
		jobs[j_id - 1].id = j_temp;
		j_temp++;
	}
	else if(fb==0) { // when a new job is added in foreground
		jobs[j_id - 1].id = 0;
	}
	jobs[j_id - 1].state = state;
	strcpy(jobs[j_id - 1].cmd, cmd);
	j_id++;
}

/*delete jobs from job table*/
void deljob(pid_t pid) {
	job_l* del_job = getjbpid(pid);
	for (int i = 0;i < MAXJOBS;i++) {
		if (jobs[i].pid == pid) {
			jobs[i].cmd[0] = '\0';
			jobs[i].fb = 2;
			jobs[i].id = 0;
			jobs[i].state = 0;
			jobs[i].pid = 0;
		}
	}

}

/*print jobs by its status*/
void printjob()
{
	
	for (int i = 0;i < j_id-1;i++) {
		if (jobs[i].id == 0) {
			continue;
		}
		else {

			printf("[%d]  ", jobs[i].id);
			if (jobs[i].state == 'r') {
				printf("Running  ");
			}
			else if (jobs[i].state == 's') {
				printf("Stopped  ");
			}
			else if (jobs[i].state == 't') {
				printf("Terminated  ");
			}
			else if (jobs[i].state == 'd') {
				printf("Done  ");
				jobs[i].id = 0;
			}
			printf("%s \n", jobs[i].cmd);
		}
	}
}

/*print single job status*/
void prsinglejob(pid_t pid) {
	for (int i = 0;i < MAXJOBS;i++) {
		if (jobs[i].pid == pid && jobs[i].id != 0 ) {
			printf("[%d]  ", jobs[i].id);
			if (jobs[i].state == 'r') {
				printf("Running  ");
			}
			else if (jobs[i].state == 's') {
				printf("Stopped  ");
			}
			else if (jobs[i].state == 't') {
				printf("Terminated  ");
			}
			printf("%s \n", jobs[i].cmd);
		}
	}
}
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char* cmdline)
{
	char* arg3[MAXARGS];
	char* arg2[MAXARGS];
	char* arg1[MAXARGS];
	char* argv[MAXARGS]; /* Argument list execve() */
	char buf[MAXLINE]; /* Holds modified command line */
	char buf1[MAXLINE];
	char buf2[MAXLINE];
	char buf3[MAXLINE];
	pid_t pid;   /* Process id */
	int bg;
	int i;
	int sigttou_handle=0; // 0-> no "less" command, 1-> exist "less" command
	sigset_t mask;
	
	strcpy(buf, cmdline);
	int a = pipeparse(buf, argv); //parse by '|'
	
	if (argv[0] == NULL) {
		return; 
	} /* Ignore empty lines */
		 
	strcpy(buf1, argv[0]);
	int b = parseline(buf1, arg1);  //parse by space
	if (arg1[0] == NULL) {
		return;
	}
	for (int i = 0;i <= count;i++) { // check if cmd has '&" information
		strcpy(buf3, argv[i]);
		bg = parseline(buf3, arg3);
		if (!(strcmp(arg3[0], "less"))) {  //sheck if there are  "less" command
			sigttou_handle = 1;
		}
	}
	
	
	if (!builtin_command(arg1)) {
		
			Sigemptyset(&mask);
			Sigaddset(&mask, SIGCHLD);
			if (bg) {  //if background, block sigchild
				Sigprocmask(SIG_BLOCK, &mask, NULL);
			}
		if ((pid = Fork()) == 0) {
			
			if ((sigttou_handle==0)||bg) { //if there is no "less"command or its background, set group id
				Setpgid(0, 0);
			}
			Sigprocmask(SIG_UNBLOCK, &mask, NULL);
			if (count == 0) {
				if (execvp(arg1[0], arg1) < 0) {
					printf("%s: Command not found.\n", arg1[0]);
					exit(0);
				}
					
			}
			else
			{
				for (i = 0;i < count;i++) {
					execpipe(argv[i], argv[i + 1], i);

				}
			}
		}
		/* Parent waits for child to terminate*/
		else
		{
			for (int k = 0;k <= count;k++) { //inserts jobs
				if (count >= 1) k = count;
				strcpy(buf2, argv[k]);
				int tbg = parseline(buf2, arg2);
				insertjb(pid, 'r', argv[k], tbg);
			}
			
			if (bg)
				Sigprocmask(SIG_UNBLOCK, &mask, NULL);
			
			else {   //wait for same group id child to terminate 
				int status;
				waitpid(0, &status, 0);
			}
		}
	}
	
   return; 
}

/*executing pipe */
void execpipe(char *arg, char *arg2,int i)
{
		int fds[2];
		int a = pipe(fds);	
		pid_t pid; 
		char buf1[MAXLINE];
		char buf[MAXLINE];
		char *arga[MAXARGS];
		char *argb[MAXARGS];
		int bg;
		sigset_t mask;

		strcpy(buf1, arg);
		strcpy(buf, arg2);
		bg = parseline(buf, argb);
		int b = parseline(buf1, arga);

		Sigemptyset(&mask);
		Sigaddset(&mask, SIGCHLD);
		if (bg) {
			Sigprocmask(SIG_BLOCK, &mask, NULL);
		}
			if ((pid = Fork()) == 0) {   /* Child runs user job */
				
				
				Sigprocmask(SIG_UNBLOCK, &mask, NULL);
					dup2(fds[1], 1); //set stdout to pipe
					close(fds[0]);  //close reading pipe
					if (execvp(arga[0], arga) < 0) {
						printf("%s: Command not found.\n", arga[0]);
						exit(0);
					}
			}
			else
			{
					
					dup2(fds[0], 0); //set stdin to pipe
					close(fds[1]);  //close writing pipi
					if (i == count - 1) {
						if (execvp(argb[0], argb) < 0) {
							printf("%s: Command not found.\n", argb[0]);
							exit(0);
						}
					}
					Sigprocmask(SIG_UNBLOCK, &mask, NULL);
			}

		return;
}
/*cmd = "fg"*/
void execfg(char** argv) {

	char* arg = argv[1];
	int jid=0;
	int temp = 0;
	job_l* cur_job;

	if (argv[1] == NULL) // no parameter
	{
		printf("Need to type # of job id\n");
		return;
	}
	for (int i = 0;i < strlen(arg);i++)
	{
		if (arg[i] == '%') { //find "%" character
			temp = 1;
			jid = atoi(&arg[i + 1]);
		}
	}
	if (jid == 0) { //if the input is 0
		printf("Not in correct fg format. please enter a %%number.\n");
		return;
	}
	if ((cur_job = getjbid(jid)) == NULL) // there  are no jobs to exectute fg
	{
		printf("No such job\n");
		return;
	}
	else
	{
		/*change status of jobs*/
		cur_job->fb = 0;
		cur_job->state = 'r';
		prsinglejob(cur_job->pid);  // print job status
		if (kill(-cur_job->pid, SIGCONT) < -1) { // send continue signal to jobs
			printf("kill error\n");
		} 
		
		waitfg(cur_job->pid); // wait for its foregroud jobs to terminate
		
	}
}
 /*function that waits its forground job to terminate*/
void waitfg(pid_t pid)
{
	if (pid == 0) return;                      

	while (pid == getfgpid()) {    //wait until there are no forground jobs            
		sleep(1);                            
	}

	return;
}

/*cmd = "bg "*/
void execbg(char** argv) {
	char* arg = argv[1];
	int jid = 0;
	int temp = 0;
	job_l* cur_job;
	if (argv[1] == NULL)  // no parameter
	{
		printf("Need to type # of job id\n");
		return;
	}
	for (int i = 0;i < strlen(arg);i++)
	{
		if (arg[i] == '%') { //find "%" character
			temp = 1;
			jid = atoi(&arg[i + 1]);
		}
	}
	if (jid == 0) { //if the input is 0
		printf("Not in correct bg format. please enter a %%number.\n");
		return;
	}
	if ((cur_job = getjbid(jid)) == NULL)// there  are no jobs to exectute bg
	{
		printf("No such job\n");
		return;
	}
	else
	{
		/*change status of jobs*/
		cur_job->fb = 1;
		cur_job->state = 'r';
		prsinglejob(cur_job->pid); // print job status
		if (kill(-cur_job->pid, SIGCONT) < -1) { // send continue signal to jobs
			printf("kill error\n");
		}  
		
	}
}

/*cmd = "kill"*/
void execkill(char** argv) {
	char* arg = argv[1];
	int jid = 0;
	int temp = 0;
	job_l* cur_job;
	if (argv[1] == NULL)
	{
		printf("Need to type # of job id\n");
		return;
	}
	for (int i = 0;i < strlen(arg);i++)
	{
		if (arg[i] == '%') {
			temp = 1;
			jid = atoi(&arg[i + 1]);
		}
	}
	if (jid == 0) {
		printf("Not in correct kill format. please enter a %%number between 1~9\n");
		return;
	}
	if ((cur_job = getjbid(jid)) == NULL)
	{
		printf("No such job\n");
		return;
	}
	else   
	{
		if (kill(cur_job->pid, SIGKILL) < -1) { //send SIGKILL signal to given pid job
			printf("kill error\n");
		}  
		return;
	}
}

/*get a job with given job id*/
job_l *getjbid(int jid) {
	
	for (int i = 0;i < MAXJOBS;i++) {
		if (jobs[i].id == jid) {
			return &jobs[i];
		}
	}
	return NULL;
}



/* get a job with given pid*/
job_l* getjbpid(pid_t pid) {
	for (int i = 0;i < MAXJOBS;i++) {
		if (jobs[i].pid == pid) {
			return &jobs[i];
		}
	}
	return NULL;
}

/*get job id with given pid*/
int getidpid(pid_t pid) {
	for (int i = 0;i < MAXJOBS;i++) {
		if (jobs[i].pid == pid) {
			return jobs[i].id;
		}
	}
	return 0;
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
	if (!strcmp(argv[0], "jobs")) {
		printjob();
		return 1;
	}
	if (!strcmp(argv[0], "kill")) {
		execkill(argv);
		return 1;
	}
	if (!strcmp(argv[0], "fg")) {
		execfg(argv);
		return 1;
	}
	if (!strcmp(argv[0], "bg")) {
		execbg(argv);
		return 1;
	}
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
	int bg = 0;
   // buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
	if (buf[strlen(buf) - 1] == 38) //if it is background job with &, parse it
	{
		if (buf[strlen(buf) - 2] != ' ') {
			buf[strlen(buf) - 1] = ' ';
			buf[strlen(buf)] = '&';
			buf[strlen(buf) + 1] = ' ';
			
		}
		bg = 1;
	}
    else
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
			return 0;/*works only for first input directory*/
		}
        	while ((delim = strchr(buf, '/'))) {/*parse line with '/'*/
            		argv[argc++] = buf;
            		*delim = '\0';
            		buf = delim + 1;
		}
	argv[argc] = NULL;
	return 0;
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
	return 0;

	
	/* Should the job run in the background? */
	/*if ((bg = (*argv[argc - 1] == '&')) != 0)
		argv[--argc] = NULL;*/
	//printf("bg=%d ", bg);
	return bg;

}
/* $end parseline */
int pipeparse(char* line, char** arg1)
{
	char* delim;         /* Points to first space delimiter */
	int argc;            /* Number of args */

	line[strlen(line) - 1] = '|';  /* Replace trailing '\n' with space */
	while (*line && (*line == ' ')) /* Ignore leading spaces */
		line++;

	/* Build the argv list */
	argc = 0;
	while ((delim = strchr(line, '|'))) {
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

/*delete " or '*/
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

