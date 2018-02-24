#include <zconf.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boolean.h"
#include "shell.h"
#include "parser.h"
#include "builtins.h"

job *all_jobs = NULL;

void free_all_jobs();

//strings for built in functions
char *exit_string = "exit\0";
char *kill_string = "kill\0";
char *jobs_string = "jobs\0";
char *fg_string = "fg\0";
char *bg_string = "bg\0";

//strings for jobs printout
char *running = "Running\0";
char *stopped = "Stopped\0";
char *done = "Done\0";

char *builtInTags[NUMBER_OF_BUILT_IN_FUNCTIONS];

struct builtin allBuiltIns[NUMBER_OF_BUILT_IN_FUNCTIONS];

/* Main method and body of the function. */
int main (int argc, char **argv) {
    int status, read_command_status, parse_command_status;
    pid_t pid;
    char **commands = NULL;
    char *cmd = NULL;
    process p;

    // initializeShell();
     buildBuiltIns(); //store all builtins in built in array

    while (!EXIT) {
        printPrompt();
        parse();
        executeBuiltInCommand(&p, 1); //testing an exit
        printf("EXIT VALUE %d\n", EXIT);
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
    char *builtInTags[NUMBER_OF_BUILT_IN_FUNCTIONS] = {exit_string, kill_string, jobs_string, fg_string, bg_string};

    for (int i = 0; i < NUMBER_OF_BUILT_IN_FUNCTIONS; i++) {
        allBuiltIns[i].tag = builtInTags[i];

        if (i == 0) {
            allBuiltIns[i].function = exit_builtin;
        } else if (i == 1) {
            allBuiltIns[i].function = kill_builtin;
        } else if (i == 2) {
            allBuiltIns[i].function = jobs_builtin;
        } else if (i == 3) {
            allBuiltIns[i].function = foreground_builtin;
        } else {
            allBuiltIns[i].function = background_builtin;
        }
    }
}

/* Frees commands and displays error message */
void handleError(char* message, char **commands, int numCommands) {
    printf("%s", message);
    for(int i=0; i<numCommands; i++) {
        printf("%s ", commands[i]); //something like "I am sorry, but the input string is invalid"
    }
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
    for(int i=0; i<NUMBER_OF_BUILT_IN_FUNCTIONS; i++) {
        if(process_equals(cmd, allBuiltIns[i])) {
            return TRUE;
        }
    }
    return FALSE;
}

int process_equals(process process1, builtin builtin1) {
    int compare_value = strcmp(process1.args[0], builtin1.tag);
    if (compare_value == FALSE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* Passes in the built-in command to be executed along with the index of the command in the allBuiltIns array. This method returns true upon success and false upon failure/error. */
int executeBuiltInCommand(process *process1, int index) {
    //give process foreground

    //execute built in (testing...)
    char *c = "hi\0"; //testing
    char **d = &c; //testing

    int success =(*(allBuiltIns[index].function))(d);

    //restore shell (if needed?)

    //return success; //for success
    return TRUE;
}

/* Method to launch our process in either the foreground or the background. */
void launchProcess(process *process1, pid_t pgid, int foreground);

/* Method to make sure the shell is running interactively as the foreground job before proceeding. Modeled after method found on https://www.gnu.org/software/libc/manual/html_mono/libc.html#Foreground-and-Background. */
void initializeShell();

//TODO: Implement all built in functions to use in the program
/*Method that exits the shell. This command sets the global EXIT variable that breaks out of the while loop in the main function.*/
int exit_builtin(char** args) {
    EXIT = TRUE;
    return EXIT; //success
}

/* Method to iterate through the linked list and print out node parameters. */
int jobs_builtin(char** args) {
    job *currentJob = all_jobs;
    process *currentProcess = currentJob->first_process;
    int jobID = 1;

    char *status[3] = {running, stopped, done};

    while(currentJob != NULL) {
        while(currentProcess != NULL) {
            //print out formatted information for processes in job
            printf("[%d]\t %d %s \t\t %s", jobID, currentProcess->pid, status[currentProcess->status], currentProcess->args[0]); //TODO: fix for all arguments!
            jobID++;
            currentProcess = currentProcess->next_process;
        }
        //get next job
        currentJob = currentJob->next_job;
    }
    return 0;
}

/* Method to take a job id and send a SIGTERM to terminate the process.*/
int kill_builtin(char** args) {
    return 0;
}

/* Method that sends continue signal to suspended process in background -- this is bg*/
int background_builtin(char** args) {
    return 0;
}

/* Method that uses tcsetpgrp() to foreground the process -- this is fg*/
int foreground_builtin(char** args) {
    return 0;
}

