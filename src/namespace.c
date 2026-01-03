#include "namespace.h"

#include <sched.h>
#include <signal.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>
    return clone(entry, (char *)stack + CHILD_STACK,
                 CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWIPC | CLONE_NEWCGROUP |
                     CLONE_NEWNET | SIGCHLD,
                 arg);
