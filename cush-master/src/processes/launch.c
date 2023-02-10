/**
 * Code that receives parsed commands and launches them.
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "launch.h"
#include "pid.h"
#include "builtins.h"
#include "../signal_support.h"
#include "../termstate_management.h"
#include "../utils.h"

#define READ_END 0
#define WRITE_END 1


/* Close file descriptors only when they are not STDIN or STDOUT */
static void
try_close(int fd_in, int fd_out) {
    if (fd_in != STDIN_FILENO) {
        if (close(fd_in) == -1)
            utils_error("close: ");
    }
    if (fd_out != STDOUT_FILENO) {
        if (close(fd_out) == -1)
            utils_error("close: ");
    }
}

/**
 * Launch the parsed command and associate it with the given job
 * Additionally accept file descriptors used as STDIN and STDOUT
 */
static void
launch_command(struct ast_command *cmd, struct job *job, int fd_in,
    int fd_out) {
    /* Child may exit even before the parent returns from fork() */
    signal_block(SIGCHLD);

    /* Regular commands: spawn several dedicated child processes */    
    pid_t child_pid = fork();
    if (child_pid == -1)
        utils_fatal_error("creating a child process failed: ");
    
    /* Set PGID in both the parent and child, for extra security */
    if (setpgid(child_pid, job->pgid) == -1)
        utils_error("setpgid: ");

    /* Execute requested program by replacing the forked process */
    if (child_pid == 0) {
        if (job->pgid == 0 && !job->pipe->bg_job)
            /* Though a system call, getpid is always successful */
            termstate_give_terminal_to(NULL, getpid());
        signal(SIGTSTP, SIG_DFL);       /* Reset back to default */
        signal_unblock(SIGCHLD);        /* Inherited across exec */
        if (dup2(fd_in, STDIN_FILENO) == -1)
            utils_error("dup2: ");      /* Redirect input stream */
        if (dup2(fd_out, STDOUT_FILENO) == -1)
            utils_error("dup2: ");      /* Forward output stream */
        if (cmd->dup_stderr_to_stdout) {
            if (dup2(fd_out, STDERR_FILENO) == -1)
                utils_error("dup2: ");  /* Also the error stream */
        }
        try_close(fd_in, fd_out);
        if (execvp(cmd->argv[0], cmd->argv) == -1)
            utils_fatal_error("%s: ", cmd->argv[0]);
    }
    add_pid_to_job(child_pid, job);     /* Needs SIGCHLD blocked */
    job->num_processes_alive++;
    signal_unblock(SIGCHLD);
    try_close(fd_in, fd_out);
    if (job->pgid == 0)                 /* Only for group leader */
        job->pgid = child_pid;
}

/* Spawn and connect several processes */
static void
launch_pipeline(struct ast_pipeline *pipeline) {
    /* Built-ins: run directly */
    struct list_elem *e = list_begin (&pipeline->commands);
    if (builtins_try(list_entry(e, struct ast_command, elem))) {
        ast_pipeline_free(pipeline);
        return;
    }
    struct job *job = add_job(pipeline);

    /* First child reads from input */
    int pipe_before[2];
    pipe_before[READ_END] = STDIN_FILENO;
    int permis = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if (pipeline->iored_input) {
        int file = open(pipeline->iored_input, O_RDONLY, permis);
        if (file == -1)
            utils_error("Could not open %s, reading from stdin: ",
                pipeline->iored_input);
        else
            pipe_before[READ_END] = file;
    }

    /* Launch and link several children */
    for (;
        e != list_end (&pipeline->commands);
        e = list_next (e)) {
        struct ast_command *cmd = list_entry(e, struct ast_command, elem);

        int pipe_after[2];
        if (e != list_back (&pipeline->commands)) {
            /* For N children, make N-1 pipes */ 
            if (pipe(pipe_after) == -1) {
                utils_error("pipe: ");
                pipe_after[READ_END] = STDIN_FILENO;
                pipe_after[WRITE_END] = STDOUT_FILENO;
            }

            /* To be automatically closed on exec */
            if (utils_set_cloexec(pipe_after[READ_END]))
                utils_error("cloexec: ");
        }
        else {
            /* Last child writes to output */
            pipe_after[WRITE_END] = STDOUT_FILENO;
            if (pipeline->iored_output) {
                int flags = O_WRONLY | O_CREAT;
                if (pipeline->append_to_output)
                    flags |= O_APPEND;
                int file = open(pipeline->iored_output, flags, permis);
                if (file == -1)
                    utils_error("Could not open %s, writing to stdout: ",
                        pipeline->iored_output);
                else
                    pipe_after[WRITE_END] = file;
            }
        }
        launch_command(cmd, job,
            pipe_before[READ_END],
            pipe_after[WRITE_END]);
        
        /* Store the new pipe for the next iteration */
        pipe_before[READ_END] = pipe_after[READ_END];
        pipe_before[WRITE_END] = pipe_after[WRITE_END];
    }

    if (job->pipe->bg_job) {
        print_job(job, false);
    }
    else {
        /* Wait for foreground to finish */
        signal_block(SIGCHLD);
        wait_for_job(job);              /* Needs SIGCHLD blocked */
        signal_unblock(SIGCHLD);
    }
}

/* Execute all jobs in the given order */
void
launch_command_line(struct ast_command_line *cline) {
    while (!list_empty (&cline->pipes)) {
        struct list_elem *e = list_pop_front (&cline->pipes);
        struct ast_pipeline *pipeline = list_entry(e, struct ast_pipeline, elem);
        launch_pipeline(pipeline);
    }
}