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

trydelete trydeleteValue = {-1, NULL};
trydelete *trydelete1 = &trydeleteValue;

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
            if (!(currentJob->pass)) {
                launchJob(currentJob, !(currentJob->run_in_background));
            }
            currentJob = currentJob->next_job;
        }
        try_delete_method();
        //TODO: fix seg fault that happens here while trying to free memory not allocated try_delete_free();
        free_all_jobs();
    }

    /* free any background jobs still in LL before exiting */
    free_background_jobs();
    return EXIT_SUCCESS;
}

void try_delete_method() {
    trydelete *value = trydelete1;

    while (value != NULL) {
        pid_t calling_id = value->pidToDelete;
        job_suspend_helper(calling_id);
        value = value->next;
    }
}

void try_delete_free() {
    trydelete *value = trydelete1;
    trydelete *next = NULL;
    while(value->pidToDelete != -1) {
//        printf("hit here, %d\n", value->pidToDelete);
        next = value->next;
        free(value);
        value = next;
    }
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
        signal(SIGTERM, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

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
        childreturn.sa_flags = SA_SIGINFO | SA_RESTART;
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
    printf("%s\n", message);
}

/* Method that takes a command pointer and checks if the command is a background or foreground job.
 * This method returns 0 if foreground and 1 if background */
int isBackgroundJob(job* job1) {
    return job1->run_in_background == TRUE;
}

/* takes pgid to remove and removes corresponding pgid, if it exists */
void trim_background_process_list(pid_t pid_to_remove) {
    background_job *cur_background_job = all_background_jobs;
    background_job *prev_background_job = NULL;
    while (cur_background_job != NULL) {
        if (cur_background_job->pgid == pid_to_remove) {
            if (prev_background_job == NULL) {
                background_job *temp = cur_background_job->next_background_job;
                all_background_jobs = temp;
                free(cur_background_job->job_string);
                free(cur_background_job);
                return;
            }
            else {
                prev_background_job->next_background_job = cur_background_job->next_background_job;
                free(cur_background_job->job_string);
                free(cur_background_job);
                return;
            }
        }
        else{
            prev_background_job = cur_background_job;
            cur_background_job = cur_background_job->next_background_job;
        }
    }
}

job *package_job(background_job *cur_job) {
    job *to_return = malloc(sizeof(job));

    to_return->pgid = cur_job->pgid;
    to_return->status = cur_job->status;
    to_return->full_job_string = malloc(lengthOf(cur_job->job_string) + 1);
    strcpy(to_return->full_job_string, cur_job->job_string);
    return to_return;
}

void job_suspend_helper(pid_t calling_id) {
    /* if job is in background, just update status */
    background_job *cur_job = all_background_jobs;
    while (cur_job != NULL) {
        if (cur_job->pgid == calling_id) {
            cur_job->status = SUSPENDED;
            return;
        }
        cur_job = cur_job->next_background_job;
    }

    /* other wise it must be a job being run for a first time */
    job *check_foreground = all_jobs;
    while (check_foreground != NULL) {
        printf("here\n");
        if (check_foreground->pgid == calling_id) {
            put_job_in_background(check_foreground, 0, SUSPENDED);
            return;
        }
        check_foreground = check_foreground->next_job;
    }
}

/* child process has terminated and so we need to remove the process from the linked list (by pid) */
void childReturning(int sig, siginfo_t *siginfo, void *context) {
    int signum = siginfo->si_signo;
    pid_t calling_id = siginfo->si_pid;

    if (signum == SIGCHLD) {
        //TODO: finish covering all SIGCHLD codes and ensure this is working correctly...
        //in the case of the child being killed, remove it from the list of jobs
        if(siginfo->si_code == CLD_KILLED || siginfo->si_code == CLD_DUMPED || siginfo->si_code == CLD_EXITED) {
            trim_background_process_list(calling_id); //it has been killed and should be removed from the list of processes for job
        }
        //else if (siginfo->si_status != 0) {
        else if(siginfo->si_code == CLD_STOPPED) {
//          job_suspend_helper(calling_id);
//            printf("sigchld\n");
            trydelete *newTryDelete = malloc(sizeof(trydelete));
            newTryDelete->pidToDelete = calling_id;
            newTryDelete->next = trydelete1;
            trydelete1 = newTryDelete;
        }
    }
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
                    //found the pidToRemove, free, and an reset pointers
                    currentProcess->next_process = nextProcess->next_process;
                    free(nextProcess);
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
    /* problem occurring with white space args -- temporary fix, need to make sure parser doesn't give back whitespace commands */
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
    //execute built in (testing...)
    return (*(allBuiltIns[index].function))(process1->args);
}

void launchJob(job *j, int foreground) {
    process *p;
    pid_t pid;
    int isBuiltIn;
    for (p = j->first_process; p; p = p->next_process) {
        isBuiltIn = isBuiltInCommand(*p);

        //run as a built-in command
        if (isBuiltIn != NOT_FOUND) {
            //printf("built in");
            executeBuiltInCommand(p, isBuiltIn);
        }
            //run through execvp
        else {
            /* Fork the child processes.  */
            pid = fork();

            if (pid == 0) {
                /* This is the child process.  */
                launchProcess(p, j->pgid, j->stdin, j->stdout, j->stderr, foreground);
            } else if (pid < 0) {
                /* The fork failed.  */
                perror("fork");
                exit(EXIT_FAILURE);
            } else {
                /* This is the parent process.  */
                p->pid = pid;

                if (!j->pgid) {
                    j->pgid = pid;
                }

                setpgid(pid, j->pgid);
            }
        }
    }

    if (isBuiltIn == NOT_FOUND) {
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
}

/* Method to launch our process in either the foreground or the background. */
//method  based off of https://www.gnu.org/software/libc/manual/html_node/Launching-Jobs.html#Launching-Jobs
void launchProcess (process *p, pid_t pgid, int infile, int outfile, int errfile, int foreground) {
    pid_t pid;

    /* Put the process into the process group and give the process group
       the terminal, if appropriate.
       This has to be done both by the shell and in the individual
       child processes because of potential race conditions.  */

    pid = getpid();

    if (pgid == 0) {
        pgid = pid;
    }

    setpgid(pid, pgid);

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
    execvp(p->args[0], p->args);
    fprintf(stderr, "Error: %s: command not found\n", p->args[0]);
    free_all_jobs();
    exit(TRUE);
}

/* Put job j in the foreground.  If cont is nonzero,
   restore the saved terminal modes and send the process group a
   SIGCONT signal to wake it up before we block.  */

void put_job_in_foreground (job *j, int cont) {
    int status;
    /* Put the job into the foreground.  */
    tcsetpgrp(shell_terminal, j->pgid);

    tcgetattr(shell_terminal, &j->termios_modes);

    /* Wait for it to report.  */
    waitpid (j->pgid, &status, WUNTRACED);

    /* Put the shell back in the foreground.  */
    tcsetpgrp(shell_terminal, shell_pgid);
    // printf("giving back shell new \n");
    /* Restore the shell’s terminal modes.  */
    // tcgetattr(shell_terminal, &shell_tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/* takes background job and gives it to background job */
void simple_background_job_setup(background_job *dest, job *org, int status)
{
    dest->pgid = org->pgid;
    dest->status = status;
    dest->termios_modes = org->termios_modes; // <<< potential source of error here? valgrind and fg seems to be complaing about unitialized bytes
    char *js = malloc(sizeof(char) * lengthOf(org->full_job_string) + 1);
    strcpy(js, org->full_job_string);
    dest->job_string = js;
    dest->next_background_job = NULL;
}

/* Put a job in the background.  If the cont argument is true, send
   the process group a SIGCONT signal to wake it up.  */
void put_job_in_background(job *j, int cont, int status) { //TODO: check on merge here

    /* Add job to the background list with status of running */

    /* check if job is isn't already in background */
    // background_job *does_exist_bj;

    // if ((does_exist_bj = get_background_from_pgid(j->pgid)) != NULL) {
    //     does_exist_bj->status = status;
    // }

    if (!cont) {
        background_job *copyOfJ = malloc(sizeof(background_job));
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
    } else {
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

    background_job *cur_background_job = all_background_jobs;
    while (cur_background_job != NULL) {
        if (kill(-cur_background_job->pgid, SIGKILL) < 0){
            perror("kill (SIGKILL)");
        }
        else {
            printf("KILLED pgid: %d job: %s \n", cur_background_job->pgid, cur_background_job->job_string);
        }
        cur_background_job = cur_background_job->next_background_job;
    }

    EXIT = TRUE;
    return EXIT; //success
}

void background_built_in_helper(background_job *bj, int cont, int status) {
    if (kill(-bj->pgid, SIGCONT) < 0) {
        perror("kill (SIGCONT)");
    }

    background_job *current_job = all_background_jobs;
    int index = 0;
    while (current_job != NULL) {
        index ++;
        if (current_job->pgid == bj->pgid) {
            current_job->status = RUNNING;
            printf("[%d]\t\t%s\n", index, bj->job_string);
        }
        current_job = current_job->next_background_job;
    }
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
    int status;
    /* Put the job into the foreground.  */
    tcsetpgrp(shell_terminal, bj->pgid);

    tcgetattr(shell_terminal, &bj->termios_modes);

    /* Send the job a continue signal, if necessary.  */
    tcsetattr(shell_terminal, TCSADRAIN, &bj->termios_modes);

    if (kill(-bj->pgid, SIGCONT) < 0)
        perror("kill (SIGCONT)");

    printf("%s\n", bj->job_string); //print statement

    /* if the system call is interrupted, wait again */
    waitpid(bj->pgid , &status, WUNTRACED);

    /* Put the shell back in the foreground.  */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Restore the shell’s terminal modes.  */
    // tcgetattr(shell_terminal, &shell_tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
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
        printError("I am sorry, but you have passed an invalid number of arguments to kill.\n");
        return FALSE;
    } else if (argsLength == maxElements && args[pidLocation][0] == '%') {
        if (strcmp(args[flagLocation],
                   flag) == 0) { //check that -9 flag was input correctly, otherwise try sending kill with pid
            //(error checking gotten from stack overflow)
            const char *nptr = args[pidLocation] + pidLocationNoFlag;  /* string to read as a number      */
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
                currentNode++;
                //found your node
                if (currentNode == number) {
                    break;
                } else {
                    currentJob = currentJob->next_background_job;
                }
            }

            //node was not found!
            if (currentNode < number || number <= 0) {
                printError("I am sorry, but that job does not exist.\n");
                return FALSE;
            } else {
                pid_t pid = currentJob->pgid;
                printf("%d pid\n", pid);
                if (kill(pid, SIGKILL) == -1) {
                    printError("I am sorry, an error occurred with kill.\n");
                    return FALSE; //error occurred
                } else {
                    /* sig proc mask this */
                    sigset_t mask;
                    sigemptyset(&mask);
                    sigaddset(&mask, SIGCHLD);
                    sigprocmask(SIG_BLOCK, &mask, NULL);

                    trim_background_process_list(pid);

                    sigprocmask(SIG_UNBLOCK, &mask, NULL);
                    return TRUE;
                }
            }
        }
    } else { //we have no flags and only kill with a pid
        if (args[pidLocationNoFlag][0] == '%') {
            //PID is second argument
            //(error checking gotten from stack overflow)
            const char *nptr =
                    args[pidLocationNoFlag] + pidLocationNoFlag;  /* string to read as a number      */
            char *endptr = NULL;                            /* pointer to additional chars  */
            int base = 10;                                  /* numeric base (default 10)    */
            long long int number = 0;                       /* variable holding return      */

            /* reset errno to 0 before call */
            errno = 0;

            printf("job number %s\n", nptr);

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
                currentNode++;
                //found your node
                if (currentNode == number) {
                    break;
                } else {
                    currentJob = currentJob->next_background_job;
                }
            }

            //node was not found!
            if (currentNode < number || number <= 0) {
                printError("I am sorry, but that job does not exist.\n");
                return FALSE;
            } else {
                pid_t pid = currentJob->pgid;
                printf("%d pid\n", pid);
                if (kill(pid, SIGTERM) == -1) {
                    printError("I am sorry, an error occurred with kill.\n");
                    return FALSE; //error occurred
                } else {
                    /* sig proc mask this */
                    sigset_t mask;
                    sigemptyset(&mask);
                    sigaddset(&mask, SIGCHLD);
                    sigprocmask(SIG_BLOCK, &mask, NULL);

                    trim_background_process_list(pid);

                    sigprocmask(SIG_UNBLOCK, &mask, NULL);
                    return TRUE;
                }
            }
        }
        return FALSE;
    }
    return FALSE;
}

/* Method to iterate through the linked list and print out node parameters. */
int jobs_builtin(char **args) {
    background_job *currentJob = all_background_jobs;
    char *status[] = {running, suspended};
    int jobID = 1;

    if (currentJob == NULL) {} // do nothing
    else {
        while (currentJob != NULL) {
            //print out formatted information for processes in job
            printf("[%d]\t %d %s \t\t %s\n", jobID, currentJob->pgid, status[currentJob->status],
                   currentJob->job_string);
            jobID++;

            //get next job
            currentJob = currentJob->next_background_job;
        }
        return EXIT_SUCCESS;
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

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        background_built_in_helper(currentJob, TRUE, RUNNING);

        sigprocmask(SIG_UNBLOCK, &mask, NULL);

        return TRUE; //success!
    } else if (argsLength == maxArgsLength && args[locationOfPercent][0] == '%') {
        //(error checking gotten from stack overflow)
        const char *nptr = args[locationOfPercent] + locationOfPercent;     /* string to read as a number   */
        char *endptr = NULL;                            /* pointer to additional chars  */
        int base = 10;                                  /* numeric base (default 10)    */
        long long int number = 0;                       /* variable holding return      */

        printf("Number to parse: %s\n", nptr);
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
            currentNode++;
            //found your node
            if (currentNode == number) {
                /* sig proc mask this */
                sigset_t mask;
                sigemptyset(&mask);
                sigaddset(&mask, SIGCHLD);
                sigprocmask(SIG_BLOCK, &mask, NULL);

                background_built_in_helper(currentJob, TRUE, RUNNING);

                sigprocmask(SIG_UNBLOCK, &mask, NULL);
                return TRUE;
            } else {
                currentJob = currentJob->next_background_job;
            }
        }

        //node was not found!
        if (currentNode < number || number <= 0) {
            printError("I am sorry, but that job does not exist.\n");
        }
    }

    return FALSE;
}

/* Method that uses tcsetpgrp() to foreground the process -- this is fg*/
int foreground_builtin(char** args) {
    /*
     * Basic functionality included (ie foreground last pid)
     * to test to make sure this is working
     */

    int percentLocation = 1;
    int minElements = 1;
    int maxElements = 2;

    //get args length
    int argsLength = arrayLength(args);

    if (argsLength < minElements || argsLength > maxElements) {
        //invalid arguments
        printError("I am sorry, but you have passed an invalid number of arguments to fg.\n");
        return FALSE;
    } else if (argsLength == maxElements) {
        if (args[percentLocation][0] == '%') {
            //(error checking gotten from stack overflow)
            const char *nptr =
                    args[percentLocation] + percentLocation;  /* string to read as a number      */
            char *endptr = NULL;                            /* pointer to additional chars  */
            int base = 10;                                  /* numeric base (default 10)    */
            long long int number = 0;                       /* variable holding return      */

            /* reset errno to 0 before call */
            errno = 0;

            printf("job number %s\n", nptr);

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
                currentNode++;
                //found your node requested
                if (currentNode == number) {
                    // put job into the foreground here
                    pid_t pid = currentJob->pgid;

                    foreground_helper(currentJob);

                    return TRUE;
                } else {
                    currentJob = currentJob->next_background_job;
                }
            }

            //node was not found!
            if (currentNode < number || number <= 0) {
                printError("I am sorry, but that job does not exist.\n");
                return FALSE;
            }
        } else {
            printError("I am sorry, but you have passed an invalid node argument to fg.\n");
            return FALSE;
        }
    } else {

        //bring back tail of jobs list, if it exists
        background_job *currentJob = all_background_jobs;
        background_job *nextJob = NULL;
        if (currentJob == NULL) {
            printError("I am sorry, but that job does not exist.\n");
            return FALSE;
        }

        //go to the end of the list
        while (currentJob != NULL) {
            nextJob = currentJob->next_background_job;
            if (nextJob == NULL) {
                break; //want to bring back current job
            }
            currentJob = currentJob->next_background_job;
        }

        /* get last process pid_t */
        pid_t pid = currentJob->pgid;

        foreground_helper(currentJob);

        return TRUE;
    }
    return FALSE;
}