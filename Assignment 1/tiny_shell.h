/*
 * This is a header file I wrote for helper functions that  I call in my c files
 * tiny_shell.h
 *
 *  Created on: Sep 22, 2018
 *      Author: anza95
 */

#ifndef TINY_SHELL_H_
#define TINY_SHELL_H_

#include <string.h>
#include <stdio.h>


void string_tokenizer(char *user_input, char *args[]);
char *get_a_line();
int length(char *line);

//this is a function to tokenize the user input to handle cases where the user enters command flags
//an example of this being the ls -alt command
//What this does is simply remove all tabs, white spaces and new lines and replaces with the null terminating character
//as a result each args[i] is treated as a separate string v/s a long string comprised of tabs, white space and new lines
void string_tokenizer(char *user_input, char *args[])
{
	char *converted_string;
	int arg_counter = 0;

	//cycle through the input, removing all tabs, new lines and white spaces
	while((converted_string = strsep(&user_input, " \t\n")) != NULL)
	{
		for(int i =0; i < strlen(converted_string); i++)
		{
			//checking if its less than or equal to 32 because this is the ASCII value of white space
			if(converted_string[i] <= 32)
			{
				//replace with null terminating character thus separating the strings in memory
				converted_string[i] = '\0';
			}
		}
		//copying tokenized string back
		if(strlen(converted_string) > 0)
		{
			args[arg_counter] = converted_string;
			arg_counter++;

		}
	}

}

char *get_a_line()
{
	//variable that gets returned to user
	char *user_line;
	ssize_t nbytes = 0;

	//calling getline and taking input from user
	getline(&user_line, &nbytes, stdin);
	return user_line;
}

int length(char *line)
{
	//variable that will hold the size
//	int x = 0;
//	//cycle through all elements of the string, until NULL terminating character
//	while(line[x] != '\0')
//	{
//		x++;
//	}
//	return x;
	return(strlen(line));
}

#endif /* TINY_SHELL_H_ */
