/*
 * cush - the customizable shell.
 *
 * Developed by Godmar Back for CS 3214 Summer 2020 
 * Virginia Tech.
 */
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <setjmp.h>
#include <unistd.h>

#include "termstate_management.h"
#include "shell-ast.h"
#include "utils.h"
#include "history.h"
#include "custom_prompt.h"
#include "processes/jobs.h"
#include "processes/launch.h"
#include "processes/handlers.h"

/* Global for keeping track of whether the prompt is custom */
bool custom = false;


static void
usage(char *progname)
{
    printf("Usage: %s -h\n"
        " -h            print this help\n",
        progname);

    exit(EXIT_SUCCESS);
}

/* Build a prompt */
static char *
build_prompt(bool newline)
{
    if(custom){
        return buildCustomPrompt(newline);
    }
    else{
        return newline ? strdup("\ncush> ") : strdup("cush> ");
    }
}


/* Globals for jumping */
char * prompt;
sigjmp_buf prompt_jump;
volatile sig_atomic_t prompt_jump_active;

int
main(int ac, char *av[])
{
    int opt;

    /* Process command-line arguments. See getopt(3) */
    while ((opt = getopt(ac, av, "h")) > 0) {
        switch (opt) {
        case 'h':
            usage(av[0]);
            break;
        }
    }

    history_init();
    jobs_init();
    termstate_init();
    handlers_init();

    /* Read/eval loop. */
    for (;;) {
        termstate_give_terminal_back_to_shell();
        bool newline = sigsetjmp(prompt_jump, true);
        if (!newline) prompt_jump_active = true;

        /* Do not output a prompt unless shell's stdin is a terminal */
        prompt = isatty(0) ? build_prompt(newline) : NULL;
        char * cmdline = readline(prompt);
        free (prompt);


        if (cmdline == NULL)  /* User typed EOF */
            break;

        /* Attempt to parse event designators */
        cmdline = try_event(cmdline);
        history_add(cmdline);
        struct ast_command_line * cline = ast_parse_command_line(cmdline);
        free (cmdline);
        if (cline == NULL)                  /* Error in command line */
            continue;

        if (list_empty(&cline->pipes)) {    /* User hit enter */
            ast_command_line_free(cline);
            continue;
        }

        delete_jobs();
        launch_command_line(cline);
        ast_command_line_free(cline);
    }
    return 0;
}
