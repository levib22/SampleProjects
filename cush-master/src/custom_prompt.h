#include <stdbool.h>

/* Flag of whether the prompt is enabled */
extern bool custom;

/**
 * Check if the given command is a built-in,
 * and perform appropriate actions if it is.
 */
void togglePrompt(void);

/**
 * Dynamically allocate a custom prompt.
 */
char* buildCustomPrompt(bool newline);