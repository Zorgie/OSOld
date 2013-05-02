#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>

#include <stdio.h>

#define NUM_ARGS 7

int doCmd(int argc, char* argv[])
{
	if(argc < 1)
	{
		fprintf(stderr, "Please specify a command that you want to run\n");
		return 1;
	}
	return execvp(argv[0], argv);
}

int and_sign(char *args[], int words)
{
	int i;
	int res=-1;
	for(i=0; i<words; i++)
	{
		if(args[i][0] == '&')
			res = i;
		if(res != -1)
			args[i] = NULL;
	}
	return res;
}

void wait_for_child(int bg_mode)
{
	int status;
	int childpid;
	if(!bg_mode)
		childpid = wait(&status);
	else
	{
		while((childpid = waitpid(-1,&status, WNOHANG))>0);
	}
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
		char* split[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
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
		int andsign = and_sign(split, words);
		int bgprocess = 0;
		if(-1 != andsign){
			words = andsign;
			bgprocess = 1;
		}
		/* Foreground process, not one of the local commands. */
		if(0 == local_exec)
		{
			struct timeval tv1, tv2;
			if(!bgprocess)	
				gettimeofday(&tv1, 0);
			int pid = fork();
			int status;
			if(pid == 0)
			{
				status = doCmd(words, split);
				fprintf(stderr, "Execvp returned %d\n",status);
				exit(1);
			}
			else{
				if(!bgprocess)
				{
					wait_for_child(0);
					gettimeofday(&tv2,0);
					long msec = (tv2.tv_sec - tv1.tv_sec)*1000+(tv2.tv_usec - tv1.tv_usec)/1000; 
					printf("Milliseconds passed: %ld\n", msec);
				}else
				{
					wait_for_child(1);
				}
		
			}
		}
	}
	return 0;
}
