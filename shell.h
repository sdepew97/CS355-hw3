//
// Created by Sarah Depew on 2/22/18.
//

#ifndef SHELL_H_
#define SHELL_H_

#include <sys/termios.h>
#include <sys/param.h>
#include "parser.h"

#define BUFFERSIZE 4096
#define EXIT 0
#define NUMBER_OF_BUILT_IN_FUNCTIONS 5
#define RUNNING 1
#define STOPPED 2

int shell; // shell fd
pid_t shell_pgid; // shell process group

//we can add more flags for future command expansion
typedef struct process {
    struct process *next_process;
    pid_t pid;
    char **args;
    int status;
} process;

typedef struct job {i
    struct process *first_process;
    pid_t pgid;
    char *job_string;
    struct termios termios_modes;
    int stdin;
    int stdout;
    int stderr;
    int run_in_background;
    struct job *next_job;
} job;

builtin allBuiltIns[NUMBER_OF_BUILT_IN_FUNCTIONS];

/* print the prompt to the command line */
void printPrompt();

/* make structs for built in commands */
void buildBuiltIns();

/* takes char ** and returns struct array of delimited commands in “command field”, ie *“vim test.py& blah ;” creates array of two command structs. The first has fields *command_string = “vim test.py” tokenized_command = NULL run_in_background = TRUE  and *second command_string = “blah” tokenized_command = “blah” run_in_background = FALSE.  Returns -1 on failure otherwise return 1 on success. */
int readCommandLine(char ***commands);

/* parses through each command and for every command tokenizes the command string */ 
int tokenizeCommands(char ***commands);

/* Frees commands and displays error message */
void handleError(char* message, char ***commands);

/* Method that takes a command pointer and checks if the command is a background or foreground job. This method returns 0 if foreground and 1 if background */
int isBackgroundJob(process* prcs);

/* child process has terminated and so we need to remove the process from the linked list (by pid). We would call this function in the signal handler when getting a SIGCHLD signal. */
void childReturning(int sig, siginfo_t *siginfo, void *context);

/* background running process*/
void suspendProcessInBackground(int sig, siginfo_t *siginfo, void *context);

/* This method is simply the remove node method called when a node needs to be removed from the LL. */
void removeNode(pid_t pidToRemove);

/* Passes in the command to check. Returns the index of the built-in command if it’s in the array of built-in commands and -1 if it is not in the array allBuiltIns */
int isBuiltInCommand(process cmd);

/* Passes in the built-in command to be executed along with the index of the command in the allBuiltIns array. This method returns true upon success and false upon failure/error. */
int executeBuiltInCommand(process cmd, int index);

/* Method to launch our process in either the foreground or the background. */
void launchProcess(process *command, pid_t pgid, int foreground);

/* Method to make sure the shell is running interactively as the foreground job before proceeding. Modeled after method found on https://www.gnu.org/software/libc/manual/html_mono/libc.html#Foreground-and-Background. */
void initializeShell();


#endif //HW3_SHELL_H
