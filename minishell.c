#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


#include <stdio.h>

/* The maximum number of initial arguments (including the &-sign and the command name)*/
#define NUM_ARGS 7
/* The maximum number of child processes that we will be able to keep track of (for termination purposes). */
#define MAX_PROCESSES 100

/* Array containing all the child processes that will be spawned. */
int processes[MAX_PROCESSES];

/* Performs a command stored in the first slot of the argv[] array. Also does some basic error checking. */
int doCmd(int argc, char* argv[])
{
	/* Checks so there are enough arguments */
	if(argc < 1)
	{
		fprintf(stderr, "Please specify a command that you want to run\n");
		return 1;
	}
	return execvp(argv[0], argv);
}

/* 
	Finds a &-sign within an argument vector. If found, the string starting with & and all strings thereafter will be set to NULL.
	Returns the number of strings before the &-sign if such a sign was found as the first sign of a string, or -1 if it was not found.
 */
int and_sign(char *args[], int words)
{
	int i;
	int res=-1;
	for(i=0; i<words; i++)
	{
		if(res ==-1 && args[i][0] == '&')
			res = i;
		/* If we have found a &-sign, set the remaining strings to NULL */
		if(res != -1)
			args[i] = NULL;
	}
	return res;
}


/*
 	Performs an action upon one or several saved child processes.
 	Actions: 1 = init, 2 = save, 3 = remove, 4 = kill all 
	Returns the number of processes affected by the action.
*/
int process_action(int action, int process)
{
	int i;
	int affected = 0;
	/* Iterates over all our possible processes. */
	for(i=0; i<MAX_PROCESSES; i++)
	{
		/* 	Performs different actions for different values of the action variable,
		 	as specified in the function documentation. */
		switch(action)
		{
			case 1:
			processes[i] = 0;
			affected++;
			break;
			case 2:
			if(processes[i] == 0)
				{
					processes[i] = process;
					return 1;
				}
			break;
			case 3:
			if(processes[i] == process)
			{
				processes[i] = 0;
				return 1;
			}
			break;
			case 4:
			if(processes[i] != 0)
			{
				affected++;
				kill(processes[i],SIGINT);
			}
			break;
		}
	}
	return affected;
}


/* Waits for a child process. If bg_mode is set to 1, it will check for any finished background processes and terminate them. */
void wait_for_child(int bg_mode)
{
	/* Variable for storing the resulting status. */
	int status;
	/* Variable for storing the terminated child pid. */
	int childpid;
	if(bg_mode == 0)
	{
		childpid = wait(&status);
		process_action(3,childpid);
	}else
	{
		/* Looks for any terminated background processes that are waiting to signal, and cleans them up. */
		while((childpid = waitpid(-1,&status, WNOHANG))>0)
		{
			process_action(3,childpid);
			printf("Background process %d terminated.\n", childpid);
		}
	}
}

/* Takes a buffer of chars, and splits them into an array of strings, with whitespace as a delimitor. */
int split_on_whitespace(char buf[70], char *res[])
{
	int i;
	/* Variable for tokenizing. */
	char* temp = strtok(buf, " \n");
	int count = 0;
	for(i=0; i<NUM_ARGS; i++)
	{
		if(temp == NULL)
			break;
		/* Allocates space for the argument vector. */
		res[i] = malloc(sizeof(char)*70);
		/* Fills the argument vector with data from the split string */
		strcpy(res[i], temp);
		/* Continues the tokenizing */
		temp = strtok(NULL, " \n");
		count++;
	}
	return count;
}
 
/*	A handler for catching <ctrl-c> interrupts (SIGINT)
	Will catch interrupts, and send it on to every child processes, but ignore it in the main shell.	
 */
void my_handler (int sig)
{
	/* Asks every process to terminate by sending SIGINT */
	int killed = process_action(4,0);
	printf("\n%d processes terminated.\n",killed);
	/* Re-registers the handler, to catch future SIGINTs */
	signal(SIGINT, my_handler);
	return;
}

pid_t base_pid;

void my_handler2(int sig)
{
	kill(base_pid, SIGINT);
	main();
}


int main()
{
	pid_t parent = getpid();
	base_pid = fork();
	if(0 == base_pid)
	{
		main2(parent);
	}else
	{
		
	/* Registers a signal handler to catch SIGINTs, interrupts from <ctrl-c> */
	signal(SIGINT, my_handler2);
	}
	while(1);
}

/* Entry point of the program. */
int main2(pid_t parent)
{
	/* Initiates our list of child processes to 0 */
	process_action(1,0);
	/* Registers a signal handler to catch SIGINTs, interrupts from <ctrl-c> */
	/*signal(SIGINT, my_handler);*/
	while(1)
	{
		/* Variable for storing user input */
		char input[70];
		printf("> ");
		/* Retrieves input from the user */
		fgets(input, 70, stdin);
		/* Variable that will store our arguments to any system calls. */
		char* split[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
		/* Fills the split-array with arguments from our input buffer, 
		and receives the number of arguments in the array. */
		int words = split_on_whitespace(input, split);
		/* Variable for keeping track of whether the execution is a local command (cd and exit) */
		int local_exec = 0;
		if(words > 0)
		{
			/* The exit command */
			if(0 == strcmp(split[0], "exit"))
			{
				kill(parent, SIGKILL);
				exit(0);
			}else if(0 == strcmp(split[0], "^C"))
			{
				fprintf(stderr,"\nFound ctrl c\n");
				local_exec = 1;
			}
			else if(2 <= words  && 0 == strcmp(split[0], "cd")) /* The cd command. */
			{
				/* Attempts a cd. If the target dir is invalid, changes to the enviromental variable HOME. */
				if (-1 == chdir(split[1]))
					chdir(getenv("HOME"));
				local_exec=1;
			}
		}
		/* Looks through the input array for an & sign that will indicate a background execution. */
		int andsign = and_sign(split, words);
		/* Variable for keeping track of whether or not the process shall be run in the background. */
		int bgprocess = 0;
		if(-1 != andsign){
			words = andsign;
			bgprocess = 1;
		}
		/* Not one of the local commands. */
		if(0 == local_exec)
		{
			/* Variables for keeping track of the execution times. */
			struct timeval tv1, tv2;
			if(0 == bgprocess)	/* If the process is a foreground one, keep track of the time. */
				gettimeofday(&tv1, 0);
			pid_t pid = fork();
			int status;
			if(pid == 0)
			{
				/* Child process. Attempts to change process through a system call. */
				status = doCmd(words, split);
				fprintf(stderr, "Execvp returned %d\n",status);
				exit(1);
			}
			else{
				/* Parent process.*/
				printf("Process %d started.\n", pid);
				/* Stores the process ID in our array. */
				process_action(2,pid);
				if(!bgprocess)
				{
					/* Checks for any terminated background processes (non-blocking). */
					wait_for_child(1);
					/* Waits for our foreground process to terminate. */
					wait_for_child(0);
					/* Gets the execution time of the process, and prints it properly. */
					gettimeofday(&tv2,0);
					long msec = (tv2.tv_sec - tv1.tv_sec)*1000+(tv2.tv_usec - tv1.tv_usec)/1000; 
					printf("Process %d terminated.\nWallclock time: %ld ms\n", pid, msec);
				}else
				{
					/* Background process. Just checks for any terminated background processes, and keeps moving.*/
					wait_for_child(1);
				}
		
			}
		}
	}
	return 0;
}
