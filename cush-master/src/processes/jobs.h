#ifndef __JOB_H
#define __JOB_H

#include <termios.h>

#include "../list.h"

enum job_status {
    FOREGROUND,     /* job is running in foreground.  Only one job can be
                       in the foreground state. */
    BACKGROUND,     /* job is running in background */
    STOPPED,        /* job is stopped via SIGSTOP */
    NEEDSTERMINAL,  /* job is stopped because it was a background job
                       and requires exclusive terminal access */
};

struct job {
    struct list_elem elem;          /* Link element for jobs list. */
    struct ast_pipeline *pipe;      /* The pipeline of commands this job represents */
    int     jid;                    /* Job id. */
    int     pgid;                   /* The group id of all processes in this job */
    enum job_status status;         /* Job status. */ 
    int  num_processes_alive;       /* The number of processes that we know to be alive */
    struct termios saved_tty_state; /* The state of the terminal when this job was */
    int has_tty_state;              /* stopped after having been in foreground */
};

/* Check against several possible stopped states */
bool is_stopped(struct job *job);

/* String representation of job status */
const char * get_status(enum job_status status);

/* Initialize the job list */
void jobs_init(void);

/* Return job corresponding to jid */
struct job * get_job_from_jid(int jid);

/* Add a new job to the job list */
struct job * add_job(struct ast_pipeline *pipe);

/**
 * Delete a job.
 * This should be called only when all processes that were
 * forked for this job are known to have terminated.
 */
void delete_job(struct job *job);

/**
 * Remove all completed jobs.
 * Allows removal of jobs from outside of signal handlers.
 */
int delete_jobs(void);

/* Print the command line that belongs to one job. */
void print_cmdline(struct ast_pipeline *pipeline);

/* Print all jobs */
void print_jobs(int verbose);

/* Print a job */
void print_job(struct job *job, int verbose);

/**
 * Wait for all processes in this job to complete, or for
 * the job no longer to be in the foreground.
 * You should call this function from a) where you wait for
 * jobs started without the &; and b) where you implement the
 * 'fg' command.
 * 
 * Implement handle_child_status such that it records the 
 * information obtained from waitpid() for pid 'child.'
 *
 * If a process exited, it must find the job to which it
 * belongs and decrement num_processes_alive.
 *
 * However, not that it is not safe to call delete_job
 * in handle_child_status because wait_for_job assumes that
 * even jobs with no more num_processes_alive haven't been
 * deallocated.  You should postpone deleting completed
 * jobs from the job list until when your code will no
 * longer touch them.
 *
 * The code below relies on `job->status` having been set to FOREGROUND
 * and `job->num_processes_alive` having been set to the number of
 * processes successfully forked for this job.
 */
void wait_for_job(struct job *job);

#endif /* __JOB_H */