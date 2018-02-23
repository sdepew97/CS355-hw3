#include <zconf.h>
#include <sys/wait.h>
#include <stdio.h>
// #include "llist.h"
// #include "llist_node.h"
#include "shell.h"
// #include "parser.h" 

void initializeShell() {}

/* Main method and body of the function. */
int main (int argc, char **argv) {
    int status, read_command_status, parse_command_status;
    pid_t pid;
    char **commands = NULL;
    char *cmd = NULL;
    /*LL holds child process id*/
    // llist backgroundProcess;
    initializeShell();
    buildBuiltIns(); //store all builtins in built in array

    // while (!EXIT) {
    //     job *first_job = NULL;

    // }

    job first_job;
    // first_job.first_process = fp;
    // int i = parse(&first_job);

    // printf("%s \n", first_job.first_process -> args[0]);
    return 0;  
}
