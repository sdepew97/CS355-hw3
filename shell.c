#include <zconf.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
// #include "llist.h"
// #include "llist_node.h"
#include "shell.h"
#include "parser.h" 

tokenizer *t;
tokenizer *pt;
job *all_jobs;

void free_all_jobs();

/* Main method and body of the function. */
int main (int argc, char **argv) {
    int status, read_command_status, parse_command_status;
    pid_t pid;
    char **commands = NULL;
    char *cmd = NULL;
    /*LL holds child process id*/
    // llist backgroundProcess;
    int blah = 0 ;
    // initializeShell();
    // buildBuiltIns(); //store all builtins in built in array

    // while (!EXIT) {
    //     job *first_job = NULL;

    // }
    // first_job.first_process = fp;
    int number_jobs = parse();
    char **targs = all_jobs->first_process->args;
    int c = 0;
    while (targs[c] != NULL) {
        printf("%s\n", targs[c]);
        c++;
    }

    free_all_jobs();

    // printf("%s \n", first_job.first_process -> args[0]);
    return 0;  
}

/* free memory for jobs */
void free_all_jobs() {
    while (all_jobs != NULL) {
        free(all_jobs->job_string);
        process *temp_p = all_jobs->first_process;
        while (temp_p != NULL) {
            free(temp_p->args);
            process *t = temp_p->next_process;
            free(temp_p);
            temp_p = t;
        }
        job *j = all_jobs->next_job;
        free (all_jobs);
        all_jobs = j;

    } 
}