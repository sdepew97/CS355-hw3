#include <zconf.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
// #include "llist.h"
// #include "llist_node.h"
#include "shell.h"
#include "parser.h"
#include "builtins.h"

job *all_jobs = NULL;

void free_all_jobs();

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

    }
}