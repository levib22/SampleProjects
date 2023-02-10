#include "jobs.h"

/**
 * Return job corresponding to pid,
 * or NULL if no such process has been added.
 * Optional parameter specifies whether to
 * remove the item upon retrieving it.
 */
struct job * get_job_from_pid(int pid, int remove);

/**
 * Add a correspondence of pid <-> job.
 * Does not check for duplicates.
 */
void add_pid_to_job(int pid, struct job *job);

/**
 * Remove all correspondences that were marked for deletion.
 * Useful for calling outside of signal handlers.
 */
int delete_pids(void);