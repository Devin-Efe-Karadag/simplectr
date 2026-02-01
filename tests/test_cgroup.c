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
