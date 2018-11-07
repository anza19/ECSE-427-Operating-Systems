/*
 * This is a main source for the implementations of tshell using various Linux system calls
 * tshell.c
 *  Created on: Sep 22, 2018
 *      Author: anza95
 */

#define _GNU_SOURCE

//this is a header file I made for helper functions I made for the assignment
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

//setting the stack size for our clone implementation of my_system
#define STACK_SIZE 8192

//function prototypes
int childFunc(void *str_ptr);
int my_system_f(char *user_command);
int my_system_v(char *user_command);
int my_system_c(char *user_command);

int main(int argc, char *argv[])
{
	setbuf(stdout, NULL);
	while(1)
	{
		char *command;
		struct timespec startTime, stopTime;

		//ask user for command
		puts("Enter a line:");
		command = get_a_line();
		//we check the size of the command and based on that execute
		if(length(command) > 1)
		{

			//start timer after user has entered command
			if(clock_gettime(CLOCK_REALTIME, &startTime) == -1)
			{
				perror("clock_gettime");
			}
			//preprocessing compiler flags for testing various versions of my_system()
#ifdef FORK
			my_system_f(command);
#elif V_FORK
			my_system_v(command);
#elif CLONE
			my_system_c(command);
#elif SYSTEM
			if(strcasecmp(command,"exit\n") == 0)
			{
				exit(0);
			}
			system(command);
#endif

			if(clock_gettime(CLOCK_REALTIME, &stopTime) == -1)
			{
				perror("clock_gettime");
			}

			//calculating the time taken to execute command
			double time_passed_mili = ((stopTime.tv_sec - startTime.tv_sec)*1000)+(stopTime.tv_nsec - startTime.tv_nsec)/1000000.0;
			printf("time elapsed per command  = %f ms \n",time_passed_mili);
		}

		else
		{
			exit(0);
		}

	}

	return(0);
}

//this is an implementation of the UNIX system call system() using fork() system call
int my_system_f(char *user_command)
{
	//this local variable will check the process ID, this is to differentiate between parent and child
	pid_t pid_checker;
	char *args[20];
	int status;
	int child_exit_status;

	//this for loop will set to NULL the args array that will hold user input after tokenizing the command
	for(int i=0; i<20; i++) {
		args[i] = NULL;
	}
	//after initilization, we now tokenize the input
	string_tokenizer(user_command, args);

	//we now call fork() here to create a child process which will execute user command
	pid_checker = fork();

	if(strcasecmp("exit",args[0]) == 0)
		exit(0);

	// now we check the value of pid_checker, if 0 it's the child process being executed, if not parent
	if(pid_checker == 0)
	{
		//we are now in the child process, it is here that we execute the user's command
		//we call execvp because the child process is an identical clone of the parent
		//it will change the program to the shell command entered by the user

		child_exit_status = execvp(args[0], args);
		if(child_exit_status == -1)
		{
			perror("child failed");
			exit(EXIT_FAILURE); //Exit the failed child process
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
			perror("The child process failed.");
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

//this is an implementation of the UNIX shell using vfork() to create the child process instead of fork()
int my_system_v(char *user_command)
{
	//this local variable will check the process ID, this is to differentiate between parent and child
	char *args[20];
	int status;
	pid_t pid_checker;
	int child_exit_status;

	for(int i=0; i<20; i++) {
		args[i] = NULL;
	}

	//tokenize the string to handle tabs, newline and convert to null char
	string_tokenizer(user_command, args);

	//we now call vfork() here to create a child process which will execute user command
	pid_checker = vfork();

	if(strcasecmp("exit",args[0]) == 0)
		exit(0);

	// now we check the value of pid_checker, if 0 it's the child process being executed, if not parent
	if(pid_checker == 0)
	{
		//we are now in the child process, it is here that we execute the user's command
		//we call execvp because the child process is an identical clone of the parent
		//it will change the program to the shell command entered by the user
		child_exit_status = execvp(args[0], args);
		if(child_exit_status == -1)
		{
			perror("child failed");
			exit(EXIT_FAILURE); //Exit the failed child process

		}
	}

	//parent has to wait for the child
	else if(pid_checker > 0)
	{
		waitpid(pid_checker, &status, 0);
		if(status == -1)
		{
			perror("waitpid");
		}

	}

	//vforking a child process failed
	else
	{
		perror("vfork failed");
		exit(EXIT_FAILURE);
	}

	return(0);
}

//implementation of the UNIX system call using clone() system call to create the child process
int my_system_c(char *user_command)
{
	int status;
	char *stack, *stackTop;
	pid_t pid;
	pid_t parent_wait;

	//firstly check if user entered exit, if so exit
	if(strcasecmp("exit\n",user_command) == 0)//  || strcasecmp("exit",user_command) == 0)
	{
		exit(0);
	}

	//firstly we need to create our stack
	//allocated a buffer for stack creation
	stack = malloc(STACK_SIZE);
	if(stack == NULL)
	{
		perror("Could not memory for the stack");
		exit(EXIT_FAILURE);
	}
	stackTop = stack + STACK_SIZE;

	//calling clone for the child process
	pid = clone(childFunc,stackTop, CLONE_VFORK | CLONE_FS , user_command);
	if(pid == -1)
	{
		perror("clone");
	}

	//have the parent wait until the child is done executing
	parent_wait = waitpid(pid, &status, 0);
	if(status == -1)
	{
		perror("waitpid");
	}

	//free stack
	free(stack);
	return (0);
}

//this is the function that we call for child in my_system_c to execute user command
int childFunc(void *str_ptr)
{
	int directory_change, exit_checker;
	char *args[20];
	int child_exit_status;

	//calling string tokenzier
	string_tokenizer((char *)str_ptr, args);

	//added for cd command
	directory_change = strcasecmp(args[0],"cd");
	if(directory_change == 0)
	{
		//path gets passed to args[1], so we call chdir on that
		child_exit_status = chdir(args[1]);
	}

	else
	{
		//run user command
		child_exit_status = execvp(args[0], args);
	}

	if(child_exit_status == -1)
	{
		perror("child exit status");
		exit(EXIT_FAILURE); //exit current failed child

	}
	return(0);

}
