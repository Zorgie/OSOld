#include <sys/types.h> /* definierar typen pid_t */
#include <sys/wait.h> /* definierar bland annat WIFEXITED */
#include <errno.h> /* definierar errno */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <unistd.h> /* definierar bland annat pipe() och STDIN_FILENO */
#define PIPE_READ_SIDE ( 0 )
#define PIPE_WRITE_SIDE ( 1 )
pid_t childpid; /* för child-processens PID vid fork() */
int return_value; /* för returvärden från systemanrop */
/* Vänta på child processerna  */
void waitForChild()
{
	/* Variabel som håller koll på returvärden från systemanrop */
    int status;
	/* Variabel som lagrar IDt för barnprocesser. */
	int childpid;
	childpid = wait( &status ); /* Vänta på ena child-processen */
    if( -1 == childpid )
    {
        perror( "wait() failed unexpectedly" ); exit( 1 );
    }
    if( WIFEXITED( status ) ) /* child-processen har kört klart */
    {
        int child_status = WEXITSTATUS( status );
        if( 0 != child_status ) /* child had problems */
        {
            fprintf( stderr, "Child (pid %ld) failed with exit code %d\n",
            (long int) childpid, child_status );
        }
    }
    else
    {
        if( WIFSIGNALED( status ) ) /* child-processen avbröts av signal */
        {
            int child_signal = WTERMSIG( status );
            fprintf( stderr, "Child (pid %ld) was terminated by signal no. %d\n",
            (long int) childpid, child_signal );
        }
    }
}

/* Stäng pipes, skriv ut felmeddelande vid fel */
void close_pipes(int count, int pipes[3][2])
{
	int i;
	for(i = 0; i<count; i++)
	{
		int j;
		for(j =0; j<2; j++)
		{
			if(-1 == close(pipes[i][j]))
			{
				fprintf(stderr, "Error closing pipe %d %d\n", i, j);
			}
		}
	}
}

/* Om return_value är -1 skriv ut felmeddelande och avsluta */
void check_return_value_pipe()
{
  if( -1 == return_value)
    {
      perror( "Cannot bind to pipe." ); exit( 1 );
    }
}

/*
Jämför de första <limit> tecknen av två strängar. Om en sträng termineras
(char-värde 0) före den andra inom gränsen för limit returneras 0.
*/
int strcmp_lim(char *str1, char *str2, int limit)
{
	int i;
	for(i = 0; i<limit; i++)
	{
		/* Båda strängarna termineras samtidigt, returnerar sant. */
		if(str1[i] == 0 && str2[i] == 0)
			return 1;
		if(str1[i] != str2[i])
			return 0;
	}
	return 1;
}

int main(int argc, char **argv, char **envp)
{
	/* Pager 1 = less, 2 = more */
	char* pager = "less";
	int i;

	/* Letar igenom miljövariablerna efter PAGER. Finns den, använd less om 
	den finns, om den finns men inte innehåller less används more.*/	
	for(i = 0; envp[i] != NULL; i++)
	{
		if(strcmp_lim(envp[i], "PAGER", 5))
		{
			if(!strcmp_lim(envp[i]+6,"less",4))
				pager = "more";
		}
	}
    /* Pipes */
	int pipe_filedesc[3][2];
	
	/* Skapa så många pipes som behövs, beror på om vi fick parametrar eller inte */	
	int child_count = (argc > 1)?4:3;
	for(i = 0; i< child_count-1; i++)
	{
		return_value = pipe(pipe_filedesc[i]);
		check_return_value_pipe();
	}
	/* Håller koll på vilken pipe som skall användas. */
	int current_pipe = 0;

  	childpid = fork(); /* skapa första child-processen (för printenv) */
	if(0 == childpid)
	{
		/* Bind om stdout till pipe. */
		return_value = dup2( pipe_filedesc[current_pipe][ PIPE_WRITE_SIDE ], STDOUT_FILENO ); /* STDOUT_FILENO == 1 */
	    /* Kolla om allt gick bra */
	    check_return_value_pipe();
	    /* Stäng alla pipes */
		close_pipes(child_count-1, pipe_filedesc);

	    (void) execlp( "printenv", "printenv", (char *) 0 );
	}
	current_pipe++;
  	/* Skriv ut processens childpid */
  	fprintf(stderr, "printenv: %d\n",childpid );
	
  	/* Om vi har fått en parameterlista skall vi utföra grep här. */
	if(argc > 1)
	{
		childpid = fork();
		if( 0 == childpid )
		{
			/* Bind om stdin till pipe. */
			return_value = dup2( pipe_filedesc[current_pipe-1][ PIPE_READ_SIDE ], STDIN_FILENO ); /* STDIN_FILENO == 0 */
      		/* Kolla om allt gick bra */
		    check_return_value_pipe();
			/* Bind om stdout till pipe. */
			return_value = dup2( pipe_filedesc[current_pipe][ PIPE_WRITE_SIDE ], STDOUT_FILENO ); /* STDOUT_FILENO == 1 */
	      	/* Kolla om allt gick bra */
    	  	check_return_value_pipe();
			/* Stäng alla pipes */
      		close_pipes(child_count-1, pipe_filedesc);

			argv[0] = "grep";
			execvp( "grep", argv);
      		fprintf(stderr, "Error in grep\n");
		}
		current_pipe++;
    	/* Skriv ut processens childpid */
    	fprintf(stderr, "grep: %d\n",childpid );
	}

	childpid = fork(); /* skapa andra child-processen (för sort) */
    if (0 == childpid )
    {
		/* Bind om stdin */
		return_value = dup2( pipe_filedesc[current_pipe-1][ PIPE_READ_SIDE ], STDIN_FILENO ); /* STDIN_FILENO == 0 */
	    /* Kolla om allt gick bra */
		check_return_value_pipe();
		/* Bind om stdout */
		return_value = dup2( pipe_filedesc[current_pipe][ PIPE_WRITE_SIDE ], STDOUT_FILENO ); /* STOUT_FILENO == 1 */
		/* Kolla om allt gick bra */
    	check_return_value_pipe();
    	/* Stäng alla pipes */
		close_pipes(child_count-1, pipe_filedesc);
		
    	(void) execlp( "sort", "sort", (char *) 0 );
	}
  	/* Skriv ut processens childpid */
  	fprintf(stderr, "sort: %d\n",childpid );

	current_pipe++;


	childpid = fork();
	if( 0 == childpid ) 
	{
		/* bind om stdin */
		return_value = dup2( pipe_filedesc[current_pipe-1][ PIPE_READ_SIDE ], STDIN_FILENO ); /* STDIN_FILENO == 0 */
		/* Kolla om allt gick bra */
    	check_return_value_pipe();

    	/* Stäng alla pipes */
		close_pipes(child_count-1, pipe_filedesc);	
    	(void) execlp( pager, pager, (char *) 0 );
	}
  	/* Skriv ut processens childpid */
  	fprintf(stderr, "less: %d\n",childpid );



	/* Stäng alla pipes */
	close_pipes(child_count-1, pipe_filedesc);
	
  	/* Vänta på alla barn */
	for(i = 0; i<child_count; i++)
	{
	    waitForChild();
	}
    exit( 0 ); /* Avsluta parent-processen på normalt sätt */
}	
