Student Information
-------------------
Alexey Didenkov <adidenkov@vt.edu>
Levi Banos <levii@vt.edu>

How to execute the shell
------------------------
1. Navigate to the 'src' folder and run `make`.
2. Run the produced executable via `./cush`.

Important Notes
---------------
Built-in commands currently don't support pipe redirection.
Built-ins are expected to be the leading commands in a pipe
sequence, in which case the remaining commands are ignored.
If the first command is not a built-in, any built-ins that
follow it will attempt to be parsed as system commands.

Job completion messages are not yet printed or queued.
When a job finishes in the background, it simply vanishes
from the job list.

For consistency with `cush-gback`, stopped jobs must be
launched before they can react to the `kill` command.

Description of Base Functionality
---------------------------------
`jobs`:
Cycles the job list and prints them in their scheduled order.
Omitted from this output are jobs that have reaped all their
children and now await deletion. This deletion happens right
before a new command line is parsed.
Processes are associated with their jobs upon creation by
storing correspondences in a hashtable-like structure.
Group ids are stored in the job struct, and are recorded on
creation of the group leader. This value is then assigned to
each child process, between the `fork` and `exec` calls.

`fg`:
Moves the given job to the foreground. This includes resuming
(via `SIGCONT`) its process group if it has been stopped,
giving it terminal access, and waiting for it to exit.
Note that a terminal state may have previously been sampled
(if stopped by `ˆZ`), or may be invalid (if run in background).
This validity is recorded in a parameter of the job struct.

`bg`:
Moves the given job to the background. This includes resuming
(via `SIGCONT`) its process group if it has been stopped.

`kill`:
Terminates the given job by sending a `SIGTERM` signal to all
processes in its process group. These processes then trip
the `SIGCHLD` handler, hence marking the job for deletion.

`stop`:
Suspends the given process by sending a `SIGSTP` signal. Note
that this signal is also caught by the same handler used for
`ˆZ`, which already updates the job status to stopped.

`ˆC`:
At the prompt, this will cancel the current command and print
a new prompt. This is accomplished by setting a jump location
right before the prompt is printed, then calling `siglongjump`.
At a foreground job, this will terminate all processes of said
job by sending `SIGINT`. The returning `SIGCHLD` confirmations
update the process counter and mark the job for deletion.

`ˆZ`:
At the prompt, this will be ignored.
At a foreground job, this will suspend all processes of said
job by sending `SIGTSTP`. The returning `SIGCHLD` confirmations
sample the terminal state, so it can later be restored by `fg`.

Description of Extended Functionality
-------------------------------------
I/O:
Files for I/O are opened in read-only and write-only modes,
with the write end being created when it does not exist, and
appended to when the `>>` syntax is used. Similarly, `>&` and
`|&` send stderr to the stdout file/pipe and not the terminal.
Opened I/O is kept as file descriptors. In the parent, they are
closed soon after forking the child. In the child, they are
used to replace stdin/stdout via `dup2` and `close` calls.
For consistency, the function in charge of spawning children
also accepts stdin/stdout file descriptors. This causes the
`dup2` call to vacuously do nothing, and the `close` calls
to be ignored explicitly.

Pipes:
At any given point, the program only keeps track of two pipes:
the one preceeding a given child, and the one following it.
For consistency, I/O file descriptors overwrite the pipe ends
before being passed to the child spawner.
After creating a child, the parent process closes the two file
descriptors given to the child. This causes all pipes and I/O
to get eventually closed. The child process does exactly the
same, but it also faces the problem of closing the output end
of the new pipe, which is not one of its used file descriptors.
In terms of implementation, this is easiest done by calling
the provided `utils_set_cloexec` method before the child is
created. This causes the pipe end to get closed on `exec`.

Exclusive Access:
Jobs that require terminal access but are run in the background
send `SIGTTIN` and `SIGTTOUT` signals. In the parent process,
these arrive through the `SIGCHLD` handler, updating the job
status to indicate that a terminal was requested. Note that
this special status does not prevent the job from getting
launched in the background by `bg`, in which case it will
(most likely) immediately request the terminal again and stop.

List of Additional Builtins Implemented
---------------------------------------
Tests can be run as follows from the `src` directory:
``` stdriver.py -b -a custom_tests.tst ```

`custom`:
Allows switching on/off a custom prompt that includes the
current machine's hostname, as well as the full working
directory. Full functionality of the regular prompt is also
available at this custom prompt.

`history`:
Prints an enumerated history of entered commands. Optional
parameter restricts this list to the given number of trailing
entries. Uses the GNU history library.
When at the prompt, history can be iterated by up/down arrow
keys or ˆP/ˆN. Alt-Shift-,/Alt-Shift-. get the beginning/end.
History also supports i-searching with ˆR/ˆS. The latter can
be enabled by calling `stty -ixon` before launching `cush`.
Also supported are some event designators; for instance, `!e`
will find the last command that started with 'e', `!1` will
get the first ever command, and `!-2` the second-to-last.