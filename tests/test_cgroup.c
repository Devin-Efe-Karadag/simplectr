#include "cgroup.h"
#include "process.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    char id[33];
    snprintf(id, sizeof id, "test-%ld", (long)getpid());

    struct config c = {.memory = 16777216, .swap = 0, .quota = 50000, .pids = 4};
    assert(!cgroup_create(id, &c));

    char buf[256];
    assert(!cgroup_read(id, "memory.max", buf, sizeof buf) && !strcmp(buf, "16777216\n"));
    assert(!cgroup_read(id, "cpu.max", buf, sizeof buf) && !strcmp(buf, "50000 100000\n"));
    assert(!cgroup_read(id, "pids.max", buf, sizeof buf) && !strcmp(buf, "4\n"));
    assert(!cgroup_remove(id));

    char path[256];
