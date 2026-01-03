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
}

int namespace_prepare(const char *hostname) {
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL))
        return -1;
    return sethostname(hostname, strlen(hostname));
}
