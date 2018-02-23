//
// Created by Sarah Depew on 2/22/18.
//

#ifndef HW3_SHELL_H
#define HW3_SHELL_H

#include <sys/termios.h>
#include <sys/param.h>

#ifndef _REENTRANT
#define _REENTRANT

#define BUFFERSIZE 4096
#define EXIT 0
#define NUMBER_OF_BUILT_IN_FUNCTIONS 5
#define RUNNING 1
#define STOPPED 2

int shell; // shell fd
pid_t shell_pgid; // shell process group

typedef int (*Operation)(char**);

//Structs
typedef struct builtin
{
    char* tag;
    Operation function;
} builtin;

//we can add more flags for future command expansion
typedef struct process {
    struct process *next_process;
    pid_t pid;
    char **args;
    int status;
} process;

typedef struct job {
    struct process *first_process;
    pid_t pgid;
    struct termios termios_modes;
    int stdin;
    int stdout;
    int stderr;
    struct job *next_job;
} job;

builtin allBuiltIns[NUMBER_OF_BUILT_IN_FUNCTIONS];

/* print the prompt to the command line */
void printPrompt();
    //choose a prompt to print with printf()

/* make structs for built in commands */
void buildBuiltIns();
//initialize all commands here

/* takes char ** and returns struct array of delimited commands in “command field”, ie *“vim test.py& blah ;” creates array of two command structs. The first has fields *command_string = “vim test.py” tokenized_command = NULL run_in_background = TRUE  and *second command_string = “blah” tokenized_command = “blah” run_in_background = FALSE.  Returns -1 on failure otherwise return 1 on success. */
int readCommandLine(char ***commands);
    // this is going to require some work with malloc


/* parses through each command and for every command tokenizes the command string
* i.e. takes “vim test.py” in command_string field and creates [“vim”, “test.py”] in tokenized_comand field. We will use separation by whitespace to separate commands. Returns -1 on failure and returns 1 on success. */
int tokenizeCommands(char ***commands);

/* Frees commands and displays error message */
void handleError(char* message, char ***commands);

/* Method that takes a command pointer and checks if the command is a background or foreground job. This method returns 0 if foreground and 1 if background */
int isBackgroundJob(process* prcs);

/* child process has terminated and so we need to remove the process from the linked list (by pid). We would call this function in the signal handler when getting a SIGCHLD signal. */
void childReturning(int sig, siginfo_t *siginfo, void *context);
// sigproc mask to block SIGTSTP
// delete LL node

/* background running process*/
void suspendProcessInBackground(int sig, siginfo_t *siginfo, void *context);
// raise sigstop
// sigprocmask to block SIGCHILD
// update LL
// give foreground back to terminal

/* This method is simply the remove node method called when a node needs to be removed from the LL. */
void removeNode(pid_t pidToRemove);
    //iterate through LL and remove the node with the pid of the argument passed in

/* Passes in the command to check. Returns the index of the built-in command if it’s in the array of built-in commands and -1 if it is not in the array allBuiltIns */
int isBuiltInCommand(process cmd);

/* Passes in the built-in command to be executed along with the index of the command in the allBuiltIns array. This method returns true upon success and false upon failure/error. */
int executeBuiltInCommand(process cmd, int index);

/* Method to launch our process in either the foreground or the background. */
void launchProcess(process *command, pid_t pgid, int foreground);
    // put process into its own process group, which is the pid of the process
    //procmask SIGCHLD and then add node to LL
    // if foreground: tcsteprgr(shell, pgid)
    // Reset the defaults for signal handling
    // Register SIGTSTP handler
/* Execvp the new process.  Make sure we exit.  */

/* Method to make sure the shell is running interactively as the foreground job before proceeding. Modeled after method found on https://www.gnu.org/software/libc/manual/html_mono/libc.html#Foreground-and-Background. */
void initializeShell();
// set globals, shell process group (shell_pgid), shell file descriptor
// ignore SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU

    /* handle signal */
    /*
    struct sigaction childreturn;
    memset(&childreturn, 0, sizeof(childreturn));
    childreturn.sa_sigaction = &childReturning;
    childreturn .sa_flags = SA_SIGINFO;
    if (sigaction(SIGCHLD, &childreturn, NULL) < 0) { //handles when the child
        //returns for asynchronous approach to forking
        //handle error
        //return 1;
    }
    tcsetpgrp(shell, shell_pgid);
    */

#endif //HW3_SHELL_H
#endif
