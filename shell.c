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

//strings for jobs printout
char *running = "Running\0";
char *stopped = "Stopped\0";
char *done = "Done\0";

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

            //get next job
            currentJob = currentJob->next_job;
        }
        printf("EXIT VALUE %d\n", EXIT);
        //break;
    }

    /* use case example to get second token */
    /* NOTE EXAMPLE HARDCODED INTO parse.c b/ memory leaks w/ readline */
    printoutargs();

    /* free memory for all_jobs -- should be called after every prompt */
    free_all_jobs();

    return 0;
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
   before proceeding. */ //copied and pasted from https://www.gnu.org/software/libc/manual/html_node/Initializing-the-Shell.html
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

/* child process has terminated and so we need to remove the process from the linked list (by pid).
 * We would call this function in the signal handler when getting a SIGCHLD signal. */
void childReturning(int sig, siginfo_t *siginfo, void *context) {
    printf("child handler hit with code");
    printf("%d\n", siginfo->si_code);
    printf("signal number %d\n", siginfo->si_signo);
    //if(siginfo->)
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
            return i; //return index of command
        }
    }
    return NOT_FOUND;
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
    //TODO: give process foreground

    //execute built in (testing...)
    int success =(*(allBuiltIns[index].function))(process1->args);

    //TODO: restore shell (if needed?)

    //return success; //for success
    return TRUE;
}

void launchJob(job *j, int foreground) {
    process *p;
    pid_t pid;

    for (p = j->first_process; p; p = p->next_process) {

        int isBuiltIn = isBuiltInCommand(*p);

        printf("is built in %d\n", isBuiltIn);

        //run as a built-in command
        if (isBuiltIn != NOT_FOUND) {
            executeBuiltInCommand(p, isBuiltIn);
        }
            //run through execvp
        else {
            /* Fork the child processes.  */
            pid = fork();
            if (pid == 0)
                /* This is the child process.  */
                launchProcess(p, j->pgid, j->stdin, j->stdout, j->stderr, foreground);
            else if (pid < 0) {
                /* The fork failed.  */
                perror("fork");
                exit(1);
            } else {
                /* This is the parent process.  */
                p->pid = pid;

                if (!j->pgid)
                    j->pgid = pid;
                setpgid(pid, j->pgid); //TODO: check process group ids being altered correctly

            }
        }
    }

    //TODO: find something to replace this! :)
    if (foreground) {
        put_job_in_foreground(j, 0);
    } else {
        put_job_in_background(j, 0);
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

    if (foreground)
        tcsetpgrp(shell_terminal, pgid);

    /* Set the handling for job control signals back to the default.  */ //TODO: set handling properly to what we want!!
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);

    /* Set the standard input/output channels of the new process.  */
    if (infile != STDIN_FILENO) {
        dup2(infile, STDIN_FILENO);
        close(infile);
    }
    if (outfile != STDOUT_FILENO) {
        dup2(outfile, STDOUT_FILENO);
        close(outfile);
    }
    if (errfile != STDERR_FILENO) {
        dup2(errfile, STDERR_FILENO);
        close(errfile);
    }

    /* Exec the new process.  Make sure we exit.  */
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

    printf("job in foreground\n");

    /* Wait for it to report.  */
    //wait_for_job(j); //TODO: wait for job here (add further code!!)?
    waitpid (WAIT_ANY, &status, WUNTRACED);
    //waitpid (WAIT_ANY, &status, WUNTRACED | WNOHANG);

    /* Put the shell back in the foreground.  */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Restore the shell’s terminal modes.  */
    tcgetattr(shell_terminal, &j->termios_modes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/* Put a job in the background.  If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.  */

void put_job_in_background (job *j, int cont) {
    /* Add job to the background list */
    //first job
    int i=0;
    if(list_of_jobs == NULL) {
        job *first_job = malloc(sizeof(job)); //TODO: make sure list is freed at the end!!
        first_job = j;
        list_of_jobs = first_job;
    } else { //there are already jobs in the list
        job *current_job = list_of_jobs;

        while(current_job!=NULL) {
            current_job = current_job->next_job; //get to the end of the list
        }

        //insert j at the end of the list
        current_job->next_job = j;
    }

    /* Send the job a continue signal, if necessary.  */
    if (cont)
        if (kill (-j->pgid, SIGCONT) < 0)
            perror ("kill (SIGCONT)");
}

int arrayLength(char **array) {
    int i = 0;
    while (array[i] != NULL) {
        i++;
    }

    return i;
}

//TODO: Implement all built in functions to use in the program
/*Method that exits the shell. This command sets the global EXIT variable that breaks out of the while loop in the main function.*/
int exit_builtin(char **args) {
    EXIT = TRUE;
    return EXIT; //success
}

/* Method to take a job id and send a SIGTERM to terminate the process.*/
int kill_builtin(char **args) {
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
            kill(pid, SIGKILL);
            return TRUE;
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
        kill(pid, SIGTERM);
        return TRUE;
    }
    return FALSE;
}

/* Method to iterate through the linked list and print out node parameters. */
int jobs_builtin(char **args) {
    job *currentJob = list_of_jobs;
    process *currentProcess;
    // int numberOfStats = 3;

    if (currentJob != NULL) {
        currentProcess = currentJob->first_process;
    }

    int jobID = 1;

    char *status[] = {running, stopped, done};

    while (currentJob != NULL) {
        while (currentProcess != NULL) {
            //print out formatted information for processes in job
            printf("[%d]\t %d %s \t\t %s\n", jobID, currentProcess->pid, status[currentProcess->status],
                   currentProcess->args[0]); //TODO: fix for all arguments!
            jobID++;
            currentProcess = currentProcess->next_process;
        }
        //get next job
        currentJob = currentJob->next_job;
    }
    return EXIT_FAILURE;
}

/* Method that sends continue signal to suspended process in background -- this is bg*/
int background_builtin(char **args) {
    //get size of args
    int argsLength = arrayLength(args);
    int locationOfPercent = 1;
    int minArgsLength = 1;
    int maxArgsLength = 2;

    if (argsLength < minArgsLength || argsLength > maxArgsLength) {
        printError("I am sorry, but that is an invalid list of commands to bg.\n");
        return FALSE;
    }

    if (argsLength == minArgsLength) {
        //bring back tail of jobs list, if it exists
        job *currentJob = list_of_jobs;
        job *nextJob = NULL;
        if (currentJob == NULL) {
            printError("I am sorry, but that job does not exist.\n");
            return FALSE;
        }

        while (currentJob != NULL) {
            nextJob = currentJob -> next_job;
            if (nextJob == NULL) {
                break; //want to bring back current job
            }
            currentJob = currentJob->next_job;
        }

        put_job_in_background(currentJob, TRUE);
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
        job *currentJob = list_of_jobs;

        while (currentJob != NULL) {
            currentNode ++;
            //found your node
            if(currentNode == number) {
                put_job_in_background(currentJob, TRUE);
                return TRUE;
            }
            else {
                currentJob = currentJob->next_job;
            }
        }

        //node was not found!
        if(currentNode < number) {
            printError("I am sorry, but that job does not exist.\n");
        }
    }

    return FALSE;
}

/* Method that uses tcsetpgrp() to foreground the process -- this is fg*/
int foreground_builtin(char** args) {
    //put_job_in_foreground(job1, TRUE);
    return TRUE;
}

