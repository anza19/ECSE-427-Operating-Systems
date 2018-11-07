Hello to anyone reading the README.txt.
It contains instructions on how to compile and run my code for the first assignment for ECSE 427.

There are 4 files of interest:

1) tshell.c
2) tiny_shell.h
3) my_system_f_write.c
4) my_system_f_read.c

The file tshell.c contains code to run the various versions of system using system, fork, vfork and clone.
They contain compiler flags which are nested within a series of preprocessor conditional directives which can be compiled
using the compiler commands in the grading rubric.

The following are the said commands

1) gcc -D FORK tshell.c -o tshell
2) gcc -D SYSTEM tshell.c -o tshell
3) gcc -D V_FORK tshell.c -o tshell
4) gcc -D CLONE tshell.c -o tshell

The code for the fifo part of the assignment is broken into files: my_system_f_write.c and my_system_f_read.c
To run the fifo part firstly you must create a fifo using the mkfifo command along with ( I used mkfifo -m pathname 777)
along with respected permissions (777).

One you have that, you first compile the writing end of fifo using:

1) gcc my_system_f_write.c -o write

Than you run the binary along with the fifo
2) ./write fifo_name
 
Than you do a similar exercise for read:
1) gcc my_system_f_read.c -o read

Than you run the binary along with the fifo
2) ./read fifo_name

Also, you wish to test exit in the IPC part of the assignment you must exit in both terminals because we are running two separate
processes. So you must exit in the write and read.

It is very, very important that you RUN WRITE FIRST, THAN READ. I REPEAT. YOU RUN WRITE FIRST, THAN READ.
This is because read expects an input to be written to the fifo that it can read, if nothing is written, it won't read 
and it will simply block until some input is written to the fifo.

The file tiny_shell.h contains helper functions I wrote. Making a separate header file just consolidates them into one place.
Please make sure that when you compile the code you keep the header file in the SAME DIRECTORY as the C source file, thanks! :)

PS:
I have also attached the report and two .PNG images showing the outputs of IPC using mkfifo! 
:)