#include <zconf.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include "boolean.h"
// #include "llist.h"
// #include "llist_node.h"
#include "shell.h"
#include "parser.h"
#include "builtins.h"

job *all_jobs = NULL;

void free_all_jobs();

//strings for built in functions
char *ext = "exit\0";
char *kill = "kill\0";
char *jobs = "jobs\0";
char *fg = "fg\0";
char *bg = "bg\0";
char **builtInTags[NUMBER_OF_BUILT_IN_FUNCTIONS] = {ext, kill, jobs, fg, bg};
operation *operations[NUMBER_OF_BUILT_IN_FUNCTIONS] = {ext, kill, jobs, foreground, background};

struct builtin allBuiltIns[NUMBER_OF_BUILT_IN_FUNCTIONS];

/* Main method and body of the function. */
int main (int argc, char **argv) {
    int status, read_command_status, parse_command_status;
    pid_t pid;
    char **commands = NULL;
    char *cmd = NULL;

    // initializeShell();
    // buildBuiltIns(); //store all builtins in built in array

    while (!EXIT) {
        parse();
        break;
    }

    /* use case example to get second token */
    /* NOTE EXAMPLE HARDCODED INTO parse.c b/ memory leaks w/ readline */
    int rib = all_jobs->run_in_background;
    printf("%d \n", rib);

    /* free memory for all_jobs -- should be called after every prompt */
    free_all_jobs();

    return 0;
}

/* print the prompt to the command line */
void printPrompt() {
    printf(">>");
}

/* make structs for built in commands */
void buildBuiltIns() {
    for(int i=0; i<NUMBER_OF_BUILT_IN_FUNCTIONS; i++) {
        allBuiltIns[i].tag = builtInTags[i];

    }
}

int readCommandLine(char **commands) {
    /* use case example to get second token */
    /* NOTE EXAMPLE HARDCODED INTO parse.c b/ memory leaks w/ readline */
    int number_jobs = parse();
    char **targs = all_jobs->first_process->args;
    int c = 0;
    while (targs[c] != NULL) {
        printf("%s\n", targs[c]);
        c++;
    }

    return number_jobs;
}


/* Frees commands and displays error message */
void handleError(char* message, char **commands) {
    printf("%s, %s", message, commands); //something like "I am sorry, but the input string is invalid"
}

/* Method that takes a command pointer and checks if the command is a background or foreground job.
 * This method returns 0 if foreground and 1 if background */
int isBackgroundJob(job* job1) {
    return job1->run_in_background == TRUE;
}

/* child process has terminated and so we need to remove the process from the linked list (by pid).
 * We would call this function in the signal handler when getting a SIGCHLD signal. */
void childReturning(int sig, siginfo_t *siginfo, void *context) {

}

/* background running process*/
void suspendProcessInBackground(int sig, siginfo_t *siginfo, void *context) {

}

/* This method is simply the remove node method called when a node needs to be removed from the list of jobs. */
void removeNode(pid_t pidToRemove) {
    //look through jobs for pid of child to remove
    job *currentJob = all_jobs;

    process *currentProcess = NULL;
    process *nextProcess = NULL;

    while (currentJob != NULL) {
        //the pid of the first job process matches, then update job pointer!
        currentProcess = currentJob->first_process;
        if (currentProcess != NULL && currentProcess->pid == pidToRemove) {
            currentJob->first_process = currentProcess->next_process;
            return;
        } else { //look at all processes w/in job for pid not the first one
            while (currentProcess != NULL) {
                nextProcess = currentProcess->next_process;
                if (nextProcess->pid == pidToRemove) {
                    //found the pidToRemove and an reset pointers
                    currentProcess->next_process = nextProcess->next_process;
                    return;
                }
                currentProcess = nextProcess;
            }
        }

        //pid not found in list of current processes for prior viewed job, get next job
        currentJob = currentJob->next_job;
    }
}

/* Passes in the command to check. Returns the index of the built-in command if it’s in the array of
 * built-in commands and -1 if it is not in the array allBuiltIns
 */
int isBuiltInCommand(process cmd) {

}