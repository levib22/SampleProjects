/* Initialize an empty history */
void history_init(void);

/* Add an entry to the current history */
void history_add(const char * cmdline);

/* Iterate and print the history */
void history_print(int length);

/**
 * Check for an event designator pattern.
 * If matched, the string will be deallocated,
 * and the corresponding replacement allocated.
 */
char * try_event(char * str);