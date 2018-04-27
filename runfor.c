// Run a program with a timeout
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "errno.h"

#define die(...) fprintf(stderr, __VA_ARGS__), exit(125)

#define usage() die("\
Usage:\n\
\n\
    runfor [-]timeout /path/to/program [args]\n\
\n\
Run 'program args' and return its exit status. However if the program runs\n\
longer than 'timeout' seconds, kill it and its children and the exit status\n\
will be 143 if the program responded to SIGTERM or 137 if it had to be\n\
SIGKILL'ed.\n\
\n\
Exit with status 125 on error.\n\
\n\
Normally pausing the process group with ^Z does not pause the timeout. If you\n\
want the timeout to pause as well specify the timeout as a negative number.\n\
")


int main(int argc, char *argv[]) 
{
    int timeout, target=0;
    
    if (argc < 3) usage();
    timeout = atoi(argv[1]);
    
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
                    if (timeout > 0) setsid();          // normally setsid now so timeout not affected by ^Z 
                    else timeout*=-1;                   // but not if negative          

                    while (timeout--)                   
                    {
                        sleep(1);
                        if (kill(target,0)) _exit(0);   // done if target is no more
                    }

                    // target process ran too long
                    setsid();                           // setsid now if we didn't before (so we don't kill ourselves)
                    kill(-target, SIGTERM);             // kill target's process group politely
                    for (timeout=3; timeout; timeout--) // give it a few seconds to respond
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
            execvp(argv[2], &argv[2]);                          
            die("exec failed: %s\n", strerror(errno));
        }
    }
    return 0;
}   
