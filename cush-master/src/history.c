/**
 * Call GNU History library and keep track of entered commands.
 * Also detect and parse event designators.
 */
#include <stdio.h>
#include <stdlib.h>
#include <readline/history.h>
#include <string.h>

#include "history.h"

/* Initialize an empty history */
void history_init(void) {
    void using_history (void);
}

/* Add a command to the current history */
void history_add(const char * cmdline) {
    add_history(cmdline);
}

/* Iterate and print the history */
void history_print(int length) {
    register HIST_ENTRY **list;
    register int i;

    /* Retrieve history */
    list = history_list();
    if (!list)
        printf("Could not retrieve history\n");
    
    /* Determine where to begin */
    i = length < 0 ? 0 :
        history_length > length ? history_length - length : 0;

    /* Iterate until completion */
    for (; list[i]; i++)
        printf("%5d %s\n", i+1, list[i]->line);
}

/* Check for an event designator pattern */
char * try_event(char * str) {
    char * result;
    switch(str[0]) {
        case '!':;
            /* Attempt to parse as index */
            char *end;
            int idx = strtol(str + 1, &end, 10);
            if (str + 1 != end) {
                HIST_ENTRY * hist = NULL;
                for (; idx <= history_length; idx += history_length + 1) {
                    if (history_base <= idx)
                        hist = history_get(idx);
                }
                if (hist) {
                    result = strdup(hist->line);
                    break;
                }
            }
            /* Attempt to get by name */
            if (history_search_prefix(str + 1, -1) == -1)
                return str;
            result = strdup(current_history()->line);
            break;
        default:
            return str;
    }
    free(str);
    return result;
}