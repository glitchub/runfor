Usage:

    runfor [-]timeout /path/to/program [args]

Run 'program args' and return its exit status. However if the program runs
longer than 'timeout' seconds, kill it and its children and the exit status
will be 143 if the program responded to SIGTERM or 137 if it had to be
SIGKILL'ed.

Exit with status 125 on error.

Normally pausing the process group with ^Z does not pause the timeout. If you
want the timeout to pause as well specify the timeout as a negative number.
