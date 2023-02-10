/**
 * Functionality for building a shell prompt
 * that includes the current host and working directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "custom_prompt.h"
#include "utils.h"

/* Turn the prompt on/off */
void togglePrompt(void) {
    custom = !custom;
}

/* Generate a custom prompt */
char* buildCustomPrompt(bool newline){
    //get the host name into dynamically allocated string
    char* hostBuff = malloc(sizeof(char)*256);
    gethostname(hostBuff, sizeof(char)*257);
    char* host = strdup(hostBuff);
    free(hostBuff);
    //get the cwd name into a dynamically allocated string
    char* cwdBuff = malloc(sizeof(char)*256);
    //previously had a weird issue with cwd, if statement here for testing
    if(getcwd(cwdBuff, sizeof(char)*257) == NULL) {
        utils_error("getcwd : ");
    }
    char* cwd = strdup(cwdBuff);

    //format the custom prompt and return it as a dynamically allocated string
    char* str = malloc(sizeof(char)*256);
    if(newline){
        snprintf(str, 258, "\nhost[%s] in: %s>", host, cwd);
    }
    else{
        snprintf(str, 258, "host[%s] in: %s>", host, cwd);
    }
    return str;
}