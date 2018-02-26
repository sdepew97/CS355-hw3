#include "shell.h"
#include "parser.h"
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>

job *all_jobs;
job *list_of_jobs = NULL;
background_job *all_background_jobs = NULL;
pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;

//strings for built in functions
char *exit_string = "exit\0";
char *kill_string = "kill\0";
char *jobs_string = "jobs\0";
char *fg_string = "fg\0";
char *bg_string = "bg\0";

//strings and variables for jobs printout
char *running = "Running\0";
char *suspended = "Suspended\0";

char *builtInTags[NUMBER_OF_BUILT_IN_FUNCTIONS];
struct builtin allBuiltIns[NUMBER_OF_BUILT_IN_FUNCTIONS];

/* Main method and body of the function. */
int main (int argc, char **argv) {
    int status, read_command_status, parse_command_status;
    EXIT = FALSE;
    pid_t pid;
    char **commands = NULL;
    char *cmd = NULL;

    process p;
    initializeShell();
    buildBuiltIns(); //store all builtins in built in array
    while (!EXIT) {

        perform_parse();

        job *currentJob = all_jobs;
        
        while (currentJob != NULL) {  
            launchJob(currentJob, !(currentJob->run_in_background));
            currentJob = currentJob->next_job;
        }

        free_all_jobs(); 
        // break;
        // printf("%d \n", all_background_jobs->termios_modes.c_iflag);
    }

    // free_background_jobs();
    // printoutargs();

    // return 0;
}

void printoutargs() {
    job *temp_job;
    temp_job = all_jobs;
    while (temp_job != NULL) {
        process *temp_proc = temp_job->first_process;
        while (temp_proc != NULL) {
            int i = 0;
            while ((temp_proc->args)[i] != NULL) {
                printf("%s \n", (temp_proc->args)[i]);
                i++;
            }
            temp_proc = temp_proc->next_process;
        }
        temp_job = temp_job->next_job;
    }
}

/* Make sure the shell is running interactively as the foreground job
   before proceeding. */ // modeled after https://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html
void initializeShell() {
    /* See if we are running interactively.  */
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive) {
        /* Loop until we are in the foreground.  */
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        /* Ignore interactive and job-control signals.  */
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        //register a signal handler for SIGCHILD
        /* Handle Signal */
        struct sigaction childreturn;
        memset(&childreturn, 0, sizeof(childreturn));
        childreturn.sa_sigaction = &childReturning;

        sigset_t mask;
        sigemptyset (&mask);
        sigaddset(&mask, SIGCHLD);
        childreturn.sa_mask = mask;
        /* add sig set for sig child and sigtstp */ 
        childreturn.sa_flags = SA_SIGINFO;
        if (sigaction(SIGCHLD, &childreturn, NULL) < 0) {
            printError("Error with sigaction for child.\n");
            return;
        }

        /* Put ourselves in our own process group.  */
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group.\n");
            exit(1);
        }

        /* Grab control of the terminal.  */
        tcsetpgrp(shell_terminal, shell_pgid);

        /* Save default terminal attributes for shell.  */
        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

/* Make array for built in commands. */
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

/* Displays error message */
void printError(char* message) {
    printf("%s", message);
}

//TODO: use this function
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

/* takes pgid to remove and removes corresponding pgid */ 
void trim_background_process_list(pid_t pid_to_remove) {

    background_job *cur_background_job = all_background_jobs;
    background_job *prev_background_job = NULL;

    while (cur_background_job != NULL) {
        if (cur_background_job->pgid == pid_to_remove) { //todo: check this code, since I don't think it works correctly...
            if (prev_background_job == NULL) {
                all_background_jobs = NULL;
                return;
            }
            else {
                prev_background_job->next_background_job = cur_background_job->next_background_job;
            }
        }
        else{
            prev_background_job = cur_background_job;
            cur_background_job = cur_background_job->next_background_job;
        }
    }

}

void job_suspend_helper(pid_t calling_id, int cont, int status) {
    job *cur_job = all_jobs;
    while (cur_job != NULL) {
        if (cur_job->pgid == calling_id) {
            put_job_in_background(cur_job, 0, status);
            return;
        }
        cur_job = cur_job->next_job;
    }
}

/* child process has terminated and so we need to remove the process from the linked list (by pid).
 * We would call this function in the signal handler when getting a SIGCHLD signal. */
void childReturning(int sig, siginfo_t *siginfo, void *context) {
    int signum = siginfo->si_signo;
    pid_t calling_id = siginfo->si_pid;
    printf("handler hit, pid:%d\n", calling_id);

    if (signum == SIGCHLD) { //todo: ask about choices here; I don't think this is going to work...
        if (siginfo->si_status != 0) {
            job_suspend_helper(calling_id, 0, SUSPENDED);
        }
        else {
            trim_background_process_list(calling_id);
        }  
    }
}



/* Passes in the command to check. Returns the index of the built-in command if it’s in the array of
 * built-in commands and -1 if it is not in the array allBuiltIns
 */
int isBuiltInCommand(process cmd) {
    for(int i=0; i<NUMBER_OF_BUILT_IN_FUNCTIONS; i++) {
        if(process_equals(cmd, allBuiltIns[i])) {
            return i; //return index of command
        }
    }
    return NOT_FOUND;
}

int process_equals(process process1, builtin builtin1) {
    /* problem ocurring with white space args -- temporary fix, need to make sure parser doesn't give back whitespace commands */
    if (process1.args[0] == NULL) { return FALSE; }
    int compare_value = strcmp(process1.args[0], builtin1.tag);
    if (compare_value == FALSE) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* Passes in the built-in command to be executed along with the index of the command in the allBuiltIns array. This method returns true upon success and false upon failure/error. */
int executeBuiltInCommand(process *process1, int index) {

    int success =(*(allBuiltIns[index].function))(process1->args);

    return TRUE;
}

void launchJob(job *j, int foreground) {
    process *p;
    pid_t pid;
    for (p = j->first_process; p; p = p->next_process) {
        int isBuiltIn = isBuiltInCommand(*p);

        //run as a built-in command
        if (isBuiltIn != NOT_FOUND) {
            executeBuiltInCommand(p, isBuiltIn);
            foreground = TRUE;
        }
        else {
            pid = fork();

            if (pid == 0) {
                launchProcess(p, j->pgid, j->stdin, j->stdout, j->stderr, foreground);
            } else if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else {
                p->pid = pid;

                if (!j->pgid) {
                    j->pgid = pid;
                }

                setpgid(pid, j->pgid); //TODO: check process group ids being altered correctly
            }
        }
    }

    if (foreground) {
        put_job_in_foreground(j, 0);
    } else {
        sigset_t mask;
        sigemptyset (&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        put_job_in_background(j, 0, RUNNING);

        sigprocmask(SIG_UNBLOCK, &mask, NULL);
    }
}

/* Method to launch our process in either the foreground or the background. */
//method  based off of https://www.gnu.org/software/libc/manual/html_node/Launching-Jobs.html#Launching-Jobs
void launchProcess (process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground) {
    pid_t pid;

    /* Put the process into the process group and give the process group
       the terminal, if appropriate.
       This has to be done both by the shell and in the individual
       child processes because of potential race conditions.  */ //TODO: consider race conditions arising here!!
    pid = getpid(); 

    if (pgid == 0) {
        pgid = pid;
    }

    setpgid(pid, pgid);

    printf("pid: %d\n", pid);

    if (foreground) {
        if (tcsetpgrp(shell_terminal, pgid) < 0) {
            perror("tcsetpgrp");
            exit(EXIT_FAILURE);
        }
    }

    /* Set the handling for job control signals back to the default.  */
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);

    /* Exec the new process.  Make sure we exit.  */
    if (p->args[0] != NULL)
        execvp(p->args[0], p->args);
    perror("execvp");
    exit(TRUE);
}

/* Put job j in the foreground.  If cont is nonzero,
   restore the saved terminal modes and send the process group a
   SIGCONT signal to wake it up before we block.  */

void put_job_in_foreground (job *j, int cont) {
    int status;
    /* Put the job into the foreground.  */
    tcsetpgrp(shell_terminal, j->pgid);

    /* Send the job a continue signal, if necessary.  */
    if (cont) {
        tcsetattr(shell_terminal, TCSADRAIN, &j->termios_modes);
        if (kill(-j->pgid, SIGCONT) < 0)
            perror("kill (SIGCONT)");
    }

    /* Wait for it to report.  */
    //wait_for_job(j); //TODO: wait for job here (add further code!!)?
    waitpid (j->pgid, &status, WUNTRACED);

    /* Put the shell back in the foreground.  */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Restore the shell’s terminal modes.  */
    tcgetattr(shell_terminal, &j->termios_modes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}


int lengthOf(char *str){
    int i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
}

/* takes background job and gives it to background job */
void simple_background_job_setup(background_job *dest, job *org, int status) 
{
    dest->pgid = org->pgid;
    dest->status = status;
    dest->termios_modes = org->termios_modes; // <<< potential source of error here? valgrind and fg seems to be complaing about unitialized bytes
    char *js = malloc(sizeof(char) * lengthOf(org->job_string) + 1);
    strcpy(js, org->job_string);
    dest->job_string = js;
    dest->next_background_job = NULL;
}

/* Put a job in the background.  If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.  */
void put_job_in_background(job *j, int cont, int status) {

    /* Add job to the background list with status of running */
    background_job *copyOfJ = malloc(sizeof(background_job));
    
    if (!cont) {
        simple_background_job_setup(copyOfJ, j, status);
        if (all_background_jobs == NULL) {
            all_background_jobs = copyOfJ;
        } else {
            background_job *cur_job = all_background_jobs;
            background_job *next_job = all_background_jobs->next_background_job;
            while (next_job != NULL) {
                background_job *temp = next_job;
                next_job = cur_job->next_background_job;    
                cur_job = temp;
            }
            cur_job->next_background_job = copyOfJ;
        }
    }
    else {
        printf("here \n");
        if (kill(-j->pgid, SIGCONT) < 0) {
            perror("kill (SIGCONT)");
        }
    }

}

int arrayLength(char **array) {
    int i = 0;
    while (array[i] != NULL) {
        i++;
    }
    return i;
}


/* Let's have this clean up the job list */
int exit_builtin(char **args) {
    free_background_jobs();
    EXIT = TRUE;
    return EXIT; //success
}

///* Method to take a job id and send a SIGTERM to terminate the process.*/
//int kill_builtin(char **args) {
//    char *flag = "-9\0";
//    int flagLocation = 1;
//    int pidLocationNoFlag = 1;
//    int pidLocation = 2;
//    int minElements = 2;
//    int maxElements = 3;
//
//    //get args length
//    int argsLength = arrayLength(args);
//
//    if (argsLength < minElements || argsLength > maxElements) {
//        //invalid arguments
//        return FALSE;
//    } else if (argsLength == maxElements) {
//        if (!strcmp(args[flagLocation],
//                    flag)) { //check that -9 flag was input correctly, otherwise try sending kill with pid
//            //(error checking gotten from stack overflow)
//            const char *nptr = args[pidLocation];                     /* string to read as a PID      */
//            char *endptr = NULL;                            /* pointer to additional chars  */
//            int base = 10;                                  /* numeric base (default 10)    */
//            long long int number = 0;                       /* variable holding return      */
//
//            /* reset errno to 0 before call */
//            errno = 0;
//
//            /* call to strtol assigning return to number */
//            number = strtoll(nptr, &endptr, base);
//
//            /* test return to number and errno values */
//            if (nptr == endptr) {
//                printf(" number : %lld  invalid  (no digits found, 0 returned)\n", number);
//                return FALSE;
//            } else if (errno == ERANGE && number == LONG_MIN) {
//                printf(" number : %lld  invalid  (underflow occurred)\n", number);
//                return FALSE;
//            } else if (errno == ERANGE && number == LONG_MAX) {
//                printf(" number : %lld  invalid  (overflow occurred)\n", number);
//                return FALSE;
//            } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
//                printf(" number : %lld  invalid  (base contains unsupported value)\n", number);
//                return FALSE;
//            } else if (errno != 0 && number == 0) {
//                printf(" number : %lld  invalid  (unspecified error occurred)\n", number);
//                return FALSE;
//            } else if (errno == 0 && nptr && *endptr != 0) {
//                printf(" number : %lld    invalid  (since additional characters remain)\n", number);
//                return FALSE;
//            }
//
//            pid_t pid = number;
//            kill(pid, SIGKILL);
//            return TRUE;
//        }
//    } else { //we have no flags and only kill with a pid
//        //PID is second argument
//        //(error checking gotten from stack overflow)
//        const char *nptr = args[pidLocationNoFlag];                     /* string to read as a PID      */
//        char *endptr = NULL;                            /* pointer to additional chars  */
//        int base = 10;                                  /* numeric base (default 10)    */
//        long long int number = 0;                       /* variable holding return      */
//
//        /* reset errno to 0 before call */
//        errno = 0;
//
//        /* call to strtol assigning return to number */
//        number = strtoll(nptr, &endptr, base);
//
//        /* test return to number and errno values */
//        if (nptr == endptr) {
//            printf(" number : %lld  invalid  (no digits found, 0 returned)\n", number);
//            return FALSE;
//        } else if (errno == ERANGE && number == LONG_MIN) {
//            printf(" number : %lld  invalid  (underflow occurred)\n", number);
//            return FALSE;
//        } else if (errno == ERANGE && number == LONG_MAX) {
//            printf(" number : %lld  invalid  (overflow occurred)\n", number);
//            return FALSE;
//        } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
//            printf(" number : %lld  invalid  (base contains unsupported value)\n", number);
//            return FALSE;
//        } else if (errno != 0 && number == 0) {
//            printf(" number : %lld  invalid  (unspecified error occurred)\n", number);
//            return FALSE;
//        } else if (errno == 0 && nptr && *endptr != 0) {
//            printf(" number : %lld    invalid  (since additional characters remain)\n", number);
//            return FALSE;
//        }
//
//        pid_t pid = number;
//        kill(pid, SIGTERM);
//        return TRUE;
//    }
//    return FALSE;
//}

/* Get kill working... */
/* Method to take a job id and send a SIGTERM to terminate the process.*/
int kill_builtin(char **args) {
    printf("hit kill\n");
    char *flag = "-9\0";
    int flagLocation = 1;
    int pidLocationNoFlag = 1;
    int pidLocation = 2;
    int minElements = 2;
    int maxElements = 3;

    //get args length
    int argsLength = arrayLength(args);

    if (argsLength < minElements || argsLength > maxElements) {
        //invalid arguments
        printError("I am sorry, but you have passed an invalid number of arguments to kill.\n");
        return FALSE;
    } else if (argsLength == maxElements) {
        if (!strcmp(args[flagLocation],
                    flag)) { //check that -9 flag was input correctly, otherwise try sending kill with pid
            //(error checking gotten from stack overflow)
            const char *nptr = args[pidLocation];                     /* string to read as a PID      */
            char *endptr = NULL;                            /* pointer to additional chars  */
            int base = 10;                                  /* numeric base (default 10)    */
            long long int number = 0;                       /* variable holding return      */

            /* reset errno to 0 before call */
            errno = 0;

            /* call to strtol assigning return to number */
            number = strtoll(nptr, &endptr, base);

            /* test return to number and errno values */
            if (nptr == endptr) {
                printf(" number : %lld  invalid  (no digits found, 0 returned)\n", number);
                return FALSE;
            } else if (errno == ERANGE && number == LONG_MIN) {
                printf(" number : %lld  invalid  (underflow occurred)\n", number);
                return FALSE;
            } else if (errno == ERANGE && number == LONG_MAX) {
                printf(" number : %lld  invalid  (overflow occurred)\n", number);
                return FALSE;
            } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
                printf(" number : %lld  invalid  (base contains unsupported value)\n", number);
                return FALSE;
            } else if (errno != 0 && number == 0) {
                printf(" number : %lld  invalid  (unspecified error occurred)\n", number);
                return FALSE;
            } else if (errno == 0 && nptr && *endptr != 0) {
                printf(" number : %lld    invalid  (since additional characters remain)\n", number);
                return FALSE;
            }

            pid_t pid = number;
            printf("%d pid\n", pid);
            if (kill(pid, SIGKILL) == -1) {
                printError("I am sorry, an error occurred with kill.\n");
                return FALSE; //error occurred
            } else {
                return TRUE;
            }
        } else {
            printError("I am sorry, an error occurred with kill.\n");
            return FALSE; //error occurred
        }
    } else { //we have no flags and only kill with a pid
        //PID is second argument
        //(error checking gotten from stack overflow)
        const char *nptr = args[pidLocationNoFlag];                     /* string to read as a PID      */
        char *endptr = NULL;                            /* pointer to additional chars  */
        int base = 10;                                  /* numeric base (default 10)    */
        long long int number = 0;                       /* variable holding return      */

        /* reset errno to 0 before call */
        errno = 0;

        /* call to strtol assigning return to number */
        number = strtoll(nptr, &endptr, base);

        /* test return to number and errno values */
        if (nptr == endptr) {
            printf(" number : %lld  invalid  (no digits found, 0 returned)\n", number);
            return FALSE;
        } else if (errno == ERANGE && number == LONG_MIN) {
            printf(" number : %lld  invalid  (underflow occurred)\n", number);
            return FALSE;
        } else if (errno == ERANGE && number == LONG_MAX) {
            printf(" number : %lld  invalid  (overflow occurred)\n", number);
            return FALSE;
        } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
            printf(" number : %lld  invalid  (base contains unsupported value)\n", number);
            return FALSE;
        } else if (errno != 0 && number == 0) {
            printf(" number : %lld  invalid  (unspecified error occurred)\n", number);
            return FALSE;
        } else if (errno == 0 && nptr && *endptr != 0) {
            printf(" number : %lld    invalid  (since additional characters remain)\n", number);
            return FALSE;
        }

        pid_t pid = number;
        printf("%d pid\n", pid);
        if (kill(pid, SIGKILL) == -1) {
            printError("I am sorry, an error occurred with kill.\n");
            return FALSE; //error occurred
        } else {
            return TRUE;
        }
    }
    return FALSE;
}

/* Method to iterate through the linked list and print out node parameters. */
int jobs_builtin(char **args) {
    background_job *currentJob = all_background_jobs;
    char *status[] = {running, suspended};
    int jobID = 1;

    if (currentJob == NULL) {

    } 
    else {
        while (currentJob != NULL) {
            //print out formatted information for processes in job
            printf("[%d]\t %d %s \t\t %s\n", jobID, currentJob->pgid, status[currentJob->status], currentJob->job_string);
            jobID++;

            //get next job
            currentJob = currentJob->next_background_job;
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

void background_built_in_helper(background_job *bj, int cont, int status) {
    if (kill(-bj->pgid, SIGCONT) < 0) {
        perror("kill (SIGCONT)");
    }

    background_job *current_job = all_background_jobs;
    while (current_job != NULL) {
        if (current_job->pgid == bj->pgid) {
            current_job->status = RUNNING;
        }
        current_job = current_job->next_background_job;
    }

}

/* Method that sends continue signal to suspended process in background -- this is bg*/
int background_builtin(char **args) {
    //get size of args
    int argsLength = arrayLength(args);
    int locationOfPercent = 1;
    int minArgsLength = 1;
    int maxArgsLength = 2;
    printf("ya?\n ");

    if (argsLength < minArgsLength || argsLength > maxArgsLength) {
        printError("I am sorry, but that is an invalid list of commands to bg.\n");
        return FALSE;
    }

    if (argsLength == minArgsLength) {
        //bring back tail of jobs list, if it exists
        background_job *currentJob = all_background_jobs;
        background_job *nextJob = NULL;
        if (currentJob == NULL) {
            printError("I am sorry, but that job does not exist.\n");
            return FALSE;
        }

        while (currentJob != NULL) {
            nextJob = currentJob->next_background_job;
            if (nextJob == NULL) {
                break; //want to bring back current job
            }
            currentJob = currentJob->next_background_job;
        }
        printf("here ?\n");
        

        sigset_t mask;
        sigemptyset (&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        background_built_in_helper(currentJob, TRUE, RUNNING);

        sigprocmask(SIG_UNBLOCK, &mask, NULL);

        printf("out ? \n");
        return TRUE; //success!
    } else if (argsLength == maxArgsLength) {
        //(error checking gotten from stack overflow)
        const char *nptr = args[locationOfPercent];     /* string to read as a number   */
        char *endptr = NULL;                            /* pointer to additional chars  */
        int base = 10;                                  /* numeric base (default 10)    */
        long long int number = 0;                       /* variable holding return      */

        /* reset errno to 0 before call */
        errno = 0;

        /* call to strtol assigning return to number */
        number = strtoll(nptr, &endptr, base);

        /* test return to number and errno values */
        if (nptr == endptr) {
            printf(" number : %lld  invalid  (no digits found, 0 returned)\n", number);
            return FALSE;
        } else if (errno == ERANGE && number == LONG_MIN) {
            printf(" number : %lld  invalid  (underflow occurred)\n", number);
            return FALSE;
        } else if (errno == ERANGE && number == LONG_MAX) {
            printf(" number : %lld  invalid  (overflow occurred)\n", number);
            return FALSE;
        } else if (errno == EINVAL) { /* not in all c99 implementations - gcc OK */
            printf(" number : %lld  invalid  (base contains unsupported value)\n", number);
            return FALSE;
        } else if (errno != 0 && number == 0) {
            printf(" number : %lld  invalid  (unspecified error occurred)\n", number);
            return FALSE;
        } else if (errno == 0 && nptr && *endptr != 0) {
            printf(" number : %lld    invalid  (since additional characters remain)\n", number);
            return FALSE;
        }

        //have location now in linked list
        int currentNode = 0;
        background_job *currentJob = all_background_jobs;

        while (currentJob != NULL) {
            currentNode ++;
            //found your node
            if(currentNode == number) {
                /* sig proc mask this */ 
                sigset_t mask;
                sigemptyset (&mask);
                sigaddset(&mask, SIGCHLD);
                sigprocmask(SIG_BLOCK, &mask, NULL);

                background_built_in_helper(currentJob, TRUE, RUNNING);

                sigprocmask(SIG_UNBLOCK, &mask, NULL);
                return TRUE;
            }
            else {
                currentJob = currentJob->next_background_job;
            }
        }

        //node was not found!
        if(currentNode < number) {
            printError("I am sorry, but that job does not exist.\n");
        }
    }

    return FALSE;
}

background_job *get_background_from_pgid(pid_t pgid) {
    background_job *current_job = all_background_jobs;
    while (current_job != NULL) {
        if (current_job->pgid == pgid) {
            return current_job;
        }
        current_job = current_job->next_background_job;
    }
    return NULL;
}

void foreground_helper(background_job *bj) {
    int status, err;
    /* Put the job into the foreground.  */
    tcsetpgrp(shell_terminal, bj->pgid);

    /* Send the job a continue signal, if necessary.  */
    tcsetattr(shell_terminal, TCSADRAIN, &bj->termios_modes);
    if (kill(-bj->pgid, SIGCONT) < 0)
        perror("kill (SIGCONT)");

    /* Wait for it to report.  */
    //wait_for_job(j); //TODO: wait for job here (add further code!!)?
    /* taking advice from https://cboard.cprogramming.com/c-programming/113860-recvfrom-getting-interrupted-system-call.html on interrupt system call */
    /*
     * Getting weird error here from valgrind
     * 
     * Syscall param ioctl(TCSET{S,SW,SF}) points to uninitialised byte(s)
     * 
     * Perhaps we're not allocating this struct correctly when we assign it to a background job
     * 
     */
    while ((err = waitpid(bj->pgid , &status, WUNTRACED)) && errno == EINTR) 
        ; 
    if (err < 0) {
        perror("Waitpid");
    }

    /* Put the shell back in the foreground.  */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Restore the shell’s terminal modes.  */
    tcgetattr(shell_terminal, &bj->termios_modes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/* Method that uses tcsetpgrp() to foreground the process -- this is fg*/
int foreground_builtin(char** args) {
    
    /*
     * Basic functionality included (ie foreground last pid)
     * to test to make sure this is working
     */

    /* get last process pid_t */
    pid_t pgid = all_background_jobs->pgid;

    background_job *bj = get_background_from_pgid(pgid);

    sigset_t mask;
    sigemptyset (&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    trim_background_process_list(pgid);

    sigprocmask(SIG_UNBLOCK, &mask, NULL);


    foreground_helper(bj);


    return TRUE;
}

