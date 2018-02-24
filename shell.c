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
//
///* Main method and body of the function. */
//int main (int argc, char **argv) {
//    int status, read_command_status, parse_command_status;
//    EXIT = FALSE;
//    pid_t pid;
//    char **commands = NULL;
//    char *cmd = NULL;
//    process p;
//    int num_jobs;
//
//    initializeShell(); //TESTING NOTE: Seems to be working/no set faults
//    buildBuiltIns(); //store all builtins in built in array //TESTING NOTE: Seems to be working/no seg faults
//
//    while (!EXIT) {
//        //TODO: check that this takes the place of printing out the prompt?
//        if ((num_jobs = perform_parse()) < 0) {
//            printError("Error parsing.\n");
//        }
//
//        break;
//    }
//    /*
//
//        job *currentJob = all_jobs;
//        process *currentProcess = NULL;
//
//        while(currentJob != NULL) {
//            launchJob(currentJob, !(currentJob->run_in_background));
//            //get next job
//            currentJob = currentJob->next_job;
//        }
//
//        //executeBuiltInCommand(&p, 0); //testing an exit
//        //printf("EXIT VALUE %d\n", EXIT);
//        //break;
//    }
//
//    /* use case example to get second token */
//    /* NOTE EXAMPLE HARDCODED INTO parse.c b/ memory leaks w/ readline */
////    int rib = all_jobs->run_in_background;
////    printf("%d \n", rib);
//
//    /* free memory for all_jobs -- should be called after every prompt */
//    free_all_jobs();
//
//    return EXIT_SUCCESS;
//}

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
        //executeBuiltInCommand(&p, 0); //testing an exit
        //printf("EXIT VALUE %d\n", EXIT);

//        job *currentJob = all_jobs;
//        process *currentProcess = NULL;
//
//        while (currentJob != NULL) {
//            launchJob(currentJob, !(currentJob->run_in_background));
//            //get next job
//            currentJob = currentJob->next_job;
//        }

        break;
    }

    /* use case example to get second token */
    /* NOTE EXAMPLE HARDCODED INTO parse.c b/ memory leaks w/ readline */
    //char *rib = all_jobs->first_process->args[0];
  //  printf("%s \n", rib);
//    rib = all_jobs->first_process->args[1];
//    printf("%s \n", rib);
//    rib = all_jobs->next_job->first_process->args[0];
//    printf("%s \n", rib);

    /* free memory for all_jobs -- should be called after every prompt */
    //free_all_jobs();

    return 0;
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
        signal(SIGCHLD, SIG_IGN);

        //register a signal handler for SIGCHILD
        /* Handle Signal */
        /*
        struct sigaction childreturn;
        memset(&childreturn, 0, sizeof(childreturn));
        childreturn.sa_sigaction = &childReturning;
        childreturn.sa_flags = SA_SIGINFO;
        if (sigaction(SIGCHLD, &childreturn, NULL) < 0) {
            printError("Error with sigaction for child.\n");
            return;
        }
        */

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
    printf("child handler hit\n");
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
    //give process foreground

    //execute built in (testing...)
    char *c = "hi\0"; //testing
    char **d = &c; //testing

    int success =(*(allBuiltIns[index].function))(d);

    //restore shell (if needed?)

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
                setpgid(pid, j->pgid);

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
    if (pgid == 0) pgid = pid;
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

    printf("hello world\n");

    /* Wait for it to report.  */
    //wait_for_job(j); //TODO: wait for job here (add further code!!)?
    waitpid (WAIT_ANY, &status, WUNTRACED);

    /* Put the shell back in the foreground.  */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Restore the shell’s terminal modes.  */
    tcgetattr(shell_terminal, &j->termios_modes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/* Put a job in the background.  If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.  */

void put_job_in_background (job *j, int cont) {
    /* Send the job a continue signal, if necessary.  */
    if (cont)
        if (kill (-j->pgid, SIGCONT) < 0)
            perror ("kill (SIGCONT)");
}

/* Method to make sure the shell is running interactively as the foreground job before proceeding. Modeled after method found on https://www.gnu.org/software/libc/manual/html_mono/libc.html#Foreground-and-Background. */
void initializeShell();

//TODO: Implement all built in functions to use in the program
/*Method that exits the shell. This command sets the global EXIT variable that breaks out of the while loop in the main function.*/
int exit_builtin(char** args) {
    EXIT = TRUE;
    return EXIT; //success
}

/* Method to take a job id and send a SIGTERM to terminate the process.*/
int kill_builtin(char** args) {
    char *flag = "-9\0";
    if (args[1] == NULL) {
        //invalid arguments
        return FALSE;
    }
        //check that -9 flag was input correctly, otherwise try sending kill with pid
    else if (!strcmp(args[1], flag)) {
        //(error checking gotten from stack overflow)
        const char *nptr = args[2];                     /* string to read as a PID      */
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

        //passed all the if statements and so can assigned the seconds value as the one input
        pid_t pid = number;
        kill(pid, SIGKILL);
        return TRUE;
    } else {
        //PID is second argument
        //(error checking gotten from stack overflow)
        const char *nptr = args[1];                     /* string to read as a PID      */
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

        //passed all the if statements and so can assigned the seconds value as the one input
        pid_t pid = number;
        kill(pid, SIGTERM);
        return TRUE;
    }
    return FALSE;
}

//TODO: figure out why this is segfaulting?
/* Method to iterate through the linked list and print out node parameters. */
int jobs_builtin(char** args) {
    job *currentJob = all_jobs;
    process *currentProcess;

    if (currentJob != NULL) {
        currentProcess = currentJob->first_process;
    }

    int jobID = 1;

    char *status[3] = {running, stopped, done};

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
int background_builtin(char** args) {
    return 0;
}

/* Method that uses tcsetpgrp() to foreground the process -- this is fg*/
int foreground_builtin(char** args) {
    return 0;
}

