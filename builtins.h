//
// Created by Sarah Depew on 2/23/18.
//

#ifndef HW3_BUILTINS_H
#define HW3_BUILTINS_H

#ifndef _REENTRANT
#define _REENTRANT

//strings for built in functions
char *exit = "exit\0";
char *kill = "kill\0";
char *jobs = "jobs\0";
char *fg = "fg\0";
char *bg = "bg\0";

typedef int (*operation)(char**);
char **builtInTags = {exit, kill, jobs, fg, bg};
operation *operations = {exit, kill, jobs, foreground, background};

//Structs
typedef struct builtin
{
    char* tag;
    operation function;
} builtin;

/* iterates through LL and returns true if there are >= nodes as pnum */
int processExists(int pnum);

/*Method that exits the shell. This command sets the global EXIT variable that breaks out of the while loop in the main function.*/
int exit(char** args);

    //set exit to break out of while loop
//return success or failure


/* Method to iterate through the linked list and print out node parameters. */
int jobs(char** args);

    //print out the linked list nodes and their parameters line by line (print the data
    //information)
//return success or failure


/* Method to take a job id and send a SIGTERM to terminate the process.*/
int kill(char** args)
{
    //Iterate through args and find one that begins with %
    //get value after %, if it is an integer and that process number exists :
//if there is also a -9 flag in the arguments and if there is a -9 flag, send a SIGKILL to the process
    else //send a SIGTERM
    //if successful, remove node from LL, blocking for SIGTSTP and SIGCHLD and unblock
    //return success or failure
}

/* Method that sends continue signal to suspended process in background -- this is bg*/
int background(char** args)
{
    //iterate through args and find one that begins with %
    //get value of %, if it is an integer and if that process number exists
    //send continue signal to process if it is stopped
    //if this works
    //block SIGTSTP and SIGCHLD
    //change status to running
    //unblock
//return success or failure
}

/* Method that uses tcsetpgrp() to foreground the process -- this is fg*/
int foreground(char** args)
{
    //use tcsetpgrp() to put the process in the foreground
    //move process out of ll (make sure to block while doing this using sigproc mask)
    // send sigcont signal
    // waitpid (same format as in main loop)
    // tcsetpgrp() to put shell in foreground
//return success or failure
}


#endif //HW3_BUILTINS_H
#endif