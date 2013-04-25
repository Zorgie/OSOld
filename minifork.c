#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "docmd.h"
#include <string.h>

#include <stdio.h>

#define NUM_ARGS 6

void wait_for_child()
{
	int status;
	int childpid;
	childpid = wait(&status);
	if(-1==childpid)
	{
		fprintf(stderr, "LOLWAITSOBAD");
		exit(1);
	}
}

int split_on_whitespace(char buf[70], char *res[])
{
	int i;
	char* temp = strtok(buf, " \n");
	int count = 0;
	for(i=0; i<NUM_ARGS; i++)
	{
		if(temp == NULL)
			break;
		res[i] = malloc(sizeof(char)*70);
		strcpy(res[i], temp);
		temp = strtok(NULL, " \n");
		count++;
	}
	return count;
}

int main(int argc, char* argv[], char* env[])
{
	while(1)
	{
		char input[70];
		printf("> ");
		fgets(input, 70, stdin);
		char* split[] = {NULL, NULL, NULL, NULL, NULL, NULL};
		int words = split_on_whitespace(input, split);
		int local_exec = 0;
		if(words > 0)
		{
			if(0 == strcmp(split[0], "exit"))
			{
				exit(0);
			}else if(2 <= words  && 0 == strcmp(split[0], "cd"))
			{
				chdir(split[1]);
				local_exec=1;
			}
		}
		if(0 == local_exec)
		{
			int pid = fork();
			int status;
			if(pid == 0)
			{
				status = doCmd(words, split);
				fprintf(stderr, "Execvp returned %d\n",status);
				exit(1);
			}
			else{
				wait_for_child();
			}
		}
	}
	return 0;
}
