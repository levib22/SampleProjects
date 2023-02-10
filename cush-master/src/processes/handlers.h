#include <unistd.h>
#include <setjmp.h>
#include <sys/wait.h>

extern char * prompt;
extern sigjmp_buf prompt_jump;
extern volatile sig_atomic_t prompt_jump_active;

/* SIGCHLD handler may also be called when waiting */
void handle_child_status(pid_t pid, int status);

/* Initialize all signal handlers */
void handlers_init(void);