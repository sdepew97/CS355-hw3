//
// Created by Sarah Depew on 2/23/18.
//

#ifndef HW3_BUILTINS_H
#define HW3_BUILTINS_H

#ifndef _REENTRANT
#define _REENTRANT

#include "shell.h"
#include "parser.h"

#define NUMBER_OF_BUILT_IN_FUNCTIONS 5

typedef int (*operation)(char**);

//Structs
typedef struct builtin {
    char *tag;
    operation function;
} builtin;

/* iterates through LL and returns true if there are >= nodes as pnum */
int processExists(int pnum);

/*Method that exits the shell. This command sets the global EXIT variable that breaks out of the while loop in the main function.*/
int ext(char** args);

/* Method to iterate through the linked list and print out node parameters. */
int jobs(char** args);

/* Method to take a job id and send a SIGTERM to terminate the process.*/
int kill(char** args);

/* Method that sends continue signal to suspended process in background -- this is bg*/
int background(char** args);

/* Method that uses tcsetpgrp() to foreground the process -- this is fg*/
int foreground(char** args);

#endif //HW3_BUILTINS_H
#endif