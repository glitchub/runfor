Usage:

    runfor [options] [--] timeout /path/to/program [args]

Run 'program args' and return its exit status. However if the program runs
longer than 'timeout' seconds, SIGTERM it and its children. If that doesn't
work then SIGKILL it.

Options: 

    -g N - grace time to allow after sending SIGTERM before resorting to SIGKILL,
           default is 3 seconds.
    -s N - Signal to initially send, default is 15 (aka SIGTERM).
    -p   - keep the runfor timer in the target program's process group, so it
           will be paused if the target is paused with ^Z for example.

Exits with 125 if program can't be started. Otherwise the program's exit
status is returned. Typically will be 143 if the program responded to
SIGTERM, 137 if it had to be SIGKILL'ed, etc.
