/**
 * Commands for functionality internal to the shell.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#include "builtins.h"
#include "jobs.h"
#include "../history.h"
#include "../signal_support.h"
#include "../termstate_management.h"
#include "../utils.h"
#include "../custom_prompt.h"

/* Possible built-in commands */
typedef enum {UNKNOWN, KILL, FG, BG, JOBS, STOP, EXIT, HISTORY, CUSTOM} BUILTIN;
const static struct {
    BUILTIN     bin;
    const char *str;
} convert [] = {
    {KILL,      "kill"},
    {FG,        "fg"},
    {BG,        "bg"},
    {JOBS,      "jobs"},
    {STOP,      "stop"},
    {EXIT,      "exit"},
    {HISTORY,   "history"},
    {CUSTOM,    "custom"}
};

/* Check if a string is a built-in command */
static BUILTIN
builtins_check(char* str) {
    int n = sizeof(convert) / sizeof(convert[0]);
    for (int i = 0; i < n; i++) {
        if (strcmp(convert[i].str, str) == 0)
            return convert[i].bin;
    }
    return UNKNOWN;
}

/* Retrieve the job referenced by the argument */
static struct job *
get_job(char *argv[]) {
    if (!argv[1]) {
        fprintf(stderr, "%1$s: job id missing\n", argv[0]);
        return NULL;
    }
    char *end;
    int jid = strtol(argv[1], &end, 10);
    if (argv[1] == end) {
        fprintf(stderr, "%1$s: usage %1$s <job>\n", argv[0]);
        return NULL;
    }
    struct job *job = get_job_from_jid(jid);
    if (!job)
        fprintf(stderr, "%1$s %2$s: No such job\n", argv[0], argv[1]);
    return job;
}

/* Attempt to launch command as a built-in */
int builtins_try(struct ast_command *cmd) {
    struct job *job;
    switch (builtins_check(cmd->argv[0])) {
        case UNKNOWN:
            return false;
        
        case KILL:
            if (!(job = get_job(cmd->argv))) break;
            if (killpg(job->pgid, SIGTERM) == -1)
                utils_error("killpg: ");
            break;

        case FG:
            if (!(job = get_job(cmd->argv))) break;
            termstate_give_terminal_to(job->has_tty_state ?
                &job->saved_tty_state : NULL, job->pgid);

            /* Revive the stopped process */
            if (is_stopped(job))
                if (killpg(job->pgid, SIGCONT) == -1)
                    utils_error("killpg: ");
            job->status = FOREGROUND;
            print_cmdline(job->pipe);
            printf("\n");

            /* Wait for new foreground to complete */
            signal_block(SIGCHLD);
            wait_for_job(job);
            signal_unblock(SIGCHLD);
            break;

        case BG:
            if (!(job = get_job(cmd->argv))) break;
            
            /* Revive the stopped process */
            if (is_stopped(job))
                if (killpg(job->pgid, SIGCONT) == -1)
                    utils_error("killpg: ");
            job->status = BACKGROUND;
            print_job(job, false);
            break;

        case JOBS:
            print_jobs(true);
            break;
        
        case STOP:
            if (!(job = get_job(cmd->argv))) break;

            /* Stop the background process */
            if (!is_stopped(job))
                if (killpg(job->pgid, SIGTSTP) == -1)
                    utils_error("killpg: ");
            break;

        case EXIT:
            exit(EXIT_SUCCESS);
            break;

        case HISTORY:;
            int length = -1;
            if (cmd->argv[1]) {
                /* Parse optional parameter */
                char *end;
                length = strtol(cmd->argv[1], &end, 10);
                if (cmd->argv[1] == end || length < 0) {
                    fprintf(stderr, "%1$s: usage %1$s [len]\n", cmd->argv[0]);
                    break;
                }
            }
            history_print(length);
            break;

        case CUSTOM:
            togglePrompt();
            break;
        
        default:
            fprintf(stderr, "%s not yet implemented\n", cmd->argv[0]);
    }
    return true;
}