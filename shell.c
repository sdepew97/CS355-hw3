#include <zconf.h>
#include <sys/wait.h>
#include "shell.h"
#include "llist.h"
#include "llist_node.h"

/* Main method and body of the function. */
int main (int argc, char **argv) {
    int status, read_command_status, parse_command_status;
    pid_t pid;
    char **commands = NULL;
    char *cmd = NULL;
    /*LL holds child process id*/
    llist backgroundProcess;

    initializeShell();
    builtBuiltIns(); //store all builtins in built in array

    while (!EXIT) {
        process **commands = NULL;
        process *cmd = NULL;

        printPrompt();

        if (readCommandLine(&commands) < 0) {
            handleError("read command failure", &commands);


            if (tokenizeCommands(&commands) < 0) {
                handleError("tokenize failure", &commands);
            }

            int i = 0;
            if (commands[i] == NULL) {
                free(commands, cmd);
                continue;
            } else {
                cmd = commands[0];
            }

            while (cmd != NULL) {
                int isBuiltIn = isBuiltInCommand(*cmd);
                if (isBuiltIn >= 0) {
                    // we don’t support background builtins currently
                    executeBuiltInCommand(*cmd, isBuiltIn);
                } else {
                    pid = fork();

/* This is the child process.  */
                    if (pid == 0) {
                        launchProcess(cmd, getpid(), isBackgroundJob(&cmd));
                    } else if (pid < 0) {
                        /* The fork failed.  */
                        //raise error, free memory, and exit
                    }

/* This is the parent process.  */
                    else {
                        if (!isBackgroundJob(&cmd)) {
                            waitpid(pid, &status,
                                    WNOHANG); //wait on the forked child... (using WUNTRACED so that we continue in SIGSTOP case of c-z)
                            tcsetpgrp(shell, shell_pgid);
                        }

                    }
                }
                cmd = commands[++i];
            }
        }
        free(commands, cmd);

    }
    llist_free(backgroundProcess);
}
