/**
 * Utility functions for job list management.
 * We use 2 data structures: 
 * (a) an array jid2job to quickly find a job based on its id
 * (b) a linked list to support iteration
 * 
 * Note: originally located in cush.c (all but delete_jobs)
 * Moved here for easier imports from student code,
 * which is located outside of cush.c for better separation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <assert.h>
#include <limits.h>

#include "jobs.h"
#include "handlers.h"
#include "../shell-ast.h"
#include "../signal_support.h"
#include "../utils.h"


#define MAXJOBS (1<<16)
static struct list job_list;

static struct job * jid2job[MAXJOBS];

/* Return job corresponding to jid */
struct job * 
get_job_from_jid(int jid)
{
    if (jid > 0 && jid < MAXJOBS && jid2job[jid] != NULL)
        return jid2job[jid];

    return NULL;
}

/* Add a new job to the job list */
struct job *
add_job(struct ast_pipeline *pipe)
{
    struct job * job = malloc(sizeof *job);
    job->pipe = pipe;
    job->pgid = 0;
    job->status = pipe->bg_job ? BACKGROUND : FOREGROUND;
    job->num_processes_alive = 0;
    job->has_tty_state = false;
    list_push_back(&job_list, &job->elem);
    for (int i = 1; i < MAXJOBS; i++) {
        if (jid2job[i] == NULL) {
            jid2job[i] = job;
            job->jid = i;
            return job;
        }
    }
    fprintf(stderr, "Maximum number of jobs exceeded\n");
    abort();
    return NULL;
}

/* Delete a job */
void
delete_job(struct job *job)
{
    int jid = job->jid;
    assert(jid != -1);
    jid2job[jid]->jid = -1;
    jid2job[jid] = NULL;
    ast_pipeline_free(job->pipe);
    free(job);
}

/* Remove all completed jobs */
int
delete_jobs(void) {
    int unblock;
    if ((unblock = !signal_is_blocked(SIGCHLD)))
        signal_block(SIGCHLD);

    int deleted = 0;
    for (struct list_elem * e = list_begin (&job_list);
        e != list_end (&job_list); ) {
        struct job *job = list_entry(e, struct job, elem);

        switch (job->num_processes_alive) {
            case INT_MIN ... -1:
                fprintf(stderr, "Extra SIGCHLD caught\n");
            case 0:
                /* Delete the job */
                e = list_remove (e);
                delete_job(job);
                deleted++;
                break;
            default:
                e = list_next (e);
                break;
        }
    }
    
    if (unblock)
        signal_unblock(SIGCHLD);
    return deleted;
}

/* String representation of job status */
const char *
get_status(enum job_status status)
{
    switch (status) {
    case FOREGROUND:
        return "Foreground";
    case BACKGROUND:
        return "Running";
    case STOPPED:
        return "Stopped";
    case NEEDSTERMINAL:
        return "Stopped (tty)";
    default:
        return "Unknown";
    }
}

/* There are several possible stopped states */
bool
is_stopped(struct job *job) {
    return
        job->status == STOPPED ||
        job->status == NEEDSTERMINAL;
}

/* Print the command line that belongs to one job. */
void
print_cmdline(struct ast_pipeline *pipeline)
{
    struct list_elem * e = list_begin (&pipeline->commands); 
    for (; e != list_end (&pipeline->commands); e = list_next(e)) {
        struct ast_command *cmd = list_entry(e, struct ast_command, elem);
        if (e != list_begin(&pipeline->commands))
            printf("| ");
        char **p = cmd->argv;
        printf("%s", *p++);
        while (*p)
            printf(" %s", *p++);
    }
}

/* Print all jobs */
void
print_jobs(int verbose)
{
    for (struct list_elem * e = list_begin (&job_list);
        e != list_end (&job_list);
        e = list_next (e)) {
        struct job *job = list_entry(e, struct job, elem);
        if (job->num_processes_alive > 0)
            print_job(job, verbose);
    }
}

/* Print a job */
void
print_job(struct job *job, int verbose)
{
    printf("[%d]", job->jid);
    if (verbose) {
        printf("\t%s\t\t(", get_status(job->status));
        print_cmdline(job->pipe);
        printf(")\n");
    }
    else {
        printf(" %d\n", job->pgid);
    }
}

/* Initialize the job list */
void
jobs_init(void)
{
    list_init(&job_list);
}

/* Wait for all processes in this job to complete */
void
wait_for_job(struct job *job)
{
    assert(signal_is_blocked(SIGCHLD));

    while (job->status == FOREGROUND && job->num_processes_alive > 0) {
        int status;

        pid_t child = waitpid(-1, &status, WUNTRACED);

        // When called here, any error returned by waitpid indicates a logic
        // bug in the shell.
        // In particular, ECHILD "No child process" means that there has
        // already been a successful waitpid() call that reaped the child, so
        // there's likely a bug in handle_child_status where it failed to update
        // the "job" status and/or num_processes_alive fields in the required
        // fashion.
        // Since SIGCHLD is blocked, there cannot be races where a child's exit
        // was handled via the SIGCHLD signal handler.
        if (child != -1)
            handle_child_status(child, status);
        else
            utils_fatal_error("waitpid failed, see code for explanation: ");
    }
}