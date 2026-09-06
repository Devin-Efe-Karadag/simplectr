#include "process.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    assert(!process_signals());

    pid_t pid = fork();
    assert(pid >= 0);

    if (!pid) {
        char *args[] = {"/bin/sh", "-c", "test \"$1\" = \"a b\" && exit 7", "sh", "a b", NULL};

        char *env[] = {"PATH=/bin:/usr/bin", NULL};
        execve(args[0], args, env);
        _exit(127);
    }
    assert(process_wait(pid) == 7);
    pid = fork();
    assert(pid >= 0);

    if (!pid) {
        process_child_signals();
        raise(SIGTERM);
        _exit(1);
    }
    assert(process_wait(pid) == 143);
    puts("Lifecycle tests passed");
}
