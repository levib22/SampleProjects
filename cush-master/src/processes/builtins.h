#include "../shell-ast.h"

/**
 * Check if the given command is a built-in,
 * and perform appropriate actions if it is.
 */
int builtins_try(struct ast_command *cmd);