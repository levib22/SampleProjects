#include "../shell-ast.h"

/**
 * Execute all jobs in the given order.
 * 
 * Takes the parsed command-line object, launching
 * the jobs in the order that they are given.
 * If a foreground job is encountered, execution of
 * further jobs will wait for said job to complete.
 * 
 * Pipelines are removed upon processing, and
 * ownership is transferred to the spawned jobs.
 */
void launch_command_line(struct ast_command_line *cline);