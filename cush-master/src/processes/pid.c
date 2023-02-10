/**
 * A hashtable-like structure for keeping pid -> job correspondences.
 */
#include <stdlib.h>
#include <assert.h>
#include <sys/wait.h>

#include "pid.h"
#include "../list.h"
#include "../signal_support.h"

struct corresp {
    struct list_elem elem;  /* Link element for list */
    int pid;                /* Process id */
    struct job *job;        /* The job corresponding to the process id */
};

/* Hash table of size pow(2, HASH) */
#define HASH 6
#define SIZE (1<<HASH)
static struct list * table[SIZE];

/* Get the hash index in the table */
static int
hash(int pid) {
    return pid & (SIZE - 1);
}

/* Retrieve a list containing the pid */
static struct list *
get(int pid, int init) {
    int h = hash(pid);
    struct list *list = table[h]; 

    /* Initialize the list if haven't already */
    if (init && list == NULL) {
        list = malloc(sizeof *list);
        list_init(list);
        table[h] = list;
    }
    return list;
}

/* Remove all correspondences that were marked for deletion */
int delete_pids(void) {
    int unblock;
    if ((unblock = !signal_is_blocked(SIGCHLD)))
        signal_block(SIGCHLD);

    int deleted = 0;
    for (int h = 0; h < SIZE; h++) {
        struct list *list = table[h];
        if (list == NULL) continue;
        for (struct list_elem * e = list_begin (list);
            e != list_end (list); ) {
            struct corresp *corr = list_entry(e, struct corresp, elem);

            /* Check if element should be deleted */
            if (corr->job == NULL) {
                e = list_remove (e);
                free(corr);
                deleted++;
            }
            else {
                e = list_next (e);
            }
        }
        /* Deallocate if the list is now empty */
        if (list_empty(list))
            free(list);
    }
    
    if (unblock)
        signal_unblock(SIGCHLD);
    return deleted;
}

/* Return job corresponding to pid */
struct job * 
get_job_from_pid(int pid, int remove) {
    struct list *list = get(pid, false);
    if (list == NULL)
        return NULL;
    
    /* Iterate list to find the needed pid */
    for (struct list_elem * e = list_begin (list); 
        e != list_end (list);
        e = list_next (e)) {
        struct corresp *corr = list_entry(e, struct corresp, elem);
        if (corr->pid == pid) {
            if (remove)
                /* Can't remove node since called from handler */
                corr->job = NULL;
            return corr->job;
        }
    }
    return NULL;
}

/* Add a correspondence of pid -> job */
void
add_pid_to_job(int pid, struct job *job) {
    assert(signal_is_blocked(SIGCHLD));

    struct list *list = get(pid, true);

    /* Create a new correspondence and insert */
    struct corresp *corr = malloc(sizeof *corr);
    corr->pid = pid;
    corr->job = job;
    list_push_back(list, &corr->elem);
}