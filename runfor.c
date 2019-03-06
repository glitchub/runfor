// Run a program with a timeout

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#define die(...) fprintf(stderr, __VA_ARGS__), exit(125)

#define usage() die("\
Usage:\n\
\n\
    runfor [options] [--] timeout /path/to/program [args]\n\
\n\
Run 'program args' and return its exit status. However if the program runs\n\
longer than 'timeout' seconds, SIGTERM it and its children. If that doesn't\n\
work then SIGKILL it.\n\
\n\
Options: \n\
\n\
    -g N - grace time to allow after sending SIGTERM before resorting to SIGKILL,\n\
           default is 3 seconds.\n\
    -s N - Signal to initially send, default is 15 (aka SIGTERM).\n\
    -p   - keep the runfor timer in the target program's process group, so it\n\
           will be paused if the target is paused with ^Z for example.\n\
\n\
Exits with 125 if program can't be started. Otherwise the program's exit\n\
status is returned. Typically will be 143 if the program responded to\n\
SIGTERM, 137 if it had to be SIGKILL'ed, etc.\n\
")


int main(int argc, char *argv[])
{
    int timeout, target, send=15, grace=3, pause=0;

    while (1) switch (getopt(argc,argv,"+:a:s:p"))
    {
        case 's': send=atoi(optarg); if (send < 1 || send > 64) die("Signal must be 1 to 64\n"); break;
        case 'g': grace=atoi(optarg); if (grace < 1) die("Grace time must be 1t least 1\n"); break;
        case 'p': pause=1; break;
        case ':':            // missing
        case '?': usage();   // or invalid options
        case -1: goto optx;  // no more options
    } optx:
    if ((argc - optind) < 2) usage();

    timeout = atoi(argv[optind]);
    if (timeout < 1) die("Timeout must be at least 1 second\n");

    setpgid(0,0);       // we're our own process group

    target=getpid();    // get our pid, this will be the pid of the program we're about to exec

    switch (fork())
    {
        case -1: die("Parent fork failed: %s\n", strerror(errno));
        case 0:
        {
            // Child, fork again
            switch (fork())
            {
                case -1: die("Child fork failed: %s\n", strerror(errno));
                case 0:
                {
                    // Here we're a child of init, not the target process
                    if (!pause) setsid();               // normally setsid now so timeout not affected by ^Z

                    while (timeout--)
                    {
                        sleep(1);
                        if (kill(target,0)) _exit(0);   // done if target is no more
                    }

                    // target process ran too long
                    setsid();                           // setsid now if we didn't before (so we don't kill ourselves)
                    kill(-target, send);                // kill target's process group
                    while(grace--)                      // allow grace time
                    {
                        sleep(1);
                        if (kill(target,0)) _exit(0);   // done if target is no more
                    }
                    kill(-target, SIGKILL);             // ugh, kill it impolitely
                }
            }
            break;
        }
        default:
        {
            // Parent, wait for the child to fork again
            int status;
            if (wait(&status) < 0) die("Child failed: %d", status);
            // Now exec the target program
            execvp(argv[optind+1], &argv[optind+1]);
            die("exec failed: %s\n", strerror(errno));
        }
    }
    return 0;
}
