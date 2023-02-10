/**
 * Methods for handling signals.
 *
 * Note: SIGCHLD handlers originally located in cush.c
 * Moved here for easier imports from student code,
 * which is located outside of cush.c for better separation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "handlers.h"
#include "jobs.h"
#include "pid.h"
#include "../signal_support.h"
#include "../termstate_management.h"


/**
 * Adjust process info when a child terminates.
 *
 * Step 1. Given the pid, determine which job this pid is a part of
 *         (how to do this is not part of the provided code.)
 * Step 2. Determine what status change occurred using the
 *         WIF*() macros.
 * Step 3. Update the job status accordingly, and adjust
 *         num_processes_alive if appropriate.
 *         If a process was stopped, save the terminal state.
 */
void
handle_child_status(pid_t pid, int status)
{
    assert(signal_is_blocked(SIGCHLD));
    /* Note: Removing the job here would cause
    a use-after-free error in wait_for_job. */

    /* 1. Retrieve the pid->job correspondence */
    struct job *job = get_job_from_pid(pid, false);
    if (job == NULL) {
        fprintf(stderr, "PID record does not exist\n");
        return;
    }

    /* 2. Determine what to do based on status */

    /* Process exited on its own terms */
    if (WIFEXITED(status)) {
        bool completed = --job->num_processes_alive <= 0;
        switch (job->status) {
            case FOREGROUND:
                if (WEXITSTATUS(status) == 0) {
                    termstate_sample();
                }
                break;
            case BACKGROUND:
                if (completed) {
                    /* TODO: Queue response */
                }
                break;
            default:
                fprintf(stderr, "Process in status %s was still running",
                    get_status(job->status));
                break;
        }
    }

    /* Process killed by signal */
    else if (WIFSIGNALED(status)) {
        --job->num_processes_alive;
        int sig = WTERMSIG(status);
        switch (sig) {
            case SIGINT:    break;
            default:        fprintf(stderr, "%s\n", strsignal(sig));
        }
    }
    
    /* Process stopped due to a signal */
    else if (WIFSTOPPED(status)) {

        /* 3. Save the terminal state */
        if (job->status == FOREGROUND) {
            termstate_save(&job->saved_tty_state);
            job->has_tty_state = true;
        }
        
        /* 3. Suspend process for later revival */
        int sig = WSTOPSIG(status);
        switch (sig) {
            case SIGTSTP:               
                job->status = STOPPED;
                /* FIXME: Printed once for every running process */
                /* Example: `sleep 10 | sleep 9`, `^Z` */
                print_job(job, true);
                break;
            case SIGTTIN:
            case SIGTTOU:
                job->status = NEEDSTERMINAL;
                break;
            default:
                fprintf(stderr, "Stopped by signal: %d\n", sig);
                break;
        }
    }
    
    else {
        fprintf(stderr, "Unchecked status: %d\n", status);
    }
}

/*
 * Suggested SIGCHLD handler.
 *
 * Call waitpid() to learn about any child processes that
 * have exited or changed status (been stopped, needed the
 * terminal, etc.)
 * Just record the information by updating the job list
 * data structures.  Since the call may be spurious (e.g.
 * an already pending SIGCHLD is delivered even though
 * a foreground process was already reaped), ignore when
 * waitpid returns -1.
 * Use a loop with WNOHANG since only a single SIGCHLD
 * signal may be delivered for multiple children that have
 * exited. All of them need to be reaped.
 */
static void
sigchld_handler(int sig, siginfo_t *info, void *_ctxt)
{
    pid_t child;
    int status;

    assert(sig == SIGCHLD);

    while ((child = waitpid(-1, &status, WUNTRACED|WNOHANG)) > 0) {
        handle_child_status(child, status);
    }
}

/*
 * SIGINT (^C) handler.
 *
 * Caught when waiting at the prompt. For handling SIGINT
 * in spawned processes, refer to handle_child_status.
 * 
 * Clear character buffer and print a new prompt.
 */
static void
sigint_handler(int sig, siginfo_t *info, void *_ctxt)
{
    assert(sig == SIGINT);

    /* Soft-reset by jumping to the top of the main.c loop */
    free(prompt);
    if (prompt_jump_active)
        siglongjmp(prompt_jump, SIGINT);
}

/**
 * Initialize all signal handlers.
 * 
 * Should be called once at the start of the shell.
 */
void
handlers_init(void)
{
    signal_set_handler(SIGCHLD, sigchld_handler);
    signal_set_handler(SIGINT, sigint_handler);
    
    /* Hard-ignore the signal to avoid printing ^Z */
    /* Don't forget to unignore in spawned children*/
    signal(SIGTSTP, SIG_IGN);
}