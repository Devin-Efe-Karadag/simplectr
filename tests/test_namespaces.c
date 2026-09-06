#include "namespace.h"
#include "process.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

static int child(void *arg) {
    (void)arg;

    char h[64];

    if (getpid() != 1 || namespace_prepare("simplectr-test"))
        return 1;
    if (gethostname(h, sizeof h) || strcmp(h, "simplectr-test"))
        return 2;
    if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL))
        return 3;
    return access("/proc/1", F_OK) != 0;
}

int main(void) {
    char before[64], after[64];
    assert(!gethostname(before, sizeof before));

    void *stack = malloc(CHILD_STACK);
    assert(stack);

    pid_t pid = namespace_clone(child, NULL, stack);
    assert(pid > 0);
    assert(process_wait(pid) == 0);
    assert(!gethostname(after, sizeof after));
    assert(!strcmp(before, after));
    free(stack);
    puts("Namespace tests passed");
}
