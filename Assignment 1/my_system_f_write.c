/*
 * my_system_f_write.c
 *
 *  Created on: Sep 22, 2018
 *      Author: anza95
 */

#include "tiny_shell.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

int my_system_f_write(char *user_command, char *my_fifo);

//this is an implementation of the UNIX system call system() using fork()
int main(int argc, char *argv[])
{
	//name of the fifo will come from the second command line argument
	char *my_fifo = argv[1];
	while(1)
	{
		char *command;
		struct timespec startTime, stopTime;

		//ask user for command
		puts("Enter a command:");
		command = get_a_line();

		//we check the size of the command and based on that execute
		if(length(command) > 1)
		{

			//start timer after user has entered command
			if(clock_gettime(CLOCK_REALTIME, &startTime) == -1)
			{
				perror("clock_gettime");
			}

			my_system_f_write(command, my_fifo);

			if(clock_gettime(CLOCK_REALTIME, &stopTime) == -1)
			{
				perror("clock_gettime");
			}
			double time_passed_mili = ((stopTime.tv_sec - startTime.tv_sec)*1000)+(stopTime.tv_nsec - startTime.tv_nsec)/1000000.0;
			printf("time elapsed per command  = %f ms \n",time_passed_mili);

		}

		//else, no command exit
		else
		{
			exit(0);
		}
	}

	return(0);
}


//this is an implementation of the UNIX system call system() using fork() but with additional support to support the writing
//end of a named pipe

int my_system_f_write(char *user_command, char *my_fifo)
{
	//this local variable will check the process ID, this is to differentiate between parent and child
	pid_t pid_checker;
	char *args[20];
	int status;
	int fd_write;
	int child_exit_status;


	for(int i=0; i<20; i++) {
		args[i] = NULL;
	}

	//tokenize the string to handle tabs, newline and convert to null char
	string_tokenizer(user_command, args);


	//we now call fork() here to create a child process which will execute user command
	pid_checker = fork();

	//if user entered exit, exit
	if(strcasecmp("exit",args[0]) == 0)
		exit(0);

	// now we check the value of pid_checker, if 0 it's the child process being executed, if not parent
	if(pid_checker == 0)
	{
		//we are now in the child process, it is here that we execute the user's command

		//we are now rewiring the stdout to the writing end of our fifo
		//this means that anything writing to stdout will instead write to our fifo.
		fd_write = open(my_fifo, O_WRONLY);
		if(fd_write == -1)
		{
			perror("open");
		}
		close(1);
		dup2(fd_write, 1);

		//we call execvp because the child process is an identical clone of the parent
		//it will change the program to the shell command entered by the user
		child_exit_status = execvp(args[0], args);
		if(child_exit_status == -1)
		{
			perror("child failed");
			exit(EXIT_FAILURE);//Exit the failed child process
		}
	}

	//we want to ensure that the child successfully finishes execution of user command
	//thus in parent we call wait
	else if(pid_checker > 0)
	{
		//we call waitpid so that we can wait for the child to execute in proper time
		waitpid(pid_checker, &status, 0);
		if(status == -1)
		{
			perror("The child process failed.\n");
		}
	}

	//if the value of pid_checker is negative, we know fork failed, so exit my_system_f
	else
	{
		perror("fork failed");
		exit(EXIT_FAILURE);
	}

	return(0);
}
